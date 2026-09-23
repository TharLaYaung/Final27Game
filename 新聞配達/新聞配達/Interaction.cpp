#include "Interaction.h"

// コンストラクタ
Interaction::Interaction()
{
    // 最初は何も調べられない
    canInteract = false;

    // 最初はEキーを押していない
    oldEKey = false;

    // メッセージ表示時間を0にする
    messageTimer = 0;
}

// インタラクト処理
void Interaction::Update(Player& player)
{
    // 赤い箱を撤去したため、現在の汎用インタラクトは無効化
    canInteract = false;

    // メッセージ表示時間を減らす
    if (messageTimer > 0)
    {
        messageTimer--;
    }
}

// インタラクトUIを描画する
void Interaction::Draw()
{
    // インタラクトできる場合
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

    // Eキーを押した後
    if (messageTimer > 0)
    {
        DrawString(
            570,
            550,
            "Interaction!",
            GetColor(
                255,
                255,
                0
            )
        );
    }
}