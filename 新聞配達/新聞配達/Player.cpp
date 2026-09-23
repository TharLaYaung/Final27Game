#include "Player.h"
#include <cmath>

// コンストラクタ
Player::Player()
{
    // プレイヤーの初期位置
    position = VGet(
        0.0f,
        1.7f,
        -5.0f
    );

    // 最初は正面を見る
    yaw = 0.0f;

    // 最初は水平を見る
    pitch = 0.0f;

    // 徒歩の移動速度
    moveSpeed = 0.08f;

    // マウス感度
    mouseSensitivity = 0.003f;

    // 目の高さ
    eyeHeight = 1.7f;

    // 当たり判定サイズ
    collisionRadius = 0.4f;

    // 自転車の初期位置
    bicyclePosition = VGet(
        0.0f,
        0.0f,
        0.0f
    );

    // 最初は自転車との当たり判定を使わない
    bicycleCollisionEnabled = false;

    // マウスカーソルを非表示にする
    SetMouseDispFlag(FALSE);

    // マウスを画面中央へ移動する
    SetMousePoint(
        640,
        360
    );
}


// プレイヤーを更新する
void Player::Update()
{
    // マウス視点を更新する
    UpdateLook();

    // 前方向を計算する
    VECTOR forward = VGet(
        sinf(yaw),
        0.0f,
        cosf(yaw)
    );

    // 右方向を計算する
    VECTOR right = VGet(
        cosf(yaw),
        0.0f,
        -sinf(yaw)
    );

    // 次の位置
    VECTOR nextPosition = position;

    // Wキーで前進する
    if (CheckHitKey(KEY_INPUT_W))
    {
        nextPosition.x +=
            forward.x * moveSpeed;

        nextPosition.z +=
            forward.z * moveSpeed;
    }

    // Sキーで後退する
    if (CheckHitKey(KEY_INPUT_S))
    {
        nextPosition.x -=
            forward.x * moveSpeed;

        nextPosition.z -=
            forward.z * moveSpeed;
    }

    // Aキーで左へ移動する
    if (CheckHitKey(KEY_INPUT_A))
    {
        nextPosition.x -=
            right.x * moveSpeed;

        nextPosition.z -=
            right.z * moveSpeed;
    }

    // Dキーで右へ移動する
    if (CheckHitKey(KEY_INPUT_D))
    {
        nextPosition.x +=
            right.x * moveSpeed;

        nextPosition.z +=
            right.z * moveSpeed;
    }

    // 移動できる場合
    if (CanMove(nextPosition))
    {
        // プレイヤーの位置を更新する
        position = nextPosition;
    }
}


// マウス視点だけ更新する
void Player::UpdateLook()
{
    // マウスの現在位置
    int mouseX;
    int mouseY;

    // マウス位置を取得する
    GetMousePoint(
        &mouseX,
        &mouseY
    );

    // 画面中央からの移動量を取得する
    int moveX =
        mouseX - 640;

    int moveY =
        mouseY - 360;

    // 左右の視点を変更する
    yaw +=
        moveX * mouseSensitivity;

    // 上下の視点を変更する
    pitch +=
        moveY * mouseSensitivity;

    // 上を向きすぎないようにする
    if (pitch < -1.4f)
    {
        pitch = -1.4f;
    }

    // 下を向きすぎないようにする
    if (pitch > 1.4f)
    {
        pitch = 1.4f;
    }

    // マウスを画面中央へ戻す
    SetMousePoint(
        640,
        360
    );
}


// カメラを更新する
void Player::UpdateCamera()
{
    // 視線方向を取得する
    VECTOR forward =
        GetForward();

    // カメラが見る位置を計算する
    VECTOR target =
        VAdd(
            position,
            forward
        );

    // カメラを設定する
    SetCameraPositionAndTarget_UpVecY(
        position,
        target
    );
}


// プレイヤーの現在位置を取得する
VECTOR Player::GetPosition() const
{
    return position;
}


// プレイヤーが見ている方向を取得する
VECTOR Player::GetForward() const
{
    // 視線方向
    VECTOR forward;

    // 左右方向
    forward.x =
        sinf(yaw) *
        cosf(pitch);

    // 上下方向
    forward.y =
        -sinf(pitch);

    // 前後方向
    forward.z =
        cosf(yaw) *
        cosf(pitch);

    return forward;
}


// プレイヤーの位置を変更する
void Player::SetPosition(VECTOR newPosition)
{
    // 新しい位置を設定する
    position = newPosition;
}


// 自転車の位置を設定する
void Player::SetBicyclePosition(VECTOR newBicyclePosition)
{
    // 自転車の現在位置を保存する
    bicyclePosition = newBicyclePosition;
}


// 自転車との当たり判定を使うか設定する
void Player::SetBicycleCollisionEnabled(bool enabled)
{
    // 当たり判定の有効状態を保存する
    bicycleCollisionEnabled = enabled;
}


// 移動できるか確認する
bool Player::CanMove(VECTOR nextPosition)
{
    // テスト用の赤い箱の範囲
    float minX =
        -1.0f - collisionRadius;

    float maxX =
        1.0f + collisionRadius;

    float minZ =
        5.0f - collisionRadius;

    float maxZ =
        7.0f + collisionRadius;

    // 赤い箱の中に入る場合
    if (
        nextPosition.x > minX &&
        nextPosition.x < maxX &&
        nextPosition.z > minZ &&
        nextPosition.z < maxZ
        )
    {
        // 移動できない
        return false;
    }

    // 自転車との当たり判定を使う場合
    if (bicycleCollisionEnabled == true)
    {
        // X方向の距離
        float diffX =
            nextPosition.x - bicyclePosition.x;

        // Z方向の距離
        float diffZ =
            nextPosition.z - bicyclePosition.z;

        // 自転車までの水平方向の距離
        float distance =
            sqrtf(
                diffX * diffX +
                diffZ * diffZ
            );

        // 自転車に近すぎる場合
        if (distance < 0.8f)
        {
            // 自転車をすり抜けないようにする
            return false;
        }
    }

    // 移動できる
    return true;
}