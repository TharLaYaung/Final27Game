#include "WeatherManager.h"
#include "Player.h"
#include "Bicycle.h"
#include "NightEnvironment.h"
#include "HorrorUI.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

// 線形補間ユーティリティ関数群
static float LerpFloat(float a, float b, float t)
{
    return a + (b - a) * t;
}

static int LerpInt(int a, int b, float t)
{
    return static_cast<int>(std::round(static_cast<float>(a) + static_cast<float>(b - a) * t));
}

static COLOR_F LerpColorF(const COLOR_F& a, const COLOR_F& b, float t)
{
    return GetColorF(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        1.0f
    );
}

static VECTOR LerpVector(const VECTOR& a, const VECTOR& b, float t)
{
    return VGet(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    );
}

// コンストラクタ
WeatherManager::WeatherManager()
    : currentWeather(WeatherType::Clear)
    , targetWeather(WeatherType::Clear)
    , isTransitioning(false)
    , transitionProgress(1.0f)
    , transitionDuration(8.0f)
    , autoWeatherEnabled(true)
    , weatherTimer(0.0f)
    , nextWeatherDuration(90.0f)
    , isExtremeFogActive(false)
    , isFogOverrideActive(false)
    , overridePhase(0)
    , overrideTargetFogEnd(8.0f)
    , overrideBaseFogEnd(11.0f)
    , overrideTimer(0.0f)
    , overrideDuration(4.0f)
    , overrideRecoverDuration(5.0f)
    , showDebugHUD(false)
    , rainSoundHandle(-1)
    , windSoundHandle(-1)
    , snowSoundHandle(-1)
{
    currentParams = GetPresetParameters(WeatherType::Clear);
    sourceParams = currentParams;
    destParams = currentParams;
    preExtremeParams = currentParams;

    for (int i = 0; i < MAX_RAIN_COUNT; ++i)
    {
        rainPool[i].active = false;
    }
    for (int i = 0; i < MAX_SNOW_COUNT; ++i)
    {
        snowPool[i].active = false;
    }
}

// デストラクタ
WeatherManager::~WeatherManager()
{
    Finalize();
}

// 初期化処理
bool WeatherManager::Initialize()
{
    currentWeather = WeatherType::Clear;
    targetWeather = WeatherType::Clear;
    isTransitioning = false;
    transitionProgress = 1.0f;
    transitionDuration = 8.0f;

    isExtremeFogActive = false;
    isFogOverrideActive = false;
    overridePhase = 0;
    overrideTimer = 0.0f;

    currentParams = GetPresetParameters(currentWeather);
    sourceParams = currentParams;
    destParams = currentParams;
    preExtremeParams = currentParams;

    autoWeatherEnabled = true;
    nextWeatherDuration = 60.0f + static_cast<float>(GetRand(120));
    weatherTimer = nextWeatherDuration;

    InitializeParticles();
    return true;
}

// パーティクル配列の初期化
void WeatherManager::InitializeParticles()
{
    for (int i = 0; i < MAX_RAIN_COUNT; ++i)
    {
        rainPool[i].position = VGet(0.0f, -100.0f, 0.0f);
        rainPool[i].length = 0.45f + static_cast<float>(GetRand(25)) / 100.0f;
        rainPool[i].speed = 22.0f + static_cast<float>(GetRand(60)) / 10.0f;
        rainPool[i].fallAngle = 0.0f;
        rainPool[i].active = true;
    }

    for (int i = 0; i < MAX_SNOW_COUNT; ++i)
    {
        snowPool[i].position = VGet(0.0f, -100.0f, 0.0f);
        snowPool[i].size = 0.06f + static_cast<float>(GetRand(4)) / 100.0f;
        snowPool[i].fallSpeed = 2.5f + static_cast<float>(GetRand(20)) / 10.0f;
        snowPool[i].wobblePhase = static_cast<float>(GetRand(628)) / 100.0f;
        snowPool[i].wobbleSpeed = 2.0f + static_cast<float>(GetRand(20)) / 10.0f;
        snowPool[i].driftStrength = 0.3f + static_cast<float>(GetRand(30)) / 100.0f;
        snowPool[i].active = true;
    }
}

// 天候プリセットパラメータ定義
WeatherParameters WeatherManager::GetPresetParameters(WeatherType type) const
{
    WeatherParameters p;

    switch (type)
    {
    case WeatherType::Clear:
        // 通常夜間（深夜基本フォグ: 8m〜28m、遠景は暗闇に溶け込み全景は見通せない）
        p.fogStart = 8.0f;
        p.fogEnd = 28.0f;
        p.fogR = 7;
        p.fogG = 10;
        p.fogB = 14;
        p.ambientColor = GetColorF(0.028f, 0.032f, 0.042f, 1.0f);
        p.targetRainCount = 0;
        p.targetSnowCount = 0;
        p.rainSpeed = 20.0f;
        p.snowSpeed = 3.0f;
        p.windVelocity = VGet(0.0f, 0.0f, 0.0f);
        break;

    case WeatherType::Rain:
        // 雨（冷たい雨夜: 6m〜22m）
        p.fogStart = 6.0f;
        p.fogEnd = 22.0f;
        p.fogR = 6;
        p.fogG = 9;
        p.fogB = 13;
        p.ambientColor = GetColorF(0.024f, 0.028f, 0.038f, 1.0f);
        p.targetRainCount = 320;
        p.targetSnowCount = 0;
        p.rainSpeed = 22.0f;
        p.snowSpeed = 3.0f;
        p.windVelocity = VGet(1.4f, 0.0f, 0.7f);
        break;

    case WeatherType::HeavyRain:
        // 豪雨（視界不良の暗夜: 3.5m〜14m）
        p.fogStart = 3.5f;
        p.fogEnd = 14.0f;
        p.fogR = 5;
        p.fogG = 8;
        p.fogB = 11;
        p.ambientColor = GetColorF(0.018f, 0.022f, 0.030f, 1.0f);
        p.targetRainCount = 680;
        p.targetSnowCount = 0;
        p.rainSpeed = 29.0f;
        p.snowSpeed = 3.0f;
        p.windVelocity = VGet(3.0f, 0.0f, 1.5f);
        break;

    case WeatherType::Snow:
        // 雪（舞い散る粉雪夜: 6m〜22m）
        p.fogStart = 6.0f;
        p.fogEnd = 22.0f;
        p.fogR = 7;
        p.fogG = 10;
        p.fogB = 14;
        p.ambientColor = GetColorF(0.026f, 0.030f, 0.040f, 1.0f);
        p.targetRainCount = 0;
        p.targetSnowCount = 200;
        p.rainSpeed = 20.0f;
        p.snowSpeed = 3.2f;
        p.windVelocity = VGet(0.8f, 0.0f, 0.4f);
        break;

    case WeatherType::HeavySnow:
        // 猛吹雪（激しい吹雪と視界遮蔽: 3.0m〜13m）
        p.fogStart = 3.0f;
        p.fogEnd = 13.0f;
        p.fogR = 6;
        p.fogG = 9;
        p.fogB = 12;
        p.ambientColor = GetColorF(0.020f, 0.024f, 0.032f, 1.0f);
        p.targetRainCount = 0;
        p.targetSnowCount = 450;
        p.rainSpeed = 20.0f;
        p.snowSpeed = 4.8f;
        p.windVelocity = VGet(2.2f, 0.0f, 1.2f);
        break;

    case WeatherType::Foggy:
        // 濃霧（立ち込める暗霧: 4.0m〜17.0m、10m以遠は暗闇の影）
        p.fogStart = 4.0f;
        p.fogEnd = 17.0f;
        p.fogR = 6;
        p.fogG = 9;
        p.fogB = 13;
        p.ambientColor = GetColorF(0.022f, 0.025f, 0.034f, 1.0f);
        p.targetRainCount = 0;
        p.targetSnowCount = 0;
        p.rainSpeed = 20.0f;
        p.snowSpeed = 3.0f;
        p.windVelocity = VGet(0.0f, 0.0f, 0.0f);
        break;

    case WeatherType::ThickFog:
        // 超濃霧（丑三つ時の極限恐怖暗霧: 2.5m〜11.0m、15m以遠は完全消失）
        p.fogStart = 2.5f;
        p.fogEnd = 11.0f;
        p.fogR = 6;
        p.fogG = 8;
        p.fogB = 12;
        p.ambientColor = GetColorF(0.016f, 0.018f, 0.026f, 1.0f);
        p.targetRainCount = 0;
        p.targetSnowCount = 0;
        p.rainSpeed = 20.0f;
        p.snowSpeed = 3.0f;
        p.windVelocity = VGet(0.0f, 0.0f, 0.0f);
        break;
    }

    return p;
}

// 重み付き確率およびゲーム内時間帯による次天候決定（天候を特定時刻で強制せず動的推移を維持）
WeatherType WeatherManager::ChooseNextWeather(WeatherType current)
{
    int roll = GetRand(99);
    int timeLevel = HorrorUI::Instance().GetHorrorTimeLevel();

    switch (current)
    {
    case WeatherType::Clear:
        // Clearからの推移
        // 01時台: 穏やかな天候(Rain/Clear)が高め、03時台以降: 霧の発生確率が上昇
        if (timeLevel >= 2)
        {
            if (roll < 30) return WeatherType::Rain;
            if (roll < 75) return WeatherType::Foggy;   // 霧の発生率上昇
            if (roll < 90) return WeatherType::Snow;
            return WeatherType::Clear;
        }
        else
        {
            if (roll < 40) return WeatherType::Rain;
            if (roll < 70) return WeatherType::Foggy;
            if (roll < 88) return WeatherType::Snow;
            return WeatherType::Clear;
        }

    case WeatherType::Rain:
        // Rainからの推移: 深夜が進むほど豪雨への発展確率が微増
        if (timeLevel >= 2)
        {
            if (roll < 25) return WeatherType::Clear;
            if (roll < 70) return WeatherType::HeavyRain;
            if (roll < 95) return WeatherType::Foggy;
            return WeatherType::Rain;
        }
        else
        {
            if (roll < 35) return WeatherType::Clear;
            if (roll < 65) return WeatherType::HeavyRain;
            if (roll < 95) return WeatherType::Foggy;
            return WeatherType::Rain;
        }

    case WeatherType::HeavyRain:
        // HeavyRainからの推移: Rain 80%, Foggy 20%（直接の晴天化を防止）
        if (roll < 80) return WeatherType::Rain;
        return WeatherType::Foggy;

    case WeatherType::Snow:
        // Snowからの推移: Clear 40%, HeavySnow 35%, Foggy 25%
        if (roll < 40) return WeatherType::Clear;
        if (roll < 75) return WeatherType::HeavySnow;
        return WeatherType::Foggy;

    case WeatherType::HeavySnow:
        // HeavySnowからの推移: Snow 85%, Foggy 15%
        if (roll < 85) return WeatherType::Snow;
        return WeatherType::Foggy;

    case WeatherType::Foggy:
        // Foggyからの推移: 03時台以降（丑三つ時）は超濃霧(ThickFog)への発展確率が上昇
        if (timeLevel >= 2)
        {
            if (roll < 25) return WeatherType::Clear;
            if (roll < 72) return WeatherType::ThickFog;   // 03時台: 超濃霧確率47%
            if (roll < 88) return WeatherType::Rain;
            return WeatherType::Snow;
        }
        else if (timeLevel == 1)
        {
            if (roll < 35) return WeatherType::Clear;
            if (roll < 68) return WeatherType::ThickFog;   // 02時台: 超濃霧確率33%
            if (roll < 88) return WeatherType::Rain;
            return WeatherType::Snow;
        }
        else
        {
            if (roll < 45) return WeatherType::Clear;
            if (roll < 65) return WeatherType::ThickFog;   // 01時台: 超濃霧確率20%
            if (roll < 88) return WeatherType::Rain;
            return WeatherType::Snow;
        }

    case WeatherType::ThickFog:
        // ThickFogからの推移: Foggy 90%, Rain 10%（必ず一段階薄い霧に戻る）
        if (roll < 90) return WeatherType::Foggy;
        return WeatherType::Rain;
    }

    return WeatherType::Clear;
}

// 特定天候へのスムーズ遷移
void WeatherManager::TransitionTo(WeatherType newWeather, float durationSeconds)
{
    if (newWeather == currentWeather && !isTransitioning)
    {
        return;
    }

    targetWeather = newWeather;
    sourceParams = currentParams;
    destParams = GetPresetParameters(newWeather);
    transitionDuration = (durationSeconds > 0.5f) ? durationSeconds : 0.5f;
    transitionProgress = 0.0f;
    isTransitioning = true;
}

// 手動天候順送り切り替え (F7)
void WeatherManager::CycleWeather()
{
    WeatherType next;
    switch (targetWeather)
    {
    case WeatherType::Clear:     next = WeatherType::Rain; break;
    case WeatherType::Rain:      next = WeatherType::HeavyRain; break;
    case WeatherType::HeavyRain: next = WeatherType::Snow; break;
    case WeatherType::Snow:      next = WeatherType::HeavySnow; break;
    case WeatherType::HeavySnow: next = WeatherType::Foggy; break;
    case WeatherType::Foggy:     next = WeatherType::ThickFog; break;
    case WeatherType::ThickFog:  next = WeatherType::Clear; break;
    default:                     next = WeatherType::Clear; break;
    }

    TransitionTo(next, 7.0f);
}

// 自動天候変化の有効/無効切り替え (F9)
void WeatherManager::ToggleAutoWeather()
{
    autoWeatherEnabled = !autoWeatherEnabled;
    if (autoWeatherEnabled)
    {
        nextWeatherDuration = 60.0f + static_cast<float>(GetRand(120));
        weatherTimer = nextWeatherDuration;
    }
}

// デバッグ表示ON/OFF切り替え (F10)
void WeatherManager::ToggleDebugHUD()
{
    showDebugHUD = !showDebugHUD;
}

// 超濃霧の強制開始
void WeatherManager::ForceThickFog(float durationSeconds)
{
    TransitionTo(WeatherType::ThickFog, durationSeconds);
}

// 7日間天候設定
void WeatherManager::SetDayWeather(int day, NightEnvironment& nightEnv)
{
    if (day < 1) day = 1;
    if (day > 7) day = 7;

    WeatherType dayWeather = WeatherType::Clear;
    switch (day)
    {
    case 1: dayWeather = WeatherType::Clear; break;
    case 2: dayWeather = WeatherType::Rain; break;
    case 3: dayWeather = WeatherType::Foggy; break;
    case 4: dayWeather = WeatherType::HeavyRain; break;
    case 5: dayWeather = WeatherType::Snow; break;
    case 6: dayWeather = WeatherType::HeavySnow; break;
    case 7:
    default:
        dayWeather = WeatherType::ThickFog; break;
    }

    currentWeather = dayWeather;
    targetWeather = dayWeather;
    currentParams = GetPresetParameters(dayWeather);
    destParams = currentParams;
    sourceParams = currentParams;
    isTransitioning = false;
    transitionProgress = 1.0f;
    weatherTimer = 90.0f;

    // 即座に環境側へフォグ・照明パラメータを反映
    nightEnv.SetWeatherFogAndLighting(
        currentParams.fogStart,
        currentParams.fogEnd,
        currentParams.fogR,
        currentParams.fogG,
        currentParams.fogB,
        currentParams.ambientColor
    );
}

// 天候名称文字列取得
const char* WeatherManager::GetWeatherName(WeatherType type) const
{
    switch (type)
    {
    case WeatherType::Clear:     return "CLEAR / NORMAL NIGHT";
    case WeatherType::Rain:      return "RAIN";
    case WeatherType::HeavyRain: return "HEAVY RAIN";
    case WeatherType::Snow:      return "SNOW";
    case WeatherType::HeavySnow: return "HEAVY SNOW";
    case WeatherType::Foggy:     return "FOGGY";
    case WeatherType::ThickFog:  return "THICK FOG";
    }
    return "UNKNOWN";
}

// 毎フレーム更新処理
void WeatherManager::Update(float dt, const Player& player, const Bicycle& bicycle, NightEnvironment& nightEnv)
{
    // 自動天候切り替えタイマー更新
    if (autoWeatherEnabled && !isTransitioning && !isExtremeFogActive && !isFogOverrideActive)
    {
        weatherTimer -= dt;
        if (weatherTimer <= 0.0f)
        {
            WeatherType next = ChooseNextWeather(currentWeather);
            TransitionTo(next, 8.0f);
            nextWeatherDuration = 60.0f + static_cast<float>(GetRand(120));
            weatherTimer = nextWeatherDuration;
        }
    }

    // 遷移進捗更新とパラメータ線形補間
    if (isTransitioning)
    {
        transitionProgress += dt / transitionDuration;
        if (transitionProgress >= 1.0f)
        {
            transitionProgress = 1.0f;
            isTransitioning = false;
            currentWeather = targetWeather;
            currentParams = destParams;
        }
        else
        {
            float t = transitionProgress;
            currentParams.fogStart = LerpFloat(sourceParams.fogStart, destParams.fogStart, t);
            currentParams.fogEnd = LerpFloat(sourceParams.fogEnd, destParams.fogEnd, t);
            currentParams.fogR = LerpInt(sourceParams.fogR, destParams.fogR, t);
            currentParams.fogG = LerpInt(sourceParams.fogG, destParams.fogG, t);
            currentParams.fogB = LerpInt(sourceParams.fogB, destParams.fogB, t);
            currentParams.ambientColor = LerpColorF(sourceParams.ambientColor, destParams.ambientColor, t);
            currentParams.targetRainCount = LerpInt(sourceParams.targetRainCount, destParams.targetRainCount, t);
            currentParams.targetSnowCount = LerpInt(sourceParams.targetSnowCount, destParams.targetSnowCount, t);
            currentParams.rainSpeed = LerpFloat(sourceParams.rainSpeed, destParams.rainSpeed, t);
            currentParams.snowSpeed = LerpFloat(sourceParams.snowSpeed, destParams.snowSpeed, t);
            currentParams.windVelocity = LerpVector(sourceParams.windVelocity, destParams.windVelocity, t);
        }
    }

    // ホラーイベント用一時的フォグ圧縮処理
    if (isFogOverrideActive)
    {
        overrideTimer += dt;
        if (overridePhase == 1)
        {
            // フェーズ1: 1.5秒かけて目標圧縮フォグへ遷移
            float t = overrideTimer / 1.5f;
            if (t >= 1.0f)
            {
                t = 1.0f;
                overridePhase = 2;
                overrideTimer = 0.0f;
            }
            currentParams.fogEnd = LerpFloat(overrideBaseFogEnd, overrideTargetFogEnd, t);
        }
        else if (overridePhase == 2)
        {
            // フェーズ2: 目標フォグを一定時間維持
            currentParams.fogEnd = overrideTargetFogEnd;
            if (overrideTimer >= overrideDuration)
            {
                overridePhase = 3;
                overrideTimer = 0.0f;
            }
        }
        else if (overridePhase == 3)
        {
            // フェーズ3: 復帰時間かけて元の天候フォグへ徐々に戻る
            float t = (overrideRecoverDuration > 0.1f) ? (overrideTimer / overrideRecoverDuration) : 1.0f;
            if (t >= 1.0f)
            {
                t = 1.0f;
                isFogOverrideActive = false;
                overridePhase = 0;
            }
            currentParams.fogEnd = LerpFloat(overrideTargetFogEnd, overrideBaseFogEnd, t);
        }
    }

    // エクストリームフォグ（F8デバッグ用）の上書き適用
    if (isExtremeFogActive)
    {
        currentParams.fogStart = 1.5f;
        currentParams.fogEnd = 8.0f;
        currentParams.fogR = 5;
        currentParams.fogG = 7;
        currentParams.fogB = 10;
        currentParams.ambientColor = GetColorF(0.012f, 0.014f, 0.020f, 1.0f);
    }

    // 環境システムへのフォグおよびアンビエント光の伝達
    nightEnv.SetWeatherFogAndLighting(
        currentParams.fogStart,
        currentParams.fogEnd,
        currentParams.fogR,
        currentParams.fogG,
        currentParams.fogB,
        currentParams.ambientColor
    );

    // フォグ濃度に応じた自転車ヘッドライト到達距離の動的調整
    // （濃霧時にヘッドライトが通り全体を見通すのを防ぎ、手元・路面のみを確実に照らす）
    float hlRange = 16.0f;
    float atten1 = 0.08f;
    float atten2 = 0.018f;
    if (currentParams.fogEnd <= 9.0f)
    {
        hlRange = 8.5f;
        atten1 = 0.16f;
        atten2 = 0.050f;
    }
    else if (currentParams.fogEnd <= 12.0f)
    {
        hlRange = 10.5f;
        atten1 = 0.12f;
        atten2 = 0.035f;
    }
    else if (currentParams.fogEnd <= 18.0f)
    {
        hlRange = 13.0f;
        atten1 = 0.10f;
        atten2 = 0.025f;
    }
    nightEnv.SetBicycleHeadlightParams(hlRange, 1.0f, atten1, atten2);

    // プレイヤー位置を中心とするパーティクル更新
    VECTOR playerPos = player.GetPosition();
    UpdateRain(dt, playerPos);
    UpdateSnow(dt, playerPos);
}

// 雨パーティクルの更新
void WeatherManager::UpdateRain(float dt, const VECTOR& playerPos)
{
    const float radius = 14.0f;
    const float radiusSq = radius * radius;
    int activeTarget = currentParams.targetRainCount;

    for (int i = 0; i < MAX_RAIN_COUNT; ++i)
    {
        if (i >= activeTarget)
        {
            rainPool[i].active = false;
            continue;
        }

        rainPool[i].active = true;

        // 雨粒の落下移動
        rainPool[i].position.y -= (rainPool[i].speed * dt);
        rainPool[i].position.x += (currentParams.windVelocity.x * dt);
        rainPool[i].position.z += (currentParams.windVelocity.z * dt);

        // プレイヤーからの水平距離計算
        float dx = rainPool[i].position.x - playerPos.x;
        float dz = rainPool[i].position.z - playerPos.z;
        float distSq = dx * dx + dz * dz;

        // 地面への着地またはエリア外への離脱時に上空へリサイクル配置
        if (rainPool[i].position.y < 0.0f || distSq > radiusSq)
        {
            float ang = static_cast<float>(GetRand(628)) / 100.0f;
            float r = std::sqrt(static_cast<float>(GetRand(1000)) / 1000.0f) * radius;
            rainPool[i].position.x = playerPos.x + std::cos(ang) * r;
            rainPool[i].position.z = playerPos.z + std::sin(ang) * r;
            rainPool[i].position.y = playerPos.y + 6.0f + static_cast<float>(GetRand(500)) / 100.0f;
            rainPool[i].speed = currentParams.rainSpeed + static_cast<float>(GetRand(40)) / 10.0f;
        }
    }
}

// 雪パーティクルの更新
void WeatherManager::UpdateSnow(float dt, const VECTOR& playerPos)
{
    const float radius = 13.0f;
    const float radiusSq = radius * radius;
    int activeTarget = currentParams.targetSnowCount;

    for (int i = 0; i < MAX_SNOW_COUNT; ++i)
    {
        if (i >= activeTarget)
        {
            snowPool[i].active = false;
            continue;
        }

        snowPool[i].active = true;

        // 粉雪の揺らぎフェーズ更新と降下移動
        snowPool[i].wobblePhase += snowPool[i].wobbleSpeed * dt;
        float wobbleX = std::sin(snowPool[i].wobblePhase) * snowPool[i].driftStrength;
        float wobbleZ = std::cos(snowPool[i].wobblePhase * 0.8f) * snowPool[i].driftStrength;

        snowPool[i].position.y -= (snowPool[i].fallSpeed * dt);
        snowPool[i].position.x += (currentParams.windVelocity.x + wobbleX) * dt;
        snowPool[i].position.z += (currentParams.windVelocity.z + wobbleZ) * dt;

        float dx = snowPool[i].position.x - playerPos.x;
        float dz = snowPool[i].position.z - playerPos.z;
        float distSq = dx * dx + dz * dz;

        // リサイクル配置
        if (snowPool[i].position.y < 0.0f || distSq > radiusSq)
        {
            float ang = static_cast<float>(GetRand(628)) / 100.0f;
            float r = std::sqrt(static_cast<float>(GetRand(1000)) / 1000.0f) * radius;
            snowPool[i].position.x = playerPos.x + std::cos(ang) * r;
            snowPool[i].position.z = playerPos.z + std::sin(ang) * r;
            snowPool[i].position.y = playerPos.y + 5.5f + static_cast<float>(GetRand(450)) / 100.0f;
            snowPool[i].fallSpeed = currentParams.snowSpeed + static_cast<float>(GetRand(15)) / 10.0f;
        }
    }
}

// 街灯および自転車ヘッドライトによるパーティクル照明判定
bool WeatherManager::CheckParticleIllumination(const VECTOR& pos, const Bicycle& bicycle, const NightEnvironment& nightEnv) const
{
    // 自転車ヘッドライトのコーン判定
    if (bicycle.IsHeadlightOn() && bicycle.GetHeadlightIntensity() > 0.05f)
    {
        VECTOR lightPos = bicycle.GetHeadlightPosition();
        VECTOR lightDir = bicycle.GetHeadlightDirection();
        float range = bicycle.GetHeadlightRange();

        VECTOR toParticle = VSub(pos, lightPos);
        float dist = VSize(toParticle);
        if (dist > 0.1f && dist < range)
        {
            VECTOR normToP = VScale(toParticle, 1.0f / dist);
            float dot = VDot(normToP, lightDir);
            // スポットライト外側角度42度 (cos(42度) 約0.743)
            if (dot > 0.743f)
            {
                return true;
            }
        }
    }

    // 街灯ポイントライトの距離判定
    const std::vector<StreetLight>& lights = nightEnv.GetStreetLights();
    for (size_t i = 0; i < lights.size(); ++i)
    {
        if (lights[i].state == StreetLightState::Off || lights[i].currentIntensity < 0.05f)
        {
            continue;
        }

        VECTOR lampPos = VGet(lights[i].position.x, lights[i].position.y + 4.0f, lights[i].position.z);
        VECTOR toP = VSub(pos, lampPos);
        float dist = VSize(toP);
        if (dist < 9.5f)
        {
            return true;
        }
    }

    return false;
}

// 3Dパーティクル描画処理
void WeatherManager::DrawParticles(const Bicycle& bicycle, const NightEnvironment& nightEnv)
{
    // 雨パーティクル描画
    if (currentParams.targetRainCount > 0)
    {
        for (int i = 0; i < currentParams.targetRainCount; ++i)
        {
            if (!rainPool[i].active) continue;

            const VECTOR& p = rainPool[i].position;
            VECTOR dropDir = VGet(
                currentParams.windVelocity.x * 0.02f,
                -rainPool[i].length,
                currentParams.windVelocity.z * 0.02f
            );
            VECTOR pEnd = VAdd(p, dropDir);

            bool illuminated = CheckParticleIllumination(p, bicycle, nightEnv);

            if (illuminated)
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
                DrawLine3D(p, pEnd, GetColor(215, 225, 240));
            }
            else
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 110);
                DrawLine3D(p, pEnd, GetColor(95, 110, 125));
            }
        }
    }

    // 雪パーティクル描画
    if (currentParams.targetSnowCount > 0)
    {
        for (int i = 0; i < currentParams.targetSnowCount; ++i)
        {
            if (!snowPool[i].active) continue;

            const VECTOR& p = snowPool[i].position;
            float sz = snowPool[i].size;
            bool illuminated = CheckParticleIllumination(p, bicycle, nightEnv);

            if (illuminated)
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 240);
                unsigned int c = GetColor(235, 240, 255);
                DrawLine3D(p, VAdd(p, VGet(0.0f, -sz, 0.0f)), c);
                DrawLine3D(VAdd(p, VGet(-sz * 0.5f, -sz * 0.5f, 0.0f)), VAdd(p, VGet(sz * 0.5f, -sz * 0.5f, 0.0f)), c);
            }
            else
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
                unsigned int c = GetColor(155, 165, 180);
                DrawLine3D(p, VAdd(p, VGet(0.0f, -sz, 0.0f)), c);
                DrawLine3D(VAdd(p, VGet(-sz * 0.5f, -sz * 0.5f, 0.0f)), VAdd(p, VGet(sz * 0.5f, -sz * 0.5f, 0.0f)), c);
            }
        }
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 天候・フォグデバッグHUD描画 (F10)
void WeatherManager::DrawDebugHUD()
{
    if (!showDebugHUD) return;

    HorrorUI::Instance().DrawRetroPanel(18, 246, 380, 440,
        HorrorUI::COL_BORDER, HorrorUI::COL_PANEL_BG, 225);

    HorrorUI::Instance().DrawJitterString(26, 252, "[ WEATHER & FOG DEBUG (F10) ]",
        GetColor(230, 190, 80), HorrorUI::Instance().GetFontSmall());

    char buf[128];
    const char* wName = isExtremeFogActive ? "EXTREME FOG (DEBUG)" : GetWeatherName(currentWeather);
    snprintf(buf, sizeof(buf), "WEATHER:    %s", wName);
    HorrorUI::Instance().DrawJitterString(26, 274, buf,
        isExtremeFogActive ? GetColor(245, 90, 75) : HorrorUI::COL_TEXT, HorrorUI::Instance().GetFontSmall());

    snprintf(buf, sizeof(buf), "FOG START:  %.1fm", currentParams.fogStart);
    HorrorUI::Instance().DrawJitterString(26, 292, buf,
        HorrorUI::COL_TEXT, HorrorUI::Instance().GetFontSmall());

    snprintf(buf, sizeof(buf), "FOG END:    %.1fm", currentParams.fogEnd);
    HorrorUI::Instance().DrawJitterString(26, 310, buf,
        GetColor(220, 150, 90), HorrorUI::Instance().GetFontSmall());

    snprintf(buf, sizeof(buf), "FOG COLOR:  RGB(%d, %d, %d)",
        currentParams.fogR, currentParams.fogG, currentParams.fogB);
    HorrorUI::Instance().DrawJitterString(26, 328, buf,
        HorrorUI::COL_TEXT_DIM, HorrorUI::Instance().GetFontSmall());

    // 視界評価の判定
    const char* visStr = "MODERATE (NIGHT)";
    if (currentParams.fogEnd <= 8.5f) visStr = "EXTREME (CRITICAL)";
    else if (currentParams.fogEnd <= 12.0f) visStr = "VERY LOW";
    else if (currentParams.fogEnd <= 18.0f) visStr = "LOW";
    else if (currentParams.fogEnd > 30.0f) visStr = "CLEAR";

    snprintf(buf, sizeof(buf), "VISIBILITY: %s", visStr);
    HorrorUI::Instance().DrawJitterString(26, 346, buf,
        (currentParams.fogEnd <= 12.0f) ? GetColor(240, 75, 75) : GetColor(110, 195, 145), HorrorUI::Instance().GetFontSmall());

    snprintf(buf, sizeof(buf), "AUTO:       %s", autoWeatherEnabled ? "ON" : "OFF");
    HorrorUI::Instance().DrawJitterString(26, 364, buf,
        HorrorUI::COL_TEXT_DIM, HorrorUI::Instance().GetFontSmall());

    snprintf(buf, sizeof(buf), "PARTICLES:  Rain %d / Snow %d", currentParams.targetRainCount, currentParams.targetSnowCount);
    HorrorUI::Instance().DrawJitterString(26, 382, buf,
        HorrorUI::COL_TEXT_DIM, HorrorUI::Instance().GetFontSmall());

    HorrorUI::Instance().DrawJitterString(26, 404, "[F7:Cycle] [F8:Extreme Fog] [F9:Auto] [F10:HUD]",
        GetColor(130, 160, 150), HorrorUI::Instance().GetFontSmall());
}

// エクストリームフォグ（極限超濃霧テスト・F8キー）の切り替え
void WeatherManager::ToggleExtremeFog()
{
    isExtremeFogActive = !isExtremeFogActive;
    if (isExtremeFogActive)
    {
        preExtremeParams = currentParams;
        currentParams.fogStart = 1.5f;
        currentParams.fogEnd = 8.0f;
        currentParams.fogR = 5;
        currentParams.fogG = 7;
        currentParams.fogB = 10;
        currentParams.ambientColor = GetColorF(0.012f, 0.014f, 0.020f, 1.0f);
    }
    else
    {
        currentParams = preExtremeParams;
    }
}

// ホラーイベント用一時的フォグ圧縮制御
void WeatherManager::TriggerTemporaryFogOverride(float targetFogEnd, float durationSeconds, float recoverDurationSeconds)
{
    isFogOverrideActive = true;
    overridePhase = 1;
    overrideTargetFogEnd = targetFogEnd;
    overrideBaseFogEnd = currentParams.fogEnd;
    overrideTimer = 0.0f;
    overrideDuration = durationSeconds;
    overrideRecoverDuration = recoverDurationSeconds;
}

void WeatherManager::ClearFogOverride()
{
    isFogOverrideActive = false;
    overridePhase = 0;
}

// 終了処理
void WeatherManager::Finalize()
{
    // 将来のサウンドハンドル破棄フック
    if (rainSoundHandle != -1)
    {
        DeleteSoundMem(rainSoundHandle);
        rainSoundHandle = -1;
    }
    if (windSoundHandle != -1)
    {
        DeleteSoundMem(windSoundHandle);
        windSoundHandle = -1;
    }
    if (snowSoundHandle != -1)
    {
        DeleteSoundMem(snowSoundHandle);
        snowSoundHandle = -1;
    }
}
