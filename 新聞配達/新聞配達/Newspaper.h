#pragma once

#include "DxLib.h"

// 新聞を管理するクラス
class Newspaper
{
public:

    // コンストラクタ
    Newspaper();

    // デストラクタ
    ~Newspaper();

    // 新聞モデルを初期化する
    bool Initialize();

    // 新聞を更新する
    void Update(VECTOR bicyclePosition);

    // 新聞を描画する
    void Draw();

    // 新聞モデルを削除する
    void Finalize();

private:

    // 新聞モデル
    int modelHandle;

    // 新聞の位置
    VECTOR position;
};
