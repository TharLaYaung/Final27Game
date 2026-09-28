#pragma once

#include "DxLib.h"

class Player;

// 自転車を管理するクラス
class Bicycle
{
public:

    // コンストラクタ
    Bicycle();

    // デストラクタ
    ~Bicycle();

    // 自転車モデルを初期化する
    bool Initialize();

    // 自転車を更新する
    void Update(Player& player);

    // 自転車を描画する
    void Draw();

    // UIを描画する
    void DrawUI();

    // 自転車モデルを削除する
    void Finalize();

    // 自転車に乗っているか取得する
    bool IsRiding() const;

    // 自転車を選択可能か取得する
    bool CanRide() const;

    // 自転車の位置を取得する
    VECTOR GetPosition() const;

    // 自転車の向きを取得する
    float GetAngle() const;

    // ==========================================
    // ヘッドライト・バッテリーAPI
    // ==========================================

    // ヘッドライトが点灯しているか
    bool IsHeadlightOn() const;

    // 現在のバッテリー残量 (0.0f〜100.0f)
    float GetBatteryCurrent() const;

    // バッテリー最大容量 (100.0f)
    float GetBatteryMax() const;

    // バッテリー残量パーセント (0.0f〜100.0f)
    float GetBatteryPercent() const;

    // バッテリーが空かどうか
    bool IsBatteryEmpty() const;

    // ヘッドライトの点灯/消灯切り替え（Fキー）
    void ToggleHeadlight();

    // 高速消費デバッグモードが有効か
    bool IsDebugFastDrain() const;

    // 高速消費デバッグモードの切り替え（F8キー）
    void ToggleDebugFastDrain();

    // ヘッドライトのワールド位置（ローカルオフセット適用済み）
    VECTOR GetHeadlightPosition() const;

    // ヘッドライトの照射方向ベクトル（自転車の前方向）
    VECTOR GetHeadlightDirection() const;

    // ヘッドライトの現在の輝度倍率（0.0f〜1.0f、ちらつき・減光反映）
    float GetHeadlightIntensity() const;

    // ヘッドライトの照射距離（バッテリー残量により微小変化）
    float GetHeadlightRange() const;

    // 将来のホラーイベント用フック
    void TriggerHeadlightFlicker();
    void TriggerHeadlightBlackout(float durationSeconds);
    void SetBatteryDrainMultiplier(float multiplier);

    // バッテリー空時の通知フラグ（UI表示用）
    bool IsEmptyWarningActive() const;

private:

    // 自転車モデル
    int modelHandle;

    // 自転車の位置
    VECTOR position;

    // 自転車の向き
    float angle;

    // 自転車に乗っているか
    bool isRiding;

    // 自転車をカーソルで選択しているか
    bool canRide;

    // 前のフレームのEキー状態
    bool oldEKey;

    // 自転車の移動速度
    float moveSpeed;

    // 自転車が移動できるか確認する
    bool CanMove(VECTOR nextPosition);

    // ==========================================
    // ヘッドライト・バッテリー内部メンバ
    // ==========================================

    // ヘッドライト点灯フラグ
    bool headlightOn;

    // 前フレームのFキー状態（エッジ検出用）
    bool oldFKey;

    // 前フレームのF8キー状態
    bool oldF8Key;

    // ヘッドライト配置オフセット（ローカル座標系）
    float headlightOffsetForward;
    float headlightOffsetHeight;
    float headlightOffsetSide;

    // ヘッドライト基本有効距離
    float headlightBaseRange;

    // ヘッドライト輝度倍率
    float headlightIntensity;

    // 一時的なフリッカー・ブラックアウト制御
    float flickerMultiplier;
    int flickerTimer;
    int flickerPhase;
    float externalBlackoutTimer;

    // バッテリー関連
    float batteryMax;
    float batteryCurrent;
    float batteryDrainMultiplier;
    bool debugFastDrain;

    // 時間計測用（フレームレート非依存）
    LONGLONG lastUpdateTime;

    // バッテリー切れ警告タイマー
    int emptyWarningTimer;

    // 内部更新処理
    void UpdateBatteryAndLighting(float dt);
    void UpdateFlickerLogic(float dt);
    void PlayElectricalSoundHook();
};