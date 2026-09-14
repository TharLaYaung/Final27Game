#pragma once

#include "DxLib.h"

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

    // UIを描画する
    void DrawUI();

    // 自転車モデルを削除する
    void Finalize();

    // 自転車に乗っているか取得する
    bool IsRiding() const;

    // 自転車の位置を取得する
    VECTOR GetPosition() const;

private:

    // 自転車モデル
    int modelHandle;

    // 自転車の位置
    VECTOR position;

    // 自転車の向き
    float angle;

    // 自転車に乗っているか
    bool isRiding;

    // 自転車をカーソルで選択しているか
    bool canRide;

    // 前のフレームのEキー状態
    bool oldEKey;

    // 自転車の移動速度
    float moveSpeed;

    // 自転車が移動できるか確認する
    bool CanMove(VECTOR nextPosition);
};