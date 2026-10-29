#include "Mailbox.h"
#include "Player.h"
#include "Newspaper.h"
#include "DeliveryManager.h"
#include "HorrorUI.h"
#include <cmath>

// 敷石コンクリート台座を描画するヘルパー関数
static void DrawMailboxFootingBox(float cx, float cy, float cz, float hw, float hh, float hd, unsigned int faceCol, unsigned int topCol)
{
    float x1 = cx - hw, x2 = cx + hw;
    float y1 = cy - hh, y2 = cy + hh;
    float z1 = cz - hd, z2 = cz + hd;

    // 側面
    DrawTriangle3D(VGet(x1, y1, z1), VGet(x2, y1, z1), VGet(x1, y2, z1), faceCol, TRUE);
    DrawTriangle3D(VGet(x2, y1, z1), VGet(x2, y2, z1), VGet(x1, y2, z1), faceCol, TRUE);
    DrawTriangle3D(VGet(x1, y1, z2), VGet(x1, y2, z2), VGet(x2, y1, z2), faceCol, TRUE);
    DrawTriangle3D(VGet(x2, y1, z2), VGet(x1, y2, z2), VGet(x2, y2, z2), faceCol, TRUE);
    DrawTriangle3D(VGet(x1, y1, z1), VGet(x1, y2, z1), VGet(x1, y1, z2), faceCol, TRUE);
    DrawTriangle3D(VGet(x1, y2, z1), VGet(x1, y2, z2), VGet(x1, y1, z2), faceCol, TRUE);
    DrawTriangle3D(VGet(x2, y1, z1), VGet(x2, y1, z2), VGet(x2, y2, z1), faceCol, TRUE);
    DrawTriangle3D(VGet(x2, y2, z1), VGet(x2, y1, z2), VGet(x2, y2, z2), faceCol, TRUE);
    // 天面
    DrawTriangle3D(VGet(x1, y2, z1), VGet(x2, y2, z1), VGet(x1, y2, z2), topCol, TRUE);
    DrawTriangle3D(VGet(x2, y2, z1), VGet(x2, y2, z2), VGet(x1, y2, z2), topCol, TRUE);
}

// ポスト直下の接地影を描画するヘルパー関数
static void DrawMailboxContactShadow(VECTOR pos, float rx, float rz, int alpha)
{
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    unsigned int shCol = GetColor(5, 6, 8);
    const int segs = 14;
    for (int i = 0; i < segs; ++i)
    {
        float a1 = (float)i / segs * DX_PI_F * 2.0f;
        float a2 = (float)(i + 1) / segs * DX_PI_F * 2.0f;
        VECTOR c = VGet(pos.x, 0.015f, pos.z);
        VECTOR p1 = VGet(pos.x + cosf(a1) * rx, 0.015f, pos.z + sinf(a1) * rz);
        VECTOR p2 = VGet(pos.x + cosf(a2) * rx, 0.015f, pos.z + sinf(a2) * rz);
        DrawTriangle3D(c, p1, p2, shCol, TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// コンストラクタ
Mailbox::Mailbox()
{
    modelHandle = -1;
    scale = 2.2f;
    canDeliver = false;
    aimedMailboxId = -1;
    oldLeftClick = false;
}

// デストラクタ
Mailbox::~Mailbox()
{
    Finalize();
}

// 初期化
bool Mailbox::Initialize()
{
    // 郵便ポストの3Dモデルを読み込み
    modelHandle = MV1LoadModel("Data/Model/Mailbox.mv1");
    if (modelHandle == -1)
    {
        return false;
    }

    // ポストのリアルな材質設定（不気味な深夜に馴染む暗色スチールと赤色マーカー）
    // 本体・支柱（落ち着いたスチールブラック）
    MV1SetMaterialDifColor(modelHandle, 0, GetColorF(0.24f, 0.27f, 0.29f, 1.0f));
    MV1SetMaterialAmbColor(modelHandle, 0, GetColorF(0.18f, 0.20f, 0.22f, 1.0f));
    MV1SetMaterialSpcColor(modelHandle, 0, GetColorF(0.40f, 0.40f, 0.40f, 1.0f));
    MV1SetMaterialEmiColor(modelHandle, 0, GetColorF(0.0f, 0.0f, 0.0f, 1.0f));

    // 投入口フラップ（わずかに金属光沢）
    MV1SetMaterialDifColor(modelHandle, 1, GetColorF(0.32f, 0.35f, 0.38f, 1.0f));
    MV1SetMaterialAmbColor(modelHandle, 1, GetColorF(0.22f, 0.24f, 0.26f, 1.0f));
    MV1SetMaterialSpcColor(modelHandle, 1, GetColorF(0.50f, 0.50f, 0.50f, 1.0f));
    MV1SetMaterialEmiColor(modelHandle, 1, GetColorF(0.0f, 0.0f, 0.0f, 1.0f));

    // ポストフラグ（褪せた赤色マーカー）
    MV1SetMaterialDifColor(modelHandle, 2, GetColorF(0.70f, 0.15f, 0.12f, 1.0f));
    MV1SetMaterialAmbColor(modelHandle, 2, GetColorF(0.35f, 0.08f, 0.06f, 1.0f));
    MV1SetMaterialSpcColor(modelHandle, 2, GetColorF(0.30f, 0.15f, 0.15f, 1.0f));
    MV1SetMaterialEmiColor(modelHandle, 2, GetColorF(0.0f, 0.0f, 0.0f, 1.0f));

    return true;
}

// 更新処理
void Mailbox::Update(Player& player, Newspaper& newspaper, DeliveryManager& deliveryManager)
{
    if (modelHandle == -1)
    {
        return;
    }

    aimedMailboxId = -1;
    canDeliver = false;

    // 左クリックのエッジトリガー検出（押した瞬間のみtrue）
    bool currentLeftClick = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;
    bool leftClickDown = currentLeftClick && !oldLeftClick;

    // プレイヤーが新聞を所持している場合、各ポストとの距離・照準を判定
    if (newspaper.IsHolding())
    {
        VECTOR playerPosition = player.GetPosition();
        VECTOR playerForward = player.GetForward();

        const std::vector<DeliveryPoint>& points = deliveryManager.GetDeliveryPoints();

        for (size_t i = 0; i < points.size(); i++)
        {
            // ポストの投入口付近（高さ1.2m）を目標点とする
            VECTOR targetPos = VGet(
                points[i].position.x,
                points[i].position.y + 1.2f,
                points[i].position.z
            );

            VECTOR toMailbox = VSub(targetPos, playerPosition);
            float distance = VSize(toMailbox);

            // 3.2m以内に接近している場合
            if (distance <= 3.2f)
            {
                VECTOR direction = VNorm(toMailbox);
                float dot = VDot(playerForward, direction);

                // プレイヤーがポストの方向を向いている場合
                if (dot >= 0.88f)
                {
                    aimedMailboxId = points[i].id;
                    canDeliver = true;
                    break;
                }
            }
        }
    }

    // エッジトリガーで左クリックされた時の配達処理
    if (leftClickDown && canDeliver && newspaper.IsHolding())
    {
        // 配達受付中（通常状態）の場合のみ処理
        if (deliveryManager.GetState() == DeliveryState::Active)
        {
            if (aimedMailboxId != -1 && aimedMailboxId == deliveryManager.GetCurrentTargetId())
            {
                // 正しい現在の配達目標ポストへの配達
                newspaper.Deliver();
                deliveryManager.OnDeliverySuccess(aimedMailboxId);
                canDeliver = false;
            }
            else
            {
                // 誤ったポストへの配達試行
                deliveryManager.TriggerWrongDelivery();
            }
        }
    }

    // 次フレーム判定用にクリック状態を保持
    oldLeftClick = currentLeftClick;
}

// 全ポストの3Dモデル描画
void Mailbox::Draw(const DeliveryManager& deliveryManager)
{
    if (modelHandle == -1)
    {
        return;
    }

    const std::vector<DeliveryPoint>& points = deliveryManager.GetDeliveryPoints();

    for (size_t i = 0; i < points.size(); i++)
    {
        const VECTOR& pos = points[i].position;

        // 接地影（浮遊感を完全に解消し地面に定着させるアンビエントオクルージョン）
        DrawMailboxContactShadow(pos, 0.44f, 0.44f, 180);

        // 敷石コンクリート台座（幅0.52m x 奥行0.52m x 高さ0.08mの基礎ブロック）
        DrawMailboxFootingBox(pos.x, 0.04f, pos.z, 0.26f, 0.04f, 0.26f, GetColor(50, 54, 56), GetColor(68, 72, 75));

        // 支柱根元の金属アンカーフランジ金具（幅0.24m x 奥行0.24m x 高さ0.02m）
        DrawMailboxFootingBox(pos.x, 0.09f, pos.z, 0.12f, 0.01f, 0.12f, GetColor(25, 28, 30), GetColor(35, 38, 40));

        // ポスト3Dモデルの配置・描画
        MV1SetPosition(modelHandle, pos);
        MV1SetScale(modelHandle, VGet(scale, scale, scale));
        MV1SetRotationXYZ(modelHandle, VGet(0.0f, points[i].angle, 0.0f));

        MV1DrawModel(modelHandle);
    }
}

// UI描画処理（ホラーUIへのプロンプト伝達）
void Mailbox::DrawUI(const DeliveryManager& deliveryManager)
{
    // 配達可能な状態（照準が合っている）ならプロンプトを設定
    if (canDeliver)
    {
        HorrorUI::Instance().SetPrompt(PromptType::DeliverNewspaper);
    }
}

// 現在照準が合っているポストID
int Mailbox::GetAimedMailboxId() const
{
    return aimedMailboxId;
}

// 配達可能状態か取得する
bool Mailbox::CanDeliver() const
{
    return canDeliver;
}

// 終了処理
void Mailbox::Finalize()
{
    if (modelHandle != -1)
    {
        MV1DeleteModel(modelHandle);
        modelHandle = -1;
    }
}
