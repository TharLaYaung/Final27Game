#include "Mailbox.h"
#include "Player.h"
#include "Newspaper.h"
#include <cmath>

// コンストラクタ
Mailbox::Mailbox()
{
    // モデルハンドルを未読み込み状態に設定
    modelHandle = -1;

    // ポストの初期配置位置を設定（正面方向5メートル先）
    position = VGet(
        0.0f,
        0.0f,
        5.0f
    );

    // ポストの向き（プレイヤーから投函口が見えやすい角度）
    angle = DX_PI_F * 0.75f;

    // モデルのスケール（自転車やプレイヤーのサイズ感に合わせて2.2倍）
    scale = 2.2f;

    // 初期状態では配達不可
    canDeliver = false;

    // 初期状態では未配達
    delivered = false;

    // 左クリック初期状態
    oldLeftClick = false;

    // メッセージタイマー初期化
    messageTimer = 0;
}

// デストラクタ
Mailbox::~Mailbox()
{
    // モデルリソースを解放する
    Finalize();
}

// 初期化処理
bool Mailbox::Initialize()
{
    // メールボックスの3Dモデルを読み込む
    modelHandle = MV1LoadModel(
        "Data/Model/Mailbox.mv1"
    );

    // 読み込みに失敗した場合はfalseを返す
    if (modelHandle == -1)
    {
        return false;
    }

    // モデルの位置を設定する
    MV1SetPosition(
        modelHandle,
        position
    );

    // モデルの大きさを設定する
    MV1SetScale(
        modelHandle,
        VGet(
            scale,
            scale,
            scale
        )
    );

    // モデルの回転を設定する
    MV1SetRotationXYZ(
        modelHandle,
        VGet(
            0.0f,
            angle,
            0.0f
        )
    );

    return true;
}

// 更新処理
void Mailbox::Update(Player& player, Newspaper& newspaper)
{
    // モデルが読み込まれていない場合は何もしない
    if (modelHandle == -1)
    {
        return;
    }

    // 毎フレーム判定フラグを初期化する
    canDeliver = false;

    // 現在のマウス左クリック状態を取得する
    bool leftClick =
        (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;

    // プレイヤーが新聞を手に持っており、まだ配達していない場合のみ判定する
    if (newspaper.IsHolding() == true && delivered == false)
    {
        // プレイヤーの現在位置を取得する
        VECTOR playerPosition =
            player.GetPosition();

        // プレイヤーの視線方向を取得する
        VECTOR playerForward =
            player.GetForward();

        // ポストの中心目標位置（目線の高さに合わせて箱の高さ約1.2mを狙う）
        VECTOR targetPosition =
            VGet(
                position.x,
                position.y + 1.2f,
                position.z
            );

        // プレイヤーからポストへのベクトルを計算する
        VECTOR toMailbox =
            VSub(
                targetPosition,
                playerPosition
            );

        // ポストまでの距離を計算する
        float distance =
            VSize(
                toMailbox
            );

        // 3.2メートル以内に近づいている場合
        if (distance <= 3.2f)
        {
            // ポストへの方向ベクトルを正規化する
            VECTOR direction =
                VNorm(
                    toMailbox
                );

            // プレイヤーの視線とポストへの方向の内積を計算する
            float dot =
                VDot(
                    playerForward,
                    direction
                );

            // ポストの方向をしっかり見ている場合（dot >= 0.88f）
            if (dot >= 0.88f)
            {
                // 配達可能フラグを立てる
                canDeliver = true;
            }
        }
    }

    // 配達可能な状態で、左クリックを「今押した瞬間」だけ配達を実行する
    if (
        canDeliver == true &&
        leftClick == true &&
        oldLeftClick == false
        )
    {
        // 手に持っている新聞を配達する（手放す）
        newspaper.Deliver();

        // ポストを配達済みにする（二重配達を防止）
        delivered = true;

        // 配達案内を消す
        canDeliver = false;

        // 配達完了メッセージを約120フレーム（2秒間）表示する
        messageTimer = 120;
    }

    // 次のフレームのために現在の左クリック状態を保存する
    oldLeftClick =
        leftClick;

    // メッセージ表示時間をカウントダウンする
    if (messageTimer > 0)
    {
        messageTimer--;
    }
}

// 描画処理
void Mailbox::Draw()
{
    // モデルがない場合は描画しない
    if (modelHandle == -1)
    {
        return;
    }

    // 配達可能な時はポストを少し明るく照らす（プレイヤーに分かりやすくする）
    if (canDeliver == true)
    {
        SetLightPosition(
            VGet(
                position.x,
                position.y + 1.5f,
                position.z
            )
        );

        SetLightDifColor(
            GetColorF(
                1.0f,
                1.0f,
                0.7f,
                1.0f
            )
        );
    }

    // ポストの3Dモデルを描画する
    MV1DrawModel(
        modelHandle
    );
}

// UI描画処理
void Mailbox::DrawUI()
{
    // 配達可能な状態の場合、配達操作の案内を表示する
    if (canDeliver == true)
    {
        DrawString(
            530,
            410,
            "Left Click : Deliver Newspaper",
            GetColor(
                255,
                255,
                0
            )
        );
    }

    // 配達直後の場合、「Delivered!」メッセージを表示する
    if (messageTimer > 0)
    {
        DrawString(
            590,
            380,
            "Delivered!",
            GetColor(
                100,
                255,
                100
            )
        );
    }
}

// ポストの位置を取得する
VECTOR Mailbox::GetPosition() const
{
    return position;
}

// 配達済みかどうかを取得する
bool Mailbox::IsDelivered() const
{
    return delivered;
}

// 終了処理
void Mailbox::Finalize()
{
    // モデルが存在する場合は削除する
    if (modelHandle != -1)
    {
        MV1DeleteModel(
            modelHandle
        );

        modelHandle = -1;
    }
}