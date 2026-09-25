#include "HorrorUI.h"
#include <cmath>
#include <cstdlib>

// カラーパレット定数定義（彩度を落とした低照度深夜トーン）
const unsigned int HorrorUI::COL_BG         = GetColor(14, 18, 16);
const unsigned int HorrorUI::COL_PANEL_BG   = GetColor(18, 24, 20);
const unsigned int HorrorUI::COL_BORDER     = GetColor(65, 80, 72);
const unsigned int HorrorUI::COL_BORDER_DIM = GetColor(40, 50, 45);
const unsigned int HorrorUI::COL_TEXT       = GetColor(185, 195, 188);
const unsigned int HorrorUI::COL_TEXT_DIM   = GetColor(105, 115, 110);
const unsigned int HorrorUI::COL_WARNING    = GetColor(175, 75, 65);
const unsigned int HorrorUI::COL_TARGET     = GetColor(190, 95, 50);
const unsigned int HorrorUI::COL_PLAYER     = GetColor(210, 215, 205);
const unsigned int HorrorUI::COL_BIKE       = GetColor(110, 165, 175);

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
}

// デストラクタ
HorrorUI::~HorrorUI()
{
    Finalize();
}

// 初期化（フォント生成）
void HorrorUI::Initialize()
{
    // 等幅フォント "Consolas" を優先、無ければ "MS Gothic" や標準フォントへフォールバック
    fontSmall = CreateFontToHandle("Consolas", 14, 1, DX_FONTTYPE_NORMAL);
    if (fontSmall == -1) fontSmall = CreateFontToHandle("ＭＳ ゴシック", 14, 1, DX_FONTTYPE_NORMAL);

    fontMedium = CreateFontToHandle("Consolas", 18, 2, DX_FONTTYPE_NORMAL);
    if (fontMedium == -1) fontMedium = CreateFontToHandle("ＭＳ ゴシック", 18, 2, DX_FONTTYPE_NORMAL);

    fontLarge = CreateFontToHandle("Consolas", 26, 3, DX_FONTTYPE_NORMAL);
    if (fontLarge == -1) fontLarge = CreateFontToHandle("ＭＳ ゴシック", 26, 3, DX_FONTTYPE_NORMAL);

    fontClock = CreateFontToHandle("Consolas", 22, 2, DX_FONTTYPE_NORMAL);
    if (fontClock == -1) fontClock = CreateFontToHandle("ＭＳ ゴシック", 22, 2, DX_FONTTYPE_NORMAL);
}

// 終了処理（フォントハンドルの破棄）
void HorrorUI::Finalize()
{
    if (fontSmall != -1) { DeleteFontToHandle(fontSmall); fontSmall = -1; }
    if (fontMedium != -1) { DeleteFontToHandle(fontMedium); fontMedium = -1; }
    if (fontLarge != -1) { DeleteFontToHandle(fontLarge); fontLarge = -1; }
    if (fontClock != -1) { DeleteFontToHandle(fontClock); fontClock = -1; }
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

// アナログ端末風の枠付きパネル描画
void HorrorUI::DrawRetroPanel(int x1, int y1, int x2, int y2, unsigned int borderColor, unsigned int bgColor, int alpha)
{
    // 背景塗り
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawBox(x1, y1, x2, y2, bgColor, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 外枠ライン
    DrawBox(x1, y1, x2, y2, borderColor, FALSE);

    // 四隅のコーナーレトロブラケット補強（アナログ機器らしさの演出）
    int cornerLen = 5;
    DrawLine(x1, y1, x1 + cornerLen, y1, borderColor);
    DrawLine(x1, y1, x1, y1 + cornerLen, borderColor);
    DrawLine(x2 - cornerLen, y1, x2, y1, borderColor);
    DrawLine(x2, y1, x2, y1 + cornerLen, borderColor);
    DrawLine(x1, y2 - cornerLen, x1, y2, borderColor);
    DrawLine(x1, y2, x1 + cornerLen, y2, borderColor);
    DrawLine(x2 - cornerLen, y2, x2, y2, borderColor);
    DrawLine(x2, y2 - cornerLen, x2, y2, borderColor);
}

// ノイズ・ジッター付き文字列描画
void HorrorUI::DrawJitterString(int x, int y, const char* text, unsigned int color, int fontHandle)
{
    int drawX = x;
    int drawY = y;

    if (horrorEffectsEnabled)
    {
        // グリッチまたは強い干渉時、または低確率での微小な1フレームジッター
        bool shouldJitter = (jitterTimer > 0) || (glitchTimer > 0) ||
                            (horrorIntensity >= 2 && (frameCount % 120 == 0)) ||
                            (interferenceAmount > 0.5f && (rand() % 10 == 0));

        if (shouldJitter)
        {
            drawX += (rand() % 3) - 1;
            drawY += (rand() % 3) - 1;
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
    int posY = 22;

    // ごく控えめな背景
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
    DrawBox(posX - 6, posY - 4, posX + 105, posY + 26, COL_BG, TRUE);
    DrawBox(posX - 6, posY - 4, posX + 105, posY + 26, COL_BORDER_DIM, FALSE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // デジタル機器調の文字
    DrawJitterString(posX + 4, posY + 1, timeStr, COL_TEXT, fontClock);
}

// 新聞所持数・ステータスの描画（画面下部: 例 PAPERS 07）
void HorrorUI::DrawPaperCargo(int paperCount, bool isHolding)
{
    char paperStr[32];
    snprintf(paperStr, sizeof(paperStr), "PAPERS  %02d", paperCount);

    int posX = 26;
    int posY = 668;

    // 背景枠
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
    DrawBox(posX - 6, posY - 4, posX + 120 + (isHolding ? 88 : 0), posY + 24, COL_BG, TRUE);
    DrawBox(posX - 6, posY - 4, posX + 120 + (isHolding ? 88 : 0), posY + 24, COL_BORDER_DIM, FALSE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 所持部数
    DrawJitterString(posX + 4, posY + 2, paperStr, COL_TEXT, fontSmall);

    // 手持ち状態の表示
    if (isHolding)
    {
        // 控えめな点滅
        bool blink = ((frameCount / 25) % 2) == 0;
        unsigned int col = blink ? COL_TARGET : COL_TEXT_DIM;
        DrawJitterString(posX + 115, posY + 2, "[HOLDING]", col, fontSmall);
    }
}
