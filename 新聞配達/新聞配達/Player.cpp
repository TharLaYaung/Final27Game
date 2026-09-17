#include "Player.h"
#include "Map.h"
#include <cmath>

// コンストラクタ
Player::Player()
{
    position          = VGet(0.0f, 1.7f, -5.0f);
    yaw               = 0.0f;
    pitch             = 0.0f;
    moveSpeed         = 0.1f;
    mouseSensitivity  = 0.003f;
    eyeHeight         = 1.7f;
    collisionRadius   = 0.4f;
    bicyclePosition   = VGet(0.0f, 0.0f, 0.0f);
    bicycleCollisionEnabled = false;

    SetMouseDispFlag(FALSE);
    SetMousePoint(640, 360);
}


// ========================================================
//  XZ位置に対して「壁をよけながら移動できるか」を試みる
//  step-up（段差乗り越え）付き
//  戻り値: 実際に移動できたX,Z (Yは変更しない)
// ========================================================
static bool TryMoveXZ(VECTOR& pos, float nextX, float nextZ,
    float radius, float eyeH)
{
    // XZ両方試す
    VECTOR candidate = VGet(nextX, pos.y, nextZ);

    if (!MapCheckWallCollision(candidate, radius))
    {
        pos.x = nextX;
        pos.z = nextZ;
        return true;
    }

    // X方向だけ試す
    VECTOR xOnly = VGet(nextX, pos.y, pos.z);
    if (!MapCheckWallCollision(xOnly, radius))
    {
        pos.x = nextX;
        return true;
    }

    // Z方向だけ試す
    VECTOR zOnly = VGet(pos.x, pos.y, nextZ);
    if (!MapCheckWallCollision(zOnly, radius))
    {
        pos.z = nextZ;
        return true;
    }

    return false;
}

// プレイヤー更新（移動・当たり判定・床検出）
void Player::Update()
{
    UpdateLook();

    VECTOR forward = VGet(sinf(yaw), 0.0f, cosf(yaw));
    VECTOR right = VGet(cosf(yaw), 0.0f, -sinf(yaw));

    float nextX = position.x;
    float nextZ = position.z;

    if (CheckHitKey(KEY_INPUT_W)) { nextX += forward.x * moveSpeed; nextZ += forward.z * moveSpeed; }
    if (CheckHitKey(KEY_INPUT_S)) { nextX -= forward.x * moveSpeed; nextZ -= forward.z * moveSpeed; }
    if (CheckHitKey(KEY_INPUT_A)) { nextX -= right.x * moveSpeed;   nextZ -= right.z * moveSpeed; }
    if (CheckHitKey(KEY_INPUT_D)) { nextX += right.x * moveSpeed;   nextZ += right.z * moveSpeed; }

    // 既存の当たり判定（ボックス・自転車）
    VECTOR nextPos = VGet(nextX, position.y, nextZ);
    if (CanMove(nextPos))
    {
        // 家との壁・階段判定（step-up付き）
        TryMoveXZ(position, nextX, nextZ, collisionRadius, eyeHeight);
    }



    float floorY = MapGetFloorY(position);

    if (floorY > -9999.0f)
    {
        float targetY = floorY + eyeHeight;
        float diff = targetY - position.y;

        // 下りだけ許可
        if (diff < 0.0f)
        {
            position.y += diff * 0.3f;
        }
    }
    else
    {
        float groundY = eyeHeight;

        if (position.y > groundY + 0.05f)
        {
            position.y -= 0.1f;

            if (position.y < groundY)
                position.y = groundY;
        }
        else
        {
            position.y = groundY;
        }
    }
}


// マウス視点更新
void Player::UpdateLook()
{
    int mouseX, mouseY;
    GetMousePoint(&mouseX, &mouseY);

    int moveX = mouseX - 640;
    int moveY = mouseY - 360;

    yaw   += moveX * mouseSensitivity;
    pitch += moveY * mouseSensitivity;

    if (pitch < -1.4f) pitch = -1.4f;
    if (pitch >  1.4f) pitch =  1.4f;

    SetMousePoint(640, 360);
}


// カメラ更新
void Player::UpdateCamera()
{
    VECTOR forward = GetForward();
    VECTOR target  = VAdd(position, forward);
    SetCameraPositionAndTarget_UpVecY(position, target);
}


// プレイヤー位置取得
VECTOR Player::GetPosition() const { return position; }

// 前方ベクトル取得
VECTOR Player::GetForward() const
{
    return VGet(
        sinf(yaw) * cosf(pitch),
        -sinf(pitch),
        cosf(yaw) * cosf(pitch)
    );
}

// プレイヤー位置を直接設定
void Player::SetPosition(VECTOR newPosition) { position = newPosition; }

// 自転車の位置を設定
void Player::SetBicyclePosition(VECTOR newBicyclePosition) { bicyclePosition = newBicyclePosition; }

// 自転車との当たり判定の有効無効を設定
void Player::SetBicycleCollisionEnabled(bool enabled) { bicycleCollisionEnabled = enabled; }


// テストボックス・自転車との当たり判定
bool Player::CanMove(VECTOR nextPosition)
{
    float minX = -1.0f - collisionRadius;
    float maxX =  1.0f + collisionRadius;
    float minZ =  5.0f - collisionRadius;
    float maxZ =  7.0f + collisionRadius;

    if (nextPosition.x > minX && nextPosition.x < maxX &&
        nextPosition.z > minZ && nextPosition.z < maxZ)
    {
        return false;
    }

    if (bicycleCollisionEnabled)
    {
        float diffX = nextPosition.x - bicyclePosition.x;
        float diffZ = nextPosition.z - bicyclePosition.z;
        float dist  = sqrtf(diffX * diffX + diffZ * diffZ);
        if (dist < 0.8f) return false;
    }

    return true;
}
