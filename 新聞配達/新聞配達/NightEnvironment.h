#pragma once

#include "DxLib.h"
#include <vector>

// 前方宣言
class Bicycle;

// 環境プリセット種別
enum class EnvironmentPreset
{
    Normal,             // 開発・テスト用（視界良好な夜間）
    NightDeliveryStyle, // 本番用デフォルト（例外配達風: 3:00 AMの不気味で濃い闇と冷たい霧）
    ExtremeDark         // 実験用（極端な暗闇・超至近距離フォグ）
};

// 街灯の状態
enum class StreetLightState
{
    Normal,     // 通常点灯
    Weak,       // 弱点灯（薄暗い琥珀色）
    Off,        // 消灯（完全な暗闇のシルエット）
    Flickering  // 不規則な明滅・点滅
};

// 街灯データ構造体
struct StreetLight
{
    int id;                     // 街灯ID (1〜)
    VECTOR position;            // 電柱・街灯の足元ワールド座標
    StreetLightState state;     // 点灯状態
    int lightHandle;            // DxLibライトハンドル
    int flickerTimer;           // 明滅アニメーション用タイマー
    int flickerPhase;           // 明滅フェーズ
    float currentIntensity;     // 現在の輝度倍率 (0.0f〜1.0f)
    COLOR_F baseColor;          // 基本光色
};

// 3:00 AM深夜住宅街の照明・フォグ・環境統合管理クラス
class NightEnvironment
{
public:
    NightEnvironment();
    ~NightEnvironment();

    // 初期化（街灯生成、フォグ設定、ライトハンドル作成）
    bool Initialize();

    // 毎フレーム更新（自転車ヘッドライト追従、街灯明滅タイマーなど）
    void Update(const Bicycle& bicycle);

    // 描画フレームごとの環境適用（DxLibフォグ・アンビエント・背景色）
    void ApplyLightingAndFog();

    // 街灯電柱と発光シェードの3D描画
    void DrawStreetLights();

    // デバッグ情報HUD描画（Fキー操作案内と現在パラメータ）
    void DrawDebugHUD();

    // 終了処理（ライトハンドルの破棄）
    void Finalize();

    // プリセット・デバッグ制御

    // プリセット適用
    void SetPreset(EnvironmentPreset preset);

    // プリセット順送り切り替え (F2)
    void CyclePreset();

    // フォグON/OFF切り替え (F1)
    void ToggleFog();

    // 街灯一括ON/OFF切り替え (F3)
    void ToggleStreetLights();

    // 指定街灯のテスト明滅トリガー (F5)
    void TriggerStreetLightFlicker(int id);

    // 将来のホラーイベント拡張用API

    // 特定の街灯を消灯
    void TurnOffStreetLight(int id);

    // 特定の街灯を点灯
    void TurnOnStreetLight(int id);

    // フォグ距離変更
    void SetFogDistance(float start, float end);

    // 環境光の強さ変更
    void SetAmbientStrength(float strength);

    // 一時的な停電イベント（全街灯ブラックアウト）
    void TriggerTemporaryBlackout(int frames);

    // 現在のプリセット取得
    EnvironmentPreset GetCurrentPreset() const;

    // 天候システムからのフォグおよび環境光の一括設定
    void SetWeatherFogAndLighting(float start, float end, int r, int g, int b, COLOR_F ambient);

    // フォグ開始距離の取得
    float GetFogStart() const { return fogStart; }

    // フォグ終了距離の取得
    float GetFogEnd() const { return fogEnd; }

    // フォグ有効状態の取得
    bool IsFogEnabled() const { return fogEnabled; }

    // 街灯リストの取得
    const std::vector<StreetLight>& GetStreetLights() const { return streetLights; }

private:
    // プリセットごとの内部設定関数
    void ApplyPresetSettings();

    // 街灯の不規則明滅ロジック更新
    void UpdateFlicker(StreetLight& light);

    // 街灯3Dポールとランプヘッドを描画
    void DrawPoleModel(const VECTOR& pos, float intensity, COLOR_F color);

    // 現在の環境プリセット
    EnvironmentPreset currentPreset;

    // フォグ設定
    bool fogEnabled;
    float fogStart;
    float fogEnd;
    int fogColorR;
    int fogColorG;
    int fogColorB;

    // 背景色（フォグの奥の色と完全に一致させる）
    int bgR;
    int bgG;
    int bgB;

    // グローバル環境光
    COLOR_F ambientColor;

    // 自転車ヘッドライト
    int bicycleLightHandle;
    float bicycleLightRange;
    float bicycleLightPower;
    COLOR_F bicycleLightColor;

    // 街灯群
    bool streetLightsEnabled;
    std::vector<StreetLight> streetLights;

    // 一時停電タイマー
    int blackoutTimer;

    // デバッグ通知用タイマーとメッセージ
    int debugNotifyTimer;
    char debugMessage[128];
};
