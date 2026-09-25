#include "Bicycle.h"
#include "Player.h"
#include "HorrorUI.h"
#include <cmath>

// コンストラクタ
Bicycle::Bicycle()
{
    // モデル未読み込み
    modelHandle = -1;

    // 自転車の初期位置
    position = VGet(
        3.0f,
        0.18f,
        3.0f
    );

    // 自転車の向き
    angle = 0.0f;

    // 最初は乗っていない
    isRiding = false;

    // 最初は選択していない
    canRide = false;

    // 最初はEキーを押していない
    oldEKey = false;

    // 自転車の移動速度
    moveSpeed = 0.14f;
}


// デストラクタ
Bicycle::~Bicycle()
{
    // モデルを削除する
    Finalize();
}


// 自転車モデルを初期化する
bool Bicycle::Initialize()
{
    // 自転車モデルを読み込む
    modelHandle = MV1LoadModel(
        "Data/Model/Bicycle.mv1"
    );

    // 読み込み失敗
    if (modelHandle == -1)
    {
        return false;
    }

    // 自転車の位置を設定する
    MV1SetPosition(
        modelHandle,
        position
    );

    // 自転車の大きさを設定する
    MV1SetScale(
        modelHandle,
        VGet(
            0.01f,
            0.01f,
            0.01f
        )
    );

    // 自転車の向きを設定する
    MV1SetRotationXYZ(
        modelHandle,
        VGet(
            0.0f,
            angle,
            0.0f
        )
    );

    return true;
}


// 自転車を更新する
void Bicycle::Update(Player& player)
{
    // モデルがない場合
    if (modelHandle == -1)
    {
        return;
    }

    // Eキーの状態を取得する
    bool currentEKey =
        CheckHitKey(KEY_INPUT_E) != 0;

    // 自転車に乗っていない場合
    if (isRiding == false)
    {
        // プレイヤーの位置を取得する
        VECTOR playerPosition =
            player.GetPosition();

        // プレイヤーが見ている方向を取得する
        VECTOR playerForward =
            player.GetForward();

        // プレイヤーから自転車への方向を計算する
        VECTOR toBicycle =
            VSub(
                position,
                playerPosition
            );

        // プレイヤーと自転車の距離を計算する
        float distance =
            sqrtf(
                toBicycle.x * toBicycle.x +
                toBicycle.y * toBicycle.y +
                toBicycle.z * toBicycle.z
            );

        // 自転車を選択していない状態にする
        canRide = false;

        // 3メートル以内の場合
        if (distance <= 3.0f)
        {
            // 距離が0ではない場合
            if (distance > 0.001f)
            {
                // 自転車方向を正規化する
                VECTOR direction;

                direction.x =
                    toBicycle.x / distance;

                direction.y =
                    toBicycle.y / distance;

                direction.z =
                    toBicycle.z / distance;

                // 視線と自転車方向の一致度を計算する
                float dot =
                    playerForward.x * direction.x +
                    playerForward.y * direction.y +
                    playerForward.z * direction.z;

                // カーソルが自転車を向いている場合
                if (dot >= 0.95f)
                {
                    // 自転車を選択状態にする
                    canRide = true;
                }
            }
        }

        // 自転車を選択中にEキーを押した場合
        if (
            canRide == true &&
            currentEKey == true &&
            oldEKey == false
            )
        {
            // 自転車に乗る
            isRiding = true;

            // 選択状態を解除する
            canRide = false;

            // プレイヤーを自転車の位置へ移動する
            player.SetPosition(
                VGet(
                    position.x,
                    position.y + 1.2f,
                    position.z
                )
            );
        }
    }
    else
    {
        // 自転車の前方向を計算する
        VECTOR forward =
            VGet(
                cosf(angle),
                0.0f,
                -sinf(angle)
            );

        // 次の位置
        VECTOR nextPosition =
            position;

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

        // 移動できる場合
        if (CanMove(nextPosition))
        {
            // 自転車の位置を更新する
            position =
                nextPosition;
        }

        // Aキーで左に曲がる
        if (CheckHitKey(KEY_INPUT_A))
        {
            angle -= 0.03f;
        }

        // Dキーで右に曲がる
        if (CheckHitKey(KEY_INPUT_D))
        {
            angle += 0.03f;
        }

        // プレイヤーを自転車に合わせる
        player.SetPosition(
            VGet(
                position.x,
                position.y + 1.2f,
                position.z
            )
        );

        // Eキーを押した場合
        if (
            currentEKey == true &&
            oldEKey == false
            )
        {
            // 自転車から降りる
            isRiding = false;

            // プレイヤーを自転車の横へ移動する
            player.SetPosition(
                VGet(
                    position.x + 1.5f,
                    position.y + 1.7f,
                    position.z
                )
            );
        }
    }

    // Eキーの状態を保存する
    oldEKey =
        currentEKey;

    // 自転車の位置を更新する
    MV1SetPosition(
        modelHandle,
        position
    );

    // 自転車の向きを更新する
    MV1SetRotationXYZ(
        modelHandle,
        VGet(
            0.0f,
            angle,
            0.0f
        )
    );
}


// 自転車を描画する
void Bicycle::Draw()
{
    // モデルがない場合
    if (modelHandle == -1)
    {
        return;
    }

    // 自転車を選択している場合
    if (canRide == true)
    {
        // 選択中は自転車の近くにライトを置く
        SetLightPosition(
            VGet(
                position.x,
                position.y + 1.0f,
                position.z
            )
        );

        // ライトの色を少し明るくする
        SetLightDifColor(
            GetColorF(
                1.0f,
                1.0f,
                0.6f,
                1.0f
            )
        );
    }
    else
    {
        // 通常時のライト色
        SetLightDifColor(
            GetColorF(
                1.0f,
                1.0f,
                1.0f,
                1.0f
            )
        );
    }

    // 自転車モデルを描画する
    MV1DrawModel(
        modelHandle
    );
}


// UIを描画する（ホラーUIへのプロンプト伝達）
void Bicycle::DrawUI()
{
    // 自転車を選択している場合
    if (canRide == true)
    {
        HorrorUI::Instance().SetPrompt(PromptType::RideBicycle);
    }
    // 自転車に乗っている場合
    else if (isRiding == true)
    {
        HorrorUI::Instance().SetPrompt(PromptType::DismountBicycle);
    }
}

// 自転車を選択可能か取得する
bool Bicycle::CanRide() const
{
    return canRide;
}


// 自転車が移動できるか確認する
bool Bicycle::CanMove(VECTOR nextPosition)
{
    // 自転車の当たり判定サイズ
    float bicycleRadius =
        0.8f;

    // 赤い箱の左側
    float minX =
        -1.0f - bicycleRadius;

    // 赤い箱の右側
    float maxX =
        1.0f + bicycleRadius;

    // 赤い箱の手前
    float minZ =
        5.0f - bicycleRadius;

    // 赤い箱の奥
    float maxZ =
        7.0f + bicycleRadius;

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

    // 移動できる
    return true;
}


// 自転車モデルを削除する
void Bicycle::Finalize()
{
    // モデルがある場合
    if (modelHandle != -1)
    {
        // モデルを削除する
        MV1DeleteModel(
            modelHandle
        );

        // ハンドルをリセットする
        modelHandle = -1;
    }
}


// 自転車に乗っているか取得する
bool Bicycle::IsRiding() const
{
    return isRiding;
}


// 自転車の位置を取得する
VECTOR Bicycle::GetPosition() const
{
    return position;
}

// 自転車の向きを取得する
float Bicycle::GetAngle() const
{
    return angle;
}