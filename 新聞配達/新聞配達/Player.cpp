#include "Player.h"
#include <cmath>


// ========================================
// コンストラクタ
// ========================================
Player::Player()
{
    // プレイヤー初期位置
    position = VGet(
        0.0f,
        1.7f,
        -5.0f
    );

    // 左右の視点角度
    yaw = 0.0f;

    // 上下の視点角度
    pitch = 0.0f;

    // 移動速度
    moveSpeed = 0.1f;

    // マウス感度
    mouseSensitivity = 0.003f;

    // 目線の高さ
    eyeHeight = 1.7f;

    // 当たり判定の大きさ
    collisionRadius = 0.4f;
}


// ========================================
// プレイヤー更新
// ========================================
void Player::Update()
{
    // ----------------------------------------
    // マウス移動量を取得
    // ----------------------------------------
    int mouseX;
    int mouseY;

    GetMousePoint(
        &mouseX,
        &mouseY
    );


    // 画面中央
    const int centerX = 640;
    const int centerY = 360;


    // 中央からどれくらい動いたか計算
    int deltaX =
        mouseX - centerX;

    int deltaY =
        mouseY - centerY;


    // マウス移動で視点回転
    yaw +=
        deltaX * mouseSensitivity;

    pitch +=
        deltaY * mouseSensitivity;


    // 上下を向きすぎないように制限
    if (pitch > 1.5f)
    {
        pitch = 1.5f;
    }

    if (pitch < -1.5f)
    {
        pitch = -1.5f;
    }


    // マウスを画面中央に戻す
    SetMousePoint(
        centerX,
        centerY
    );


    // ----------------------------------------
    // 前方向を計算
    // ----------------------------------------
    VECTOR forward;

    forward.x = sinf(yaw);
    forward.y = 0.0f;
    forward.z = cosf(yaw);


    // ----------------------------------------
    // 右方向を計算
    // ----------------------------------------
    VECTOR right;

    right.x = cosf(yaw);
    right.y = 0.0f;
    right.z = -sinf(yaw);


    // 次の位置
    VECTOR nextPosition = position;


    // Wキー
    if (CheckHitKey(KEY_INPUT_W))
    {
        nextPosition.x +=
            forward.x * moveSpeed;

        nextPosition.z +=
            forward.z * moveSpeed;
    }


    // Sキー
    if (CheckHitKey(KEY_INPUT_S))
    {
        nextPosition.x -=
            forward.x * moveSpeed;

        nextPosition.z -=
            forward.z * moveSpeed;
    }


    // Aキー
    if (CheckHitKey(KEY_INPUT_A))
    {
        nextPosition.x -=
            right.x * moveSpeed;

        nextPosition.z -=
            right.z * moveSpeed;
    }


    // Dキー
    if (CheckHitKey(KEY_INPUT_D))
    {
        nextPosition.x +=
            right.x * moveSpeed;

        nextPosition.z +=
            right.z * moveSpeed;
    }


    // 高さを固定
    nextPosition.y =
        eyeHeight;


    // 移動可能なら位置更新
    if (CanMove(nextPosition))
    {
        position =
            nextPosition;
    }

} // ← ここがとても重要


// ========================================
// カメラ更新
// ========================================
void Player::UpdateCamera()
{
    // プレイヤーが向いている方向を取得
    VECTOR forward =
        GetForward();


    // カメラが見る位置
    VECTOR target;

    target.x =
        position.x + forward.x;

    target.y =
        position.y + forward.y;

    target.z =
        position.z + forward.z;


    // カメラ位置と注視点を設定
    SetCameraPositionAndTarget_UpVecY(
        position,
        target
    );

} // ← ここも重要


// ========================================
// プレイヤー位置を取得
// ========================================
VECTOR Player::GetPosition() const
{
    return position;
}


// ========================================
// プレイヤーが向いている方向を取得
// ========================================
VECTOR Player::GetForward() const
{
    VECTOR forward;


    // X方向
    forward.x =
        sinf(yaw) * cosf(pitch);


    // Y方向
    forward.y =
        -sinf(pitch);


    // Z方向
    forward.z =
        cosf(yaw) * cosf(pitch);


    return forward;
}
// プレイヤーの位置を変更する
void Player::SetPosition(VECTOR newPosition)
{
    // 新しい位置をプレイヤーの現在位置に設定する
    position = newPosition;
}
// 移動できるか確認

bool Player::CanMove(
    VECTOR nextPosition
)
{
    // ----------------------------------------
    // テスト用の赤い箱
    // ----------------------------------------

    float minX =
        -1.0f - collisionRadius;

    float maxX =
        1.0f + collisionRadius;

    float minZ =
        5.0f - collisionRadius;

    float maxZ =
        7.0f + collisionRadius;


    // 箱の中に入ろうとしている場合
    if (
        nextPosition.x > minX &&
        nextPosition.x < maxX &&
        nextPosition.z > minZ &&
        nextPosition.z < maxZ
        )
    {
        // 移動させない
        return false;
    }


    // 移動可能
    return true;


}