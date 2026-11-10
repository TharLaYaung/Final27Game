#pragma once

// DXライブラリを使用する
#include "DxLib.h"

// プレイヤーのクラスを使用する
#include "Player.h"


// インタラクトを管理するクラス

// Eキーで
// ポスト・ドア・自転車などを
// 調べるために使用する
class Interaction
{
public:

    // コンストラクタ
    Interaction();


    // インタラクト処理を更新する
    void Update(Player& player);


    // 画面表示を描画する
    void Draw();


private:

    // 現在インタラクトできるか
    bool canInteract;


    // Eキーが前のフレームで
    // 押されていたか
    bool oldEKey;


 
    // 「調べました」を表示する時間
    int messageTimer;
};