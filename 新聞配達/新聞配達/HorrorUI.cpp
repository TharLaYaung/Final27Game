#include "HorrorUI.h"
#include <cmath>
#include <cstdlib>

// カラーパレット定数定義（彩度を落とした低照度深夜・不気味なホラートーン）
const unsigned int HorrorUI::COL_BG         = GetColor(10, 14, 12);
const unsigned int HorrorUI::COL_PANEL_BG   = GetColor(14, 18, 16);
const unsigned int HorrorUI::COL_BORDER     = GetColor(55, 72, 64);
const unsigned int HorrorUI::COL_BORDER_DIM = GetColor(32, 42, 38);
const unsigned int HorrorUI::COL_TEXT       = GetColor(175, 188, 180);
const unsigned int HorrorUI::COL_TEXT_DIM   = GetColor(95, 108, 102);
const unsigned int HorrorUI::COL_WARNING    = GetColor(185, 45, 40);
const unsigned int HorrorUI::COL_TARGET     = GetColor(185, 90, 42);
const unsigned int HorrorUI::COL_PLAYER     = GetColor(200, 210, 202);
const unsigned int HorrorUI::COL_BIKE       = GetColor(90, 155, 165);

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
    currentFearParam = 0.0f;
    deathClosingProgress = 0.0f;
    deathBlinkFactor = 1.0f;
    doubleVisionAmount = 0.0f;
    desaturationAmount = 0.0f;
    focusBlurAmount = 0.0f;
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

    // ホラー演出に特化した擦れ・不気味な筆跡フォント "Chiller" を全UIフォントの基調として生成
    // 小型UI用（ミニマップ、バッテリー、所持数、恐怖度など全HUD表示用）
    fontSmall = CreateFontToHandle("Chiller", 20, 2, DX_FONTTYPE_ANTIALIASING);
    if (fontSmall == -1) fontSmall = CreateFontToHandle("MS Gothic", 14, 2, DX_FONTTYPE_NORMAL);

    // 中型UI用（プロンプト等）
    fontMedium = CreateFontToHandle("Chiller", 26, 3, DX_FONTTYPE_ANTIALIASING);
    if (fontMedium == -1) fontMedium = CreateFontToHandle("MS Gothic", 18, 2, DX_FONTTYPE_NORMAL);

    // 大型UI用
    fontLarge = CreateFontToHandle("Chiller", 34, 4, DX_FONTTYPE_ANTIALIASING);
    if (fontLarge == -1) fontLarge = CreateFontToHandle("MS Gothic", 26, 3, DX_FONTTYPE_NORMAL);

    // 時計フォント（不気味なChillerホラー時計）
    fontClock = CreateFontToHandle("Chiller", 26, 3, DX_FONTTYPE_ANTIALIASING);
    if (fontClock == -1) fontClock = CreateFontToHandle("Consolas", 20, 2, DX_FONTTYPE_NORMAL);

    // 大見出しホラー専用フォント（タイトル、GAME OVER）
    fontTitleHorror = CreateFontToHandle("Chiller", 64, 4, DX_FONTTYPE_ANTIALIASING);
    if (fontTitleHorror == -1) fontTitleHorror = CreateFontToHandle("MS Gothic", 48, 5, DX_FONTTYPE_ANTIALIASING_EDGE_8X8);

    // 操作方法 [KEY] 表記用（Chillerホラーフォント）
    fontKeyGuide = CreateFontToHandle("Chiller", 21, 3, DX_FONTTYPE_ANTIALIASING);
    if (fontKeyGuide == -1) fontKeyGuide = CreateFontToHandle("Consolas", 14, 2, DX_FONTTYPE_NORMAL);

    // 操作方法アクション説明用（Chillerホラーフォント）
    fontKeyAction = CreateFontToHandle("Chiller", 21, 3, DX_FONTTYPE_ANTIALIASING);
    if (fontKeyAction == -1) fontKeyAction = CreateFontToHandle("MS Gothic", 15, 1, DX_FONTTYPE_NORMAL);
}

// 終了処理（フォントハンドルおよびエフェクト画面の破棄）
void HorrorUI::Finalize()
{
    if (ghostScreenGraph != -1) { DeleteGraph(ghostScreenGraph); ghostScreenGraph = -1; }
    if (fontSmall != -1) { DeleteFontToHandle(fontSmall); fontSmall = -1; }
    if (fontMedium != -1) { DeleteFontToHandle(fontMedium); fontMedium = -1; }
    if (fontLarge != -1) { DeleteFontToHandle(fontLarge); fontLarge = -1; }
    if (fontClock != -1) { DeleteFontToHandle(fontClock); fontClock = -1; }
    if (fontTitleHorror != -1) { DeleteFontToHandle(fontTitleHorror); fontTitleHorror = -1; }
    if (fontKeyGuide != -1) { DeleteFontToHandle(fontKeyGuide); fontKeyGuide = -1; }
    if (fontKeyAction != -1) { DeleteFontToHandle(fontKeyAction); fontKeyAction = -1; }
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
    if (day > 7) day = 7;
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

    // 恐怖度に応じたビネット強度・幅の算出
    // CALM (0.0〜0.2): ほぼ無色・非常に薄い外周シェード
    // UNEASY (0.2〜0.4): 控えめな減衰
    // AFRAID (0.4〜0.6): 明確な外周の暗闇
    // PANIC (0.6〜0.8): 画面端を覆う強い闇
    // TERROR (0.8〜1.0): 画面中央付近まで迫る重苦しい暗闇
    float fear = currentFearParam;
    if (fear < 0.0f) fear = 0.0f;
    if (fear > 1.0f) fear = 1.0f;

    int steps = 4 + static_cast<int>(fear * 12.0f);
    int maxStrip = 10 + static_cast<int>(fear * 14.0f);
    float maxAlpha = 15.0f + fear * 195.0f;

    for (int i = 0; i < steps; i++)
    {
        int inset = i * maxStrip;
        float ratio = 1.0f - static_cast<float>(i) / static_cast<float>(steps);
        int alpha = static_cast<int>(maxAlpha * ratio * ratio);
        if (alpha <= 0) continue;
        if (alpha > 255) alpha = 255;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        // 上辺
        DrawBox(0, inset, 1280, inset + maxStrip, GetColor(0, 0, 0), TRUE);
        // 下辺
        DrawBox(0, 720 - inset - maxStrip, 1280, 720 - inset, GetColor(0, 0, 0), TRUE);
        // 左辺
        DrawBox(inset, 0, inset + maxStrip, 720, GetColor(0, 0, 0), TRUE);
        // 右辺
        DrawBox(1280 - inset - maxStrip, 0, 1280 - inset, 720, GetColor(0, 0, 0), TRUE);
    }

    // 死亡時・失神シネマティック視界狭窄（瞼の閉じ込みと最後のまばたき）
    if (deathClosingProgress > 0.001f)
    {
        float t = deathClosingProgress;
        if (t > 1.0f) t = 1.0f;

        // deathBlinkFactor によるまばたき（目が細まった後、一瞬僅かに見開き、そして永遠に閉じる）
        float blinkMod = deathBlinkFactor;
        if (blinkMod < 0.0f) blinkMod = 0.0f;
        if (blinkMod > 1.0f) blinkMod = 1.0f;

        // 瞼（上下黒バー）の閉じ込み高さ
        float closeRatio = t * (1.0f - blinkMod * 0.45f);
        if (closeRatio > 1.0f) closeRatio = 1.0f;
        int closeH = static_cast<int>(360.0f * closeRatio);

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
        if (closeH > 0)
        {
            // 上瞼
            DrawBox(0, 0, 1280, closeH, GetColor(0, 0, 0), TRUE);
            // 下瞼
            DrawBox(0, 720 - closeH, 1280, 720, GetColor(0, 0, 0), TRUE);
        }

        // 左右からの意識混濁ブラックアウト
        int closeW = static_cast<int>(640.0f * closeRatio * 0.75f);
        if (closeW > 0)
        {
            DrawBox(0, closeH, closeW, 720 - closeH, GetColor(0, 0, 0), TRUE);
            DrawBox(1280 - closeW, closeH, 1280, 720 - closeH, GetColor(0, 0, 0), TRUE);
        }

        // 視野全体へのフェード膜
        int darkPulse = static_cast<int>(255.0f * (1.0f - (1.0f - t) * blinkMod));
        if (darkPulse > 0)
        {
            if (darkPulse > 255) darkPulse = 255;
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, darkPulse);
            DrawBox(0, 0, 1280, 720, GetColor(0, 0, 0), TRUE);
        }
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 恐怖視覚エフェクト（二重像・焦点ボケ・冷色脱色）の描画
void HorrorUI::DrawPostHorrorEffects()
{
    if (!horrorEffectsEnabled) return;

    // 冷色脱色（恐怖・死に連動した青暗い彩度低下）
    if (desaturationAmount > 0.01f)
    {
        int desatAlpha = static_cast<int>(desaturationAmount * 65.0f);
        if (desatAlpha > 200) desatAlpha = 200;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, desatAlpha);
        DrawBox(0, 0, 1280, 720, GetColor(8, 16, 26), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 二重像（幽体ゴースト視覚）
    if (doubleVisionAmount > 0.02f && ghostScreenGraph != -1)
    {
        GetDrawScreenGraph(0, 0, 1280, 720, ghostScreenGraph);
        int ghostAlpha = static_cast<int>(doubleVisionAmount * 55.0f);
        if (ghostAlpha > 120) ghostAlpha = 120;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, ghostAlpha);

        int offX = static_cast<int>(std::sin(frameCount * 0.12f) * 7.0f);
        int offY = static_cast<int>(std::cos(frameCount * 0.09f) * 4.0f);
        DrawGraph(offX, offY, ghostScreenGraph, FALSE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 焦点ボケ・視覚不安定（フォーカスが偶発的に揺らぐ）
    if (focusBlurAmount > 0.02f && ghostScreenGraph != -1)
    {
        GetDrawScreenGraph(0, 0, 1280, 720, ghostScreenGraph);
        int blurAlpha = static_cast<int>(focusBlurAmount * 45.0f);
        if (blurAlpha > 100) blurAlpha = 100;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, blurAlpha);

        DrawGraph(-3, 0, ghostScreenGraph, FALSE);
        DrawGraph( 3, 0, ghostScreenGraph, FALSE);
        DrawGraph( 0, -2, ghostScreenGraph, FALSE);
        DrawGraph( 0,  2, ghostScreenGraph, FALSE);
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
        // 通常時：極小の控えめなオフホワイトのドット
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
        DrawCircle(cx, cy, 2, COL_TEXT_DIM, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    else
    {
        // インタラクト可能時：派手な黄色にはせず、わずかに明るいドット＋極細の控えめなブラケット
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawCircle(cx, cy, 2, COL_PLAYER, TRUE);

        // 微小な十字ブラケット（長さ3px、隙間4px）
        DrawLine(cx - 7, cy, cx - 4, cy, COL_TEXT);
        DrawLine(cx + 4, cy, cx + 7, cy, COL_TEXT);
        DrawLine(cx, cy - 7, cx, cy - 4, COL_TEXT);
        DrawLine(cx, cy + 4, cx, cy + 7, COL_TEXT);

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

// アナログ端末風の枠付きパネル描画（ホラー・腐食レトロCRTスタイル）
void HorrorUI::DrawRetroPanel(int x1, int y1, int x2, int y2, unsigned int borderColor, unsigned int bgColor, int alpha)
{
    // 背景塗り（不吉な薄暗い半透明）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawBox(x1, y1, x2, y2, bgColor, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 外枠ライン
    DrawBox(x1, y1, x2, y2, borderColor, FALSE);

    // 二重枠（薄暗い内側フレーム線による古びたCRT・計器パネルの立体感）
    if (x2 - x1 > 8 && y2 - y1 > 8)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
        DrawBox(x1 + 2, y1 + 2, x2 - 2, y2 - 2, COL_BORDER_DIM, FALSE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 四隅のコーナーレトロブラケット補強（アナログ機器らしさの演出）
    int cornerLen = 6;
    DrawLine(x1, y1, x1 + cornerLen, y1, borderColor);
    DrawLine(x1, y1, x1, y1 + cornerLen, borderColor);
    DrawLine(x2 - cornerLen, y1, x2, y1, borderColor);
    DrawLine(x2, y1, x2, y1 + cornerLen, borderColor);
    DrawLine(x1, y2 - cornerLen, x1, y2, borderColor);
    DrawLine(x1, y2, x1 + cornerLen, y2, borderColor);
    DrawLine(x2 - cornerLen, y2, x2, y2, borderColor);
    DrawLine(x2, y2 - cornerLen, x2, y2, borderColor);

    // 微小なノッチ（目盛りスリット）を上下に刻む
    if (x2 - x1 > 40)
    {
        int midX = (x1 + x2) / 2;
        DrawLine(midX - 3, y1, midX + 3, y1, borderColor);
        DrawLine(midX - 3, y2, midX + 3, y2, borderColor);
    }
}

// ノイズ・ジッター・幽霊的シャドウ付き文字列描画
void HorrorUI::DrawJitterString(int x, int y, const char* text, unsigned int color, int fontHandle)
{
    int drawX = x;
    int drawY = y;

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

        // 高恐怖度または干渉時の色収差ゴーストシャドウ（微小な赤シャドウ）
        if (horrorIntensity >= 2 || glitchTimer > 0 || interferenceAmount > 0.5f)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
            if (fontHandle != -1)
            {
                DrawStringToHandle(drawX + 1, drawY, text, COL_WARNING, fontHandle);
            }
            else
            {
                DrawString(drawX + 1, drawY, text, COL_WARNING);
            }
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

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

    const char* promptText = "";
    switch (currentPrompt)
    {
    case PromptType::RideBicycle:
        promptText = "[ E ] RIDE";
        break;
    case PromptType::DismountBicycle:
        promptText = "[ E ] DISMOUNT";
        break;
    case PromptType::TakeNewspaper:
        promptText = "[ LMB ] TAKE";
        break;
    case PromptType::DeliverNewspaper:
        promptText = "[ LMB ] DELIVER";
        break;
    case PromptType::TakeBattery:
        promptText = "[ LMB ] TAKE BATTERY";
        break;
    case PromptType::ReplaceBattery:
        promptText = "[ R ] REPLACE BATTERY";
        break;
    default:
        return;
    }

    if (currentPrompt == PromptType::BatteryFull)
    {
        const char* line1 = "BATTERY FULL";
        const char* line2 = "SPARE 2/2";
        int w1 = (fontMedium != -1) ? GetDrawStringWidthToHandle(line1, (int)strlen(line1), fontMedium) : 130;
        int w2 = (fontSmall != -1) ? GetDrawStringWidthToHandle(line2, (int)strlen(line2), fontSmall) : 80;
        int maxW = (w1 > w2) ? w1 : w2;
        int posX = 640 - maxW / 2;
        int posY = 458;

        int alpha = (int)(promptAlpha * 220.0f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

        DrawBox(posX - 18, posY - 6, posX + maxW + 18, posY + 48, COL_BG, TRUE);
        DrawBox(posX - 18, posY - 6, posX + maxW + 18, posY + 48, COL_WARNING, FALSE);

        if (fontMedium != -1)
        {
            DrawStringToHandle(640 - w1 / 2, posY, line1, COL_WARNING, fontMedium);
        }
        else
        {
            DrawString(640 - w1 / 2, posY, line1, COL_WARNING);
        }

        if (fontSmall != -1)
        {
            DrawStringToHandle(640 - w2 / 2, posY + 26, line2, COL_TEXT, fontSmall);
        }
        else
        {
            DrawString(640 - w2 / 2, posY + 26, line2, COL_TEXT);
        }

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        return;
    }

    int textLen = (int)strlen(promptText);
    int actualWidth = (fontMedium != -1) ? GetDrawStringWidthToHandle(promptText, textLen, fontMedium) : textLen * 11;
    int posX = 640 - actualWidth / 2;
    int posY = 470;

    int alpha = (int)(promptAlpha * 220.0f);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

    // 控えめな半透明バックプレート（文字幅に応じた動的パディング）
    DrawBox(posX - 16, posY - 4, posX + actualWidth + 16, posY + 24, COL_BG, TRUE);
    DrawBox(posX - 16, posY - 4, posX + actualWidth + 16, posY + 24, COL_BORDER_DIM, FALSE);

    // プロンプト文字列の描画
    if (fontMedium != -1)
    {
        DrawStringToHandle(posX, posY, promptText, COL_TEXT, fontMedium);
    }
    else
    {
        DrawString(posX, posY, promptText, COL_TEXT);
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// ゲーム内時計・日数HUDの描画（画面左上: 例 DAY 1/7, 01:00 AM）
void HorrorUI::DrawClock()
{
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d AM", clockHour, clockMinute);

    char dayStr[32];
    snprintf(dayStr, sizeof(dayStr), "DAY %d/7", currentDay);

    int panelX1 = 26;
    int panelY1 = 12;
    int panelX2 = 236;
    int panelY2 = 38;

    // レトロ端末パネル背景（ミニマップと横幅210pxを完全統一）
    DrawRetroPanel(panelX1, panelY1, panelX2, panelY2, COL_BORDER, COL_PANEL_BG, 185);

    // 左側：ステージ日数表記（オレンジ系）
    DrawJitterString(panelX1 + 10, panelY1 + 5, dayStr, COL_TARGET, fontSmall);

    // 右側：ゲーム内時刻表記（オフホワイト・右揃え配置で重なりを完全排除）
    int timeW = (fontSmall != -1) ? GetDrawStringWidthToHandle(timeStr, (int)strlen(timeStr), fontSmall) : 65;
    DrawJitterString(panelX2 - timeW - 10, panelY1 + 5, timeStr, COL_TEXT, fontSmall);
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

    int alpha = static_cast<int>(alphaFactor * 225.0f);
    if (alpha <= 0) return;

    int cx = 640;
    int bw = 640;
    int bh = 64;
    int by1 = 55;
    int by2 = by1 + bh;

    DrawRetroPanel(cx - bw / 2, by1, cx + bw / 2, by2, COL_BORDER, COL_PANEL_BG, alpha);

    // 見出し
    char titleBuf[64];
    snprintf(titleBuf, sizeof(titleBuf), "--- %s ---", dayName ? dayName : "MIDNIGHT SHIFT");
    int tw = (fontLarge != -1) ? GetDrawStringWidthToHandle(titleBuf, (int)strlen(titleBuf), fontLarge) : 200;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawJitterString(cx - tw / 2, by1 + 6, titleBuf, COL_TARGET, fontLarge);

    char subBuf[128];
    snprintf(subBuf, sizeof(subBuf), "QUOTA: %02d HOUSES  |  %s", quota, subtitle ? subtitle : "");
    int sw = (fontSmall != -1) ? GetDrawStringWidthToHandle(subBuf, (int)strlen(subBuf), fontSmall) : 200;
    DrawJitterString(cx - sw / 2, by1 + 34, subBuf, COL_TEXT, fontSmall);

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 新聞所持数・ステータスの描画（画面左下: ミニマップと横幅210pxを完全統一）
void HorrorUI::DrawPaperCargo(int paperCount, bool isHolding)
{
    char paperStr[32];
    snprintf(paperStr, sizeof(paperStr), "PAPERS  %02d", paperCount);

    int panelX1 = 26;
    int panelY1 = 668;
    int panelX2 = 236;
    int panelY2 = 698;

    // レトロ端末パネル枠と背景の描画（ミニマップと横幅210pxを完全統一）
    DrawRetroPanel(panelX1, panelY1, panelX2, panelY2, COL_BORDER, COL_PANEL_BG, 185);

    // 左側：所持部数
    DrawJitterString(panelX1 + 10, panelY1 + 5, paperStr, COL_TEXT, fontSmall);

    // 右側：手持ち状態の表示（右揃え配置で重なりを完全排除）
    if (isHolding)
    {
        const char* holdStr = "[HOLDING]";
        int holdW = (fontSmall != -1) ? GetDrawStringWidthToHandle(holdStr, (int)strlen(holdStr), fontSmall) : 60;
        bool blink = ((frameCount / 25) % 2) == 0;
        unsigned int col = blink ? COL_TARGET : COL_TEXT_DIM;
        DrawJitterString(panelX2 - holdW - 10, panelY1 + 5, holdStr, col, fontSmall);
    }
}

// 操作キー案内HUD描画（画面右上パネル）
void HorrorUI::DrawKeyGuide(bool isRiding, bool isHoldingPaper, bool canTakeOrDeliver, bool isHeadlightOn, bool isMapDebugOn, bool canReplaceBattery)
{
    int kx1 = 980;
    int ky1 = 20;
    int kx2 = 1255;
    int ky2 = 298;

    // レトロ端末パネル枠と背景の描画
    DrawRetroPanel(kx1, ky1, kx2, ky2, COL_BORDER, COL_PANEL_BG, 195);

    // 見出しタイトル（不気味なホラー筆跡 Chiller）
    DrawJitterString(kx1 + 10, ky1 + 4, "CONTROLS GUIDE", GetColor(200, 160, 150), fontKeyAction);

    // 区切り線
    DrawLine(kx1 + 4, ky1 + 25, kx2 - 4, ky1 + 25, COL_BORDER_DIM);

    // 各操作キーと説明
    struct KeyItem {
        const char* key;
        const char* action;
        unsigned int keyCol;
        unsigned int actCol;
    };

    unsigned int defaultKeyCol = GetColor(190, 205, 198);
    unsigned int defaultActCol = COL_TEXT_DIM;
    unsigned int activeKeyCol = GetColor(245, 235, 180);
    unsigned int activeActCol = COL_TEXT;
    unsigned int bikeCol = COL_BIKE;

    // Eキー（乗車／降車）
    const char* eAction = isRiding ? "Dismount" : "Ride Bike";
    unsigned int eKeyCol = isRiding ? bikeCol : defaultKeyCol;
    unsigned int eActCol = isRiding ? bikeCol : defaultActCol;

    // 左クリック（受取／配達）
    const char* lmbAction = isHoldingPaper ? "Deliver" : "Take Paper";
    unsigned int lmbKeyCol = canTakeOrDeliver ? activeKeyCol : defaultKeyCol;
    unsigned int lmbActCol = canTakeOrDeliver ? activeActCol : defaultActCol;

    // Fキー（ライト点消灯）
    const char* fAction = isHeadlightOn ? "Light (ON)" : "Headlight";
    unsigned int fKeyCol = isHeadlightOn ? GetColor(240, 220, 140) : defaultKeyCol;
    unsigned int fActCol = isHeadlightOn ? GetColor(220, 205, 150) : defaultActCol;

    // F6キー（マップ表示）
    const char* f6Action = isMapDebugOn ? "Map (ON)" : "Map Info";
    unsigned int f6KeyCol = isMapDebugOn ? GetColor(150, 215, 185) : defaultKeyCol;
    unsigned int f6ActCol = isMapDebugOn ? GetColor(135, 185, 165) : defaultActCol;

    const char* rAction = canReplaceBattery ? "Replace Bat" : "Replace Bat";
    unsigned int rKeyCol = canReplaceBattery ? activeKeyCol : defaultKeyCol;
    unsigned int rActCol = canReplaceBattery ? activeActCol : defaultActCol;

    KeyItem items[] = {
        { "[ WASD ]", isRiding ? "Pedal / Steer" : "Move", defaultKeyCol, defaultActCol },
        { "[ MOUSE ]", "Look", defaultKeyCol, defaultActCol },
        { "[ E ]", eAction, eKeyCol, eActCol },
        { "[ LMB ]", lmbAction, lmbKeyCol, lmbActCol },
        { "[ R ]", rAction, rKeyCol, rActCol },
        { "[ F ]", fAction, fKeyCol, fActCol },
        { "[ ESC ]", "Pause Menu", defaultKeyCol, defaultActCol },
        { "[ F6 ]", f6Action, f6KeyCol, f6ActCol },
        { "[ [ / ] ]", "Day - / +", GetColor(140, 200, 220), defaultActCol }
    };

    int startY = ky1 + 31;
    int lineSpacing = 28;
    int nItems = sizeof(items) / sizeof(items[0]);

    for (int i = 0; i < nItems; ++i)
    {
        int curY = startY + i * lineSpacing;
        int curX = kx1 + 10;

        // キー表記（不気味なホラー筆跡フォント）
        DrawJitterString(curX, curY, items[i].key, items[i].keyCol, fontKeyGuide);

        // アクション表記（十分な水平オフセットで重なりを完全排除）
        DrawJitterString(kx1 + 108, curY, items[i].action, items[i].actCol, fontKeyAction);
    }
}

// 自転車ヘッドライト・バッテリーHUD描画（画面左側、ミニマップ直下に配置）
void HorrorUI::DrawBicycleBattery(float batteryPercent, bool isLightOn, bool isRiding, bool isFastDrain, int spareBatteries, int maxSpareBatteries)
{
    (void)isRiding;

    // ミニマップ枠 (26, 74)〜(236, 269) の直下、Y=276〜324 に配置
    int panelX1 = 26;
    int panelY1 = 276;
    int panelX2 = 236;
    int panelY2 = 348;

    // レトロ端末パネル外枠と背景の描画
    DrawRetroPanel(panelX1, panelY1, panelX2, panelY2, COL_BORDER, COL_PANEL_BG, 185);

    // 上段：ラベル・点灯状態・残量パーセント表示
    int textY = panelY1 + 5;

    // 「LIGHT」固定ラベル
    DrawJitterString(panelX1 + 10, textY, "LIGHT", COL_TEXT_DIM, fontSmall);

    // 点灯状態インジケータ
    if (batteryPercent <= 0.0f)
    {
        // バッテリー完全枯渇時は点滅表示（DEAD）
        bool blink = ((frameCount / 20) % 2) == 0;
        unsigned int deadCol = blink ? COL_WARNING : GetColor(90, 30, 25);
        DrawJitterString(panelX1 + 56, textY, "DEAD", deadCol, fontSmall);
    }
    else if (isLightOn)
    {
        // 点灯中
        DrawJitterString(panelX1 + 56, textY, "ON", GetColor(210, 220, 210), fontSmall);
    }
    else
    {
        // 消灯中
        DrawJitterString(panelX1 + 56, textY, "OFF", COL_TEXT_DIM, fontSmall);
    }

    // デバッグ高速消費インジケータ（F8有効時）
    if (isFastDrain)
    {
        bool blink = ((frameCount / 15) % 2) == 0;
        unsigned int debugCol = blink ? GetColor(230, 140, 50) : GetColor(140, 80, 25);
        DrawJitterString(panelX1 + 104, textY, "[10x]", debugCol, fontSmall);
    }

    // バッテリー残量数値表示（例: 84% または 0%）
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
        pctColor = blink ? COL_WARNING : GetColor(190, 100, 70);
    }
    else if (batteryPercent <= 50.0f)
    {
        pctColor = GetColor(195, 160, 65);
    }

    DrawJitterString(panelX2 - 44, textY, pctStr, pctColor, fontSmall);

    // 下段：アナログ10セグメント・バッテリー残量ゲージ
    int barX1 = panelX1 + 10;
    int barX2 = panelX2 - 10;
    int barY1 = panelY1 + 25;
    int barY2 = panelY1 + 38;

    // ゲージ全体の黒背景と薄い枠線
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
    DrawBox(barX1, barY1, barX2, barY2, COL_BG, TRUE);
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
            // 残量に応じたセル点灯色
            unsigned int cellColor = COL_TEXT;
            if (batteryPercent <= 20.0f)
            {
                // 残量20%以下：赤色警告（5%以下で点滅）
                bool blink = (batteryPercent <= 5.0f) && (((frameCount / 10) % 2) == 0);
                cellColor = blink ? GetColor(80, 20, 15) : COL_WARNING;
            }
            else if (batteryPercent <= 50.0f)
            {
                // 残量21〜50%：くすんだ暖黄色（アンバー）
                cellColor = GetColor(190, 155, 60);
            }
            else
            {
                // 残量51〜100%：褪せたオフホワイト
                cellColor = GetColor(180, 190, 180);
            }

            DrawBox(cx1, cy1, cx2, cy2, cellColor, TRUE);
        }
        else
        {
            // 非点灯セル：極めて暗い背景スロット枠
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 90);
            DrawBox(cx1, cy1, cx2, cy2, GetColor(30, 38, 34), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }


    // 予備バッテリー区切り線
    DrawLine(panelX1 + 6, panelY1 + 41, panelX2 - 6, panelY1 + 41, COL_BORDER_DIM);

    // 予備バッテリー所持数表示（SPARE 0/2, SPARE 1/2, SPARE 2/2）
    char spareStr[32];
    snprintf(spareStr, sizeof(spareStr), "SPARE %d/%d", spareBatteries, maxSpareBatteries);

    unsigned int spareCol = COL_TEXT_DIM;
    if (spareBatteries >= maxSpareBatteries)
    {
        spareCol = GetColor(140, 225, 175);
    }
    else if (spareBatteries > 0)
    {
        spareCol = COL_TEXT;
    }

    DrawJitterString(panelX1 + 10, panelY1 + 46, spareStr, spareCol, fontSmall);

    // 予備バッテリー所持スロットランプ（2個）
    for (int s = 0; s < maxSpareBatteries; ++s)
    {
        int sx1 = panelX2 - 34 + s * 13;
        int sy1 = panelY1 + 48;
        int sx2 = sx1 + 9;
        int sy2 = sy1 + 9;

        if (s < spareBatteries)
        {
            DrawBox(sx1, sy1, sx2, sy2, GetColor(110, 215, 150), TRUE);
            DrawBox(sx1, sy1, sx2, sy2, GetColor(160, 245, 190), FALSE);
        }
        else
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
            DrawBox(sx1, sy1, sx2, sy2, COL_BG, TRUE);
            DrawBox(sx1, sy1, sx2, sy2, COL_BORDER_DIM, FALSE);
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

    // レトロ端末パネル背景（ミニマップと横幅210pxを完全統一）
    DrawRetroPanel(panelX1, panelY1, panelX2, panelY2, COL_BORDER_DIM, COL_BG, 185);

    // 恐怖段階に応じた文字色・警告色の決定
    unsigned int fearColor = COL_TEXT_DIM;
    int jitterX = 0;
    int jitterY = 0;

    if (displayedFearPercent >= 80.0f)
    {
        // 狂乱・極限 (80%〜): 暗い赤色、位置の不安定なジッター
        fearColor = GetColor(215, 45, 45);
        if ((frameCount % 4) == 0)
        {
            jitterX = (rand() % 3) - 1;
            jitterY = (rand() % 3) - 1;
        }
    }
    else if (displayedFearPercent >= 60.0f)
    {
        // 恐慌 (60〜79%): 警告オレンジ赤
        fearColor = GetColor(210, 85, 60);
        if ((frameCount % 8) == 0)
        {
            jitterX = (rand() % 2) - 1;
        }
    }
    else if (displayedFearPercent >= 40.0f)
    {
        // 恐怖 (40〜59%): くすんだアンバー
        fearColor = GetColor(200, 145, 65);
    }
    else if (displayedFearPercent >= 20.0f)
    {
        // 警戒 (20〜39%): くすんだオリーブベージュ
        fearColor = GetColor(165, 175, 135);
    }
    else
    {
        // 平穏 (0〜19%): 控えめな減衰グレー
        fearColor = COL_TEXT_DIM;
    }

    // 上段左側：恐怖度数値ラベル（Chillerホラーフォント）
    char fearStr[32];
    snprintf(fearStr, sizeof(fearStr), "FEAR %02d%%", static_cast<int>(displayedFearPercent));
    DrawJitterString(panelX1 + 10 + jitterX, panelY1 + 3 + jitterY, fearStr, fearColor, fontSmall);

    // 上段右側：安全光判定インジケータ（SAFE: 穏やかな緑 / DARK: 危険な赤橙点滅、右揃え配置で重なりを排除）
    if (inSafeLight)
    {
        const char* safeStr = "SAFE";
        int strW = (fontSmall != -1) ? GetDrawStringWidthToHandle(safeStr, (int)strlen(safeStr), fontSmall) : 36;
        DrawJitterString(panelX2 - strW - 10, panelY1 + 3, safeStr, GetColor(110, 185, 140), fontSmall);
    }
    else
    {
        const char* darkStr = "DARK";
        int strW = (fontSmall != -1) ? GetDrawStringWidthToHandle(darkStr, (int)strlen(darkStr), fontSmall) : 36;
        bool blink = ((frameCount / 20) % 2) == 0;
        unsigned int darkCol = blink ? GetColor(220, 70, 60) : GetColor(160, 45, 40);
        DrawJitterString(panelX2 - strW - 10, panelY1 + 3, darkStr, darkCol, fontSmall);
    }

    // 下段：ゲージスロット背景枠および恐怖度進行バー
    int barX1 = panelX1 + 10;
    int barX2 = panelX2 - 10;
    int barY1 = panelY1 + 19;
    int barY2 = panelY1 + 24;
    int barW = barX2 - barX1;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
    DrawBox(barX1, barY1, barX2, barY2, GetColor(20, 26, 22), TRUE);
    DrawBox(barX1, barY1, barX2, barY2, COL_BORDER_DIM, FALSE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ゲージ塗りつぶし
    int fillW = static_cast<int>((displayedFearPercent / 100.0f) * static_cast<float>(barW - 2));
    if (fillW > 0)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
        DrawBox(barX1 + 1, barY1 + 1, barX1 + 1 + fillW, barY2 - 1, fearColor, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

// 最大恐怖演出時の画面歪曲・ブラックアウト描画
void HorrorUI::DrawScareDistortion(float intensity)
{
    if (intensity <= 0.0f) return;

    // 画面外周の激しい減光
    int alpha = static_cast<int>(intensity * 240.0f);
    if (alpha > 255) alpha = 255;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha / 2);
    DrawBox(0, 0, 1280, 720, GetColor(0, 0, 0), TRUE);

    // 周辺ビネットの二重掛け
    int steps = 10;
    for (int i = 0; i < steps; ++i)
    {
        int inset = i * 20;
        int stepAlpha = static_cast<int>(intensity * 40.0f * (1.0f - static_cast<float>(i) / steps));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, stepAlpha);
        DrawBox(0, inset, 1280, inset + 20, GetColor(0, 0, 0), TRUE);
        DrawBox(0, 720 - inset - 20, 1280, 720 - inset, GetColor(0, 0, 0), TRUE);
        DrawBox(inset, 0, inset + 20, 720, GetColor(0, 0, 0), TRUE);
        DrawBox(1280 - inset - 20, 0, 1280 - inset, 720, GetColor(0, 0, 0), TRUE);
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// ゲームオーバー画面描画
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

    DrawRetroPanel(cx - 240, cy - 100, cx + 240, cy + 100, COL_BORDER_DIM, COL_BG, 230);

    // ゲームオーバータイトル（赤系・動的センタリング）
    const char* overStr = "GAME OVER";
    if (fontLarge != -1)
    {
        int ow = GetDrawStringWidthToHandle(overStr, (int)strlen(overStr), fontLarge);
        DrawStringToHandle(cx - ow / 2, cy - 65, overStr, GetColor(200, 50, 45), fontLarge);
    }
    else
    {
        int ow = GetDrawStringWidth(overStr, (int)strlen(overStr));
        DrawString(cx - ow / 2, cy - 65, overStr, GetColor(200, 50, 45));
    }

    // ホラー演出テキスト（動的センタリング）
    const char* darkMsg = "CONSUMED BY THE DARKNESS...";
    int dw = GetDrawStringWidth(darkMsg, (int)strlen(darkMsg));
    DrawString(cx - dw / 2, cy - 10, darkMsg, COL_TEXT_DIM);

    // 操作案内（リトライ・終了・動的センタリング）
    bool blink = ((frameCount / 30) % 2) == 0;
    unsigned int promptCol = blink ? COL_TEXT : COL_TEXT_DIM;
    const char* retryStr = "[ R ] RETRY DELIVERY";
    int rw = GetDrawStringWidth(retryStr, (int)strlen(retryStr));
    DrawString(cx - rw / 2, cy + 40, retryStr, promptCol);

    const char* quitStr = "[ ESC ] QUIT";
    int qw = GetDrawStringWidth(quitStr, (int)strlen(quitStr));
    DrawString(cx - qw / 2, cy + 65, quitStr, COL_TEXT_DIM);
}
