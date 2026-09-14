#include "Newspaper.h"

// コンストラクタ
Newspaper::Newspaper()
{
    // モデル未読み込み
    modelHandle = -1;

    // 初期位置
    position = VGet(
        0.0f,
        0.0f,
        0.0f
    );
}


// デストラクタ
Newspaper::~Newspaper()
{
    // モデルを削除する
    Finalize();
}


// 新聞モデルを初期化する
bool Newspaper::Initialize()
{
    // 新聞モデルを読み込む
    modelHandle = MV1LoadModel(
        "Data/Model/Newspaper.mv1"
    );

    // 読み込み失敗
    if (modelHandle == -1)
    {
        return false;
    }

    // 新聞モデルの大きさ
    MV1SetScale(
        modelHandle,
        VGet(
            0.01f,
            0.01f,
            0.01f
        )
    );

    return true;
}


// 新聞を更新する
void Newspaper::Update(VECTOR bicyclePosition)
{
    // 自転車の前カゴ付近に置く
    position = VGet(
        bicyclePosition.x + 0.7f,
        bicyclePosition.y + 0.65f,
        bicyclePosition.z
    );

    // 新聞の位置を設定する
    MV1SetPosition(
        modelHandle,
        position
    );

    // 新聞の向きを設定する
    MV1SetRotationXYZ(
        modelHandle,
        VGet(
            0.0f,
            0.0f,
            0.0f
        )
    );
}


// 新聞を描画する
void Newspaper::Draw()
{
    // モデルがない場合
    if (modelHandle == -1)
    {
        return;
    }

    // 新聞を描画する
    MV1DrawModel(
        modelHandle
    );
}


// 新聞モデルを削除する
void Newspaper::Finalize()
{
    // モデルがある場合
    if (modelHandle != -1)
    {
        MV1DeleteModel(
            modelHandle
        );

        modelHandle = -1;
    }
}