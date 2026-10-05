#include "Interaction.h"

// コンストラクタ
Interaction::Interaction()
{
    // 初期状態はインタラクト不可
    canInteract = false;

    // 初期状態はEキー非押下
    oldEKey = false;

    // メッセージ表示時間を0に初期化
    messageTimer = 0;
}

// インタラクト状態更新
void Interaction::Update(Player& player)
{
    (void)player;

    // 各個別オブジェクトで案内を行うため汎用フラグは通常OFF
    canInteract = false;

    // メッセージ表示時間の減算
    if (messageTimer > 0)
    {
        messageTimer--;
    }
}

// インタラクトUIの描画
void Interaction::Draw()
{
    // インタラクト可能な場合
    if (canInteract == true)
    {
        DrawString(
            570,
            500,
            "E : Interact",
            GetColor(
                255,
                255,
                255
            )
        );
    }

    // 調べた直後のフィードバック表示
    if (messageTimer > 0)
    {
        DrawString(
            570,
            550,
            "Interacted!",
            GetColor(
                255,
                255,
                0
            )
        );
    }
}