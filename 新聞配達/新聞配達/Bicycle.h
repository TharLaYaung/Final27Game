#pragma once

// DxLibを使用する
#include "DxLib.h"

// Playerクラスを使用する
class Player;

// 自転車を管理するクラス
class Bicycle
{
public:

    // コンストラクタ
    Bicycle();

    // デストラクタ
    ~Bicycle();

    // 自転車モデルを初期化する
    bool Initialize();

    // 自転車を更新する
    void Update(Player& player);

    // 自転車を描画する
    void Draw();

    // 自転車モデルを削除する
    void Finalize();

    // プレイヤーが自転車に乗っているか取得する
    bool IsRiding() const;

    // 自転車の現在位置を取得する
    VECTOR GetPosition() const;

private:

    // 自転車モデルのハンドル
    int modelHandle;

    // 自転車の現在位置
    VECTOR position;

    // 自転車の向き
    float angle;

    // プレイヤーが自転車に乗っているか
    bool isRiding;

    // 前のフレームでEキーが押されていたか
    bool oldEKey;

    // 自転車の移動速度
    float moveSpeed;
};