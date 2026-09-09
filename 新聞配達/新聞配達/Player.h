#pragma once

// DxLibを使うために読み込む
#include "DxLib.h"

class Player
{
public:

    // コンストラクタ
    // プレイヤーを作った時に最初に呼ばれる
    Player();

    // プレイヤーの移動やマウス操作を更新する
    void Update();

    // FPSカメラを更新する
    void UpdateCamera();

private:

    // プレイヤーの現在位置
    VECTOR position;

    // カメラの左右の角度
    float yaw;

    // カメラの上下の角度
    float pitch;

    // プレイヤーの移動速度
    float moveSpeed;

    // マウスの感度
    float mouseSensitivity;

    // プレイヤーの目線の高さ
    float eyeHeight;

    // プレイヤーの当たり判定の半径
    float collisionRadius;

    // ========================================
    // 次の位置へ移動できるか確認する関数
    //
    // true  = 移動できる
    // false = 壁に当たっている
    // ========================================
    bool CanMove(VECTOR nextPosition);
};