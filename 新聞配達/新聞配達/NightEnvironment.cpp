#include "NightEnvironment.h"
#include "Bicycle.h"
#include "HorrorUI.h"
#include "Map.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <algorithm>

namespace
{
    int CreateRadialBillboardTexture(int size, bool withMoonSurface)
    {
        int soft = MakeARGB8ColorSoftImage(size, size);
        if (soft == -1) return -1;

        for (int y = 0; y < size; ++y)
        {
            float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(size) * 2.0f - 1.0f;
            for (int x = 0; x < size; ++x)
            {
                float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(size) * 2.0f - 1.0f;
                float r = sqrtf(u * u + v * v);
                if (r >= 1.0f)
                {
                    DrawPixelSoftImage(soft, x, y, 0, 0, 0, 0);
                    continue;
                }

                if (withMoonSurface)
                {
                    // 月本体（半径0.26）と柔らかな大気ハロー
                    float halo = expf(-r * r * 6.5f) * 0.35f;
                    float disc = 0.0f;
                    if (r < 0.27f)
                    {
                        float edge = std::clamp((0.27f - r) / 0.03f, 0.0f, 1.0f);
                        float maria = 0.86f + 0.14f * sinf(u * 17.0f + cosf(v * 13.0f));
                        // 曲線状の三日月・弦月の陰影マスク
                        float shadowDx = u - 0.11f;
                        float shadowDy = v + 0.05f;
                        float shadowDist = sqrtf(shadowDx * shadowDx + shadowDy * shadowDy);
                        float crescent = std::clamp((shadowDist - 0.19f) / 0.05f, 0.12f, 1.0f);
                        disc = edge * maria * crescent;
                    }
                    float intensity = (std::max)(disc, halo);
                    int alpha = static_cast<int>(255.0f * std::clamp(intensity, 0.0f, 1.0f));
                    DrawPixelSoftImage(soft, x, y, 225, 234, 248, alpha);
                }
                else
                {
                    float radial = 1.0f - r;
                    float glow = expf(-r * r * 5.2f) * 0.70f + expf(-r * r * 18.0f) * 0.30f;
                    glow *= radial * radial;
                    int alpha = static_cast<int>(255.0f * std::clamp(glow, 0.0f, 1.0f));
                    DrawPixelSoftImage(soft, x, y, 255, 255, 255, alpha);
                }
            }
        }

        int tex = CreateGraphFromSoftImage(soft);
        DeleteSoftImage(soft);
        return tex;
    }

    int CreatePoleSteelTexture(int size)
    {
        int soft = MakeARGB8ColorSoftImage(size, size);
        if (soft == -1) return -1;

        for (int y = 0; y < size; ++y)
        {
            float vf = static_cast<float>(y) / static_cast<float>(size);
            for (int x = 0; x < size; ++x)
            {
                float uf = static_cast<float>(x) / static_cast<float>(size);
                uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u;
                h = (h ^ (h >> 13)) * 1274126177u;
                float n = static_cast<float>((h ^ (h >> 16)) & 0xFFu) / 255.0f;
                float streak = sinf(uf * DX_PI_F * 12.0f) * 6.0f + cosf(vf * DX_PI_F * 6.0f) * 5.0f;
                float seam = (fabsf(vf - 0.5f) < 0.015f) ? -14.0f : 0.0f;
                int base = std::clamp(static_cast<int>(130.0f + (n - 0.5f) * 26.0f + streak + seam), 60, 200);
                DrawPixelSoftImage(soft, x, y, base - 4, base, base + 6, 255);
            }
        }

        int tex = CreateGraphFromSoftImage(soft);
        DeleteSoftImage(soft);
        return tex;
    }

    void DrawTexturedTube(
        const VECTOR& p0, float r0,
        const VECTOR& p1, float r1,
        int sides, int texHandle,
        COLOR_U8 dif, COLOR_U8 spc,
        float vScale = 1.0f)
    {
        VECTOR axis = VSub(p1, p0);
        float len = VSize(axis);
        if (len < 0.0001f) return;
        VECTOR dir = VScale(axis, 1.0f / len);

        VECTOR ref = (fabsf(dir.y) < 0.9f) ? VGet(0.0f, 1.0f, 0.0f) : VGet(1.0f, 0.0f, 0.0f);
        VECTOR right = VNorm(VCross(ref, dir));
        VECTOR fwd = VNorm(VCross(dir, right));

        std::vector<VERTEX3D> verts;
        verts.reserve(sides * 6);

        for (int i = 0; i < sides; ++i)
        {
            float a0 = static_cast<float>(i) / static_cast<float>(sides) * DX_PI_F * 2.0f;
            float a1 = static_cast<float>(i + 1) / static_cast<float>(sides) * DX_PI_F * 2.0f;
            float u0 = static_cast<float>(i) / static_cast<float>(sides);
            float u1 = static_cast<float>(i + 1) / static_cast<float>(sides);

            VECTOR n0 = VAdd(VScale(right, cosf(a0)), VScale(fwd, sinf(a0)));
            VECTOR n1 = VAdd(VScale(right, cosf(a1)), VScale(fwd, sinf(a1)));

            VECTOR b0 = VAdd(p0, VScale(n0, r0));
            VECTOR b1 = VAdd(p0, VScale(n1, r0));
            VECTOR t0 = VAdd(p1, VScale(n0, r1));
            VECTOR t1 = VAdd(p1, VScale(n1, r1));

            auto makeV = [&](const VECTOR& pos, const VECTOR& norm, float u, float v) {
                VERTEX3D vert{};
                vert.pos = pos;
                vert.norm = norm;
                vert.dif = dif;
                vert.spc = spc;
                vert.u = u;
                vert.v = v;
                vert.su = u;
                vert.sv = v;
                return vert;
            };

            VERTEX3D vb0 = makeV(b0, n0, u0, 0.0f);
            VERTEX3D vb1 = makeV(b1, n1, u1, 0.0f);
            VERTEX3D vt1 = makeV(t1, n1, u1, vScale);
            VERTEX3D vt0 = makeV(t0, n0, u0, vScale);

            verts.push_back(vb0);
            verts.push_back(vb1);
            verts.push_back(vt1);
            verts.push_back(vb0);
            verts.push_back(vt1);
            verts.push_back(vt0);
        }

        DrawPolygon3D(verts.data(), static_cast<int>(verts.size()) / 3, texHandle, FALSE);
    }
}

// コンストラクタ
NightEnvironment::NightEnvironment()
{
    currentPreset = EnvironmentPreset::NightDeliveryStyle;

    fogEnabled = true;
    fogStart = 4.0f;
    fogEnd = 17.0f;
    fogColorR = 36;
    fogColorG = 46;
    fogColorB = 58;

    bgR = 36;
    bgG = 46;
    bgB = 58;

    ambientColor = GetColorF(0.028f, 0.035f, 0.046f, 1.0f);

    bicycleLightHandle = -1;
    bicycleLightRange = 12.0f;
    bicycleLightPower = 1.0f;
    bicycleLightColor = GetColorF(1.0f, 0.94f, 0.78f, 1.0f);

    cachedHeadlightActive = false;
    cachedHeadlightPos = VGet(0.0f, 0.0f, 0.0f);
    cachedHeadlightDir = VGet(0.0f, 0.0f, 1.0f);
    cachedHeadlightIntensity = 0.0f;
    cachedHeadlightRange = 12.0f;

    visualAssetsInitialized = false;
    glowBillboardTex = -1;
    moonDiscTex = -1;
    poleSteelTex = -1;

    streetLightsEnabled = true;
    manualPresetOverride = false;
    blackoutTimer = 0;
    debugNotifyTimer = 0;
    debugMessage[0] = '\0';
}

// デストラクタ
NightEnvironment::~NightEnvironment()
{
    Finalize();
}

// 初期化処理
bool NightEnvironment::Initialize()
{
    // 自転車用スポットライト生成（前方を下向きに照らす）
    bicycleLightHandle = CreateSpotLightHandle(
        VGet(0.0f, 1.0f, 0.0f),
        VGet(0.0f, -0.15f, 1.0f),
        82.0f * DX_PI_F / 180.0f, // 外側コーン角度
        56.0f * DX_PI_F / 180.0f, // 内側コーン角度
        bicycleLightRange,
        1.0f,
        0.035f,
        0.006f
    );

    if (bicycleLightHandle != -1)
    {
        SetLightDifColorHandle(bicycleLightHandle, bicycleLightColor);
        SetLightSpcColorHandle(bicycleLightHandle, GetColorF(0.4f, 0.4f, 0.3f, 1.0f));
    }

    // 街灯の配置設定（住宅街の交差点・路地要所に7基配置、暗い隙間を確保）
    streetLights.clear();

    // 1: スタート地点・南通り入口（通常点灯・温白色）
    streetLights.push_back({ 1, VGet( -5.0f, 0.0f, -27.0f), StreetLightState::Normal,     -1, 0, 0, 1.0f, GetColorF(1.0f, 0.80f, 0.50f, 1.0f) });
    // 2: 南西交差点（Road A & F 交差点・老朽化した弱点灯ランプ・琥珀色）
    streetLights.push_back({ 2, VGet(-22.0f, 0.0f, -27.0f), StreetLightState::Weak,       -1, 0, 0, 0.40f, GetColorF(1.0f, 0.80f, 0.50f, 1.0f) });
    // 3: 北西交差点（Road A & B 交差点・不規則に明滅する水銀灯ホラー街灯）
    streetLights.push_back({ 3, VGet(-22.0f, 0.0f,  12.0f), StreetLightState::Flickering, -1, 120, 0, 1.0f, GetColorF(1.0f, 0.80f, 0.50f, 1.0f) });
    // 4: 北路地入口（Road B & C 交差点・通常点灯）
    streetLights.push_back({ 4, VGet( -3.5f, 0.0f,  17.5f), StreetLightState::Normal,     -1, 0, 0, 1.0f, GetColorF(1.0f, 0.80f, 0.50f, 1.0f) });
    // 5: 北路地最奥行き止まり（住宅2前・球切れ消灯・不気味な暗闇のシルエット）
    streetLights.push_back({ 5, VGet(  3.5f, 0.0f,  38.0f), StreetLightState::Off,        -1, 0, 0, 0.0f, GetColorF(0.0f, 0.0f, 0.0f, 0.0f) });
    // 6: 北東交差点（Road B & D 交差点・通常点灯・温光）
    streetLights.push_back({ 6, VGet( 22.0f, 0.0f,  12.0f), StreetLightState::Normal,     -1, 0, 0, 1.0f, GetColorF(1.0f, 0.80f, 0.50f, 1.0f) });
    // 7: 中央横通り（住宅4前・弱点灯ランプ・温光）
    streetLights.push_back({ 7, VGet( -6.0f, 0.0f,  -7.0f), StreetLightState::Weak,       -1, 0, 0, 0.45f, GetColorF(1.0f, 0.80f, 0.50f, 1.0f) });

    // 各街灯のDxLibライトハンドル生成
    for (size_t i = 0; i < streetLights.size(); i++)
    {
        if (streetLights[i].state == StreetLightState::Off)
        {
            streetLights[i].lightHandle = -1;
            continue;
        }

        // 電柱のアーム先端フード直下（pos.x + 0.6f, pos.y + 3.85f, pos.z）から下方向の道路を照らすポイントライト
        // 点光源の位置を街灯の先端と電球の位置に正確に合わせる
        VECTOR lightPos = VGet(
            streetLights[i].position.x + 0.6f,
            streetLights[i].position.y + 3.85f,
            streetLights[i].position.z
        );

        float range = 12.5f;
        int handle = CreatePointLightHandle(
            lightPos,
            range,
            1.0f,
            0.08f,
            0.02f
        );

        if (handle != -1)
        {
            COLOR_F c = streetLights[i].baseColor;
            float intensity = streetLights[i].currentIntensity;
            SetLightDifColorHandle(handle, GetColorF(c.r * intensity, c.g * intensity, c.b * intensity, 1.0f));
            SetLightSpcColorHandle(handle, GetColorF(0.2f, 0.2f, 0.2f, 1.0f));
            // 初期状態は動的カリングで制御するため一旦OFFに設定
            SetLightEnableHandle(handle, FALSE);
            streetLights[i].lightHandle = handle;
        }
    }

    // 街灯電柱を静的ワールドコリジョンに登録
    std::vector<VECTOR> poles;
    for (size_t i = 0; i < streetLights.size(); ++i)
    {
        poles.push_back(streetLights[i].position);
    }
    MapRegisterLampPoles(poles);

    // デフォルトの例外配達風プリセットを適用
    SetPreset(EnvironmentPreset::NightDeliveryStyle, false);

    return true;
}

// プリセットごとの内部設定
void NightEnvironment::ApplyPresetSettings()
{
    switch (currentPreset)
    {
    case EnvironmentPreset::Normal:
        // 開発・通常視認用（全景を見通せない深夜基本フォグ: 8m〜28m）
        fogStart = 8.0f;
        fogEnd = 28.0f;
        fogColorR = 24;
        fogColorG = 32;
        fogColorB = 42;
        bgR = 24;
        bgG = 32;
        bgB = 42;
        ambientColor = GetColorF(0.028f, 0.034f, 0.044f, 1.0f);
        bicycleLightRange = 16.0f;
        break;

    case EnvironmentPreset::NightDeliveryStyle:
        // 本番用デフォルト（深夜の濃密な恐怖フォグ: 近接4mから濃霧が立ち込め17mで完全消失）
        fogStart = 4.0f;
        fogEnd = 17.0f;
        fogColorR = 36;
        fogColorG = 46;
        fogColorB = 58;
        bgR = 36;
        bgG = 46;
        bgB = 58;
        ambientColor = GetColorF(0.028f, 0.035f, 0.046f, 1.0f);
        bicycleLightRange = 12.0f;
        break;

    case EnvironmentPreset::ExtremeDark:
        // 実験・ホラーイベント用（超近接1.5m〜8.0mの極限暗闇フォグ）
        fogStart = 1.5f;
        fogEnd = 8.0f;
        fogColorR = 40;
        fogColorG = 50;
        fogColorB = 64;
        bgR = 40;
        bgG = 50;
        bgB = 64;
        ambientColor = GetColorF(0.020f, 0.024f, 0.032f, 1.0f);
        bicycleLightRange = 8.5f;
        break;
    }

    if (bicycleLightHandle != -1)
    {
        SetLightRangeAttenHandle(bicycleLightHandle, bicycleLightRange, 1.0f, 0.035f, 0.006f);
    }
}

// 自転車ヘッドライトの到達距離および減衰パラメータ設定（濃霧時の貫通防止）
void NightEnvironment::SetBicycleHeadlightParams(float range, float atten0, float atten1, float atten2)
{
    bicycleLightRange = range;
    if (bicycleLightHandle != -1)
    {
        SetLightRangeAttenHandle(bicycleLightHandle, bicycleLightRange, atten0, atten1, atten2);
    }
}

// プリセット設定
void NightEnvironment::SetPreset(EnvironmentPreset preset, bool manual)
{
    currentPreset = preset;
    manualPresetOverride = manual;
    ApplyPresetSettings();

    const char* name = "NIGHT_DELIVERY_STYLE";
    if (currentPreset == EnvironmentPreset::Normal) name = "NORMAL";
    else if (currentPreset == EnvironmentPreset::ExtremeDark) name = "EXTREME_DARK";

    snprintf(debugMessage, sizeof(debugMessage), "PRESET: %s (Fog: %.0fm-%.0fm)", name, fogStart, fogEnd);
    debugNotifyTimer = 180;
}

// プリセット順送り切り替え (F2)
void NightEnvironment::CyclePreset()
{
    if (currentPreset == EnvironmentPreset::Normal)
    {
        SetPreset(EnvironmentPreset::NightDeliveryStyle, true);
    }
    else if (currentPreset == EnvironmentPreset::NightDeliveryStyle)
    {
        SetPreset(EnvironmentPreset::ExtremeDark);
    }
    else
    {
        SetPreset(EnvironmentPreset::Normal);
    }
}

// フォグON/OFF (F1)
void NightEnvironment::ToggleFog()
{
    fogEnabled = !fogEnabled;
    snprintf(debugMessage, sizeof(debugMessage), "FOG: %s", fogEnabled ? "ENABLED" : "DISABLED");
    debugNotifyTimer = 150;
}

// 街灯ON/OFF (F3)
void NightEnvironment::ToggleStreetLights()
{
    streetLightsEnabled = !streetLightsEnabled;
    snprintf(debugMessage, sizeof(debugMessage), "STREET LIGHTS: %s", streetLightsEnabled ? "ON" : "OFF");
    debugNotifyTimer = 150;
}

// 街灯テスト明滅トリガー (F5)
void NightEnvironment::TriggerStreetLightFlicker(int id)
{
    for (size_t i = 0; i < streetLights.size(); i++)
    {
        if (streetLights[i].id == id)
        {
            streetLights[i].state = StreetLightState::Flickering;
            streetLights[i].flickerTimer = 1;
            streetLights[i].flickerPhase = 1;
            snprintf(debugMessage, sizeof(debugMessage), "TRIGGER FLICKER on Lamp ID %d", id);
            debugNotifyTimer = 150;
            break;
        }
    }
}

// 毎フレーム更新（自転車ヘッドライト追従、街灯明滅タイマー、近傍ライトの動的カリング）
void NightEnvironment::Update(const Bicycle& bicycle, const VECTOR& playerPos)
{
    // 自転車ヘッドライトの更新
    // ヘッドライトはプレイヤーの視覚と安全の根幹であるため、点灯時は最優先で有効化する
    bool isHeadlightActive = bicycle.IsHeadlightOn() && (blackoutTimer <= 0) && (bicycle.GetHeadlightIntensity() > 0.001f);
    cachedHeadlightActive = isHeadlightActive;
    if (isHeadlightActive)
    {
        cachedHeadlightPos = bicycle.GetHeadlightPosition();
        cachedHeadlightDir = bicycle.GetHeadlightDirection();
        cachedHeadlightIntensity = bicycle.GetHeadlightIntensity();
        cachedHeadlightRange = bicycle.GetHeadlightRange();
    }
    if (bicycleLightHandle != -1)
    {
        SetLightEnableHandle(bicycleLightHandle, isHeadlightActive ? TRUE : FALSE);

        if (isHeadlightActive)
        {
            VECTOR lightPos = cachedHeadlightPos;
            VECTOR lightDir = cachedHeadlightDir;
            float intensity = cachedHeadlightIntensity;
            float range = cachedHeadlightRange;

            SetLightPositionHandle(bicycleLightHandle, lightPos);
            SetLightDirectionHandle(bicycleLightHandle, lightDir);
            SetLightRangeAttenHandle(bicycleLightHandle, range, 1.0f, 0.035f, 0.006f);

            // バッテリー残量およびフリッカーによる減光をライト光色に反映
            COLOR_F difColor = GetColorF(
                bicycleLightColor.r * intensity,
                bicycleLightColor.g * intensity,
                bicycleLightColor.b * intensity,
                1.0f
            );
            SetLightDifColorHandle(bicycleLightHandle, difColor);
        }
    }

    // 一時停電タイマー更新
    if (blackoutTimer > 0)
    {
        blackoutTimer--;
    }

    // デバッグメッセージ通知タイマー更新
    if (debugNotifyTimer > 0)
    {
        debugNotifyTimer--;
    }

    // 03時台以降（丑三つ時）の街灯突発的明滅判定（稀に街灯が不気味に瞬く）
    int timeLevel = HorrorUI::Instance().GetHorrorTimeLevel();
    if (timeLevel >= 2 && (rand() % 850 == 0) && !streetLights.empty())
    {
        int targetIdx = rand() % static_cast<int>(streetLights.size());
        if (streetLights[targetIdx].state == StreetLightState::Normal)
        {
            streetLights[targetIdx].state = StreetLightState::Flickering;
            streetLights[targetIdx].flickerTimer = 1;
            streetLights[targetIdx].flickerPhase = 1;
        }
    }

    // 各街灯の内部ロジック（明滅・輝度計算）の更新
    for (size_t i = 0; i < streetLights.size(); i++)
    {
        StreetLight& light = streetLights[i];

        // 明滅制御
        if (light.state == StreetLightState::Flickering)
        {
            UpdateFlicker(light);
        }

        // 基本の光色反映
        if (light.lightHandle != -1)
        {
            COLOR_F c = light.baseColor;
            float intensity = light.currentIntensity;
            SetLightDifColorHandle(light.lightHandle, GetColorF(c.r * intensity, c.g * intensity, c.b * intensity, 1.0f));
        }
    }

    // プレイヤーに近い光源を優先する動的選別処理
    // 同時に有効化できるライト数の制限（標準では3個）を
    // 超過して街灯が突然消えるのを防ぐため、カメラ（プレイヤー）に近い順に優先してライトを割り当てる。
    // 利用できる光源数に合わせ、近くの街灯を優先して急な点滅を防ぐ
    struct LampDist
    {
        int index;
        float distSq;
    };

    std::vector<LampDist> activeLampCandidates;
    for (size_t i = 0; i < streetLights.size(); ++i)
    {
        const StreetLight& sl = streetLights[i];
        bool shouldLight = streetLightsEnabled && (blackoutTimer <= 0) && (sl.state != StreetLightState::Off) && (sl.currentIntensity > 0.05f);
        if (shouldLight && sl.lightHandle != -1)
        {
            float dx = (sl.position.x + 0.6f) - playerPos.x;
            float dz = sl.position.z - playerPos.z;
            float distSq = dx * dx + dz * dz;

            // プレイヤーから30m以内の街灯を候補とする
            if (distSq < 30.0f * 30.0f)
            {
                activeLampCandidates.push_back({ static_cast<int>(i), distSq });
            }
        }
    }

    std::sort(activeLampCandidates.begin(), activeLampCandidates.end(), [](const LampDist& a, const LampDist& b) {
        return a.distSq < b.distSq;
    });

    // 自転車のライトを切り替えても街灯の有効数を変えず、急な点灯や消灯を防ぐ
    const int maxStreetLightsActive = 1;

    for (size_t i = 0; i < streetLights.size(); ++i)
    {
        if (streetLights[i].lightHandle != -1)
        {
            bool enable = false;
            for (size_t k = 0; k < activeLampCandidates.size() && static_cast<int>(k) < maxStreetLightsActive; ++k)
            {
                if (activeLampCandidates[k].index == static_cast<int>(i))
                {
                    enable = true;
                    break;
                }
            }
            SetLightEnableHandle(streetLights[i].lightHandle, enable ? TRUE : FALSE);
        }
    }

    // 住宅玄関灯の近傍動的カリングも更新（ヘッドライト点灯時は近傍1棟、消灯時は近傍1棟）
    // 街灯と玄関灯が同時に密集しても合計有効ライト数が適正範囲に保たれる
    MapUpdatePorchLightsCulling(playerPos, 1);
}

// 街灯の不規則明滅ロジック
void NightEnvironment::UpdateFlicker(StreetLight& light)
{
    light.flickerTimer--;
    if (light.flickerTimer > 0) return;

    // 不規則なステートマシンによるリアルな明滅シーケンス
    switch (light.flickerPhase)
    {
    case 0:
        // 通常安定点灯（3〜7秒間安定）
        light.currentIntensity = 1.0f;
        light.flickerPhase = 1;
        light.flickerTimer = 180 + (GetRand(240));
        break;

    case 1:
        // わずかな減光（2〜3フレーム）
        light.currentIntensity = 0.25f;
        light.flickerPhase = 2;
        light.flickerTimer = 3;
        break;

    case 2:
        // 一瞬の復帰（4〜6フレーム）
        light.currentIntensity = 0.85f;
        light.flickerPhase = 3;
        light.flickerTimer = 5;
        break;

    case 3:
        // 完全ブラックアウト（5〜12フレームの暗闇）
        light.currentIntensity = 0.0f;
        light.flickerPhase = 4;
        light.flickerTimer = 8;
        break;

    case 4:
        // 瞬間再点灯（3フレーム）
        light.currentIntensity = 1.0f;
        light.flickerPhase = 5;
        light.flickerTimer = 3;
        break;

    case 5:
        // 小さな残響フリッカー（3フレーム）
        light.currentIntensity = 0.40f;
        light.flickerPhase = 0; // 次は長い安定フェーズへ
        light.flickerTimer = 4;
        break;

    default:
        light.flickerPhase = 0;
        light.flickerTimer = 60;
        light.currentIntensity = 1.0f;
        break;
    }
}

void NightEnvironment::EnsureVisualAssets()
{
    if (visualAssetsInitialized) return;
    glowBillboardTex = CreateRadialBillboardTexture(128, false);
    moonDiscTex = CreateRadialBillboardTexture(128, true);
    poleSteelTex = CreatePoleSteelTexture(128);
    visualAssetsInitialized = true;
}

void NightEnvironment::FinalizeVisualAssets()
{
    if (glowBillboardTex != -1) { DeleteGraph(glowBillboardTex); glowBillboardTex = -1; }
    if (moonDiscTex != -1) { DeleteGraph(moonDiscTex); moonDiscTex = -1; }
    if (poleSteelTex != -1) { DeleteGraph(poleSteelTex); poleSteelTex = -1; }
    visualAssetsInitialized = false;
}

void NightEnvironment::DrawSkyDome()
{
    EnsureVisualAssets();

    VECTOR camPos = GetCameraPosition();
    VECTOR center = VGet(camPos.x, 0.0f, camPos.z);

    // 霧の視程に応じて夜空・月・遠景樹木の可視度を滑らかに減衰させる
    float vis = fogEnabled ? std::clamp((fogEnd - 7.5f) / 18.0f, 0.05f, 1.0f) : 1.0f;

    auto lerpU8 = [](int a, int b, float t) -> BYTE {
        return static_cast<BYTE>(std::clamp(static_cast<int>(a + (b - a) * t), 0, 255));
    };

    COLOR_U8 horCol = GetColorU8(fogColorR, fogColorG, fogColorB, 255);
    COLOR_U8 lowCol = GetColorU8(
        lerpU8(fogColorR, 18, 0.45f + 0.40f * vis),
        lerpU8(fogColorG, 24, 0.45f + 0.40f * vis),
        lerpU8(fogColorB, 36, 0.45f + 0.40f * vis),
        255
    );
    COLOR_U8 midCol = GetColorU8(
        lerpU8(fogColorR, 10, 0.45f + 0.50f * vis),
        lerpU8(fogColorG, 14, 0.45f + 0.50f * vis),
        lerpU8(fogColorB, 24, 0.45f + 0.50f * vis),
        255
    );
    COLOR_U8 zenCol = GetColorU8(
        lerpU8(fogColorR, 5, 0.50f + 0.50f * vis),
        lerpU8(fogColorG, 7, 0.50f + 0.50f * vis),
        lerpU8(fogColorB, 14, 0.50f + 0.50f * vis),
        255
    );

    SetUseLighting(FALSE);
    SetFogEnable(FALSE);
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    const float domeR = 340.0f;
    const int slices = 32;
    const float elevBands[5] = { -0.06f, 0.10f, 0.30f, 0.62f, 1.0f };
    const COLOR_U8 ringCols[5] = { horCol, lowCol, midCol, zenCol, zenCol };

    std::vector<VERTEX3D> skyVerts;
    skyVerts.reserve(slices * 4 * 6);
    auto makeSkyV = [](const VECTOR& p, COLOR_U8 c) {
        VERTEX3D v{};
        v.pos = p;
        v.norm = VGet(0.0f, -1.0f, 0.0f);
        v.dif = c;
        v.spc = GetColorU8(0, 0, 0, 0);
        return v;
    };

    for (int b = 0; b < 4; ++b)
    {
        float el0 = elevBands[b] * DX_PI_F * 0.5f;
        float el1 = elevBands[b + 1] * DX_PI_F * 0.5f;
        float r0 = cosf(el0) * domeR, y0 = sinf(el0) * domeR;
        float r1 = cosf(el1) * domeR, y1 = sinf(el1) * domeR;

        for (int i = 0; i < slices; ++i)
        {
            float a0 = static_cast<float>(i) / static_cast<float>(slices) * DX_PI_F * 2.0f;
            float a1 = static_cast<float>(i + 1) / static_cast<float>(slices) * DX_PI_F * 2.0f;

            VECTOR p00 = VGet(center.x + cosf(a0) * r0, center.y + y0, center.z + sinf(a0) * r0);
            VECTOR p10 = VGet(center.x + cosf(a1) * r0, center.y + y0, center.z + sinf(a1) * r0);
            VECTOR p11 = VGet(center.x + cosf(a1) * r1, center.y + y1, center.z + sinf(a1) * r1);
            VECTOR p01 = VGet(center.x + cosf(a0) * r1, center.y + y1, center.z + sinf(a0) * r1);

            VERTEX3D v00 = makeSkyV(p00, ringCols[b]);
            VERTEX3D v10 = makeSkyV(p10, ringCols[b]);
            VERTEX3D v11 = makeSkyV(p11, ringCols[b + 1]);
            VERTEX3D v01 = makeSkyV(p01, ringCols[b + 1]);

            skyVerts.push_back(v00); skyVerts.push_back(v10); skyVerts.push_back(v11);
            skyVerts.push_back(v00); skyVerts.push_back(v11); skyVerts.push_back(v01);
        }
    }
    DrawPolygon3D(skyVerts.data(), static_cast<int>(skyVerts.size()) / 3, DX_NONE_GRAPH, FALSE);

    // 淡い夜星と月（晴天・薄霧時のみ微かな針穴ほどの星が見え、濃霧時は隠れる）
    float starFade = fogEnabled ? std::clamp((fogEnd - 19.0f) / 9.0f, 0.0f, 1.0f) : 1.0f;
    if (starFade > 0.02f)
    {
        SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(115.0f * starFade));
        for (int s = 0; s < 48; ++s)
        {
            uint32_t h = static_cast<uint32_t>(s + 1) * 2654435761u;
            float az = static_cast<float>(h & 0xFFFFu) / 65535.0f * DX_PI_F * 2.0f;
            float el = 0.24f + (static_cast<float>((h >> 16) & 0xFFFFu) / 65535.0f) * 0.95f;
            float sr = 315.0f;
            VECTOR sp = VGet(
                center.x + cosf(az) * cosf(el) * sr,
                center.y + sinf(el) * sr,
                center.z + sinf(az) * cosf(el) * sr
            );
            float sz = 0.32f + static_cast<float>(s % 3) * 0.18f;
            DrawBillboard3D(sp, 0.5f, 0.5f, sz, 0.0f, glowBillboardTex, TRUE);
        }
    }

    if (moonDiscTex != -1)
    {
        VECTOR moonPos = VGet(center.x + 150.0f, center.y + 125.0f, center.z + 235.0f);
        int moonAlpha = static_cast<int>(55.0f + 165.0f * vis);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, moonAlpha);
        DrawBillboard3D(moonPos, 0.5f, 0.5f, 34.0f, 0.0f, moonDiscTex, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(38.0f * vis + 18.0f));
        DrawBillboard3D(moonPos, 0.5f, 0.5f, 68.0f, 0.0f, glowBillboardTex, TRUE);
    }

    // 遠景の山・防風林シルエット（住宅街外の地平線に奥行きを与える）
    const int treeSegs = 64;
    const float treeR = 260.0f;
    float treeMix = 0.72f + 0.25f * (1.0f - vis);
    COLOR_U8 treeBaseCol = GetColorU8(
        lerpU8(8, fogColorR, treeMix),
        lerpU8(11, fogColorG, treeMix),
        lerpU8(16, fogColorB, treeMix),
        255
    );
    COLOR_U8 treeTopCol = GetColorU8(
        lerpU8(12, fogColorR, (std::min)(1.0f, treeMix + 0.08f)),
        lerpU8(16, fogColorG, (std::min)(1.0f, treeMix + 0.08f)),
        lerpU8(22, fogColorB, (std::min)(1.0f, treeMix + 0.08f)),
        255
    );

    std::vector<VERTEX3D> treeVerts;
    treeVerts.reserve(treeSegs * 6);
    auto treeHeightAt = [](float a) {
        return 10.5f + sinf(a * 5.0f + 0.7f) * 3.8f + cosf(a * 11.0f) * 2.4f + sinf(a * 23.0f) * 1.4f;
    };

    for (int i = 0; i < treeSegs; ++i)
    {
        float a0 = static_cast<float>(i) / static_cast<float>(treeSegs) * DX_PI_F * 2.0f;
        float a1 = static_cast<float>(i + 1) / static_cast<float>(treeSegs) * DX_PI_F * 2.0f;
        float h0 = treeHeightAt(a0);
        float h1 = treeHeightAt(a1);

        VECTOR b0 = VGet(center.x + cosf(a0) * treeR, -2.0f, center.z + sinf(a0) * treeR);
        VECTOR b1 = VGet(center.x + cosf(a1) * treeR, -2.0f, center.z + sinf(a1) * treeR);
        VECTOR t0 = VGet(b0.x, h0, b0.z);
        VECTOR t1 = VGet(b1.x, h1, b1.z);

        VERTEX3D v0 = makeSkyV(b0, treeBaseCol);
        VERTEX3D v1 = makeSkyV(b1, treeBaseCol);
        VERTEX3D v2 = makeSkyV(t1, treeTopCol);
        VERTEX3D v3 = makeSkyV(t0, treeTopCol);

        treeVerts.push_back(v0); treeVerts.push_back(v1); treeVerts.push_back(v2);
        treeVerts.push_back(v0); treeVerts.push_back(v2); treeVerts.push_back(v3);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawPolygon3D(treeVerts.data(), static_cast<int>(treeVerts.size()) / 3, DX_NONE_GRAPH, FALSE);

    SetWriteZBuffer3D(TRUE);
    SetFogEnable(fogEnabled ? TRUE : FALSE);
    SetUseLighting(TRUE);
}

void NightEnvironment::DrawOverheadWires()
{
    if (streetLights.size() < 7) return;

    static const int wirePairs[][2] = {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 5 }, { 3, 4 }, { 0, 6 }
    };

    SetUseLighting(FALSE);
    unsigned int wireCol = GetColor(16, 18, 22);
    const int segs = 10;

    for (const auto& pair : wirePairs)
    {
        VECTOR a = streetLights[pair[0]].position;
        VECTOR b = streetLights[pair[1]].position;
        float dist = VSize(VSub(b, a));
        float sag = dist * 0.024f;

        for (int w = 0; w < 2; ++w)
        {
            float zOff = (w == 0) ? -0.14f : 0.14f;
            VECTOR pStart = VGet(a.x, a.y + 4.30f, a.z + zOff);
            VECTOR pEnd = VGet(b.x, b.y + 4.30f, b.z + zOff);

            VECTOR prev = pStart;
            for (int s = 1; s <= segs; ++s)
            {
                float t = static_cast<float>(s) / static_cast<float>(segs);
                VECTOR cur = VAdd(VScale(pStart, 1.0f - t), VScale(pEnd, t));
                cur.y -= 4.0f * t * (1.0f - t) * sag;
                DrawLine3D(prev, cur, wireCol);
                prev = cur;
            }
        }
    }

    SetUseLighting(TRUE);
}

void NightEnvironment::DrawStreetLightAtmosphere(const VECTOR& pos, float intensity, COLOR_F color, StreetLightState state)
{
    if (glowBillboardTex == -1) return;

    VECTOR hoodCenter = VGet(pos.x + 0.60f, pos.y + 4.00f, pos.z);
    VECTOR bulbPos = VGet(hoodCenter.x, hoodCenter.y - 0.12f, hoodCenter.z);

    if (intensity > 0.05f)
    {

    int cr = static_cast<int>(std::clamp(color.r * 255.0f, 0.0f, 255.0f));
    int cg = static_cast<int>(std::clamp(color.g * 255.0f, 0.0f, 255.0f));
    int cb = static_cast<int>(std::clamp(color.b * 255.0f, 0.0f, 255.0f));

    SetUseLighting(FALSE);
    SetWriteZBuffer3D(FALSE);

    // 1. やわらかなランプ光のビルボードグロー（多角形球の置き換え）
    SetDrawBright(cr, cg, cb);
    SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(135.0f * intensity));
    DrawBillboard3D(bulbPos, 0.5f, 0.5f, 1.25f, 0.0f, glowBillboardTex, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(175.0f * intensity));
    DrawBillboard3D(bulbPos, 0.5f, 0.5f, 0.52f, 0.0f, glowBillboardTex, TRUE);

    // 2. 路面に落ちる街灯光の温かな光溜まりデカール
    SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(64.0f * intensity));
    float poolR = 4.4f;
    float yPool = 0.0148f;
    COLOR_U8 poolDif = GetColorU8(cr, cg, cb, 255);
    COLOR_U8 zeroSpc = GetColorU8(0, 0, 0, 0);
    auto makePoolV = [&](float x, float z, float u, float v) {
        VERTEX3D vert{};
        vert.pos = VGet(x, yPool, z);
        vert.norm = VGet(0.0f, 1.0f, 0.0f);
        vert.dif = poolDif;
        vert.spc = zeroSpc;
        vert.u = u;
        vert.v = v;
        vert.su = u;
        vert.sv = v;
        return vert;
    };
    VERTEX3D poolVerts[6] = {
        makePoolV(hoodCenter.x - poolR, hoodCenter.z - poolR, 0.0f, 0.0f),
        makePoolV(hoodCenter.x - poolR, hoodCenter.z + poolR, 0.0f, 1.0f),
        makePoolV(hoodCenter.x + poolR, hoodCenter.z + poolR, 1.0f, 1.0f),
        makePoolV(hoodCenter.x - poolR, hoodCenter.z - poolR, 0.0f, 0.0f),
        makePoolV(hoodCenter.x + poolR, hoodCenter.z + poolR, 1.0f, 1.0f),
        makePoolV(hoodCenter.x + poolR, hoodCenter.z - poolR, 1.0f, 0.0f)
    };
    SetTextureAddressMode(DX_TEXADDRESS_CLAMP);
    DrawPolygon3D(poolVerts, 2, glowBillboardTex, TRUE);
    SetDrawBright(255, 255, 255);

    // 3. 霧の中に差し込む街灯ボリュームライトコーン（視線方向に応じ輪郭を滑らかにフェード）
    VECTOR camPos = GetCameraPosition();
    VECTOR toCamXZ = VGet(camPos.x - hoodCenter.x, 0.0f, camPos.z - hoodCenter.z);
    float toCamLen = sqrtf(toCamXZ.x * toCamXZ.x + toCamXZ.z * toCamXZ.z);
    VECTOR viewDirXZ = (toCamLen > 0.001f) ? VGet(toCamXZ.x / toCamLen, 0.0f, toCamXZ.z / toCamLen) : VGet(0.0f, 0.0f, 1.0f);

    int coneAlpha = static_cast<int>(30.0f * intensity);
    SetDrawBlendMode(DX_BLENDMODE_ADD, coneAlpha);
    const int coneSides = 24;
    const float rTop = 0.18f;
    const float rMid = 1.05f;
    const float rBot = 2.15f;
    const float yTop = bulbPos.y - 0.04f;
    const float yMid = pos.y + (bulbPos.y - pos.y) * 0.50f;
    const float yBot = pos.y + 0.30f;

    std::vector<VERTEX3D> coneVerts;
    coneVerts.reserve(coneSides * 12);
    for (int i = 0; i < coneSides; ++i)
    {
        float a0 = static_cast<float>(i) / static_cast<float>(coneSides) * DX_PI_F * 2.0f;
        float a1 = static_cast<float>(i + 1) / static_cast<float>(coneSides) * DX_PI_F * 2.0f;
        VECTOR radialA = VGet(cosf(a0), 0.0f, sinf(a0));
        VECTOR radialB = VGet(cosf(a1), 0.0f, sinf(a1));

        // 輪郭エッジ（radial・view == 0）で光量を0へ落とすソフトエッジ減衰
        float f0 = powf(fabsf(radialA.x * viewDirXZ.x + radialA.z * viewDirXZ.z), 1.85f);
        float f1 = powf(fabsf(radialB.x * viewDirXZ.x + radialB.z * viewDirXZ.z), 1.85f);

        auto scaleCol = [&](float k) {
            return GetColorU8(
                static_cast<BYTE>(cr * k),
                static_cast<BYTE>(cg * k),
                static_cast<BYTE>(cb * k),
                static_cast<BYTE>(255.0f * k)
            );
        };
        COLOR_U8 cTop0 = scaleCol(f0 * 0.90f), cTop1 = scaleCol(f1 * 0.90f);
        COLOR_U8 cMid0 = scaleCol(f0 * 0.36f), cMid1 = scaleCol(f1 * 0.36f);
        COLOR_U8 cBot = GetColorU8(0, 0, 0, 0);

        auto makeCV = [&](float x, float y, float z, COLOR_U8 c) {
            VERTEX3D v{};
            v.pos = VGet(x, y, z);
            v.norm = VGet(0.0f, 1.0f, 0.0f);
            v.dif = c;
            v.spc = zeroSpc;
            return v;
        };

        VERTEX3D t0 = makeCV(hoodCenter.x + radialA.x * rTop, yTop, hoodCenter.z + radialA.z * rTop, cTop0);
        VERTEX3D t1 = makeCV(hoodCenter.x + radialB.x * rTop, yTop, hoodCenter.z + radialB.z * rTop, cTop1);
        VERTEX3D m0 = makeCV(hoodCenter.x + radialA.x * rMid, yMid, hoodCenter.z + radialA.z * rMid, cMid0);
        VERTEX3D m1 = makeCV(hoodCenter.x + radialB.x * rMid, yMid, hoodCenter.z + radialB.z * rMid, cMid1);
        VERTEX3D b0 = makeCV(hoodCenter.x + radialA.x * rBot, yBot, hoodCenter.z + radialA.z * rBot, cBot);
        VERTEX3D b1 = makeCV(hoodCenter.x + radialB.x * rBot, yBot, hoodCenter.z + radialB.z * rBot, cBot);

        coneVerts.push_back(t0); coneVerts.push_back(t1); coneVerts.push_back(m1);
        coneVerts.push_back(t0); coneVerts.push_back(m1); coneVerts.push_back(m0);
        coneVerts.push_back(m0); coneVerts.push_back(m1); coneVerts.push_back(b1);
        coneVerts.push_back(m0); coneVerts.push_back(b1); coneVerts.push_back(b0);
    }
    DrawPolygon3D(coneVerts.data(), static_cast<int>(coneVerts.size()) / 3, DX_NONE_GRAPH, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetWriteZBuffer3D(TRUE);
    SetUseLighting(TRUE);
    }

    // 不規則明滅・漏電時の電気アークスパーク＆落下火花（瞬間消灯中もバチッと発光）
    bool sparkBurst = (state == StreetLightState::Flickering && streetLightsEnabled && blackoutTimer <= 0)
        && ((intensity < 0.70f && (rand() % 3 == 0)) || (rand() % 10 == 0));
    if (sparkBurst)
    {
        SetUseLighting(FALSE);
        SetWriteZBuffer3D(FALSE);
        SetDrawBright(210, 235, 255);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 165);
        DrawBillboard3D(bulbPos, 0.5f, 0.5f, 0.55f, 0.0f, glowBillboardTex, TRUE);
        SetDrawBright(255, 255, 255);

        SetDrawBlendMode(DX_BLENDMODE_ADD, 225);
        for (int s = 0; s < 5; ++s)
        {
            float sx1 = (static_cast<float>(rand() % 44) - 22.0f) * 0.01f;
            float sy1 = -0.10f - static_cast<float>(rand() % 30) * 0.01f;
            float sz1 = (static_cast<float>(rand() % 44) - 22.0f) * 0.01f;
            VECTOR mid = VGet(bulbPos.x + sx1, bulbPos.y + sy1, bulbPos.z + sz1);
            VECTOR end = VGet(mid.x + sx1 * 0.7f, mid.y - 0.18f - static_cast<float>(rand() % 28) * 0.01f, mid.z + sz1 * 0.7f);
            unsigned int arcCol = (s % 2 == 0) ? GetColor(220, 240, 255) : GetColor(255, 210, 120);
            DrawLine3D(bulbPos, mid, arcCol);
            DrawLine3D(mid, end, GetColor(255, 165, 65));
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        SetWriteZBuffer3D(TRUE);
        SetUseLighting(TRUE);
    }
}

void NightEnvironment::DrawBicycleHeadlightBeam()
{
    if (!cachedHeadlightActive || cachedHeadlightIntensity <= 0.05f || glowBillboardTex == -1)
    {
        return;
    }

    VECTOR dir = VNorm(cachedHeadlightDir);
    VECTOR ref = (fabsf(dir.y) < 0.9f) ? VGet(0.0f, 1.0f, 0.0f) : VGet(1.0f, 0.0f, 0.0f);
    VECTOR right = VNorm(VCross(ref, dir));
    VECTOR up = VNorm(VCross(dir, right));

    float beamLen = (std::min)(cachedHeadlightRange * 0.45f, 5.2f);
    VECTOR startPos = VAdd(cachedHeadlightPos, VScale(dir, 0.14f));
    VECTOR endPos = VAdd(cachedHeadlightPos, VScale(dir, beamLen));

    VECTOR camPos = GetCameraPosition();
    VECTOR toBulb = VSub(camPos, cachedHeadlightPos);
    float camDist = VSize(toBulb);

    SetUseLighting(FALSE);
    SetWriteZBuffer3D(FALSE);

    // 視界を妨げない極薄ボリュメトリックヘッドライトコーン（輪郭ソフトエッジ減衰）
    int coneAlpha = static_cast<int>(18.0f * cachedHeadlightIntensity);
    SetDrawBlendMode(DX_BLENDMODE_ADD, coneAlpha);

    int lr = static_cast<int>(bicycleLightColor.r * 255.0f * cachedHeadlightIntensity);
    int lg = static_cast<int>(bicycleLightColor.g * 255.0f * cachedHeadlightIntensity);
    int lb = static_cast<int>(bicycleLightColor.b * 255.0f * cachedHeadlightIntensity);

    float nearFade = std::clamp((camDist - 1.1f) / 1.4f, 0.0f, 1.0f);
    VECTOR camSide = (camDist > 0.01f) ? VNorm(VSub(toBulb, VScale(dir, VDot(toBulb, dir)))) : up;
    COLOR_U8 endC = GetColorU8(0, 0, 0, 0);
    COLOR_U8 zeroSpc = GetColorU8(0, 0, 0, 0);

    const int sides = 20;
    const float r0 = 0.07f;
    const float r1 = 1.10f;
    std::vector<VERTEX3D> beamVerts;
    beamVerts.reserve(sides * 6);

    for (int i = 0; i < sides; ++i)
    {
        float a0 = static_cast<float>(i) / static_cast<float>(sides) * DX_PI_F * 2.0f;
        float a1 = static_cast<float>(i + 1) / static_cast<float>(sides) * DX_PI_F * 2.0f;

        VECTOR radial0 = VAdd(VScale(right, cosf(a0)), VScale(up, sinf(a0)));
        VECTOR radial1 = VAdd(VScale(right, cosf(a1)), VScale(up, sinf(a1)));

        float f0 = powf(fabsf(VDot(radial0, camSide)), 2.0f) * nearFade;
        float f1 = powf(fabsf(VDot(radial1, camSide)), 2.0f) * nearFade;

        COLOR_U8 tc0 = GetColorU8(static_cast<BYTE>(lr * f0), static_cast<BYTE>(lg * f0), static_cast<BYTE>(lb * f0), static_cast<BYTE>(200.0f * f0));
        COLOR_U8 tc1 = GetColorU8(static_cast<BYTE>(lr * f1), static_cast<BYTE>(lg * f1), static_cast<BYTE>(lb * f1), static_cast<BYTE>(200.0f * f1));

        auto makeBV = [&](const VECTOR& p, COLOR_U8 c) {
            VERTEX3D v{};
            v.pos = p;
            v.norm = VGet(0.0f, 1.0f, 0.0f);
            v.dif = c;
            v.spc = zeroSpc;
            return v;
        };

        VERTEX3D v0 = makeBV(VAdd(startPos, VScale(radial0, r0)), tc0);
        VERTEX3D v1 = makeBV(VAdd(startPos, VScale(radial1, r0)), tc1);
        VERTEX3D v2 = makeBV(VAdd(endPos, VScale(radial1, r1)), endC);
        VERTEX3D v3 = makeBV(VAdd(endPos, VScale(radial0, r1)), endC);

        beamVerts.push_back(v0); beamVerts.push_back(v1); beamVerts.push_back(v2);
        beamVerts.push_back(v0); beamVerts.push_back(v2); beamVerts.push_back(v3);
    }
    DrawPolygon3D(beamVerts.data(), static_cast<int>(beamVerts.size()) / 3, DX_NONE_GRAPH, TRUE);

    if (camDist > 0.3f && VDot(VScale(toBulb, 1.0f / camDist), dir) > -0.1f)
    {
        SetDrawBright(std::clamp(lr, 0, 255), std::clamp(lg, 0, 255), std::clamp(lb, 0, 255));
        SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(150.0f * cachedHeadlightIntensity));
        DrawBillboard3D(cachedHeadlightPos, 0.5f, 0.5f, 0.42f, 0.0f, glowBillboardTex, TRUE);
        SetDrawBright(255, 255, 255);
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetWriteZBuffer3D(TRUE);
    SetUseLighting(TRUE);
}

// 描画フレームごとの環境適用
void NightEnvironment::ApplyLightingAndFog()
{
    // 背景色（フォグの終端色と一致させて境界線を完全になくす）
    SetBackgroundColor(bgR, bgG, bgB);

    // フォグ設定
    SetFogEnable(fogEnabled ? TRUE : FALSE);
    if (fogEnabled)
    {
        SetFogMode(DX_FOGMODE_LINEAR);
        SetFogColor(fogColorR, fogColorG, fogColorB);
        SetFogStartEnd(fogStart, fogEnd);
    }

    // デフォルトのディレクショナルライトを無効化（3:00 AMの暗闇を構築）
    SetLightEnable(FALSE);

    // グローバルアンビエント光（ごくわずかな冷たい青灰色）
    SetGlobalAmbientLight(ambientColor);
}

// 街灯3Dポールとランプヘッドを描画（リアルな根巻きコンクリート基礎・警戒帯・接地影付き）
void NightEnvironment::DrawPoleModel(const VECTOR& pos, float intensity, COLOR_F color, StreetLightState state)
{
    EnsureVisualAssets();

    int prevDrawMode = GetDrawMode();
    SetDrawMode(DX_DRAWMODE_BILINEAR);
    SetTextureAddressMode(DX_TEXADDRESS_WRAP);

    const float poleRadiusBottom = 0.105f;
    const float poleRadiusTop = 0.088f;
    const float poleHeight = 4.35f;
    const int poleSides = 12;

    // 電柱直下の接地影（地面にしっかりと埋設された定着感・アンビエントオクルージョン）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
    unsigned int shCol = GetColor(5, 6, 8);
    const int shadowSegs = 16;
    for (int i = 0; i < shadowSegs; ++i)
    {
        float a1 = (float)i / shadowSegs * DX_PI_F * 2.0f;
        float a2 = (float)(i + 1) / shadowSegs * DX_PI_F * 2.0f;
        VECTOR c = VGet(pos.x, 0.015f, pos.z);
        VECTOR p1 = VGet(pos.x + cosf(a1) * 0.42f, 0.015f, pos.z + sinf(a1) * 0.42f);
        VECTOR p2 = VGet(pos.x + cosf(a2) * 0.42f, 0.015f, pos.z + sinf(a2) * 0.42f);
        DrawTriangle3D(c, p1, p2, shCol, TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 根巻きコンクリート基礎台座＆アンカーカラー
    const float baseHeight = 0.42f;
    const float flangeHeight = 0.06f;
    COLOR_U8 concDif = GetColorU8(105, 110, 118, 255);
    COLOR_U8 concSpc = GetColorU8(24, 26, 30, 255);
    DrawTexturedTube(
        VGet(pos.x, pos.y, pos.z), 0.22f,
        VGet(pos.x, pos.y + baseHeight, pos.z), 0.21f,
        poleSides, poleSteelTex, concDif, concSpc, 0.4f
    );
    COLOR_U8 ironDif = GetColorU8(48, 52, 58, 255);
    COLOR_U8 ironSpc = GetColorU8(60, 66, 74, 255);
    DrawTexturedTube(
        VGet(pos.x, pos.y + baseHeight, pos.z), 0.15f,
        VGet(pos.x, pos.y + baseHeight + flangeHeight, pos.z), 0.135f,
        poleSides, poleSteelTex, ironDif, ironSpc, 0.2f
    );

    // 主スチール電柱柱体（滑らか法線＋微細テクスチャ）
    COLOR_U8 poleDif = GetColorU8(95, 102, 112, 255);
    COLOR_U8 poleSpc = GetColorU8(55, 62, 72, 255);
    DrawTexturedTube(
        VGet(pos.x, pos.y + baseHeight + flangeHeight, pos.z), poleRadiusBottom,
        VGet(pos.x, pos.y + poleHeight, pos.z), poleRadiusTop,
        poleSides, poleSteelTex, poleDif, poleSpc, 3.0f
    );

    // 日本の電柱特有の黄色・黒の安全反射警戒帯（高さ1.10m〜1.42m）
    float bandY0 = pos.y + 1.10f;
    float bandHeight = 0.08f;
    float stripeRadius = 0.106f;
    for (int b = 0; b < 4; ++b)
    {
        float yBottom = bandY0 + b * bandHeight;
        float yTop = yBottom + bandHeight;
        COLOR_U8 bDif = (b % 2 == 0) ? GetColorU8(225, 178, 26, 255) : GetColorU8(22, 24, 28, 255);
        DrawTexturedTube(
            VGet(pos.x, yBottom, pos.z), stripeRadius,
            VGet(pos.x, yTop, pos.z), stripeRadius,
            poleSides, DX_NONE_GRAPH, bDif, ironSpc, 1.0f
        );
    }

    // 上部柱上変圧器ボックス・配電腕金・碍子
    COLOR_U8 xfDif = GetColorU8(68, 74, 82, 255);
    DrawTexturedTube(
        VGet(pos.x - 0.16f, pos.y + 3.05f, pos.z), 0.085f,
        VGet(pos.x - 0.16f, pos.y + 3.48f, pos.z), 0.085f,
        poleSides, poleSteelTex, xfDif, poleSpc, 0.6f
    );
    DrawTexturedTube(
        VGet(pos.x, pos.y + 4.30f, pos.z - 0.28f), 0.022f,
        VGet(pos.x, pos.y + 4.30f, pos.z + 0.28f), 0.022f,
        8, DX_NONE_GRAPH, ironDif, ironSpc, 1.0f
    );
    for (int w = -1; w <= 1; w += 2)
    {
        float insZ = pos.z + static_cast<float>(w) * 0.14f;
        DrawTexturedTube(
            VGet(pos.x, pos.y + 4.30f, insZ), 0.032f,
            VGet(pos.x, pos.y + 4.36f, insZ), 0.024f,
            8, DX_NONE_GRAPH, GetColorU8(150, 158, 165, 255), ironSpc, 1.0f
        );
    }

    // 立体ランプブラケットアーム＆筋交いストラット
    VECTOR armStart = VGet(pos.x + 0.02f, pos.y + 3.98f, pos.z);
    VECTOR armEnd = VGet(pos.x + 0.60f, pos.y + 4.10f, pos.z);
    VECTOR braceStart = VGet(pos.x + 0.04f, pos.y + 3.68f, pos.z);
    VECTOR braceEnd = VGet(pos.x + 0.42f, pos.y + 4.06f, pos.z);
    DrawTexturedTube(armStart, 0.028f, armEnd, 0.024f, 8, poleSteelTex, poleDif, poleSpc, 1.0f);
    DrawTexturedTube(braceStart, 0.018f, braceEnd, 0.016f, 8, poleSteelTex, ironDif, ironSpc, 1.0f);

    // ランプシェード笠＆発光レンズ部分
    VECTOR hoodCenter = VGet(armEnd.x, armEnd.y - 0.10f, armEnd.z);
    DrawTexturedTube(
        VGet(hoodCenter.x, hoodCenter.y - 0.07f, hoodCenter.z), 0.24f,
        VGet(hoodCenter.x, hoodCenter.y + 0.07f, hoodCenter.z), 0.11f,
        12, poleSteelTex, ironDif, ironSpc, 0.5f
    );
    // 笠上面キャップ
    DrawTexturedTube(
        VGet(hoodCenter.x, hoodCenter.y + 0.06f, hoodCenter.z), 0.115f,
        VGet(hoodCenter.x, hoodCenter.y + 0.10f, hoodCenter.z), 0.04f,
        12, DX_NONE_GRAPH, ironDif, ironSpc, 0.2f
    );

    // 自己発光ディフューザー（外部ライトを遮断して街灯自身の点灯強度で描画）
    SetUseLighting(FALSE);
    if (intensity > 0.05f)
    {
        int lr = (std::min)(255, static_cast<int>(color.r * 255.0f * intensity));
        int lg = (std::min)(255, static_cast<int>(color.g * 255.0f * intensity));
        int lb = (std::min)(255, static_cast<int>(color.b * 255.0f * intensity));
        unsigned int bulbCol = GetColor(lr, lg, lb);
        DrawSphere3D(VGet(hoodCenter.x, hoodCenter.y - 0.09f, hoodCenter.z), 0.115f, 10, bulbCol, bulbCol, TRUE);
    }
    else
    {
        unsigned int offBulbCol = GetColor(24, 28, 32);
        DrawSphere3D(VGet(hoodCenter.x, hoodCenter.y - 0.09f, hoodCenter.z), 0.110f, 10, offBulbCol, offBulbCol, TRUE);
    }

    SetDrawMode(prevDrawMode);
    SetUseLighting(TRUE);
    (void)state;
}

// 街灯電柱と発光シェードの3D描画
void NightEnvironment::DrawStreetLights()
{
    EnsureVisualAssets();
    DrawSkyDome();

    for (size_t i = 0; i < streetLights.size(); i++)
    {
        const StreetLight& sl = streetLights[i];
        float intensity = (sl.state == StreetLightState::Off || !streetLightsEnabled || blackoutTimer > 0)
            ? 0.0f
            : sl.currentIntensity;

        DrawPoleModel(sl.position, intensity, sl.baseColor, sl.state);
    }

    DrawOverheadWires();

    for (size_t i = 0; i < streetLights.size(); i++)
    {
        const StreetLight& sl = streetLights[i];
        float intensity = (sl.state == StreetLightState::Off || !streetLightsEnabled || blackoutTimer > 0)
            ? 0.0f
            : sl.currentIntensity;

        DrawStreetLightAtmosphere(sl.position, intensity, sl.baseColor, sl.state);
    }

    DrawBicycleHeadlightBeam();
}

// デバッグ情報HUD描画
void NightEnvironment::DrawDebugHUD(const Bicycle* bicycle, const char* weatherName)
{
    // 一時通知メッセージ
    if (debugNotifyTimer > 0)
    {
        HorrorUI::Instance().DrawRetroPanel(18, 120, 360, 160,
            HorrorUI::COL_BORDER, HorrorUI::COL_PANEL_BG, 210);
        HorrorUI::Instance().DrawJitterString(26, 132, debugMessage,
            HorrorUI::COL_TEXT, HorrorUI::Instance().GetFontSmall());
    }

    // デバッグキー案内・ライティング状態の詳細表示（F1/F2/F3/F5操作時などに通知枠下へ表示）
    // 現在有効になっているライト識別子の数を取得して表示
    if (debugNotifyTimer > 0)
    {
        int enableLightCount = GetEnableLightHandleNum();
        int activeStreetLights = 0;
        for (const auto& sl : streetLights)
        {
            if (sl.state != StreetLightState::Off && sl.currentIntensity > 0.05f)
            {
                activeStreetLights++;
            }
        }

        char lightInfo[128];
        snprintf(lightInfo, sizeof(lightInfo), "ACTIVE LIGHTS: %d (Street: %d/7, HL: %s)",
            enableLightCount,
            activeStreetLights,
            (bicycle && bicycle->IsHeadlightOn()) ? "ON" : "OFF"
        );
        HorrorUI::Instance().DrawJitterString(26, 150, lightInfo,
            GetColor(160, 220, 240), HorrorUI::Instance().GetFontSmall());
    }
}

// 7日間環境・照明設定
void NightEnvironment::SetDayEnvironment(int day)
{
    if (day < 1) day = 1;
    if (day > 5) day = 5;

    // まず全ての街灯を通常状態に戻す
    for (size_t i = 0; i < streetLights.size(); i++)
    {
        streetLights[i].state = StreetLightState::Normal;
        streetLights[i].currentIntensity = 1.0f;
    }

    // ステージごとの街灯状態（進むほど電柱が減る）
    switch (day)
    {
    case 1:
        // Day 1: 6本点灯（ID5のみOFF）
        TurnOffStreetLight(5);
        break;

    case 2:
        // Day 2: 5本点灯（ID5/7 OFF、ID3 フリッカー）
        TriggerStreetLightFlicker(3);
        TurnOffStreetLight(5);
        TurnOffStreetLight(7);
        break;

    case 3:
        // Day 3: 4本点灯（ID3/5/7 OFF、ID2 フリッカー、ID1 弱め）
        if (streetLights.size() > 0)
        {
            streetLights[0].state = StreetLightState::Weak;
            streetLights[0].currentIntensity = 0.50f;
        }
        TriggerStreetLightFlicker(2);
        TurnOffStreetLight(3);
        TurnOffStreetLight(5);
        TurnOffStreetLight(7);
        break;

    case 4:
        // Day 4: 2本点灯（ID1/3/5/6/7 OFF、ID2 フリッカー、ID4 弱め）
        TurnOffStreetLight(1);
        TriggerStreetLightFlicker(2);
        TurnOffStreetLight(3);
        if (streetLights.size() > 3)
        {
            streetLights[3].state = StreetLightState::Weak;
            streetLights[3].currentIntensity = 0.40f;
        }
        TurnOffStreetLight(5);
        TurnOffStreetLight(6);
        TurnOffStreetLight(7);
        break;

    case 5:
    default:
        // Day 5 (FINAL): 1本点灯（ほぼ全消灯、ID1のみフリッカー）
        TriggerStreetLightFlicker(1);
        TurnOffStreetLight(2);
        TurnOffStreetLight(3);
        TurnOffStreetLight(4);
        TurnOffStreetLight(5);
        TurnOffStreetLight(6);
        TurnOffStreetLight(7);
        break;
    }
}

// 特定の街灯を消灯
void NightEnvironment::TurnOffStreetLight(int id)
{
    for (size_t i = 0; i < streetLights.size(); i++)
    {
        if (streetLights[i].id == id)
        {
            streetLights[i].state = StreetLightState::Off;
            streetLights[i].currentIntensity = 0.0f;
            break;
        }
    }
}

// 特定の街灯を点灯
void NightEnvironment::TurnOnStreetLight(int id)
{
    for (size_t i = 0; i < streetLights.size(); i++)
    {
        if (streetLights[i].id == id)
        {
            streetLights[i].state = StreetLightState::Normal;
            streetLights[i].currentIntensity = 1.0f;
            break;
        }
    }
}

// フォグ距離変更
void NightEnvironment::SetFogDistance(float start, float end)
{
    fogStart = start;
    fogEnd = end;
}

// 環境光の強さ変更
void NightEnvironment::SetAmbientStrength(float strength)
{
    ambientColor = GetColorF(strength * 0.7f, strength * 0.8f, strength * 1.0f, 1.0f);
}

// 一時停電イベント
void NightEnvironment::TriggerTemporaryBlackout(int frames)
{
    blackoutTimer = frames;
}

// 現在のプリセット取得
EnvironmentPreset NightEnvironment::GetCurrentPreset() const
{
    return currentPreset;
}

// 天候システムからのフォグおよび環境光の一括設定
void NightEnvironment::SetWeatherFogAndLighting(float start, float end, int r, int g, int b, COLOR_F ambient)
{
    // F2キーで手動切り替えした場合、天候による霧と環境光の自動上書きを抑える
    // 天候管理によって手動設定の環境が即座に上書きされるのを防ぐ
    if (manualPresetOverride)
    {
        return;
    }

    fogStart = start;
    fogEnd = end;
    fogColorR = r;
    fogColorG = g;
    fogColorB = b;
    bgR = r;
    bgG = g;
    bgB = b;
    ambientColor = ambient;
}

// 終了処理
void NightEnvironment::Finalize()
{
    FinalizeVisualAssets();

    // 自転車ライト削除
    if (bicycleLightHandle != -1)
    {
        DeleteLightHandle(bicycleLightHandle);
        bicycleLightHandle = -1;
    }

    // 街灯ライト削除
    for (size_t i = 0; i < streetLights.size(); i++)
    {
        if (streetLights[i].lightHandle != -1)
        {
            DeleteLightHandle(streetLights[i].lightHandle);
            streetLights[i].lightHandle = -1;
        }
    }
    streetLights.clear();
}
