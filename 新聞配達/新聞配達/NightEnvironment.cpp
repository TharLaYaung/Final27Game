#include "NightEnvironment.h"
#include "Bicycle.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

// コンストラクタ
NightEnvironment::NightEnvironment()
{
    currentPreset = EnvironmentPreset::NightDeliveryStyle;

    fogEnabled = true;
    fogStart = 5.0f;
    fogEnd = 22.0f;
    fogColorR = 8;
    fogColorG = 12;
    fogColorB = 16;

    bgR = 8;
    bgG = 12;
    bgB = 16;

    ambientColor = GetColorF(0.035f, 0.040f, 0.055f, 1.0f);

    bicycleLightHandle = -1;
    bicycleLightRange = 14.0f;
    bicycleLightPower = 1.0f;
    bicycleLightColor = GetColorF(1.0f, 0.95f, 0.80f, 1.0f);

    streetLightsEnabled = true;
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
        42.0f * DX_PI_F / 180.0f, // 外側コーン角度
        20.0f * DX_PI_F / 180.0f, // 内側コーン角度
        bicycleLightRange,
        1.0f,
        0.06f,
        0.015f
    );

    if (bicycleLightHandle != -1)
    {
        SetLightDifColorHandle(bicycleLightHandle, bicycleLightColor);
        SetLightSpcColorHandle(bicycleLightHandle, GetColorF(0.4f, 0.4f, 0.3f, 1.0f));
    }

    // 街灯の配置設定（住宅街の交差点・路地要所に7基配置、暗い隙間を確保）
    streetLights.clear();

    // 1: スタート地点・南通り入口（通常点灯・温白色）
    streetLights.push_back({ 1, VGet( -5.0f, 0.0f, -27.0f), StreetLightState::Normal,     -1, 0, 0, 1.0f, GetColorF(1.0f, 0.92f, 0.75f, 1.0f) });
    // 2: 南西交差点（Road A & F 交差点・老朽化した弱点灯ランプ・琥珀色）
    streetLights.push_back({ 2, VGet(-22.0f, 0.0f, -27.0f), StreetLightState::Weak,       -1, 0, 0, 0.40f, GetColorF(1.0f, 0.65f, 0.35f, 1.0f) });
    // 3: 北西交差点（Road A & B 交差点・不規則に明滅する水銀灯ホラー街灯）
    streetLights.push_back({ 3, VGet(-22.0f, 0.0f,  12.0f), StreetLightState::Flickering, -1, 120, 0, 1.0f, GetColorF(0.90f, 0.95f, 1.0f, 1.0f) });
    // 4: 北路地入口（Road B & C 交差点・通常点灯）
    streetLights.push_back({ 4, VGet( -3.5f, 0.0f,  17.5f), StreetLightState::Normal,     -1, 0, 0, 1.0f, GetColorF(0.95f, 0.95f, 0.85f, 1.0f) });
    // 5: 北路地最奥行き止まり（住宅2前・球切れ消灯・不気味な暗闇のシルエット）
    streetLights.push_back({ 5, VGet(  3.5f, 0.0f,  38.0f), StreetLightState::Off,        -1, 0, 0, 0.0f, GetColorF(0.0f, 0.0f, 0.0f, 0.0f) });
    // 6: 北東交差点（Road B & D 交差点・通常点灯・温光）
    streetLights.push_back({ 6, VGet( 22.0f, 0.0f,  12.0f), StreetLightState::Normal,     -1, 0, 0, 1.0f, GetColorF(1.0f, 0.90f, 0.70f, 1.0f) });
    // 7: 中央横通り（住宅4前・弱点灯ランプ・温光）
    streetLights.push_back({ 7, VGet( -6.0f, 0.0f,  -7.0f), StreetLightState::Weak,       -1, 0, 0, 0.45f, GetColorF(1.0f, 0.70f, 0.40f, 1.0f) });

    // 各街灯のDxLibライトハンドル生成
    for (size_t i = 0; i < streetLights.size(); i++)
    {
        if (streetLights[i].state == StreetLightState::Off)
        {
            streetLights[i].lightHandle = -1;
            continue;
        }

        // 電柱の高さ約4.0mから下方向の道路を照らすポイントライト
        VECTOR lightPos = VGet(
            streetLights[i].position.x,
            streetLights[i].position.y + 4.0f,
            streetLights[i].position.z
        );

        float range = 11.5f;
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
            streetLights[i].lightHandle = handle;
        }
    }

    // デフォルトの例外配達風プリセットを適用
    SetPreset(EnvironmentPreset::NightDeliveryStyle);

    return true;
}

// プリセットごとの内部設定
void NightEnvironment::ApplyPresetSettings()
{
    switch (currentPreset)
    {
    case EnvironmentPreset::Normal:
        // 開発・通常視認用（緩やかなフォグ・適度な環境光）
        fogStart = 10.0f;
        fogEnd = 45.0f;
        fogColorR = 20;
        fogColorG = 24;
        fogColorB = 32;
        bgR = 20;
        bgG = 24;
        bgB = 32;
        ambientColor = GetColorF(0.14f, 0.15f, 0.18f, 1.0f);
        bicycleLightRange = 20.0f;
        break;

    case EnvironmentPreset::NightDeliveryStyle:
        // 本番用デフォルト（例外配達風: 3:00 AMの孤立した闇・近接5m〜22mで徐々にシルエット化）
        fogStart = 5.0f;
        fogEnd = 22.0f;
        fogColorR = 8;
        fogColorG = 12;
        fogColorB = 16;
        bgR = 8;
        bgG = 12;
        bgB = 16;
        ambientColor = GetColorF(0.035f, 0.040f, 0.055f, 1.0f);
        bicycleLightRange = 14.0f;
        break;

    case EnvironmentPreset::ExtremeDark:
        // 実験用（超近接3m〜15mの極限暗闇）
        fogStart = 3.0f;
        fogEnd = 15.0f;
        fogColorR = 4;
        fogColorG = 6;
        fogColorB = 8;
        bgR = 4;
        bgG = 6;
        bgB = 8;
        ambientColor = GetColorF(0.012f, 0.015f, 0.020f, 1.0f);
        bicycleLightRange = 10.0f;
        break;
    }

    if (bicycleLightHandle != -1)
    {
        SetLightRangeAttenHandle(bicycleLightHandle, bicycleLightRange, 1.0f, 0.06f, 0.015f);
    }
}

// プリセット設定
void NightEnvironment::SetPreset(EnvironmentPreset preset)
{
    currentPreset = preset;
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
        SetPreset(EnvironmentPreset::NightDeliveryStyle);
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

// 毎フレーム更新（自転車ヘッドライト追従、街灯明滅タイマーなど）
void NightEnvironment::Update(const Bicycle& bicycle)
{
    // 自転車ヘッドライトの更新
    if (bicycleLightHandle != -1)
    {
        // ヘッドライト点灯かつ停電中でなく、輝度が有効値の場合のみライト点灯
        bool isLightActive = bicycle.IsHeadlightOn() && (blackoutTimer <= 0) && (bicycle.GetHeadlightIntensity() > 0.001f);
        SetLightEnableHandle(bicycleLightHandle, isLightActive ? TRUE : FALSE);

        if (isLightActive)
        {
            VECTOR lightPos = bicycle.GetHeadlightPosition();
            VECTOR lightDir = bicycle.GetHeadlightDirection();
            float intensity = bicycle.GetHeadlightIntensity();
            float range = bicycle.GetHeadlightRange();

            SetLightPositionHandle(bicycleLightHandle, lightPos);
            SetLightDirectionHandle(bicycleLightHandle, lightDir);
            SetLightRangeAttenHandle(bicycleLightHandle, range, 1.0f, 0.06f, 0.015f);

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

    // 街灯の更新
    for (size_t i = 0; i < streetLights.size(); i++)
    {
        StreetLight& light = streetLights[i];

        // 明滅制御
        if (light.state == StreetLightState::Flickering)
        {
            UpdateFlicker(light);
        }

        // ブラックアウトまたは無効化時の消灯処理
        bool shouldLight = streetLightsEnabled && (blackoutTimer <= 0) && (light.state != StreetLightState::Off);

        if (light.lightHandle != -1)
        {
            SetLightEnableHandle(light.lightHandle, shouldLight ? TRUE : FALSE);

            if (shouldLight)
            {
                COLOR_F c = light.baseColor;
                float intensity = light.currentIntensity;
                SetLightDifColorHandle(light.lightHandle, GetColorF(c.r * intensity, c.g * intensity, c.b * intensity, 1.0f));
            }
        }
    }
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

// 描画フレームごとの環境適用
void NightEnvironment::ApplyLightingAndFog()
{
    // 背景色（フォグの終端色と一致させて境界線を完全になくす）
    SetBackgroundColor(bgR, bgG, bgB);

    // フォグ設定
    SetFogEnable(fogEnabled ? TRUE : FALSE);
    if (fogEnabled)
    {
        SetFogColor(fogColorR, fogColorG, fogColorB);
        SetFogStartEnd(fogStart, fogEnd);
    }

    // デフォルトのディレクショナルライトを無効化（3:00 AMの暗闇を構築）
    SetLightEnable(FALSE);

    // グローバルアンビエント光（ごくわずかな冷たい青灰色）
    SetGlobalAmbientLight(ambientColor);
}

// 街灯3Dポールとランプヘッドを描画
void NightEnvironment::DrawPoleModel(const VECTOR& pos, float intensity, COLOR_F color)
{
    // 電柱の太さと高さ
    float poleRadius = 0.10f;
    float poleHeight = 4.2f;

    unsigned int poleColor = GetColor(35, 38, 42);

    // 電柱の柱体（8面柱）
    int sides = 8;
    for (int i = 0; i < sides; i++)
    {
        float a1 = (float)i / sides * DX_PI_F * 2.0f;
        float a2 = (float)(i + 1) / sides * DX_PI_F * 2.0f;

        VECTOR b1 = VGet(pos.x + cosf(a1) * poleRadius, pos.y, pos.z + sinf(a1) * poleRadius);
        VECTOR b2 = VGet(pos.x + cosf(a2) * poleRadius, pos.y, pos.z + sinf(a2) * poleRadius);
        VECTOR t1 = VGet(b1.x, pos.y + poleHeight, b1.z);
        VECTOR t2 = VGet(b2.x, pos.y + poleHeight, b2.z);

        DrawTriangle3D(b1, t1, t2, poleColor, TRUE);
        DrawTriangle3D(b1, t2, b2, poleColor, TRUE);
    }

    // 上部アーム（道路側に少し突き出る）
    VECTOR armStart = VGet(pos.x, pos.y + poleHeight - 0.2f, pos.z);
    VECTOR armEnd = VGet(pos.x + 0.6f, pos.y + poleHeight - 0.1f, pos.z);
    DrawLine3D(armStart, armEnd, poleColor);

    // ランプフード外枠
    VECTOR hoodCenter = VGet(armEnd.x, armEnd.y - 0.1f, armEnd.z);
    DrawCube3D(
        VGet(hoodCenter.x - 0.18f, hoodCenter.y - 0.08f, hoodCenter.z - 0.18f),
        VGet(hoodCenter.x + 0.18f, hoodCenter.y + 0.08f, hoodCenter.z + 0.18f),
        GetColor(25, 28, 30),
        GetColor(15, 18, 20),
        TRUE
    );

    // 発光レンズ部分（点灯状態に応じた輝度で描画）
    if (intensity > 0.05f)
    {
        int lr = (int)(color.r * 255.0f * intensity);
        int lg = (int)(color.g * 255.0f * intensity);
        int lb = (int)(color.b * 255.0f * intensity);
        if (lr > 255) lr = 255;
        if (lg > 255) lg = 255;
        if (lb > 255) lb = 255;

        unsigned int bulbCol = GetColor(lr, lg, lb);
        DrawCube3D(
            VGet(hoodCenter.x - 0.12f, hoodCenter.y - 0.12f, hoodCenter.z - 0.12f),
            VGet(hoodCenter.x + 0.12f, hoodCenter.y - 0.07f, hoodCenter.z + 0.12f),
            bulbCol,
            bulbCol,
            TRUE
        );
    }
}

// 街灯電柱と発光シェードの3D描画
void NightEnvironment::DrawStreetLights()
{
    for (size_t i = 0; i < streetLights.size(); i++)
    {
        const StreetLight& sl = streetLights[i];
        float intensity = (sl.state == StreetLightState::Off || !streetLightsEnabled || blackoutTimer > 0)
            ? 0.0f
            : sl.currentIntensity;

        DrawPoleModel(sl.position, intensity, sl.baseColor);
    }
}

// デバッグ情報HUD描画
void NightEnvironment::DrawDebugHUD()
{
    // 一時通知メッセージ
    if (debugNotifyTimer > 0)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawBox(18, 120, 360, 160, GetColor(10, 14, 20), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawBox(18, 120, 360, 160, GetColor(50, 70, 90), FALSE);
        DrawString(26, 130, debugMessage, GetColor(200, 220, 255));
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

// 終了処理
void NightEnvironment::Finalize()
{
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
