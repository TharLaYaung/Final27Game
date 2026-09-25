#pragma once

#include "DxLib.h"

// 前方宣言
class Player;
class Bicycle;
class DeliveryManager;
class Newspaper;

// レトロ・低照度ナビゲーションGPS端末風ミニマップ
class Minimap
{
public:
    Minimap();
    ~Minimap();

    // 初期化
    void Initialize();

    // 毎フレーム更新（アニメーション、フェード、ノイズ管理）
    void Update();

    // ミニマップおよび配達進行情報の描画
    void Draw(
        const Player& player,
        const Bicycle& bicycle,
        const DeliveryManager& deliveryManager,
        const Newspaper& newspaper
    );

    // ==========================================
    // ホラー演出・グリッチAPI
    // ==========================================

    // ミニマップの電波障害・干渉度設定 (0.0f〜1.0f)
    void SetMinimapInterference(float amount);
    float GetMinimapInterference() const;

    // ミニマップのグリッチをトリガー
    void TriggerMinimapGlitch(int durationFrames = 20);

    // ミニマップの表示有効/無効設定（停電やロストイベント用）
    void SetMinimapEnabled(bool enabled);
    bool IsMinimapEnabled() const;

private:
    int panelX1;            // GPS端末枠の左上X
    int panelY1;            // GPS端末枠の左上Y
    int panelX2;            // GPS端末枠の右下X
    int panelY2;            // GPS端末枠の右下Y

    int displayCenterX;     // 地図ビューの中央X
    int displayCenterY;     // 地図ビューの中央Y
    int viewHalfWidth;      // 地図ビューの描画幅の半分
    int viewHalfHeight;     // 地図ビューの描画高さの半分
    float mapScale;         // ワールド座標から地図ピクセルへの倍率

    int animFrame;          // アニメーションフレームカウンタ
    int glitchTimer;        // グリッチタイマー
    float interference;     // 電波障害度
    bool isEnabled;         // 表示有効フラグ

    // メッセージフェード用アルファ値
    float feedbackAlpha;
    float wrongAlpha;
    float allCompleteAlpha;
};
