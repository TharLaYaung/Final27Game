#pragma once

#include "DxLib.h"

class Player;

// 新聞を管理するクラス
class Newspaper
{
public:

    // コンストラクタ
    Newspaper();

    // デストラクタ
    ~Newspaper();

    // 初期化
    bool Initialize();

    // 更新
    void Update(
        Player& player,
        VECTOR bicyclePosition,
        float bicycleAngle,
        bool bicycleRiding
    );

    // 描画
    void Draw();

    // UI描画
    void DrawUI();

    // 終了処理
    void Finalize();

    // 配達処理（手持ち状態を解除する）
    void Deliver();

    // 新聞を持っているか
    bool IsHolding() const;

    // 新聞を取れる状態か
    bool CanTake() const;

    // 残り新聞枚数
    int GetNewspaperCount() const;

private:

    // 新聞モデル
    int modelHandle;

    // 自転車の位置
    VECTOR bicyclePosition;

    // 自転車の角度
    float bicycleAngle;

    // カゴ内での固定位置
    VECTOR localPosition;

    // カゴ内での固定角度
    VECTOR localRotation;

    // 新聞のサイズ
    float modelScale;

    // 新聞同士の間隔
    float stackInterval;

    // 残り新聞枚数
    int newspaperCount;

    // 新聞を手に持っているか
    bool isHolding;

    // 新聞を取れる状態か
    bool canTake;

    // 前フレームの左クリック状態
    bool oldLeftClick;

    // プレイヤー位置
    VECTOR playerPosition;

    // プレイヤーの向き
    VECTOR playerForward;

    // カゴの新聞のワールド位置を取得
    VECTOR GetBasketNewspaperPosition() const;

    // カゴの新聞を描画
    void DrawBasketNewspapers();

    // 手に持っている新聞を描画
    void DrawHeldNewspaper();
};