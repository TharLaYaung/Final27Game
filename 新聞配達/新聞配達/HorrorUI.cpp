#include "HorrorUI.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

// カラーパレット定数定義（1998 VHS監視端末・Jホラー調 深淵ダーク＆褪せた血錆トーン）
const unsigned int HorrorUI::COL_BG         = GetColor(8, 11, 10);
const unsigned int HorrorUI::COL_PANEL_BG   = GetColor(12, 16, 15);
const unsigned int HorrorUI::COL_BORDER     = GetColor(68, 86, 78);
const unsigned int HorrorUI::COL_BORDER_DIM = GetColor(36, 48, 43);
const unsigned int HorrorUI::COL_TEXT       = GetColor(196, 206, 198);
const unsigned int HorrorUI::COL_TEXT_DIM   = GetColor(118, 132, 125);
const unsigned int HorrorUI::COL_WARNING    = GetColor(205, 48, 44);
const unsigned int HorrorUI::COL_TARGET     = GetColor(212, 108, 48);
const unsigned int HorrorUI::COL_PLAYER     = GetColor(215, 224, 214);
const unsigned int HorrorUI::COL_BIKE       = GetColor(102, 175, 185);

// シングルトンインスタンス
HorrorUI& HorrorUI::Instance()
{
    static HorrorUI instance;
    return instance;
}

// コンストラクタ
HorrorUI::HorrorUI()
{
    fontSmall = -1;
    fontMedium = -1;
    fontLarge = -1;
    fontClock = -1;
    fontTitleHorror = -1;
    fontKeyGuide = -1;
    fontKeyAction = -1;
    fontMono = -1;
    fontHorrorSerif = -1;

    // ゲーム内時計の初期化（午前1時00分スタート、実時間1秒あたり1ゲーム分進行）
    clockHour = 1;
    clockMinute = 0;
    totalGameMinutes = 0.0f;
    gameMinutesPerRealSecond = 1.0f;
    isTimePaused = false;

    currentPrompt = PromptType::None;
    requestedPrompt = PromptType::None;
    promptAlpha = 0.0f;

    glitchTimer = 0;
    jitterTimer = 0;
    interferenceAmount = 0.0f;
    horrorIntensity = 0;
    horrorEffectsEnabled = true;
    frameCount = 0;
    displayedFearPercent = 0.0f;

    currentDay = 1;
    dayBannerTimer = 0.0f;
    dayBannerMax = 4.5f;

    // 恐怖視覚エフェクト初期化
    ghostScreenGraph = -1;
    bloodSplatterGraph = -1;
    currentFearParam = 0.0f;
    showKeyGuide = true;
    deathClosingProgress = 0.0f;
    deathBlinkFactor = 1.0f;
    doubleVisionAmount = 0.0f;
    desaturationAmount = 0.0f;
    focusBlurAmount = 0.0f;

    glitchBurstTimer = 0.0f;
    glitchBurstMaxDuration = 0.4f;
    glitchBurstStrength = 0.0f;
    weatherRainOverlay = 0.0f;
    weatherSnowOverlay = 0.0f;
    weatherLightningFlash = 0.0f;
}

// デストラクタ
HorrorUI::~HorrorUI()
{
    Finalize();
}

// 初期化（フォント生成・ポストプロセスバッファ生成）
void HorrorUI::Initialize()
{
    // 二重像・焦点ボケ用オフスクリーンサーフェス
    if (ghostScreenGraph == -1)
    {
        ghostScreenGraph = MakeScreen(1280, 720, FALSE);
    }
    if (bloodSplatterGraph == -1)
    {
        bloodSplatterGraph = LoadGraph("Data/blood_splatter.png");
    }

    // 日本語フォント生成関数（8x8高精度アンチエイリアス、メイリオ・游ゴシック・ＭＳゴシック順）
    auto CreateJPFont = [](const char* name1, const char* name2, int size, int thick, int fontType) -> int {
        int h = CreateFontToHandle(name1, size, thick, fontType);
        if (h == -1 && name2 != nullptr) {
            h = CreateFontToHandle(name2, size, thick, fontType);
        }
        if (h == -1) {
            h = CreateFontToHandle("MS Gothic", size, thick, DX_FONTTYPE_ANTIALIASING_8X8);
        }
        return h;
    };

    // 小型UI用（ミニマップ、バッテリー、所持数、恐怖度など全HUD表示用日本語フォント）
    fontSmall = CreateJPFont("Meiryo", "Yu Gothic", 16, 2, DX_FONTTYPE_ANTIALIASING_8X8);

    // 中型UI用（プロンプト・通知用日本語フォント）
    fontMedium = CreateJPFont("Meiryo", "Yu Gothic", 20, 3, DX_FONTTYPE_ANTIALIASING_8X8);

    // 大型UI用（バナー見出し等用日本語フォント）
    fontLarge = CreateJPFont("Meiryo", "Yu Gothic", 26, 3, DX_FONTTYPE_ANTIALIASING_8X8);

    // 不気味なJホラー明朝見出し用（バナーや厳粛な告知向け）
    fontHorrorSerif = CreateJPFont("Yu Mincho Demibold", "Yu Mincho", 25, 3, DX_FONTTYPE_ANTIALIASING_8X8);
    if (fontHorrorSerif == -1) fontHorrorSerif = CreateJPFont("MS Mincho", nullptr, 25, 3, DX_FONTTYPE_ANTIALIASING_8X8);

    // 監視端末・VHS等幅英数字フォント（時刻・パーセント・テレメトリ用）
    fontMono = CreateFontToHandle("Consolas", 15, 3, DX_FONTTYPE_ANTIALIASING_8X8);
    if (fontMono == -1) fontMono = fontSmall;

    // 時計フォント（英数字専用・不気味なChillerホラー時計）
    fontClock = CreateFontToHandle("Chiller", 26, 3, DX_FONTTYPE_ANTIALIASING_8X8);
    if (fontClock == -1) fontClock = CreateFontToHandle("Consolas", 20, 2, DX_FONTTYPE_ANTIALIASING_8X8);

    // 大見出し用ホラーフォント（題名やゲーム終了表示などに使用）
    fontTitleHorror = CreateFontToHandle("Chiller", 64, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    if (fontTitleHorror == -1) fontTitleHorror = CreateFontToHandle("MS Gothic", 48, 5, DX_FONTTYPE_ANTIALIASING_EDGE_8X8);

    // 操作方法 [KEY] 表記用（視認性の高い1998監視端末等幅キーバッジ）
    fontKeyGuide = CreateFontToHandle("Consolas", 14, 3, DX_FONTTYPE_ANTIALIASING_8X8);
    if (fontKeyGuide == -1) fontKeyGuide = CreateFontToHandle("Courier New", 14, 3, DX_FONTTYPE_ANTIALIASING_8X8);

    // 操作方法アクション説明用（日本語操作案内フォント）
    fontKeyAction = CreateJPFont("Meiryo", "Yu Gothic", 16, 2, DX_FONTTYPE_ANTIALIASING_8X8);
}

// 終了処理（フォントハンドルおよびエフェクト画面の破棄）
void HorrorUI::Finalize()
{
    if (ghostScreenGraph != -1) { DeleteGraph(ghostScreenGraph); ghostScreenGraph = -1; }
    if (bloodSplatterGraph != -1) { DeleteGraph(bloodSplatterGraph); bloodSplatterGraph = -1; }
    if (fontSmall != -1) { DeleteFontToHandle(fontSmall); fontSmall = -1; }
    if (fontMedium != -1) { DeleteFontToHandle(fontMedium); fontMedium = -1; }
    if (fontLarge != -1) { DeleteFontToHandle(fontLarge); fontLarge = -1; }
    if (fontClock != -1) { DeleteFontToHandle(fontClock); fontClock = -1; }
    if (fontTitleHorror != -1) { DeleteFontToHandle(fontTitleHorror); fontTitleHorror = -1; }
    if (fontKeyGuide != -1) { DeleteFontToHandle(fontKeyGuide); fontKeyGuide = -1; }
    if (fontKeyAction != -1) { DeleteFontToHandle(fontKeyAction); fontKeyAction = -1; }
    if (fontMono != -1) { DeleteFontToHandle(fontMono); fontMono = -1; }
    if (fontHorrorSerif != -1) { DeleteFontToHandle(fontHorrorSerif); fontHorrorSerif = -1; }
}

// 毎フレーム更新（デルタ秒dt対応）
void HorrorUI::Update(float dt)
{
    frameCount++;

    // ゲーム内時計の進行（一時停止中でない場合のみデルタ時間で進行）
    if (!isTimePaused && dt > 0.0f)
    {
        totalGameMinutes += gameMinutesPerRealSecond * dt;
        int totalMins = 60 + static_cast<int>(totalGameMinutes);
        clockHour = (totalMins / 60) % 24;
        clockMinute = totalMins % 60;
    }

    // インタラクトプロンプトのフェードイン/フェードアウト処理
    if (requestedPrompt != PromptType::None)
    {
        if (currentPrompt != requestedPrompt)
        {
            // 別プロンプトへ切り替え時は一度フェードアウト
            promptAlpha -= 0.2f;
            if (promptAlpha <= 0.0f)
            {
                promptAlpha = 0.0f;
                currentPrompt = requestedPrompt;
            }
        }
        else
        {
            // スムーズなフェードイン
            promptAlpha += 0.15f;
            if (promptAlpha > 1.0f) promptAlpha = 1.0f;
        }
    }
    else
    {
        // プロンプト非要求時はフェードアウト
        promptAlpha -= 0.15f;
        if (promptAlpha <= 0.0f)
        {
            promptAlpha = 0.0f;
            currentPrompt = PromptType::None;
        }
    }

    // 次フレーム判定のためリセット
    requestedPrompt = PromptType::None;

    // グリッチ・ジッタータイマーのカウントダウン
    if (glitchTimer > 0) glitchTimer--;
    if (jitterTimer > 0) jitterTimer--;
    if (glitchBurstTimer > 0.0f && dt > 0.0f)
    {
        glitchBurstTimer -= dt;
        if (glitchBurstTimer <= 0.0f)
        {
            glitchBurstTimer = 0.0f;
            glitchBurstStrength = 0.0f;
        }
    }

    // ステージ告知バナータイマーの更新
    if (dayBannerTimer > 0.0f)
    {
        dayBannerTimer -= 1.0f / 60.0f;
        if (dayBannerTimer < 0.0f) dayBannerTimer = 0.0f;
    }
}

// プロンプト要求
void HorrorUI::SetPrompt(PromptType type)
{
    requestedPrompt = type;
}

// 7日間システム設定
void HorrorUI::SetCurrentDay(int day)
{
    if (day < 1) day = 1;
    if (day > 5) day = 5;
    currentDay = day;
}

int HorrorUI::GetCurrentDay() const
{
    return currentDay;
}

void HorrorUI::TriggerDayBanner(float duration)
{
    dayBannerMax = duration;
    dayBannerTimer = duration;
}

// ゲーム内時計の設定（指定時刻へ直接変更し通算分数を同期）
void HorrorUI::SetGameTime(int hour, int minute)
{
    clockHour = hour;
    clockMinute = minute;
    // 午前1時（01:00 AM）を基準0分として通算経過分数を同期
    int fromOneAM = (hour - 1) * 60 + minute;
    if (fromOneAM < 0) fromOneAM = 0;
    totalGameMinutes = static_cast<float>(fromOneAM);
}

void HorrorUI::GetGameTime(int& hour, int& minute) const
{
    hour = clockHour;
    minute = clockMinute;
}

// ゲーム内時刻の直接早送り（デバッグ操作用）
void HorrorUI::AdvanceGameMinutes(float minutes)
{
    totalGameMinutes += minutes;
    if (totalGameMinutes < 0.0f) totalGameMinutes = 0.0f;
    int totalMins = 60 + static_cast<int>(totalGameMinutes);
    clockHour = (totalMins / 60) % 24;
    clockMinute = totalMins % 60;
}

// ゲーム内時刻に基づくホラーレベル取得
// 0: 01:00〜01:59 (序盤・平穏)
// 1: 02:00〜02:59 (中盤・緊張上昇)
// 2: 03:00〜03:59 (怪異頻出・深夜3時の魔窟)
// 3: 04:00以降     (終盤・極限ホラー環境)
int HorrorUI::GetHorrorTimeLevel() const
{
    if (clockHour <= 1) return 0;
    if (clockHour == 2) return 1;
    if (clockHour == 3) return 2;
    return 3;
}

const char* HorrorUI::GetHorrorTimeLevelName() const
{
    switch (GetHorrorTimeLevel())
    {
    case 0: return "EARLY NIGHT (CALM)";
    case 1: return "RISING TENSION";
    case 2: return "WITCHING HOUR (PEAK)";
    case 3: return "LATE NIGHT (EXTREME)";
    default: return "UNKNOWN";
    }
}

// ホラー演出・グリッチAPI
void HorrorUI::TriggerUIGlitch(int durationFrames)
{
    glitchTimer = durationFrames;
}

void HorrorUI::TriggerTextDistortion(int durationFrames)
{
    jitterTimer = durationFrames;
}

void HorrorUI::SetUIInterference(float amount)
{
    if (amount < 0.0f) amount = 0.0f;
    if (amount > 1.0f) amount = 1.0f;
    interferenceAmount = amount;
}

void HorrorUI::TriggerHorrorGlitchBurst(float strength, float durationSeconds)
{
    float s = std::clamp(strength, 0.0f, 1.0f);
    if (durationSeconds <= 0.0f) return;
    if (s >= glitchBurstStrength || glitchBurstTimer <= 0.05f)
    {
        glitchBurstStrength = s;
        glitchBurstTimer = durationSeconds;
        glitchBurstMaxDuration = (std::max)(0.05f, durationSeconds);
    }
}

void HorrorUI::SetWeatherScreenParams(float rainAmount, float snowAmount, float lightningFlash)
{
    weatherRainOverlay = std::clamp(rainAmount, 0.0f, 1.0f);
    weatherSnowOverlay = std::clamp(snowAmount, 0.0f, 1.0f);
    weatherLightningFlash = std::clamp(lightningFlash, 0.0f, 1.0f);
}

float HorrorUI::GetUIInterference() const
{
    return interferenceAmount;
}

void HorrorUI::SetHorrorIntensity(int intensity)
{
    if (intensity < 0) intensity = 0;
    if (intensity > 4) intensity = 4;
    horrorIntensity = intensity;
}

int HorrorUI::GetHorrorIntensity() const
{
    return horrorIntensity;
}

void HorrorUI::ToggleHorrorEffects()
{
    horrorEffectsEnabled = !horrorEffectsEnabled;
}

bool HorrorUI::IsHorrorEffectsEnabled() const
{
    return horrorEffectsEnabled;
}

// ホラー視覚エフェクトパラメータ設定
void HorrorUI::SetHorrorVisualParams(float fear, float deathClose, float deathBlink, float doubleVision, float desat, float focusBlur)
{
    currentFearParam = fear;
    deathClosingProgress = deathClose;
    deathBlinkFactor = deathBlink;
    doubleVisionAmount = doubleVision;
    desaturationAmount = desat;
    focusBlurAmount = focusBlur;
}

// 画面周辺のダーク減光（ビネット）描画（恐怖度連動・死の視野狭窄対応）
void HorrorUI::DrawScreenVignette()
{
    if (!horrorEffectsEnabled) return;

    // 視界を狭める画面縮小（トンネルビジョン）を完全撤廃
    // モンスター接近時でも視界が狭く暗くならないよう、最外周（約24px）のみ極薄の陰影を配置
    float fear = currentFearParam;
    if (fear < 0.0f) fear = 0.0f;
    if (fear > 1.0f) fear = 1.0f;

    // 画面中央やゲームプレイ視界を絶対に遮らない極薄の最外縁グラデーションシャドウ（最大幅24px、最大アルファ25）
    const int steps = 6;
    const int stripSize = 4;
    float maxAlpha = 8.0f + fear * 17.0f;

    for (int i = 0; i < steps; i++)
    {
        int inset = i * stripSize;
        float t = 1.0f - static_cast<float>(i) / static_cast<float>(steps);
        int alpha = static_cast<int>(maxAlpha * t * t);
        if (alpha <= 0) continue;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawBox(inset, inset, 1280 - inset, inset + stripSize, GetColor(0, 0, 0), TRUE);
        DrawBox(inset, 720 - inset - stripSize, 1280 - inset, 720 - inset, GetColor(0, 0, 0), TRUE);
        DrawBox(inset, inset + stripSize, inset + stripSize, 720 - inset - stripSize, GetColor(0, 0, 0), TRUE);
        DrawBox(1280 - inset - stripSize, inset + stripSize, 1280 - inset, 720 - inset - stripSize, GetColor(0, 0, 0), TRUE);
    }

    // 死亡演出最終盤（VisionClosing）でのフェードアウトのみ全画面アルファフェードとして処理（画面縮小矩形は使用しない）
    if (deathClosingProgress > 0.001f)
    {
        int fadeAlpha = static_cast<int>(deathClosingProgress * 255.0f);
        if (fadeAlpha > 255) fadeAlpha = 255;
        if (fadeAlpha > 0)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, fadeAlpha);
            DrawBox(0, 0, 1280, 720, GetColor(0, 0, 0), TRUE);
        }
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// ホラー視覚エフェクトの描画（VHS色収差・トラッキング歪み・心拍エッジ脈動・天候レンズ効果）
void HorrorUI::DrawPostHorrorEffects()
{
    // 1. 天候連動スクリーングラフィック（雷フラッシュ・雨粒レンズ・寒冷霜エッジ）
    if (weatherLightningFlash > 0.01f)
    {
        int flashA = static_cast<int>(weatherLightningFlash * 48.0f);
        if (flashA > 0)
        {
            SetDrawBlendMode(DX_BLENDMODE_ADD, flashA);
            DrawBox(0, 0, 1280, 720, GetColor(165, 190, 235), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    if (weatherRainOverlay > 0.05f)
    {
        // 中央視界を遮らない外周カメラレンズ上の雨粒ボケ＆垂れ筋
        int dropCount = static_cast<int>(8.0f + weatherRainOverlay * 18.0f);
        for (int i = 0; i < dropCount; ++i)
        {
            unsigned int seed = static_cast<unsigned int>(i) * 2654435761u + static_cast<unsigned int>(frameCount / 75) * 97u;
            int px = static_cast<int>(seed % 1280u);
            int py = static_cast<int>(((seed >> 11) % 720u));
            // 中央エリアを除外して周囲のみに配置
            if (px > 260 && px < 1020 && py > 130 && py < 590) continue;

            int slideY = (frameCount * (1 + (i % 2)) + (i * 19)) % 60;
            int dy = (py + slideY) % 720;
            int r = 2 + (i % 4);
            int a = static_cast<int>((22.0f + (i % 3) * 10.0f) * weatherRainOverlay);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
            DrawCircle(px, dy, r, GetColor(180, 200, 220), FALSE);
            DrawLine(px, dy - r - 4, px, dy - r, GetColor(160, 185, 210));
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    if (weatherSnowOverlay > 0.05f)
    {
        // 寒冷な雪夜の最外周フロストエッジ（極薄）
        int frostA = static_cast<int>(18.0f * weatherSnowOverlay);
        for (int i = 0; i < 3; ++i)
        {
            int inset = i * 6;
            int a = frostA * (3 - i) / 3;
            if (a <= 0) continue;
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
            DrawBox(0, inset, 1280, inset + 6, GetColor(185, 210, 240), TRUE);
            DrawBox(0, 720 - inset - 6, 1280, 720 - inset, GetColor(185, 210, 240), TRUE);
            DrawBox(inset, 0, inset + 6, 720, GetColor(185, 210, 240), TRUE);
            DrawBox(1280 - inset - 6, 0, 1280 - inset, 720, GetColor(185, 210, 240), TRUE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    if (!horrorEffectsEnabled) return;

    float burstProgress = (glitchBurstTimer > 0.0f && glitchBurstMaxDuration > 0.001f)
        ? (glitchBurstTimer / glitchBurstMaxDuration)
        : 0.0f;
    float activeGlitch = glitchBurstStrength * burstProgress;
    if (glitchTimer > 0)
    {
        activeGlitch = (std::max)(activeGlitch, 0.45f);
    }

    bool needScreenCopy = (ghostScreenGraph != -1) &&
        (doubleVisionAmount > 0.02f || focusBlurAmount > 0.02f || activeGlitch > 0.03f || currentFearParam >= 0.62f);

    if (needScreenCopy)
    {
        GetDrawScreenGraph(0, 0, 1280, 720, ghostScreenGraph);

        // 二重像（幻覚ゴーストオフセット）
        if (doubleVisionAmount > 0.02f)
        {
            int ghostAlpha = (std::min)(115, static_cast<int>(doubleVisionAmount * 52.0f));
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, ghostAlpha);
            int offX = static_cast<int>(std::sin(frameCount * 0.12f) * 7.0f);
            int offY = static_cast<int>(std::cos(frameCount * 0.09f) * 4.0f);
            DrawGraph(offX, offY, ghostScreenGraph, FALSE);
        }

        // 焦点ボケ・視覚不安定
        if (focusBlurAmount > 0.02f)
        {
            int blurAlpha = (std::min)(95, static_cast<int>(focusBlurAmount * 42.0f));
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, blurAlpha);
            DrawGraph(-3, 0, ghostScreenGraph, FALSE);
            DrawGraph( 3, 0, ghostScreenGraph, FALSE);
            DrawGraph( 0, -2, ghostScreenGraph, FALSE);
            DrawGraph( 0,  2, ghostScreenGraph, FALSE);
        }

        // RGB色収差シフト＆VHSトラッキング水平ティア（ホラーイベント・高恐怖時）
        float chromaLevel = (std::max)(activeGlitch, (currentFearParam >= 0.62f) ? ((currentFearParam - 0.62f) / 0.38f * 0.45f) : 0.0f);
        if (chromaLevel > 0.03f)
        {
            int rgbShift = 2 + static_cast<int>(chromaLevel * 7.0f);
            int rgbAlpha = (std::min)(48, static_cast<int>(16.0f + chromaLevel * 32.0f));
            SetDrawBlendMode(DX_BLENDMODE_ADD, rgbAlpha);
            SetDrawBright(255, 55, 55);
            DrawGraph(-rgbShift, 0, ghostScreenGraph, FALSE);
            SetDrawBright(65, 190, 255);
            DrawGraph(rgbShift, 0, ghostScreenGraph, FALSE);
            SetDrawBright(255, 255, 255);

            // 水平VHSトラッキング乱れ帯
            if (activeGlitch > 0.08f)
            {
                int strips = 2 + static_cast<int>(activeGlitch * 5.0f);
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 165);
                for (int s = 0; s < strips; ++s)
                {
                    int sy = (std::rand() % 680);
                    int sh = 6 + (std::rand() % 18);
                    int dx = ((std::rand() % 2 == 0) ? 1 : -1) * (4 + static_cast<int>(activeGlitch * 20.0f));
                    DrawRectGraph(dx, sy, 0, sy, 1280, sh, ghostScreenGraph, FALSE, FALSE);
                }
            }
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 微細な1998 VHS走査線＆テープノイズ（高恐怖・イベント時にのみ浮き上がる）
    float noiseStrength = (std::max)(activeGlitch * 0.85f, interferenceAmount * 0.45f);
    if (noiseStrength > 0.06f)
    {
        int grainCount = 4 + static_cast<int>(noiseStrength * 14.0f);
        int grainA = (std::min)(65, static_cast<int>(noiseStrength * 55.0f));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, grainA);
        for (int g = 0; g < grainCount; ++g)
        {
            int gy = std::rand() % 720;
            int gx = std::rand() % 1120;
            int gw = 20 + (std::rand() % 140);
            DrawLine(gx, gy, gx + gw, gy, GetColor(190, 200, 195));
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 恐怖度・急迫イベント連動：最外周のみの紅潮アドレナリン脈動（中央視界は一切暗く縮めない）
    if (currentFearParam >= 0.48f || activeGlitch > 0.15f)
    {
        float fearFactor = (currentFearParam >= 0.48f) ? ((currentFearParam - 0.48f) / 0.52f) : 0.0f;
        float pulse = 0.60f + 0.40f * std::sin(static_cast<float>(frameCount) * (0.14f + fearFactor * 0.12f));
        float edgeRed = (std::max)(fearFactor * 0.85f, activeGlitch * 0.70f) * pulse;
        int maxRedA = (std::min)(52, static_cast<int>(edgeRed * 50.0f));
        for (int i = 0; i < 4; ++i)
        {
            int inset = i * 9;
            int a = maxRedA * (4 - i) / 4;
            if (a <= 0) continue;
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
            unsigned int col = GetColor(145, 16, 18);
            DrawBox(0, inset, 1280, inset + 9, col, TRUE);
            DrawBox(0, 720 - inset - 9, 1280, 720 - inset, col, TRUE);
            DrawBox(inset, 0, inset + 9, 720, col, TRUE);
            DrawBox(1280 - inset - 9, 0, 1280 - inset, 720, col, TRUE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

// 画面中央の最小限レティクル（照準）描画
void HorrorUI::DrawCrosshair(bool isTargetingInteractable)
{
    int cx = 640;
    int cy = 360;

    // グリッチ発生時は微小なブレを加える
    if (horrorEffectsEnabled && (glitchTimer > 0 || (interferenceAmount > 0.3f && (frameCount % 60 == 0))))
    {
        cx += (rand() % 3) - 1;
        cy += (rand() % 3) - 1;
    }

    if (!isTargetingInteractable)
    {
        // 通常時：暗縁シャドウ＋控えめなオフホワイト中心ドット
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 110);
        DrawCircle(cx, cy, 3, GetColor(4, 6, 6), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 165);
        DrawCircle(cx, cy, 2, COL_TEXT_DIM, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    else
    {
        // インタラクト可能時：外周シャドウ＋蒼白骨白ドット＋精緻なコーナー十字ブラケット
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 130);
        DrawLine(cx - 8, cy + 1, cx - 3, cy + 1, GetColor(4, 6, 6));
        DrawLine(cx + 4, cy + 1, cx + 9, cy + 1, GetColor(4, 6, 6));
        DrawLine(cx + 1, cy - 8, cx + 1, cy - 3, GetColor(4, 6, 6));
        DrawLine(cx + 1, cy + 4, cx + 1, cy + 9, GetColor(4, 6, 6));

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 225);
        DrawCircle(cx, cy, 2, COL_PLAYER, TRUE);

        // 微小な十字ブラケット（長さ4px、隙間4px）
        DrawLine(cx - 8, cy, cx - 4, cy, COL_TEXT);
        DrawLine(cx + 4, cy, cx + 8, cy, COL_TEXT);
        DrawLine(cx, cy - 8, cx, cy - 4, COL_TEXT);
        DrawLine(cx, cy + 4, cx, cy + 8, COL_TEXT);

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

// アナログ端末風の枠付きパネル描画（1998 VHS・深層ホラーCRT計器デザイン）
void HorrorUI::DrawRetroPanel(int x1, int y1, int x2, int y2, unsigned int borderColor, unsigned int bgColor, int alpha)
{
    int prevMode = DX_BLENDMODE_NOBLEND;
    int prevParam = 255;
    GetDrawBlendMode(&prevMode, &prevParam);

    float parentFade = (prevMode == DX_BLENDMODE_ALPHA) ? (static_cast<float>(prevParam) / 255.0f) : 1.0f;
    int baseA = static_cast<int>(alpha * parentFade);
    if (baseA <= 0) return;

    int w = x2 - x1;
    int h = y2 - y1;

    // 1. 霧や背景からパネルを浮かび上がらせる微細ドロップシャドウ
    int outerA = static_cast<int>(65.0f * (baseA / 255.0f));
    if (outerA > 4)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, outerA);
        DrawBox(x1 - 2, y1 - 1, x2 + 3, y2 + 3, GetColor(2, 3, 4), TRUE);
    }

    // 2. 本体ダーク塗りつぶし
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, baseA);
    DrawBox(x1, y1, x2, y2, bgColor, TRUE);

    // 3. 上部の微かなCRTガラスハイライト帯＆極薄スキャンライン質感
    if (h > 14 && w > 12)
    {
        int glossH = (std::min)(18, h / 3);
        int glossA = static_cast<int>(16.0f * (baseA / 255.0f));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, glossA);
        DrawBox(x1 + 1, y1 + 1, x2 - 1, y1 + glossH, GetColor(125, 145, 138), TRUE);

        int scanA = static_cast<int>(14.0f * (baseA / 255.0f));
        if (scanA > 2)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, scanA);
            for (int sy = y1 + 3; sy < y2 - 2; sy += 3)
            {
                DrawLine(x1 + 2, sy, x2 - 2, sy, GetColor(0, 0, 0));
            }
        }
    }

    // 4. 外枠ラインと微細内縁ベベル
    int borderA = static_cast<int>(240.0f * parentFade);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, borderA);
    DrawBox(x1, y1, x2, y2, borderColor, FALSE);

    if (w > 10 && h > 10)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(95.0f * parentFade));
        DrawBox(x1 + 2, y1 + 2, x2 - 2, y2 - 2, COL_BORDER_DIM, FALSE);

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(38.0f * parentFade));
        DrawLine(x1 + 3, y1 + 1, x2 - 3, y1 + 1, GetColor(180, 195, 188));
    }

    // 5. 四隅の強化コーナーリベット＆ブラケット（ヴィンテージ計器感）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, borderA);
    int cornerLen = (w > 48 && h > 36) ? 8 : 5;
    DrawLine(x1, y1, x1 + cornerLen, y1, borderColor);
    DrawLine(x1, y1, x1, y1 + cornerLen, borderColor);
    DrawLine(x2 - cornerLen, y1, x2, y1, borderColor);
    DrawLine(x2, y1, x2, y1 + cornerLen, borderColor);
    DrawLine(x1, y2 - cornerLen, x1, y2, borderColor);
    DrawLine(x1, y2, x1 + cornerLen, y2, borderColor);
    DrawLine(x2 - cornerLen, y2, x2, y2, borderColor);
    DrawLine(x2, y2 - cornerLen, x2, y2, borderColor);

    // コーナー内側1pxアクセントドット
    if (w > 28 && h > 22)
    {
        DrawBox(x1 + 1, y1 + 1, x1 + 3, y1 + 3, borderColor, TRUE);
        DrawBox(x2 - 2, y1 + 1, x2, y1 + 3, borderColor, TRUE);
        DrawBox(x1 + 1, y2 - 2, x1 + 3, y2, borderColor, TRUE);
        DrawBox(x2 - 2, y2 - 2, x2, y2, borderColor, TRUE);
    }

    // 上下中央の精密キャリブレーション目盛りスリット
    if (w > 54)
    {
        int midX = (x1 + x2) / 2;
        DrawLine(midX - 4, y1, midX + 4, y1, borderColor);
        DrawLine(midX - 4, y2, midX + 4, y2, borderColor);
    }

    SetDrawBlendMode(prevMode, prevParam);
}

// ドロップシャドウ付き文字列描画
void HorrorUI::DrawShadowString(int x, int y, const char* text, unsigned int color, int fontHandle, unsigned int shadowColor, int shadowAlpha)
{
    int prevMode = DX_BLENDMODE_NOBLEND;
    int prevParam = 255;
    GetDrawBlendMode(&prevMode, &prevParam);

    float parentFade = (prevMode == DX_BLENDMODE_ALPHA) ? (static_cast<float>(prevParam) / 255.0f) : 1.0f;
    unsigned int actualShadowCol = (shadowColor == 0) ? GetColor(4, 5, 6) : shadowColor;
    int actualShadowA = static_cast<int>(static_cast<float>(shadowAlpha) * parentFade);

    if (actualShadowA > 4)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, actualShadowA);
        if (fontHandle != -1)
        {
            DrawStringToHandle(x + 1, y + 1, text, actualShadowCol, fontHandle);
        }
        else
        {
            DrawString(x + 1, y + 1, text, actualShadowCol);
        }
    }

    SetDrawBlendMode(prevMode, prevParam);
    if (fontHandle != -1)
    {
        DrawStringToHandle(x, y, text, color, fontHandle);
    }
    else
    {
        DrawString(x, y, text, color);
    }
}

// ノイズ・ジッター・幽霊的シャドウ付き文字列描画
void HorrorUI::DrawJitterString(int x, int y, const char* text, unsigned int color, int fontHandle)
{
    int drawX = x;
    int drawY = y;

    int prevMode = DX_BLENDMODE_NOBLEND;
    int prevParam = 255;
    GetDrawBlendMode(&prevMode, &prevParam);
    float parentFade = (prevMode == DX_BLENDMODE_ALPHA) ? (static_cast<float>(prevParam) / 255.0f) : 1.0f;

    if (horrorEffectsEnabled)
    {
        // グリッチまたは強い干渉時、または低確率での微小な1フレームジッター
        bool shouldJitter = (jitterTimer > 0) || (glitchTimer > 0) ||
                            (horrorIntensity >= 2 && (frameCount % 60 == 0)) ||
                            (interferenceAmount > 0.4f && (rand() % 8 == 0));

        if (shouldJitter)
        {
            drawX += (rand() % 3) - 1;
            drawY += (rand() % 3) - 1;
        }
    }

    // 常設：視認性を引き締める深淵ダークドロップシャドウ
    int darkShadowA = static_cast<int>(195.0f * parentFade);
    if (darkShadowA > 4)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, darkShadowA);
        if (fontHandle != -1)
        {
            DrawStringToHandle(drawX + 1, drawY + 1, text, GetColor(4, 5, 6), fontHandle);
        }
        else
        {
            DrawString(drawX + 1, drawY + 1, text, GetColor(4, 5, 6));
        }
    }

    if (horrorEffectsEnabled)
    {
        // 高恐怖度または干渉時の色収差ゴーストシャドウ（血赤＆冷青スプリット）
        if (horrorIntensity >= 2 || glitchTimer > 0 || interferenceAmount > 0.5f)
        {
            int splitA = static_cast<int>(125.0f * parentFade);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, splitA);
            if (fontHandle != -1)
            {
                DrawStringToHandle(drawX + 1, drawY, text, COL_WARNING, fontHandle);
                DrawStringToHandle(drawX - 1, drawY, text, GetColor(55, 130, 145), fontHandle);
            }
            else
            {
                DrawString(drawX + 1, drawY, text, COL_WARNING);
                DrawString(drawX - 1, drawY, text, GetColor(55, 130, 145));
            }
        }
    }

    SetDrawBlendMode(prevMode, prevParam);
    if (fontHandle != -1)
    {
        DrawStringToHandle(drawX, drawY, text, color, fontHandle);
    }
    else
    {
        DrawString(drawX, drawY, text, color);
    }
}

// インタラクト案内のフェード描画
void HorrorUI::DrawInteractionPrompt()
{
    if (promptAlpha <= 0.01f || currentPrompt == PromptType::None)
    {
        return;
    }

    if (currentPrompt == PromptType::BatteryFull)
    {
        const char* line1 = "バッテリー満杯";
        const char* line2 = "予備 2/2";
        int w1 = (fontMedium != -1) ? GetDrawStringWidthToHandle(line1, (int)strlen(line1), fontMedium) : 130;
        int w2 = (fontSmall != -1) ? GetDrawStringWidthToHandle(line2, (int)strlen(line2), fontSmall) : 80;
        int maxW = (w1 > w2) ? w1 : w2;
        int posX = 640 - maxW / 2;
        int posY = 458;

        int alpha = (int)(promptAlpha * 230.0f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

        DrawRetroPanel(posX - 20, posY - 6, posX + maxW + 20, posY + 48, COL_WARNING, GetColor(20, 10, 12), 235);
        DrawBox(posX - 19, posY - 2, posX - 16, posY + 44, COL_WARNING, TRUE);

        DrawShadowString(640 - w1 / 2, posY, line1, COL_WARNING, fontMedium);
        DrawShadowString(640 - w2 / 2, posY + 26, line2, COL_TEXT, fontSmall);

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        return;
    }

    const char* promptText = "";
    switch (currentPrompt)
    {
    case PromptType::RideBicycle:
        promptText = "[ E ] 自転車に乗る";
        break;
    case PromptType::DismountBicycle:
        promptText = "[ E ] 自転車から降りる";
        break;
    case PromptType::TakeNewspaper:
        promptText = "[ LMB ] 新聞を取る";
        break;
    case PromptType::DeliverNewspaper:
        promptText = "[ LMB ] 新聞を配達する";
        break;
    case PromptType::TakeBattery:
        promptText = "[ LMB ] バッテリーを拾う";
        break;
    case PromptType::ReplaceBattery:
        promptText = "[ R ] バッテリーを交換する";
        break;
    default:
        return;
    }

    int textLen = (int)strlen(promptText);
    int actualWidth = (fontMedium != -1) ? GetDrawStringWidthToHandle(promptText, textLen, fontMedium) : textLen * 11;
    int posX = 640 - actualWidth / 2;
    int posY = 470;

    int alpha = (int)(promptAlpha * 230.0f);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

    // 控えめなホラー端末パネル枠付きバックプレート（文字幅に応じた動的パディング）
    DrawRetroPanel(posX - 18, posY - 5, posX + actualWidth + 18, posY + 25, COL_BORDER, COL_BG, 225);
    DrawBox(posX - 17, posY, posX - 15, posY + 20, COL_TARGET, TRUE);

    // ドロップシャドウ付きプロンプト文字列の描画
    DrawShadowString(posX, posY, promptText, COL_TEXT, fontMedium);

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// ゲーム内時計・日数HUDの描画（画面左上: 例 DAY 1/5, 01:00 AM）
void HorrorUI::DrawClock()
{
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d AM", clockHour, clockMinute);

    char dayStr[32];
    snprintf(dayStr, sizeof(dayStr), "DAY %d/5", currentDay);

    int panelX1 = 26;
    int panelY1 = 12;
    int panelX2 = 236;
    int panelY2 = 38;

    DrawRetroPanel(panelX1, panelY1, panelX2, panelY2, COL_BORDER, COL_PANEL_BG, 195);
    // 左端ステータスアクセント（褪せたアンバー）
    DrawBox(panelX1 + 2, panelY1 + 4, panelX1 + 4, panelY2 - 4, COL_TARGET, TRUE);

    int monoF = (fontMono != -1) ? fontMono : fontSmall;
    DrawJitterString(panelX1 + 11, panelY1 + 5, dayStr, COL_TARGET, monoF);

    // 中央縦区切りヘアライン
    DrawLine(panelX1 + 116, panelY1 + 6, panelX1 + 116, panelY2 - 6, COL_BORDER_DIM);

    // 右側：ゲーム内時刻表記（等幅ホラー監視時計・右揃え配置）
    int timeW = (monoF != -1) ? GetDrawStringWidthToHandle(timeStr, (int)strlen(timeStr), monoF) : 65;
    DrawJitterString(panelX2 - timeW - 10, panelY1 + 5, timeStr, COL_TEXT, monoF);
}

// 夜開始時のステージ告知バナー描画（画面上部中央フェード）
void HorrorUI::DrawDayBanner(const char* dayName, const char* subtitle, int quota)
{
    if (dayBannerTimer <= 0.0f) return;

    float alphaFactor = 1.0f;
    float elapsed = dayBannerMax - dayBannerTimer;
    if (elapsed < 0.6f)
    {
        alphaFactor = elapsed / 0.6f;
    }
    else if (dayBannerTimer < 0.8f)
    {
        alphaFactor = dayBannerTimer / 0.8f;
    }

    int alpha = static_cast<int>(alphaFactor * 232.0f);
    if (alpha <= 0) return;

    int cx = 640;
    int bw = 640;
    int bh = 68;
    int by1 = 55;
    int by2 = by1 + bh;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawRetroPanel(cx - bw / 2, by1, cx + bw / 2, by2, GetColor(115, 58, 52), COL_PANEL_BG, 235);

    // 左右装飾アクセントライン
    DrawBox(cx - bw / 2 + 3, by1 + 6, cx - bw / 2 + 5, by2 - 6, COL_WARNING, TRUE);
    DrawBox(cx + bw / 2 - 5, by1 + 6, cx + bw / 2 - 3, by2 - 6, COL_WARNING, TRUE);

    // 見出し（Jホラー明朝フォント＋暗赤グロウシャドウ）
    char titleBuf[64];
    snprintf(titleBuf, sizeof(titleBuf), "--- %s ---", dayName ? dayName : "MIDNIGHT SHIFT");
    int titleFont = (fontHorrorSerif != -1) ? fontHorrorSerif : fontLarge;
    int tw = (titleFont != -1) ? GetDrawStringWidthToHandle(titleBuf, (int)strlen(titleBuf), titleFont) : 200;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(alpha * 0.45f));
    DrawStringToHandle(cx - tw / 2, by1 + 7, titleBuf, GetColor(165, 35, 30), titleFont);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawJitterString(cx - tw / 2, by1 + 6, titleBuf, GetColor(225, 130, 72), titleFont);

    // 見出し下の区切り線
    DrawLine(cx - bw / 2 + 40, by1 + 35, cx + bw / 2 - 40, by1 + 35, GetColor(75, 45, 42));

    char subBuf[128];
    snprintf(subBuf, sizeof(subBuf), "配達目標: %02d軒  |  %s", quota, subtitle ? subtitle : "");
    int sw = (fontSmall != -1) ? GetDrawStringWidthToHandle(subBuf, (int)strlen(subBuf), fontSmall) : 200;
    DrawJitterString(cx - sw / 2, by1 + 40, subBuf, COL_TEXT, fontSmall);

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 新聞所持数・ステータスの描画（画面左下: ミニマップと横幅210pxを完全統一）
void HorrorUI::DrawPaperCargo(int paperCount, bool isHolding)
{
    char paperStr[32];
    snprintf(paperStr, sizeof(paperStr), "新聞  %02d部", paperCount);

    int panelX1 = 26;
    int panelY1 = 668;
    int panelX2 = 236;
    int panelY2 = 698;

    // レトロ端末パネル枠と背景の描画（ミニマップと横幅210pxを完全統一）
    DrawRetroPanel(panelX1, panelY1, panelX2, panelY2, COL_BORDER, COL_PANEL_BG, 195);
    DrawBox(panelX1 + 2, panelY1 + 5, panelX1 + 4, panelY2 - 5, (paperCount > 0 || isHolding) ? COL_BORDER : COL_WARNING, TRUE);

    // 左側：所持部数
    DrawJitterString(panelX1 + 11, panelY1 + 6, paperStr, COL_TEXT, fontSmall);

    // 右側：手持ち状態の表示（右揃え配置で重なりを完全排除）
    if (isHolding)
    {
        const char* holdStr = "[手持ち]";
        int holdW = (fontSmall != -1) ? GetDrawStringWidthToHandle(holdStr, (int)strlen(holdStr), fontSmall) : 60;
        bool blink = ((frameCount / 25) % 2) == 0;
        unsigned int col = blink ? GetColor(235, 130, 62) : COL_TEXT_DIM;
        DrawJitterString(panelX2 - holdW - 10, panelY1 + 6, holdStr, col, fontSmall);
    }
}

// 操作キー案内HUD描画（画面右上パネル）
void HorrorUI::DrawKeyGuide(bool isRiding, bool isHoldingPaper, bool canTakeOrDeliver, bool isHeadlightOn, bool isMapDebugOn, bool canReplaceBattery)
{
    if (!showKeyGuide)
    {
        // 操作ガイド非表示時は右上端に最小限のトグル案内のみ表示
        int minX1 = 1085;
        int minY1 = 20;
        int minX2 = 1255;
        int minY2 = 50;
        DrawRetroPanel(minX1, minY1, minX2, minY2, COL_BORDER, COL_PANEL_BG, 185);
        DrawBox(minX1 + 2, minY1 + 5, minX1 + 4, minY2 - 5, COL_BORDER_DIM, TRUE);
        DrawJitterString(minX1 + 12, minY1 + 6, "[ TAB ] 操作説明", COL_TEXT_DIM, fontKeyAction);
        return;
    }

    int kx1 = 960;
    int ky1 = 20;
    int kx2 = 1255;
    int ky2 = 326;

    // レトロ端末パネル枠と背景の描画
    DrawRetroPanel(kx1, ky1, kx2, ky2, COL_BORDER, COL_PANEL_BG, 200);

    // 見出しタイトル（赤錆アクセント付き）
    DrawBox(kx1 + 8, ky1 + 7, kx1 + 11, ky1 + 19, COL_WARNING, TRUE);
    DrawJitterString(kx1 + 17, ky1 + 5, "操作方法", GetColor(218, 178, 165), fontKeyAction);

    // 区切り線
    DrawLine(kx1 + 6, ky1 + 25, kx2 - 6, ky1 + 25, COL_BORDER);

    // 各操作キーと説明
    struct KeyItem {
        const char* key;
        const char* action;
        unsigned int keyCol;
        unsigned int actCol;
    };

    unsigned int defaultKeyCol = GetColor(198, 212, 204);
    unsigned int defaultActCol = COL_TEXT_DIM;
    unsigned int activeKeyCol = GetColor(248, 232, 168);
    unsigned int activeActCol = COL_TEXT;
    unsigned int bikeCol = COL_BIKE;

    // Eキー（乗車／降車）
    const char* eAction = isRiding ? "自転車から降りる" : "自転車に乗る";
    unsigned int eKeyCol = isRiding ? bikeCol : defaultKeyCol;
    unsigned int eActCol = isRiding ? bikeCol : defaultActCol;

    // 左クリック（受取／配達）
    const char* lmbAction = isHoldingPaper ? "配達する" : "新聞を取る";
    unsigned int lmbKeyCol = canTakeOrDeliver ? activeKeyCol : defaultKeyCol;
    unsigned int lmbActCol = canTakeOrDeliver ? activeActCol : defaultActCol;

    // Fキー（ライト点消灯）
    const char* fAction = isHeadlightOn ? "ライト (点灯中)" : "ライト点灯";
    unsigned int fKeyCol = isHeadlightOn ? GetColor(242, 218, 132) : defaultKeyCol;
    unsigned int fActCol = isHeadlightOn ? GetColor(225, 208, 152) : defaultActCol;

    // F6キー（マップ表示）
    const char* f6Action = isMapDebugOn ? "マップ (表示中)" : "マップ情報";
    unsigned int f6KeyCol = isMapDebugOn ? GetColor(145, 220, 185) : defaultKeyCol;
    unsigned int f6ActCol = isMapDebugOn ? GetColor(135, 190, 168) : defaultActCol;

    const char* rAction = "バッテリー交換";
    unsigned int rKeyCol = canReplaceBattery ? activeKeyCol : defaultKeyCol;
    unsigned int rActCol = canReplaceBattery ? activeActCol : defaultActCol;

    KeyItem items[] = {
        { "[ WASD ]", isRiding ? "ペダル / 操縦" : "移動", defaultKeyCol, defaultActCol },
        { "[ MOUSE ]", "視線移動", defaultKeyCol, defaultActCol },
        { "[ E ]", eAction, eKeyCol, eActCol },
        { "[ LMB ]", lmbAction, lmbKeyCol, lmbActCol },
        { "[ R ]", rAction, rKeyCol, rActCol },
        { "[ F ]", fAction, fKeyCol, fActCol },
        { "[ H ]", "遊び方", activeKeyCol, activeActCol },
        { "[ ESC ]", "ポーズメニュー", defaultKeyCol, defaultActCol },
        { "[ F6 ]", f6Action, f6KeyCol, f6ActCol },
        { "[ TAB ]", "ガイド非表示", defaultKeyCol, defaultActCol }
    };

    int startY = ky1 + 31;
    int lineSpacing = 28;
    int nItems = sizeof(items) / sizeof(items[0]);

    for (int i = 0; i < nItems; ++i)
    {
        int curY = startY + i * lineSpacing;

        // 交互の超薄行ハイライト（可読性・一覧性向上）
        if ((i % 2) == 1)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 34);
            DrawBox(kx1 + 6, curY - 1, kx2 - 6, curY + 23, GetColor(28, 36, 33), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // キーバッジ背景スロット（等幅キートップ風）
        int badgeX1 = kx1 + 9;
        int badgeY1 = curY + 1;
        int badgeX2 = kx1 + 91;
        int badgeY2 = curY + 21;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
        DrawBox(badgeX1, badgeY1, badgeX2, badgeY2, GetColor(18, 24, 22), TRUE);
        DrawBox(badgeX1, badgeY1, badgeX2, badgeY2, GetColor(48, 60, 55), FALSE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        int keyW = (fontKeyGuide != -1) ? GetDrawStringWidthToHandle(items[i].key, (int)strlen(items[i].key), fontKeyGuide) : 56;
        int keyDrawX = badgeX1 + (badgeX2 - badgeX1 - keyW) / 2;
        DrawJitterString(keyDrawX, curY + 3, items[i].key, items[i].keyCol, fontKeyGuide);

        // アクション表記（十分な水平オフセットとシャドウで鮮明に表示）
        DrawJitterString(kx1 + 100, curY + 2, items[i].action, items[i].actCol, fontKeyAction);
    }
}

// 自転車ヘッドライト・バッテリーHUD描画（画面左側、ミニマップ直下に配置）
void HorrorUI::DrawBicycleBattery(float batteryPercent, bool isLightOn, bool isRiding, bool isFastDrain, int spareBatteries, int maxSpareBatteries)
{
    (void)isRiding;

    // ミニマップ枠 (26, 74)〜(236, 269) の直下、Y=276〜348 に配置
    int panelX1 = 26;
    int panelY1 = 276;
    int panelX2 = 236;
    int panelY2 = 348;

    // レトロ端末パネル外枠と背景の描画
    DrawRetroPanel(panelX1, panelY1, panelX2, panelY2, COL_BORDER, COL_PANEL_BG, 195);

    // 上段：ラベル・点灯状態・残量パーセント表示
    int textY = panelY1 + 5;

    // 「ライト」固定ラベル
    DrawJitterString(panelX1 + 10, textY, "ライト", COL_TEXT_DIM, fontSmall);

    // 点灯状態インジケータ
    if (batteryPercent <= 0.0f)
    {
        // バッテリー完全枯渇時は点滅表示（充電切）
        bool blink = ((frameCount / 20) % 2) == 0;
        unsigned int deadCol = blink ? COL_WARNING : GetColor(110, 35, 30);
        DrawJitterString(panelX1 + 62, textY, "充電切", deadCol, fontSmall);
    }
    else if (isLightOn)
    {
        // 点灯中（淡い蛍光ライト色）
        DrawJitterString(panelX1 + 62, textY, "点灯", GetColor(225, 232, 210), fontSmall);
    }
    else
    {
        // 消灯中
        DrawJitterString(panelX1 + 62, textY, "消灯", COL_TEXT_DIM, fontSmall);
    }

    // デバッグ高速消費インジケータ（F8有効時）
    int monoF = (fontMono != -1) ? fontMono : fontSmall;
    if (isFastDrain)
    {
        bool blink = ((frameCount / 15) % 2) == 0;
        unsigned int debugCol = blink ? GetColor(235, 145, 55) : GetColor(145, 82, 28);
        DrawJitterString(panelX1 + 104, textY, "[10x]", debugCol, monoF);
    }

    // バッテリー残量数値表示（動的右揃えで枠端と完璧に整列）
    char pctStr[16];
    int displayPct = (int)ceilf(batteryPercent);
    if (displayPct < 0) displayPct = 0;
    if (displayPct > 100) displayPct = 100;
    snprintf(pctStr, sizeof(pctStr), "%3d%%", displayPct);

    // 残量に応じた文字色（通常: オフホワイト、低残量: アンバー、危険: 赤）
    unsigned int pctColor = COL_TEXT;
    if (batteryPercent <= 0.0f)
    {
        pctColor = COL_WARNING;
    }
    else if (batteryPercent <= 20.0f)
    {
        bool blink = ((frameCount / 25) % 2) == 0;
        pctColor = blink ? COL_WARNING : GetColor(195, 102, 72);
    }
    else if (batteryPercent <= 50.0f)
    {
        pctColor = GetColor(210, 170, 68);
    }

    int pctW = (monoF != -1) ? GetDrawStringWidthToHandle(pctStr, (int)strlen(pctStr), monoF) : 34;
    DrawJitterString(panelX2 - pctW - 10, textY, pctStr, pctColor, monoF);

    // 下段：アナログ10セグメント・バッテリー残量ゲージ
    int barX1 = panelX1 + 10;
    int barX2 = panelX2 - 10;
    int barY1 = panelY1 + 25;
    int barY2 = panelY1 + 38;

    // ゲージ全体の黒背景と薄い枠線
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 195);
    DrawBox(barX1, barY1, barX2, barY2, GetColor(6, 9, 8), TRUE);
    DrawBox(barX1, barY1, barX2, barY2, COL_BORDER_DIM, FALSE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 10個の独立した長方形セルによるレトロ液晶メーター
    int cellCount = 10;
    int cellGap = 2;
    int totalWidth = (barX2 - 2) - (barX1 + 2);
    int cellWidth = (totalWidth - (cellCount - 1) * cellGap) / cellCount;
    int startInnerX = barX1 + 2 + (totalWidth - (cellWidth * cellCount + (cellCount - 1) * cellGap)) / 2;

    for (int i = 0; i < cellCount; i++)
    {
        int cx1 = startInnerX + i * (cellWidth + cellGap);
        int cx2 = cx1 + cellWidth;
        int cy1 = barY1 + 2;
        int cy2 = barY2 - 2;

        float cellThreshold = (i + 1) * 10.0f;
        bool isCellFilled = (batteryPercent >= cellThreshold - 5.0f);

        if (isCellFilled && batteryPercent > 0.0f)
        {
            unsigned int cellColor = COL_TEXT;
            unsigned int hiColor = GetColor(225, 232, 225);
            if (batteryPercent <= 20.0f)
            {
                bool blink = (batteryPercent <= 5.0f) && (((frameCount / 10) % 2) == 0);
                cellColor = blink ? GetColor(85, 22, 18) : COL_WARNING;
                hiColor = blink ? GetColor(125, 45, 40) : GetColor(245, 110, 95);
            }
            else if (batteryPercent <= 50.0f)
            {
                cellColor = GetColor(205, 160, 60);
                hiColor = GetColor(245, 210, 120);
            }
            else
            {
                cellColor = GetColor(182, 195, 185);
                hiColor = GetColor(225, 238, 228);
            }

            DrawBox(cx1, cy1, cx2, cy2, cellColor, TRUE);
            DrawLine(cx1, cy1, cx2, cy1, hiColor);
        }
        else
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
            DrawBox(cx1, cy1, cx2, cy2, GetColor(24, 30, 27), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    // 予備バッテリー区切り線
    DrawLine(panelX1 + 6, panelY1 + 42, panelX2 - 6, panelY1 + 42, COL_BORDER_DIM);

    // 予備バッテリー所持数表示（予備 0/2, 予備 1/2, 予備 2/2）
    char spareStr[32];
    snprintf(spareStr, sizeof(spareStr), "予備 %d/%d", spareBatteries, maxSpareBatteries);

    unsigned int spareCol = COL_TEXT_DIM;
    if (spareBatteries >= maxSpareBatteries)
    {
        spareCol = GetColor(145, 225, 178);
    }
    else if (spareBatteries > 0)
    {
        spareCol = COL_TEXT;
    }

    DrawJitterString(panelX1 + 10, panelY1 + 47, spareStr, spareCol, fontSmall);

    // 予備バッテリー所持スロットLEDランプ（2個）
    for (int s = 0; s < maxSpareBatteries; ++s)
    {
        int sx1 = panelX2 - 34 + s * 13;
        int sy1 = panelY1 + 49;
        int sx2 = sx1 + 9;
        int sy2 = sy1 + 9;

        if (s < spareBatteries)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 55);
            DrawBox(sx1 - 2, sy1 - 2, sx2 + 2, sy2 + 2, GetColor(90, 210, 145), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(sx1, sy1, sx2, sy2, GetColor(115, 220, 155), TRUE);
            DrawBox(sx1, sy1, sx2, sy2, GetColor(185, 250, 208), FALSE);
        }
        else
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 130);
            DrawBox(sx1, sy1, sx2, sy2, COL_BG, TRUE);
            DrawBox(sx1, sy1, sx2, sy2, COL_BORDER, FALSE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }
}

// 恐怖度メーターHUD描画（画面左上、時計パネル直下・ミニマップ直上に配置）
void HorrorUI::DrawFearMeter(float fearPercent, bool inSafeLight)
{
    // 表示用恐怖度パーセントの滑らかな補間（急激なバーの跳ねを防止）
    displayedFearPercent += (fearPercent - displayedFearPercent) * 0.12f;
    if (displayedFearPercent < 0.0f) displayedFearPercent = 0.0f;
    if (displayedFearPercent > 100.0f) displayedFearPercent = 100.0f;

    int panelX1 = 26;
    int panelY1 = 42;
    int panelX2 = 236;
    int panelY2 = 68;

    unsigned int fearColor = COL_TEXT_DIM;
    int jitterX = 0;
    int jitterY = 0;

    if (displayedFearPercent >= 80.0f)
    {
        fearColor = GetColor(225, 48, 46);
        if ((frameCount % 4) == 0)
        {
            jitterX = (rand() % 3) - 1;
            jitterY = (rand() % 3) - 1;
        }
    }
    else if (displayedFearPercent >= 60.0f)
    {
        fearColor = GetColor(218, 92, 58);
        if ((frameCount % 8) == 0)
        {
            jitterX = (rand() % 2) - 1;
        }
    }
    else if (displayedFearPercent >= 40.0f)
    {
        fearColor = GetColor(210, 148, 62);
    }
    else if (displayedFearPercent >= 20.0f)
    {
        fearColor = GetColor(172, 182, 138);
    }
    else
    {
        fearColor = COL_TEXT_DIM;
    }

    unsigned int panelBorder = (displayedFearPercent >= 60.0f) ? GetColor(135, 50, 48) : COL_BORDER;
    DrawRetroPanel(panelX1, panelY1, panelX2, panelY2, panelBorder, COL_BG, 195);
    DrawBox(panelX1 + 2, panelY1 + 4, panelX1 + 4, panelY2 - 4, fearColor, TRUE);

    // 上段左側：恐怖度数値ラベル
    char fearStr[32];
    snprintf(fearStr, sizeof(fearStr), "恐怖度 %02d%%", static_cast<int>(displayedFearPercent));
    DrawJitterString(panelX1 + 11 + jitterX, panelY1 + 2 + jitterY, fearStr, fearColor, fontSmall);

    // 上段右側：安全光判定インジケータ（極薄グロウオーラ付き・右揃え）
    if (inSafeLight)
    {
        const char* safeStr = "安全";
        int strW = (fontSmall != -1) ? GetDrawStringWidthToHandle(safeStr, (int)strlen(safeStr), fontSmall) : 36;
        int sx = panelX2 - strW - 10;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 36);
        DrawBox(sx - 4, panelY1 + 3, sx + strW + 4, panelY1 + 17, GetColor(85, 185, 130), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawJitterString(sx, panelY1 + 2, safeStr, GetColor(125, 205, 155), fontSmall);
    }
    else
    {
        const char* darkStr = "暗闇";
        int strW = (fontSmall != -1) ? GetDrawStringWidthToHandle(darkStr, (int)strlen(darkStr), fontSmall) : 36;
        int dx = panelX2 - strW - 10;
        bool blink = ((frameCount / 20) % 2) == 0;
        unsigned int darkCol = blink ? GetColor(235, 72, 62) : GetColor(170, 48, 42);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, blink ? 48 : 22);
        DrawBox(dx - 4, panelY1 + 3, dx + strW + 4, panelY1 + 17, GetColor(180, 35, 32), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawJitterString(dx, panelY1 + 2, darkStr, darkCol, fontSmall);
    }

    // 下段：ゲージスロット背景枠・段階メモリ刻みおよび恐怖度進行バー
    int barX1 = panelX1 + 10;
    int barX2 = panelX2 - 10;
    int barY1 = panelY1 + 19;
    int barY2 = panelY1 + 24;
    int barW = barX2 - barX1;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 165);
    DrawBox(barX1, barY1, barX2, barY2, GetColor(8, 12, 10), TRUE);
    DrawBox(barX1, barY1, barX2, barY2, COL_BORDER_DIM, FALSE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ゲージ塗りつぶし＋上部ハイライト線
    int fillW = static_cast<int>((displayedFearPercent / 100.0f) * static_cast<float>(barW - 2));
    if (fillW > 0)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 230);
        DrawBox(barX1 + 1, barY1 + 1, barX1 + 1 + fillW, barY2 - 1, fearColor, TRUE);
        DrawLine(barX1 + 1, barY1 + 1, barX1 + 1 + fillW, barY1 + 1, GetColor(255, 210, 195));
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 恐怖段階しきい値目盛り (20%, 40%, 60%, 80%)
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
    for (int t = 1; t <= 4; ++t)
    {
        int tx = barX1 + (barW * t) / 5;
        DrawLine(tx, barY1 + 1, tx, barY2 - 1, GetColor(6, 8, 8));
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 最大恐怖演出時の画面歪曲・衝撃パルス描画（視界クリア保護＋衝撃ショックフレーム）
void HorrorUI::DrawScareDistortion(float intensity)
{
    if (intensity <= 0.0f) return;

    // 画面を暗く縮める処理を完全排除：モンスターの姿がはっきりと見えるよう視界をクリアに保持
    int flashAlpha = (std::min)(44, static_cast<int>(intensity * 38.0f));
    if (flashAlpha > 0)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, flashAlpha);
        DrawBox(0, 0, 1280, 720, GetColor(165, 16, 18), TRUE);
    }

    // 外周エッジのみの鮮烈な襲撃ショック枠＆ランダムVHS水平裂けライン
    int rimAlpha = (std::min)(130, static_cast<int>(intensity * 115.0f));
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, rimAlpha);
    DrawBox(4, 4, 1276, 716, GetColor(210, 30, 30), FALSE);
    DrawBox(10, 10, 1270, 710, GetColor(175, 22, 22), FALSE);

    int barCount = 3 + static_cast<int>(intensity * 7.0f);
    for (int i = 0; i < barCount; ++i)
    {
        int y = std::rand() % 720;
        int h = 1 + (std::rand() % 3);
        int x1 = std::rand() % 320;
        int x2 = 960 + (std::rand() % 320);
        DrawBox(x1, y, x2, y + h, GetColor(220, 45, 45), TRUE);
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// ゲームオーバー描画
void HorrorUI::DrawGameOver(float fadeAlpha, bool isInteractive)
{
    // 暗黒フェード
    int alpha = static_cast<int>(fadeAlpha * 255.0f);
    if (alpha > 255) alpha = 255;
    if (alpha > 0)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawBox(0, 0, 1280, 720, GetColor(3, 4, 6), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    if (!isInteractive) return;

    // 中央ダイアログパネル
    int cx = 640;
    int cy = 360;

    DrawRetroPanel(cx - 240, cy - 100, cx + 240, cy + 100, GetColor(125, 42, 40), COL_BG, 235);

    // ゲームオーバータイトル（ホラー明朝/Chiller＋暗赤色収差＋センタリング）
    const char* overStr = "GAME OVER";
    int titleF = (fontTitleHorror != -1) ? fontTitleHorror : fontLarge;
    if (titleF != -1)
    {
        int ow = GetDrawStringWidthToHandle(overStr, (int)strlen(overStr), titleF);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
        DrawStringToHandle(cx - ow / 2 + 2, cy - 82 + 2, overStr, GetColor(85, 8, 10), titleF);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawShadowString(cx - ow / 2, cy - 82, overStr, GetColor(220, 42, 38), titleF);
    }
    else
    {
        int ow = GetDrawStringWidth(overStr, (int)strlen(overStr));
        DrawString(cx - ow / 2, cy - 65, overStr, GetColor(200, 50, 45));
    }

    // ホラー演出テキスト（Jホラー調シャドウ）
    const char* darkMsg = "暗闇に呑み込まれた...";
    int darkFont = (fontMedium != -1) ? fontMedium : fontSmall;
    int dw = (darkFont != -1) ? GetDrawStringWidthToHandle(darkMsg, (int)strlen(darkMsg), darkFont) : GetDrawStringWidth(darkMsg, (int)strlen(darkMsg));
    DrawShadowString(cx - dw / 2, cy - 10, darkMsg, COL_TEXT_DIM, darkFont);

    // 操作案内（リトライ・終了・動的センタリング）
    bool blink = ((frameCount / 30) % 2) == 0;
    unsigned int promptCol = blink ? COL_TEXT : COL_TEXT_DIM;
    const char* retryStr = "[ R ] 配達をやり直す";
    int rw = (fontSmall != -1) ? GetDrawStringWidthToHandle(retryStr, (int)strlen(retryStr), fontSmall) : GetDrawStringWidth(retryStr, (int)strlen(retryStr));
    DrawShadowString(cx - rw / 2, cy + 40, retryStr, promptCol, fontSmall);

    const char* quitStr = "[ ESC ] ゲーム終了";
    int qw = (fontSmall != -1) ? GetDrawStringWidthToHandle(quitStr, (int)strlen(quitStr), fontSmall) : GetDrawStringWidth(quitStr, (int)strlen(quitStr));
    DrawShadowString(cx - qw / 2, cy + 65, quitStr, COL_TEXT_DIM, fontSmall);
}

// 襲撃・被弾時のカメラレンズ血痕スプラッター描画
// 襲撃・被弾時のリアルなカメラレンズ血痕スプラッター描画
void HorrorUI::DrawBloodSplatter(float intensity)
{
    if (intensity <= 0.001f) return;

    int alpha = static_cast<int>(intensity * 255.0f);
    if (alpha > 255) alpha = 255;

    // リアルな有機的流体・高圧動脈血飛沫・重力流下の高解像度テクスチャを描画
    if (bloodSplatterGraph != -1)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawGraph(0, 0, bloodSplatterGraph, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

// 画面内目的地案内UI（ワールド空間上の目的地マーカーおよび画面端インジケータ、距離表示付き）
void HorrorUI::DrawWorldDestinationMarker(const VECTOR& playerPos, const VECTOR& targetPos, int targetHouseId, bool isAllComplete)
{
    // 全配達完了時または有効な配達先がない場合は非表示
    if (isAllComplete || targetHouseId < 0)
    {
        return;
    }

    // プレイヤーから目的地までの水平距離を計算
    float diffX = targetPos.x - playerPos.x;
    float diffZ = targetPos.z - playerPos.z;
    float distance = sqrtf(diffX * diffX + diffZ * diffZ);
    int distMeters = static_cast<int>(distance);

    // 目的地のポスト上部（高さ約1.35m）にマーカーの中心を設定
    VECTOR worldMarkerPos = targetPos;
    worldMarkerPos.y += 1.35f;

    // 現在のカメラパラメータを取得
    VECTOR camPos = GetCameraPosition();
    VECTOR camFront = GetCameraFrontVector();
    VECTOR camRight = GetCameraRightVector();
    VECTOR camUp = GetCameraUpVector();

    // カメラから目的地への3次元ベクトル
    VECTOR toTarget = VSub(worldMarkerPos, camPos);

    // カメラのローカル（ビュー）座標系への射影成分を計算
    float localX = VDot(toTarget, camRight);
    float localY = VDot(toTarget, camUp);
    float localZ = VDot(toTarget, camFront);

    // 画面解像度
    const int screenW = 1280;
    const int screenH = 720;
    const float screenCenterX = screenW * 0.5f;
    const float screenCenterY = screenH * 0.5f;

    // 画面内判定のセーフマージン（この内側にあれば画面内マーカーとして表示）
    const float marginInScreen = 55.0f;

    int markerX = 0;
    int markerY = 0;
    bool isOffScreen = true;
    float arrowAngle = 0.0f;

    // カメラ前方（localZ > 0.3f）にある場合、画面投影を試行
    if (localZ > 0.3f)
    {
        VECTOR screenPos = ConvWorldPosToScreenPos(worldMarkerPos);

        // DxLibの射影座標が有効範囲内かつセーフエリア内にあるか判定
        if (screenPos.z >= 0.0f && screenPos.z <= 1.0f &&
            screenPos.x >= marginInScreen && screenPos.x <= (screenW - marginInScreen) &&
            screenPos.y >= marginInScreen && screenPos.y <= (screenH - marginInScreen))
        {
            markerX = static_cast<int>(screenPos.x);
            markerY = static_cast<int>(screenPos.y);
            isOffScreen = false;
        }
    }

    // 画面外（Off-Screen）の場合の方向ベクトルと画面端クランプ座標を計算
    if (isOffScreen)
    {
        float dirX = 0.0f;
        float dirY = 0.0f;

        if (localZ > 0.1f)
        {
            // カメラ前方だが画面外にある場合
            VECTOR screenPos = ConvWorldPosToScreenPos(worldMarkerPos);
            dirX = screenPos.x - screenCenterX;
            dirY = screenPos.y - screenCenterY;
        }
        else
        {
            // カメラ後方または真横にある場合：
            // カメラローカルのX（右方向）とY（上方向）から、
            // プレイヤーがカメラをその方向へ回すべきスクリーン方向を算出
            // ※スクリーン座標系では下方向が+Yなので、localYの符号を反転
            dirX = localX;
            dirY = -localY;
        }

        // ゼロ割防止
        float len = sqrtf(dirX * dirX + dirY * dirY);
        if (len < 0.001f)
        {
            dirX = 1.0f;
            dirY = 0.0f;
        }
        else
        {
            dirX /= len;
            dirY /= len;
        }

        arrowAngle = atan2f(dirY, dirX);

        // 画面外インジケーターのエッジクランプ範囲
        const float minEdgeX = 42.0f;
        const float maxEdgeX = screenW - 42.0f;
        const float minEdgeY = 42.0f;
        const float maxEdgeY = screenH - 42.0f;

        // 画面中央から角度 arrowAngle 方向の半直線と、画面端境界矩形との交点を求める
        float cosA = cosf(arrowAngle);
        float sinA = sinf(arrowAngle);

        float tX = (cosA > 0.0001f) ? (maxEdgeX - screenCenterX) / cosA
                                     : ((cosA < -0.0001f) ? (minEdgeX - screenCenterX) / cosA : 1e9f);
        float tY = (sinA > 0.0001f) ? (maxEdgeY - screenCenterY) / sinA
                                     : ((sinA < -0.0001f) ? (minEdgeY - screenCenterY) / sinA : 1e9f);
        float t = (tX < tY) ? tX : tY;

        float clampedX = screenCenterX + cosA * t;
        float clampedY = screenCenterY + sinA * t;

        // 【HUD回避セーフエリア制御】
        // 1. 左上エリア（時計、恐怖度メーター、ミニマップ、バッテリーHUD: X < 255, Y < 365）との被り回避
        if (clampedX < 255.0f && clampedY < 365.0f)
        {
            if (clampedY < 60.0f)
            {
                // 時計・上部枠より右側に押し出す
                clampedX = 255.0f;
            }
            else
            {
                // ミニマップ・バッテリーHUDの下側に押し出す
                clampedY = 365.0f;
            }
        }

        // 2. 右上エリア（操作ガイドHUD表示時: X > 940, Y < 330）との被り回避
        if (showKeyGuide && clampedX > 940.0f && clampedY < 330.0f)
        {
            if (clampedY < 60.0f)
            {
                clampedX = 940.0f;
            }
            else
            {
                clampedY = 330.0f;
            }
        }

        markerX = static_cast<int>(clampedX);
        markerY = static_cast<int>(clampedY);
    }

    // パルス明滅アニメーション（ホラー調の控えめな低照度アンバーオレンジ）
    float pulse = 0.88f + 0.12f * sinf(static_cast<float>(frameCount) * 0.12f);
    int alpha = static_cast<int>(215.0f * pulse);

    // 距離文字列の作成（例: "35m"）
    int distFont = (fontMono != -1) ? fontMono : fontSmall;
    char distText[32];
    snprintf(distText, sizeof(distText), "%dm", distMeters);
    int textW = (distFont != -1) ? GetDrawStringWidthToHandle(distText, static_cast<int>(strlen(distText)), distFont) : 32;

    if (!isOffScreen)
    {
        // --- 画面内表示（目的地の頭上に浮かぶ下向きピンマーカー＋距離バッジ） ---
        // 目標の至近距離（3.0m以内）では配達ポストや操作レティクルを遮らないようフェードアウト
        if (distance < 3.0f)
        {
            float fadeFactor = (distance > 1.0f) ? ((distance - 1.0f) / 2.0f) : 0.0f;
            alpha = static_cast<int>(alpha * fadeFactor);
        }

        if (alpha > 5)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

            // 浮遊感のある微小なボブ運動（上下に1.5px）
            int bobY = static_cast<int>(sinf(static_cast<float>(frameCount) * 0.08f) * 2.0f);
            int my = markerY + bobY;

            // 外枠ダークシャドウ＋下向き三角マーカー（ピン先端）
            int triHalfW = 6;
            int triH = 7;
            DrawTriangle(
                markerX, my + triH + 2,
                markerX - triHalfW - 2, my - 1,
                markerX + triHalfW + 2, my - 1,
                GetColor(4, 6, 6), TRUE
            );
            DrawTriangle(
                markerX, my + triH,
                markerX - triHalfW, my,
                markerX + triHalfW, my,
                COL_TARGET, TRUE
            );

            // 距離バッジ（小型レトロパネル）
            int boxW = textW + 16;
            int boxH = 19;
            int boxX1 = markerX - boxW / 2;
            int boxY1 = my - boxH - 2;
            int boxX2 = boxX1 + boxW;
            int boxY2 = boxY1 + boxH;

            DrawRetroPanel(boxX1, boxY1, boxX2, boxY2, GetColor(125, 72, 42), COL_BG, 215);

            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // 距離テキスト（アンバーオレンジ色）
            DrawJitterString(boxX1 + 8, boxY1 + 2, distText, COL_TARGET, distFont);
        }
    }
    else
    {
        // --- 画面外表示（画面端の全方位360度矢印インジケーター＋距離表示） ---
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

        // 目的地方向を鋭く指し示す矢印の描画（黒シャドウ縁取り付き）
        float arrowLen = 11.0f;
        float arrowWidth = 7.0f;
        float cosA = cosf(arrowAngle);
        float sinA = sinf(arrowAngle);

        int tipX = markerX + static_cast<int>(cosA * arrowLen);
        int tipY = markerY + static_cast<int>(sinA * arrowLen);

        int baseLX = markerX + static_cast<int>(-cosA * (arrowLen * 0.35f) - sinA * arrowWidth);
        int baseLY = markerY + static_cast<int>(-sinA * (arrowLen * 0.35f) + cosA * arrowWidth);

        int baseRX = markerX + static_cast<int>(-cosA * (arrowLen * 0.35f) + sinA * arrowWidth);
        int baseRY = markerY + static_cast<int>(-sinA * (arrowLen * 0.35f) - cosA * arrowWidth);

        DrawTriangle(tipX + 1, tipY + 1, baseLX + 1, baseLY + 1, baseRX + 1, baseRY + 1, GetColor(4, 6, 6), TRUE);
        DrawTriangle(tipX, tipY, baseLX, baseLY, baseRX, baseRY, COL_TARGET, TRUE);

        // 距離ラベルを矢印の内側（画面中心寄り）に配置
        int boxW = textW + 16;
        int boxH = 19;

        // 矢印の向きと反対側にオフセット
        int labelOffsetX = static_cast<int>(-cosA * (arrowLen + boxW * 0.5f + 4.0f));
        int labelOffsetY = static_cast<int>(-sinA * (arrowLen + boxH * 0.5f + 4.0f));

        int boxX1 = markerX + labelOffsetX - boxW / 2;
        int boxY1 = markerY + labelOffsetY - boxH / 2;
        int boxX2 = boxX1 + boxW;
        int boxY2 = boxY1 + boxH;

        DrawRetroPanel(boxX1, boxY1, boxX2, boxY2, GetColor(125, 72, 42), COL_BG, 215);

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawJitterString(boxX1 + 8, boxY1 + 2, distText, COL_TARGET, distFont);
    }
}

