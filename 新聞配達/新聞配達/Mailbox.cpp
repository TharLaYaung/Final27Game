#include "Mailbox.h"
#include "Player.h"
#include "Newspaper.h"
#include "DeliveryManager.h"
#include "HorrorUI.h"
#include <cmath>

// コンストラクタ
Mailbox::Mailbox()
{
    modelHandle = -1;
    scale = 2.2f;
    canDeliver = false;
    aimedMailboxId = 0;
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

    return true;
}

// 更新処理
void Mailbox::Update(Player& player, Newspaper& newspaper, DeliveryManager& deliveryManager)
{
    if (modelHandle == -1)
    {
        return;
    }

    aimedMailboxId = 0;
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
            if (aimedMailboxId == deliveryManager.GetCurrentTargetId())
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
        MV1SetPosition(modelHandle, points[i].position);
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