// Bicycle.hを読み込む
#include "Bicycle.h"

// Player.hを読み込む
#include "Player.h"

// sinf、cosfを使用する
#include <cmath>


// コンストラクタ
Bicycle::Bicycle()
{
    // モデルをまだ読み込んでいない状態にする
    modelHandle = -1;

    // 自転車の初期位置を設定する
    position = VGet(
        3.0f,
        0.0f,
        3.0f
    );

    // 自転車の初期角度
    angle = 0.0f;

    // 最初は自転車に乗っていない
    isRiding = false;

    // 最初はEキーを押していない
    oldEKey = false;

    // 自転車の移動速度
    moveSpeed = 0.20f;
}


// デストラクタ
Bicycle::~Bicycle()
{
    // 自転車モデルを削除する
    Finalize();
}


// 自転車モデルを初期化する
bool Bicycle::Initialize()
{
    // Bicycle.mv1を読み込む
    modelHandle = MV1LoadModel(
        "新聞配達/新聞配達/Data/Model/Bicycle.mv1"
    );

    // モデルの読み込みに失敗した場合
    if (modelHandle == -1)
    {
        // 初期化失敗
        return false;
    }

    // 自転車モデルの位置を設定する
    MV1SetPosition(
        modelHandle,
        position
    );

    // 自転車モデルの大きさを設定する
    MV1SetScale(
        modelHandle,
        VGet(
            1.0f,
            1.0f,
            1.0f
        )
    );

    // 自転車モデルの向きを設定する
    MV1SetRotationXYZ(
        modelHandle,
        VGet(
            0.0f,
            angle,
            0.0f
        )
    );

    // 初期化成功
    return true;
}


// 自転車を更新する
void Bicycle::Update(Player& player)
{
    // 自転車モデルが読み込まれていない場合は何もしない
    if (modelHandle == -1)
    {
        return;
    }

    // 自転車モデルの位置を更新する
    MV1SetPosition(
        modelHandle,
        position
    );

    // 自転車モデルの向きを更新する
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
    // モデルが正常に読み込まれている場合
    if (modelHandle != -1)
    {
        // 自転車モデルを描画する
        MV1DrawModel(
            modelHandle
        );
    }
}


// 自転車モデルを削除する
void Bicycle::Finalize()
{
    // モデルが読み込まれている場合
    if (modelHandle != -1)
    {
        // モデルを削除する
        MV1DeleteModel(
            modelHandle
        );

        // モデルが存在しない状態に戻す
        modelHandle = -1;
    }
}


// プレイヤーが自転車に乗っているか取得する
bool Bicycle::IsRiding() const
{
    // 自転車に乗っている状態を返す
    return isRiding;
}


// 自転車の現在位置を取得する
VECTOR Bicycle::GetPosition() const
{
    // 自転車の位置を返す
    return position;
}