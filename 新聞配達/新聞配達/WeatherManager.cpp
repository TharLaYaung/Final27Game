#include "WeatherManager.h"
#include "Player.h"
#include "Bicycle.h"
#include "NightEnvironment.h"
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
    , showDebugHUD(false)
    , rainSoundHandle(-1)
    , windSoundHandle(-1)
    , snowSoundHandle(-1)
{
    currentParams = GetPresetParameters(WeatherType::Clear);
    sourceParams = currentParams;
    destParams = currentParams;

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

    currentParams = GetPresetParameters(currentWeather);
    sourceParams = currentParams;
    destParams = currentParams;

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
        p.fogStart = 5.0f;
        p.fogEnd = 22.0f;
        p.fogR = 8;
        p.fogG = 12;
        p.fogB = 16;
        p.ambientColor = GetColorF(0.035f, 0.040f, 0.055f, 1.0f);
        p.targetRainCount = 0;
        p.targetSnowCount = 0;
        p.rainSpeed = 20.0f;
        p.snowSpeed = 3.0f;
        p.windVelocity = VGet(0.0f, 0.0f, 0.0f);
        break;

    case WeatherType::Rain:
        p.fogStart = 4.5f;
        p.fogEnd = 20.0f;
        p.fogR = 7;
        p.fogG = 10;
        p.fogB = 14;
        p.ambientColor = GetColorF(0.030f, 0.035f, 0.048f, 1.0f);
        p.targetRainCount = 320;
        p.targetSnowCount = 0;
        p.rainSpeed = 22.0f;
        p.snowSpeed = 3.0f;
        p.windVelocity = VGet(1.4f, 0.0f, 0.7f);
        break;

    case WeatherType::HeavyRain:
        p.fogStart = 3.5f;
        p.fogEnd = 15.0f;
        p.fogR = 6;
        p.fogG = 9;
        p.fogB = 12;
        p.ambientColor = GetColorF(0.022f, 0.025f, 0.035f, 1.0f);
        p.targetRainCount = 680;
        p.targetSnowCount = 0;
        p.rainSpeed = 29.0f;
        p.snowSpeed = 3.0f;
        p.windVelocity = VGet(3.0f, 0.0f, 1.5f);
        break;

    case WeatherType::Snow:
        p.fogStart = 4.5f;
        p.fogEnd = 21.0f;
        p.fogR = 9;
        p.fogG = 13;
        p.fogB = 18;
        p.ambientColor = GetColorF(0.038f, 0.042f, 0.058f, 1.0f);
        p.targetRainCount = 0;
        p.targetSnowCount = 200;
        p.rainSpeed = 20.0f;
        p.snowSpeed = 3.2f;
        p.windVelocity = VGet(0.8f, 0.0f, 0.4f);
        break;

    case WeatherType::HeavySnow:
        p.fogStart = 3.0f;
        p.fogEnd = 14.0f;
        p.fogR = 8;
        p.fogG = 11;
        p.fogB = 16;
        p.ambientColor = GetColorF(0.032f, 0.035f, 0.048f, 1.0f);
        p.targetRainCount = 0;
        p.targetSnowCount = 450;
        p.rainSpeed = 20.0f;
        p.snowSpeed = 4.8f;
        p.windVelocity = VGet(2.2f, 0.0f, 1.2f);
        break;

    case WeatherType::Foggy:
        p.fogStart = 4.0f;
        p.fogEnd = 18.0f;
        p.fogR = 7;
        p.fogG = 10;
        p.fogB = 15;
        p.ambientColor = GetColorF(0.028f, 0.032f, 0.045f, 1.0f);
        p.targetRainCount = 0;
        p.targetSnowCount = 0;
        p.rainSpeed = 20.0f;
        p.snowSpeed = 3.0f;
        p.windVelocity = VGet(0.0f, 0.0f, 0.0f);
        break;

    case WeatherType::ThickFog:
        p.fogStart = 2.5f;
        p.fogEnd = 11.0f;
        p.fogR = 6;
        p.fogG = 9;
        p.fogB = 13;
        p.ambientColor = GetColorF(0.020f, 0.023f, 0.032f, 1.0f);
        p.targetRainCount = 0;
        p.targetSnowCount = 0;
        p.rainSpeed = 20.0f;
        p.snowSpeed = 3.0f;
        p.windVelocity = VGet(0.0f, 0.0f, 0.0f);
        break;
    }

    return p;
}

// 重み付き確率による現実的な次天候決定
WeatherType WeatherManager::ChooseNextWeather(WeatherType current)
{
    int roll = GetRand(99);

    switch (current)
    {
    case WeatherType::Clear:
        // Clearからの推移: Rain 35%, Foggy 35%, Snow 20%, Clear維持 10%
        if (roll < 35) return WeatherType::Rain;
        if (roll < 70) return WeatherType::Foggy;
        if (roll < 90) return WeatherType::Snow;
        return WeatherType::Clear;

    case WeatherType::Rain:
        // Rainからの推移: Clear 35%, HeavyRain 35%, Foggy 25%, Rain維持 5%
        if (roll < 35) return WeatherType::Clear;
        if (roll < 70) return WeatherType::HeavyRain;
        if (roll < 95) return WeatherType::Foggy;
        return WeatherType::Rain;

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
        // Foggyからの推移: Clear 40%, ThickFog 30%, Rain 20%, Snow 10%
        if (roll < 40) return WeatherType::Clear;
        if (roll < 70) return WeatherType::ThickFog;
        if (roll < 90) return WeatherType::Rain;
        return WeatherType::Snow;

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
    if (autoWeatherEnabled && !isTransitioning)
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

    // 環境システムへのフォグおよびアンビエント光の伝達
    nightEnv.SetWeatherFogAndLighting(
        currentParams.fogStart,
        currentParams.fogEnd,
        currentParams.fogR,
        currentParams.fogG,
        currentParams.fogB,
        currentParams.ambientColor
    );

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

// 天候デバッグHUD描画
void WeatherManager::DrawDebugHUD()
{
    if (!showDebugHUD) return;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 215);
    DrawBox(18, 258, 350, 420, GetColor(8, 12, 16), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(18, 258, 350, 420, GetColor(60, 110, 130), FALSE);

    DrawString(26, 264, "[ WEATHER DEBUG (F10) ]", GetColor(255, 210, 80));

    char buf[128];
    snprintf(buf, sizeof(buf), "CURRENT:  %s", GetWeatherName(currentWeather));
    DrawString(26, 286, buf, GetColor(210, 230, 255));

    if (isTransitioning)
    {
        snprintf(buf, sizeof(buf), "TARGET:   %s (%d%%)", GetWeatherName(targetWeather), static_cast<int>(transitionProgress * 100.0f));
        DrawString(26, 304, buf, GetColor(255, 180, 100));
    }
    else
    {
        DrawString(26, 304, "TARGET:   STABLE", GetColor(140, 180, 160));
    }

    if (autoWeatherEnabled)
    {
        snprintf(buf, sizeof(buf), "AUTO:     ON (Next in %.0fs)", weatherTimer);
    }
    else
    {
        snprintf(buf, sizeof(buf), "AUTO:     OFF (Manual Only)");
    }
    DrawString(26, 322, buf, GetColor(180, 205, 200));

    snprintf(buf, sizeof(buf), "FOG:      %.1fm - %.1fm", currentParams.fogStart, currentParams.fogEnd);
    DrawString(26, 340, buf, GetColor(160, 190, 220));

    snprintf(buf, sizeof(buf), "RAIN:     %d / %d", currentParams.targetRainCount, MAX_RAIN_COUNT);
    DrawString(26, 358, buf, GetColor(160, 190, 220));

    snprintf(buf, sizeof(buf), "SNOW:     %d / %d", currentParams.targetSnowCount, MAX_SNOW_COUNT);
    DrawString(26, 376, buf, GetColor(160, 190, 220));

    DrawString(26, 396, "[F7:Cycle] [F9:Auto] [F10:HUD]", GetColor(130, 150, 160));
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
