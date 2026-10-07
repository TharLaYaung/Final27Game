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

    clockHour = 3;
    clockMinute = 17;
    clockSecondTimer = 0;

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
}

// デストラクタ
HorrorUI::~HorrorUI()
{
    Finalize();
}

// 初期化（フォント生成）
void HorrorUI::Initialize()
{
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

// 終了処理（フォントハンドルの破棄）
void HorrorUI::Finalize()
{
    if (fontSmall != -1) { DeleteFontToHandle(fontSmall); fontSmall = -1; }
    if (fontMedium != -1) { DeleteFontToHandle(fontMedium); fontMedium = -1; }
    if (fontLarge != -1) { DeleteFontToHandle(fontLarge); fontLarge = -1; }
    if (fontClock != -1) { DeleteFontToHandle(fontClock); fontClock = -1; }
    if (fontTitleHorror != -1) { DeleteFontToHandle(fontTitleHorror); fontTitleHorror = -1; }
    if (fontKeyGuide != -1) { DeleteFontToHandle(fontKeyGuide); fontKeyGuide = -1; }
    if (fontKeyAction != -1) { DeleteFontToHandle(fontKeyAction); fontKeyAction = -1; }
}

// 毎フレーム更新
void HorrorUI::Update()
{
    frameCount++;

    // ゲーム内時計の進行（実時間約60秒でゲーム内1分進行）
    clockSecondTimer++;
    if (clockSecondTimer >= 60 * 60)
    {
        clockSecondTimer = 0;
        clockMinute++;
        if (clockMinute >= 60)
        {
            clockMinute = 0;
            clockHour = (clockHour + 1) % 24;
        }
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
}

// プロンプト要求
void HorrorUI::SetPrompt(PromptType type)
{
    requestedPrompt = type;
}

// ゲーム内時計の設定
void HorrorUI::SetGameTime(int hour, int minute)
{
    clockHour = hour;
    clockMinute = minute;
    clockSecondTimer = 0;
}

void HorrorUI::GetGameTime(int& hour, int& minute) const
{
    hour = clockHour;
    minute = clockMinute;
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

// 画面周辺のダーク減光（ビネット）描画
void HorrorUI::DrawScreenVignette()
{
    if (!horrorEffectsEnabled) return;

    // 画面四隅および外周を自然に暗くする控えめなビネット効果
    // 描画負荷を最小限に抑えるため、段階的アルファ矩形ストリップを描画
    int steps = 6;
    for (int i = 0; i < steps; i++)
    {
        int inset = i * 14;
        int alpha = (int)(22.0f * (1.0f - (float)i / steps));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

        // 上辺
        DrawBox(0, inset, 1280, inset + 14, GetColor(0, 0, 0), TRUE);
        // 下辺
        DrawBox(0, 720 - inset - 14, 1280, 720 - inset, GetColor(0, 0, 0), TRUE);
        // 左辺
        DrawBox(inset, 0, inset + 14, 720, GetColor(0, 0, 0), TRUE);
        // 右辺
        DrawBox(1280 - inset - 14, 0, 1280 - inset, 720, GetColor(0, 0, 0), TRUE);
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
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
    default:
        return;
    }

    int textLen = (int)strlen(promptText);
    int approxWidth = textLen * 11;
    int posX = 640 - approxWidth / 2;
    int posY = 470;

    int alpha = (int)(promptAlpha * 220.0f);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

    // 控えめな半透明バックプレート
    DrawBox(posX - 12, posY - 4, posX + approxWidth + 12, posY + 22, COL_BG, TRUE);
    DrawBox(posX - 12, posY - 4, posX + approxWidth + 12, posY + 22, COL_BORDER_DIM, FALSE);

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

// ゲーム内時計の描画（画面左上: 例 03:17 AM）
void HorrorUI::DrawClock()
{
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d AM", clockHour, clockMinute);

    int posX = 26;
    int posY = 18;

    // ごく控えめな背景
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
    DrawBox(posX - 6, posY - 4, posX + 104, posY + 28, COL_BG, TRUE);
    DrawBox(posX - 6, posY - 4, posX + 104, posY + 28, COL_BORDER_DIM, FALSE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // デジタル・ホラー調の文字
    DrawJitterString(posX + 4, posY + 1, timeStr, COL_TEXT, fontClock);
}

// 新聞所持数・ステータスの描画（画面下部: 例 PAPERS 08）
void HorrorUI::DrawPaperCargo(int paperCount, bool isHolding)
{
    char paperStr[32];
    snprintf(paperStr, sizeof(paperStr), "PAPERS  %02d", paperCount);

    int posX = 26;
    int posY = 668;

    // 背景枠
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
    DrawBox(posX - 6, posY - 4, posX + 120 + (isHolding ? 88 : 0), posY + 26, COL_BG, TRUE);
    DrawBox(posX - 6, posY - 4, posX + 120 + (isHolding ? 88 : 0), posY + 26, COL_BORDER_DIM, FALSE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 所持部数
    DrawJitterString(posX + 4, posY + 1, paperStr, COL_TEXT, fontSmall);

    // 手持ち状態の表示
    if (isHolding)
    {
        // 控えめな点滅
        bool blink = ((frameCount / 25) % 2) == 0;
        unsigned int col = blink ? COL_TARGET : COL_TEXT_DIM;
        DrawJitterString(posX + 115, posY + 1, "[HOLDING]", col, fontSmall);
    }
}

// 操作キー案内HUD描画（画面右上パネル）
void HorrorUI::DrawKeyGuide(bool isRiding, bool isHoldingPaper, bool canTakeOrDeliver, bool isHeadlightOn, bool isMapDebugOn)
{
    int kx1 = 980;
    int ky1 = 20;
    int kx2 = 1255;
    int ky2 = 232;

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

    KeyItem items[] = {
        { "[ WASD ]", isRiding ? "Pedal / Steer" : "Move", defaultKeyCol, defaultActCol },
        { "[ MOUSE ]", "Look", defaultKeyCol, defaultActCol },
        { "[ E ]", eAction, eKeyCol, eActCol },
        { "[ LMB ]", lmbAction, lmbKeyCol, lmbActCol },
        { "[ F ]", fAction, fKeyCol, fActCol },
        { "[ ESC ]", "Pause Menu", defaultKeyCol, defaultActCol },
        { "[ F6 ]", f6Action, f6KeyCol, f6ActCol }
    };

    int startY = ky1 + 31;
    int lineSpacing = 27;
    int nItems = sizeof(items) / sizeof(items[0]);

    for (int i = 0; i < nItems; ++i)
    {
        int curY = startY + i * lineSpacing;
        int curX = kx1 + 10;

        // キー表記（不気味なホラー筆跡フォント）
        DrawJitterString(curX, curY, items[i].key, items[i].keyCol, fontKeyGuide);

        // アクション表記（不気味なホラー筆跡フォント）
        DrawJitterString(kx1 + 96, curY, items[i].action, items[i].actCol, fontKeyAction);
    }
}

// 自転車ヘッドライト・バッテリーHUD描画（画面左側、ミニマップ直下に配置）
void HorrorUI::DrawBicycleBattery(float batteryPercent, bool isLightOn, bool isRiding, bool isFastDrain)
{
    (void)isRiding;

    // ミニマップ枠 (26, 52)〜(236, 247) の直下、Y=254〜302 に配置
    int panelX1 = 26;
    int panelY1 = 254;
    int panelX2 = 236;
    int panelY2 = 302;

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
}

// 恐怖度メーターHUD描画（画面左上、時計の右横に配置）
void HorrorUI::DrawFearMeter(float fearPercent, bool inSafeLight)
{
    // 表示用恐怖度パーセントの滑らかな補間（急激なバーの跳ねを防止）
    displayedFearPercent += (fearPercent - displayedFearPercent) * 0.12f;
    if (displayedFearPercent < 0.0f) displayedFearPercent = 0.0f;
    if (displayedFearPercent > 100.0f) displayedFearPercent = 100.0f;

    int posX = 134;
    int posY = 18;
    int panelW = 102;
    int panelH = 28;

    // レトロ端末パネル背景
    DrawRetroPanel(posX - 6, posY - 4, posX + panelW, posY + panelH, COL_BORDER_DIM, COL_BG, 140);

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

    // 恐怖度数値ラベル（Chillerホラーフォント）
    char fearStr[32];
    snprintf(fearStr, sizeof(fearStr), "FEAR %02d%%", static_cast<int>(displayedFearPercent));
    DrawJitterString(posX + 4 + jitterX, posY + jitterY, fearStr, fearColor, fontSmall);

    // 安全光内フィードバック（控えめな緑系微小インジケータ）
    if (inSafeLight)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
        DrawJitterString(posX + 68, posY, "SAFE", GetColor(110, 175, 140), fontSmall);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // ゲージスロット背景枠
    int barX = posX + 4;
    int barY = posY + 19;
    int barW = 94;
    int barH = 5;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
    DrawBox(barX, barY, barX + barW, barY + barH, GetColor(25, 32, 28), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(barX, barY, barX + barW, barY + barH, COL_BORDER_DIM, FALSE);

    // ゲージ塗りつぶし
    int fillW = static_cast<int>((displayedFearPercent / 100.0f) * static_cast<float>(barW - 2));
    if (fillW > 0)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawBox(barX + 1, barY + 1, barX + 1 + fillW, barY + barH - 1, fearColor, TRUE);
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

    // ゲームオーバータイトル（赤系）
    if (fontLarge != -1)
    {
        DrawStringToHandle(cx - 80, cy - 65, "GAME OVER", GetColor(200, 50, 45), fontLarge);
    }
    else
    {
        DrawString(cx - 40, cy - 65, "GAME OVER", GetColor(200, 50, 45));
    }

    // ホラー演出テキスト
    DrawString(cx - 105, cy - 10, "CONSUMED BY THE DARKNESS...", COL_TEXT_DIM);

    // 操作案内（リトライ・終了）
    bool blink = ((frameCount / 30) % 2) == 0;
    unsigned int promptCol = blink ? COL_TEXT : COL_TEXT_DIM;
    DrawString(cx - 90, cy + 40, "[ R ] RETRY DELIVERY", promptCol);
    DrawString(cx - 50, cy + 65, "[ ESC ] QUIT", COL_TEXT_DIM);
}
