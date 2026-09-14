#pragma once

#include "DxLib.h"

// プレイヤーを管理するクラス
class Player
{
public:

    // コンストラクタ
    Player();

    // プレイヤーを更新する
    void Update();

    // マウス視点だけ更新する
    void UpdateLook();

    // カメラを更新する
    void UpdateCamera();

    // プレイヤーの現在位置を取得する
    VECTOR GetPosition() const;

    // プレイヤーが見ている方向を取得する
    VECTOR GetForward() const;

    // プレイヤーの位置を変更する
    void SetPosition(VECTOR newPosition);

    // 自転車の位置を設定する
    void SetBicyclePosition(VECTOR newBicyclePosition);

    // 自転車との当たり判定を使うか設定する
    void SetBicycleCollisionEnabled(bool enabled);

private:

    // プレイヤーの現在位置
    VECTOR position;

    // 左右の視点角度
    float yaw;

    // 上下の視点角度
    float pitch;

    // 徒歩の移動速度
    float moveSpeed;

    // マウス感度
    float mouseSensitivity;

    // プレイヤーの目の高さ
    float eyeHeight;

    // プレイヤーの当たり判定サイズ
    float collisionRadius;

    // 自転車の位置
    VECTOR bicyclePosition;

    // 自転車との当たり判定を使うか
    bool bicycleCollisionEnabled;

    // 指定した位置へ移動できるか確認する
    bool CanMove(VECTOR nextPosition);
};