#pragma once

// DxLibを使用する
#include "DxLib.h"

// プレイヤーを管理するクラス
class Player
{
public:

    // コンストラクタ
    Player();

    // プレイヤーを更新する
    void Update();

    // カメラを更新する
    void UpdateCamera();

    // プレイヤーの現在位置を取得する
    VECTOR GetPosition() const;

    // プレイヤーが向いている方向を取得する
    VECTOR GetForward() const;

    // プレイヤーの位置を変更する
    // 自転車に乗る時や降りる時に使用する
    void SetPosition(VECTOR newPosition);

private:

    // プレイヤーの現在位置
    VECTOR position;

    // 左右の視点角度
    float yaw;

    // 上下の視点角度
    float pitch;

    // プレイヤーの移動速度
    float moveSpeed;

    // マウスの感度
    float mouseSensitivity;

    // プレイヤーの目線の高さ
    float eyeHeight;

    // プレイヤーの当たり判定の大きさ
    float collisionRadius;

    // 指定した位置に移動できるか確認する
    bool CanMove(VECTOR nextPosition);
};