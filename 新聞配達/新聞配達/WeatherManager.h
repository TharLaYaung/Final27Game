#pragma once

#include "DxLib.h"
#include <vector>

class Player;
class Bicycle;
class NightEnvironment;

// 天候種別定義
enum class WeatherType
{
    Clear,      // 通常夜間（澄んだ冷たい深夜・基本視認距離）
    Rain,       // 雨（静かに降り注ぐ雨粒）
    HeavyRain,  // 豪雨（激しい雨粒と視界低下）
    Snow,       // 雪（ゆっくり舞い散る粉雪）
    HeavySnow,  // 猛吹雪（密度の高い吹雪と視界不良）
    Foggy,      // 濃霧（立ち込める霧・距離18m程度）
    ThickFog    // 超濃霧（3:00 AMの極限恐怖・距離11mで完全な闇）
};

// 天候環境パラメータ構造体
struct WeatherParameters
{
    float fogStart;
    float fogEnd;
    int fogR;
    int fogG;
    int fogB;
    COLOR_F ambientColor;
    int targetRainCount;
    int targetSnowCount;
    float rainSpeed;
    float snowSpeed;
    VECTOR windVelocity;
};

// 雨パーティクル構造体
struct RainDrop
{
    VECTOR position;
    float length;
    float speed;
    float fallAngle;
    bool active;
};

// 雪パーティクル構造体
struct SnowFlake
{
    VECTOR position;
    float size;
    float fallSpeed;
    float wobblePhase;
    float wobbleSpeed;
    float driftStrength;
    bool active;
};

// 深夜3:00の動的ランダム天候管理クラス
class WeatherManager
{
public:
    WeatherManager();
    ~WeatherManager();

    // 初期化処理
    bool Initialize();

    // 毎フレーム更新（天候タイマー、推移補間、パーティクルシミュレーション）
    void Update(float dt, const Player& player, const Bicycle& bicycle, NightEnvironment& nightEnv);

    // 3D空間内の天候パーティクル描画
    void DrawParticles(const Bicycle& bicycle, const NightEnvironment& nightEnv);

    // デバッグ情報HUDの描画（F10で切り替え）
    void DrawDebugHUD();

    // 終了処理
    void Finalize();

    // 手動天候順送り切り替え（F7）
    void CycleWeather();

    // 自動天候変化の有効/無効切り替え（F9）
    void ToggleAutoWeather();

    // 天候デバッグ表示のON/OFF切り替え（F10）
    void ToggleDebugHUD();

    // 特定の天候へのスムーズ遷移開始（将来のホラーイベント用）
    void TransitionTo(WeatherType newWeather, float durationSeconds = 8.0f);

    // 超濃霧の強制開始（ホラー演出用ショートカット）
    void ForceThickFog(float durationSeconds = 6.0f);

    // 現在および遷移先の天候状態取得
    WeatherType GetCurrentWeather() const { return currentWeather; }
    WeatherType GetTargetWeather() const { return targetWeather; }
    bool IsAutoWeatherEnabled() const { return autoWeatherEnabled; }
    bool IsDebugHUDVisible() const { return showDebugHUD; }
    float GetTransitionProgress() const { return transitionProgress; }
    const char* GetWeatherName(WeatherType type) const;

    // 将来のサウンドアセット用ハンドルフック
    void SetRainSoundHandle(int handle) { rainSoundHandle = handle; }
    void SetWindSoundHandle(int handle) { windSoundHandle = handle; }
    void SetSnowSoundHandle(int handle) { snowSoundHandle = handle; }

private:
    // 天候種別ごとの標準パラメータ取得
    WeatherParameters GetPresetParameters(WeatherType type) const;

    // 重み付き確率および現実的な遷移経路による次天候決定
    WeatherType ChooseNextWeather(WeatherType current);

    // パーティクルの初期化
    void InitializeParticles();

    // 雨パーティクルの更新
    void UpdateRain(float dt, const VECTOR& playerPos);

    // 雪パーティクルの更新
    void UpdateSnow(float dt, const VECTOR& playerPos);

    // 街灯および自転車ヘッドライトによるパーティクル照明判定
    bool CheckParticleIllumination(const VECTOR& particlePos, const Bicycle& bicycle, const NightEnvironment& nightEnv) const;

    // 現在適用中の天候種別
    WeatherType currentWeather;
    // 遷移先の天候種別
    WeatherType targetWeather;

    // 遷移中フラグおよび進捗
    bool isTransitioning;
    float transitionProgress;
    float transitionDuration;

    // 現在フレームの補間済み環境パラメータ
    WeatherParameters currentParams;
    WeatherParameters sourceParams;
    WeatherParameters destParams;

    // 自動天候変更タイマー（60秒〜180秒間隔）
    bool autoWeatherEnabled;
    float weatherTimer;
    float nextWeatherDuration;

    // デバッグ表示フラグ
    bool showDebugHUD;

    // 最大パーティクル数定数
    static constexpr int MAX_RAIN_COUNT = 700;
    static constexpr int MAX_SNOW_COUNT = 450;

    // パーティクル配列
    RainDrop rainPool[MAX_RAIN_COUNT];
    SnowFlake snowPool[MAX_SNOW_COUNT];

    // サウンドハンドル保持フック
    int rainSoundHandle;
    int windSoundHandle;
    int snowSoundHandle;
};
