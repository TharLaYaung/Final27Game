#include "SceneManager.h"
#include "Player.h"
#include "Bicycle.h"
#include "Newspaper.h"
#include "Mailbox.h"
#include "BatteryManager.h"
#include "DeliveryManager.h"
#include "Minimap.h"
#include "NightEnvironment.h"
#include "WeatherManager.h"
#include "HorrorManager.h"
#include "HorrorUI.h"
#include "Ground.h"
#include "Map.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

// プロシージャルUI音声生成ヘルパー（外部WAVファイル不要でメモリ内合成）
static int CreateProceduralSound(float freq, float durationSec, float decayRate)
{
    int sampleRate = 44100;
    int numSamples = (int)(sampleRate * durationSec);
    int dataSize = numSamples * 2;
    int fileSize = 44 + dataSize;

    std::vector<unsigned char> wav(fileSize);

    // RIFFヘッダ
    std::memcpy(&wav[0], "RIFF", 4);
    int riffSize = fileSize - 8;
    std::memcpy(&wav[4], &riffSize, 4);
    std::memcpy(&wav[8], "WAVE", 4);

    // fmtチャンク（PCM 16bit 44.1kHz モノラル）
    std::memcpy(&wav[12], "fmt ", 4);
    int fmtSize = 16;
    short audioFormat = 1;
    short numChannels = 1;
    int byteRate = sampleRate * 2;
    short blockAlign = 2;
    short bitsPerSample = 16;
    std::memcpy(&wav[16], &fmtSize, 4);
    std::memcpy(&wav[20], &audioFormat, 2);
    std::memcpy(&wav[22], &numChannels, 2);
    std::memcpy(&wav[24], &sampleRate, 4);
    std::memcpy(&wav[28], &byteRate, 4);
    std::memcpy(&wav[32], &blockAlign, 2);
    std::memcpy(&wav[34], &bitsPerSample, 2);

    // dataチャンク
    std::memcpy(&wav[36], "data", 4);
    std::memcpy(&wav[40], &dataSize, 4);

    short* samples = (short*)&wav[44];
    for (int i = 0; i < numSamples; ++i)
    {
        float t = (float)i / sampleRate;
        float env = std::exp(-t * decayRate);
        float s = std::sin(2.0f * DX_PI_F * freq * t) * env;
        samples[i] = (short)(s * 28000.0f);
    }

    return LoadSoundMemByMemImage(&wav[0], wav.size());
}

// コンストラクタ
SceneManager::SceneManager()
    : currentScene(SceneType::Title)
    , targetScene(SceneType::Title)
    , transitionState(TransitionState::None)
    , transitionTimer(0.0f)
    , transitionDuration(0.8f)
    , transitionAlpha(0.0f)
    , currentDay(1)
    , titleSelectedDay(1)
    , titleSubMenu(TitleSubMenu::Main)
    , titleMenuSelection(0)
    , settingsSelection(0)
    , masterVolume(95)
    , bgmVolume(90)
    , sfxVolume(90)
    , audioTestFeedbackTimer(0.0f)
    , mouseSensitivityLevel(50)
    , soundMenuSelect(-1)
    , soundMenuAdjust(-1)
    , soundTestPulse(-1)
    , isPaused(false)
    , pauseSubMenu(PauseSubMenu::Main)
    , pauseMenuSelection(0)
    , titleTimer(0.0f)
    , clearTimer(0.0f)
    , gameOverTimer(0.0f)
    , mouseX(640)
    , mouseY(360)
    , isMouseLeft(false)
    , oldMouseLeft(false)
    , mouseClicked(false)
    , fontTitleKanji(-1)
    , fontTitleEnglish(-1)
    , fontSubText(-1)
    , fontMenu(-1)
    , fontReportTitle(-1)
    , fontReportBody(-1)
    , oldEnterKey(false)
    , oldSpaceKey(false)
    , oldRKey(false)
    , oldEscKey(false)
    , oldUpKey(false)
    , oldDownKey(false)
    , oldLeftKey(false)
    , oldRightKey(false)
    , showRulePopup(false)
    , rulePopupFromManual(false)
    , hasShownInitialRulePopup(false)
    , oldHKey(false)
    , fontPopupTitle(-1)
    , fontPopupSection(-1)
    , fontPopupBody(-1)
    , fontPopupSub(-1)
{
}

// デストラクタ
SceneManager::~SceneManager()
{
    Finalize();
}

// 初期化
bool SceneManager::Initialize()
{
    currentScene = SceneType::Title;
    targetScene = SceneType::Title;
    transitionState = TransitionState::None;
    transitionTimer = 0.0f;
    transitionDuration = 0.8f;
    transitionAlpha = 0.0f;

    currentDay = 1;
    titleSelectedDay = 1;
    titleSubMenu = TitleSubMenu::Main;
    titleMenuSelection = 0;
    settingsSelection = 0;
    masterVolume = 95;
    bgmVolume = 90;
    sfxVolume = 90;
    audioTestFeedbackTimer = 0.0f;
    mouseSensitivityLevel = 50;
    isPaused = false;
    pauseSubMenu = PauseSubMenu::Main;
    pauseMenuSelection = 0;

    SetMasterVolume(masterVolume);
    SetBgmVolume(bgmVolume);
    SetSfxVolume(sfxVolume);

    // プロシージャルUI音声の合成
    soundMenuSelect = CreateProceduralSound(650.0f, 0.05f, 40.0f);
    soundMenuAdjust = CreateProceduralSound(420.0f, 0.04f, 45.0f);
    soundTestPulse = CreateProceduralSound(160.0f, 0.35f, 9.0f);

    titleTimer = 0.0f;
    clearTimer = 0.0f;
    gameOverTimer = 0.0f;

    mouseX = 640;
    mouseY = 360;
    isMouseLeft = false;
    oldMouseLeft = false;
    mouseClicked = false;

    oldEnterKey = false;
    oldSpaceKey = false;
    oldRKey = false;
    oldEscKey = false;
    oldUpKey = false;
    oldDownKey = false;
    oldLeftKey = false;
    oldRightKey = false;

    // タイトル画面でマウス操作を可能にするためマウスカーソルを表示
    SetMouseDispFlag(TRUE);

    // 日本語フォント生成ヘルパー（Meiryo -> Yu Gothic -> MS Gothic の順で安全に生成）
    auto CreateJPFont = [](const char* name1, const char* name2, int size, int thick, int fontType) -> int {
        int h = CreateFontToHandle(name1, size, thick, fontType);
        if (h == -1 && name2 != nullptr) {
            h = CreateFontToHandle(name2, size, thick, fontType);
        }
        if (h == -1) {
            h = CreateFontToHandle("MS Gothic", size, thick, DX_FONTTYPE_NORMAL);
        }
        return h;
    };

    // 大見出しホラーロゴ（Chiller優先：英語演出用メインタイトル）
    fontTitleKanji = CreateFontToHandle("Chiller", 68, 4, DX_FONTTYPE_ANTIALIASING);
    if (fontTitleKanji == -1) fontTitleKanji = CreateFontToHandle("MS Gothic", 56, 6, DX_FONTTYPE_ANTIALIASING_EDGE_8X8);

    // サブタイトル（英語演出用）
    fontTitleEnglish = CreateFontToHandle("Chiller", 28, 2, DX_FONTTYPE_ANTIALIASING);
    if (fontTitleEnglish == -1) fontTitleEnglish = CreateFontToHandle("MS Gothic", 22, 3, DX_FONTTYPE_NORMAL);

    // 説明文・キャッチコピー用日本語フォント
    fontSubText = CreateJPFont("Meiryo", "Yu Gothic", 16, 2, DX_FONTTYPE_ANTIALIASING);

    // ボタンメニュー用日本語フォント（タイトル、ポーズ、ゲームオーバー等のボタン項目）
    fontMenu = CreateJPFont("Meiryo", "Yu Gothic", 18, 3, DX_FONTTYPE_ANTIALIASING);

    // レポート・設定・ポーズ見出し用日本語フォント
    fontReportTitle = CreateJPFont("Meiryo", "Yu Gothic", 22, 3, DX_FONTTYPE_ANTIALIASING);

    // レポート本文用日本語フォント
    fontReportBody = CreateJPFont("Meiryo", "Yu Gothic", 15, 2, DX_FONTTYPE_ANTIALIASING);

    // ルール説明ポップアップ用日本語フォント
    fontPopupTitle = CreateJPFont("Meiryo", "Yu Gothic", 24, 4, DX_FONTTYPE_ANTIALIASING);
    fontPopupSection = CreateJPFont("Meiryo", "Yu Gothic", 17, 3, DX_FONTTYPE_ANTIALIASING);
    fontPopupBody = CreateJPFont("Meiryo", "Yu Gothic", 15, 2, DX_FONTTYPE_ANTIALIASING);
    fontPopupSub = CreateJPFont("Meiryo", "Yu Gothic", 13, 2, DX_FONTTYPE_ANTIALIASING);

    showRulePopup = false;
    rulePopupFromManual = false;
    hasShownInitialRulePopup = false;
    oldHKey = false;

    AudioManager::Instance().PlayTitleBgm();
    return true;
}

// 終了処理
void SceneManager::Finalize()
{
    if (fontTitleKanji != -1) { DeleteFontToHandle(fontTitleKanji); fontTitleKanji = -1; }
    if (fontTitleEnglish != -1) { DeleteFontToHandle(fontTitleEnglish); fontTitleEnglish = -1; }
    if (fontSubText != -1) { DeleteFontToHandle(fontSubText); fontSubText = -1; }
    if (fontMenu != -1) { DeleteFontToHandle(fontMenu); fontMenu = -1; }
    if (fontReportTitle != -1) { DeleteFontToHandle(fontReportTitle); fontReportTitle = -1; }
    if (fontReportBody != -1) { DeleteFontToHandle(fontReportBody); fontReportBody = -1; }

    if (fontPopupTitle != -1) { DeleteFontToHandle(fontPopupTitle); fontPopupTitle = -1; }
    if (fontPopupSection != -1) { DeleteFontToHandle(fontPopupSection); fontPopupSection = -1; }
    if (fontPopupBody != -1) { DeleteFontToHandle(fontPopupBody); fontPopupBody = -1; }
    if (fontPopupSub != -1) { DeleteFontToHandle(fontPopupSub); fontPopupSub = -1; }

    if (soundMenuSelect != -1) { DeleteSoundMem(soundMenuSelect); soundMenuSelect = -1; }
    if (soundMenuAdjust != -1) { DeleteSoundMem(soundMenuAdjust); soundMenuAdjust = -1; }
    if (soundTestPulse != -1) { DeleteSoundMem(soundTestPulse); soundTestPulse = -1; }
}

void SceneManager::SetMasterVolume(int vol)
{
    masterVolume = (vol < 0) ? 0 : (vol > 100 ? 100 : vol);
    AudioManager::Instance().SetMasterVolume(masterVolume);
}

void SceneManager::SetBgmVolume(int vol)
{
    bgmVolume = (vol < 0) ? 0 : (vol > 100 ? 100 : vol);
    AudioManager::Instance().SetBgmVolume(bgmVolume);
}

void SceneManager::SetSfxVolume(int vol)
{
    sfxVolume = (vol < 0) ? 0 : (vol > 100 ? 100 : vol);
    AudioManager::Instance().SetSfxVolume(sfxVolume);
}

void SceneManager::SetMouseSensitivityLevel(int lvl)
{
    mouseSensitivityLevel = (lvl < 1) ? 1 : (lvl > 100 ? 100 : lvl);
}

void SceneManager::PlayMenuClick()
{
    AudioManager::Instance().PlayMenuClick();
}

void SceneManager::PlayMenuAdjust()
{
    AudioManager::Instance().PlayMenuAdjust();
}

void SceneManager::PlayTestAudioPulse()
{
    AudioManager::Instance().PlayTestAudioPulse();
}

void SceneManager::SetPaused(bool paused)
{
    isPaused = paused;
    pauseMenuSelection = 0;
    pauseSubMenu = PauseSubMenu::Main;

    // ポーズ中はゲーム内時刻の進行を一時停止
    HorrorUI::Instance().SetTimePaused(paused);

    if (isPaused)
    {
        // ポーズ中はマウスカーソルを表示してメニュー選択可能にする
        SetMouseDispFlag(TRUE);
    }
    else
    {
        // ゲーム再開時はマウスカーソルを非表示にし画面中央へ固定
        SetMouseDispFlag(FALSE);
        SetMousePoint(640, 360);
    }
}

// ルール説明ポップアップを開く
// fromManual: プレイ中にHキーで開かれた場合はtrue、開始時の自動表示はfalse
void SceneManager::OpenRulePopup(bool fromManual)
{
    showRulePopup = true;
    rulePopupFromManual = fromManual;

    // ポップアップ表示中はゲーム内時刻の進行を完全に停止
    HorrorUI::Instance().SetTimePaused(true);

    // ポップアップ内ボタン・UI操作のためマウスカーソルを表示
    SetMouseDispFlag(TRUE);

    // 開いた瞬間のキー・クリック誤伝達を防止
    oldEnterKey = true;
    oldSpaceKey = true;
    oldEscKey = true;
    oldHKey = true;
    oldMouseLeft = true;
    mouseClicked = false;
}

// ルール説明ポップアップを閉じてゲームプレイを再開
void SceneManager::CloseRulePopup()
{
    showRulePopup = false;

    // 通常のポーズ中でなければゲーム内時刻の進行を再開
    if (!isPaused)
    {
        HorrorUI::Instance().SetTimePaused(false);
    }

    // ゲームプレイ操作への復帰：マウスカーソルを非表示にし画面中央へ戻す
    if (!isPaused && currentScene == SceneType::Game)
    {
        SetMouseDispFlag(FALSE);
        SetMousePoint(640, 360);
    }

    // 閉じた直後のキー・クリックがゲーム内操作（新聞受取・配達・自転車乗車・ポーズ等）へ誤伝達しないよう入力状態を消費
    oldEnterKey = true;
    oldSpaceKey = true;
    oldEscKey = true;
    oldHKey = true;
    oldMouseLeft = true;
    mouseClicked = false;
}

// シーン遷移の開始
void SceneManager::StartTransition(SceneType target, float duration)
{
    if (transitionState != TransitionState::None)
    {
        return;
    }

    targetScene = target;
    transitionDuration = (duration > 0.1f) ? duration : 0.8f;
    transitionTimer = 0.0f;
    transitionState = TransitionState::FadeOut;

    // ゲーム本編以外へ遷移する場合はマウスカーソルを表示
    if (target != SceneType::Game)
    {
        SetMouseDispFlag(TRUE);
    }
}

// 7日間進行管理
void SceneManager::SetCurrentDay(int day)
{
    if (day < 1) day = 1;
    if (day > 7) day = 7;
    currentDay = day;
}

// ゲーム状態の全リセット（日別開始・リトライ・タイトル復帰用）
void SceneManager::ResetGame(
    Player& player,
    Bicycle& bicycle,
    Newspaper& newspaper,
    BatteryManager& batteryManager,
    DeliveryManager& deliveryManager,
    NightEnvironment& nightEnv,
    WeatherManager& weatherManager,
    HorrorManager& horrorManager,
    int day
)
{
    if (day < 1) day = 1;
    if (day > 7) day = 7;
    currentDay = day;

    // プレイヤー位置・視線・カメラ傾きのリセット
    player.SetPosition(VGet(0.0f, 1.7f, -30.0f));
    player.SetYaw(0.0f);
    player.SetPitch(0.0f);
    player.SetCameraRoll(0.0f);
    player.SetEyeHeightOffset(0.0f);
    player.SetSwayOffset(VGet(0.0f, 0.0f, 0.0f), 0.0f, 0.0f);
    player.SetCameraShakeOffset(VGet(0.0f, 0.0f, 0.0f));

    // 配達状況の日別セットアップ（決定論的シードによる配達順設定）
    deliveryManager.SetDay(currentDay);

    // 予備バッテリー所持数および日別スポーン位置のリセット
    batteryManager.Reset(currentDay);

    // 決定論的シードに基づく玄関灯の点灯設定と安全検証
    unsigned int daySeed = deliveryManager.GetCurrentDaySeed();
    MapSetDayPorchLights(currentDay, daySeed);
    MapValidateSafety();

    // 自転車のリセットと日別バッテリー消費倍率
    bicycle.Reset();
    float batteryMultipliers[7] = { 1.0f, 1.10f, 1.25f, 1.40f, 1.60f, 1.80f, 2.00f };
    bicycle.SetBatteryDrainMultiplier(batteryMultipliers[currentDay - 1]);

    // 新聞のリセット（本日の必要部数）
    newspaper.Reset(deliveryManager.GetTotalDeliveries());

    // 深夜環境・街灯損壊プリセット
    nightEnv.SetPreset(EnvironmentPreset::NightDeliveryStyle);
    nightEnv.SetDayEnvironment(currentDay);

    // 動的天候の日別セットアップ
    weatherManager.Initialize();
    weatherManager.SetDayWeather(currentDay, nightEnv);

    // 恐怖・怪異システムの日別難易度設定および完全リセット
    horrorManager.Reset();
    horrorManager.Initialize();
    horrorManager.SetDayDifficulty(currentDay);

    // ホラーUIの日数・告知バナー・ゲーム開始時刻（午前1時）設定
    HorrorUI::Instance().SetCurrentDay(currentDay);
    HorrorUI::Instance().SetGameTime(1, 0);
    HorrorUI::Instance().SetTimePaused(false);
    HorrorUI::Instance().TriggerDayBanner();

    // ポーズ解除
    isPaused = false;
    pauseMenuSelection = 0;

    // タイマーのリセット
    clearTimer = 0.0f;
    gameOverTimer = 0.0f;
}

// マウスホバー・クリック対応ボタン描画ヘルパー（不気味なホラー端末スタイル）
bool SceneManager::DrawButton(int x1, int y1, int x2, int y2, const char* label, unsigned int baseCol, unsigned int hoverCol, int fontHandle)
{
    bool isHover = (mouseX >= x1 && mouseX <= x2 && mouseY >= y1 && mouseY <= y2);
    unsigned int borderCol = isHover ? GetColor(160, 60, 50) : HorrorUI::COL_BORDER;
    unsigned int bgCol = isHover ? GetColor(28, 14, 16) : HorrorUI::COL_PANEL_BG;
    int alpha = isHover ? 235 : 190;

    HorrorUI::Instance().DrawRetroPanel(x1, y1, x2, y2, borderCol, bgCol, alpha);

    unsigned int textCol = isHover ? hoverCol : baseCol;
    int strW = 0;
    int jx = 0;
    int jy = 0;
    if (isHover && (std::rand() % 12 == 0))
    {
        jx = (std::rand() % 3) - 1;
        jy = (std::rand() % 3) - 1;
    }

    if (fontHandle != -1)
    {
        strW = GetDrawStringWidthToHandle(label, (int)strlen(label), fontHandle);
        int tx = (x1 + x2 - strW) / 2 + jx;
        int ty = (y1 + y2 - 20) / 2 + jy;

        // ホバー時の色収差シャドウ
        if (isHover)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
            DrawStringToHandle(tx + 1, ty, label, GetColor(180, 40, 35), fontHandle);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        DrawStringToHandle(tx, ty, label, textCol, fontHandle);
    }
    else
    {
        strW = GetDrawStringWidth(label, (int)strlen(label));
        int tx = (x1 + x2 - strW) / 2 + jx;
        int ty = (y1 + y2 - 16) / 2 + jy;
        DrawString(tx, ty, label, textCol);
    }

    if (isHover)
    {
        DrawString(x1 + 10, (y1 + y2 - 16) / 2, ">", GetColor(210, 50, 45));
        DrawString(x2 - 18, (y1 + y2 - 16) / 2, "<", GetColor(210, 50, 45));
    }

    return isHover && mouseClicked;
}

// 共通設定入力処理（タイトル・ポーズ共用）
void SceneManager::HandleSettingsInput(
    bool upDown, bool downDown, bool leftDown, bool rightDown,
    bool enterDown, bool spaceDown, bool escDown,
    Player& player, bool inPause
)
{
    // 設定項目カーソル移動（6項目：0〜5）
    if (upDown)
    {
        settingsSelection = (settingsSelection <= 0) ? 5 : settingsSelection - 1;
        PlayMenuAdjust();
    }
    if (downDown)
    {
        settingsSelection = (settingsSelection >= 5) ? 0 : settingsSelection + 1;
        PlayMenuAdjust();
    }

    int cx = 640;
    int setY = 130;
    int barW = 240;
    int btnW = 28;
    int barX = cx - 280 + 160;
    int sliderX = barX + btnW + 8;
    int rightBtnX = sliderX + barW + 8;

    // スライダー操作のラムダ（矢印キー・クリック・ドラッグ完全対応）
    auto HandleSlider = [&](int row, int& val, int minVal, int maxVal, int y) {
        bool hitLeftBtn = CheckButtonHit(barX, y, barX + btnW, y + 28);
        bool hitRightBtn = CheckButtonHit(rightBtnX, y, rightBtnX + btnW, y + 28);
        bool hoverBar = CheckButtonHover(sliderX, y + 2, sliderX + barW, y + 26);

        bool dec = hitLeftBtn || (settingsSelection == row && leftDown);
        bool inc = hitRightBtn || (settingsSelection == row && rightDown);

        bool changed = false;
        if (dec)
        {
            val = (val - 5 < minVal) ? minVal : val - 5;
            changed = true;
        }
        if (inc)
        {
            val = (val + 5 > maxVal) ? maxVal : val + 5;
            changed = true;
        }
        if (hoverBar && isMouseLeft)
        {
            int relX = mouseX - sliderX;
            float rate = (float)relX / (float)barW;
            rate = (rate < 0.0f) ? 0.0f : (rate > 1.0f ? 1.0f : rate);
            int newVal = minVal + (int)(rate * (maxVal - minVal) + 0.5f);
            if (newVal != val)
            {
                val = newVal;
                changed = true;
            }
        }

        if (changed)
        {
            if (row == 0) SetMasterVolume(val);
            else if (row == 1) SetBgmVolume(val);
            else if (row == 2) SetSfxVolume(val);
            else if (row == 3)
            {
                SetMouseSensitivityLevel(val);
                player.SetMouseSensitivity(GetCalculatedMouseSensitivity());
            }
            PlayMenuAdjust();
        }
    };

    HandleSlider(0, masterVolume,           0, 100, setY + 65);
    HandleSlider(1, bgmVolume,              0, 100, setY + 125);
    HandleSlider(2, sfxVolume,              0, 100, setY + 185);
    HandleSlider(3, mouseSensitivityLevel, 1, 100, setY + 245);

    int testBtnY = setY + 315;
    int backBtnY = setY + 365;
    bool hitTest = CheckButtonHit(cx - 160, testBtnY, cx + 160, testBtnY + 36);
    bool hitBack = CheckButtonHit(cx - 160, backBtnY, cx + 160, backBtnY + 36);

    if (hitTest) settingsSelection = 4;
    if (hitBack) settingsSelection = 5;

    if ((settingsSelection == 4 && (enterDown || spaceDown)) || hitTest)
    {
        PlayTestAudioPulse();
        audioTestFeedbackTimer = 0.8f;
    }
    if ((settingsSelection == 5 && (enterDown || spaceDown)) || hitBack || escDown)
    {
        PlayMenuClick();
        if (inPause)
        {
            pauseSubMenu = PauseSubMenu::Main;
        }
        else
        {
            titleSubMenu = TitleSubMenu::Main;
        }
    }
}

// シーン毎フレーム更新
void SceneManager::Update(
    float dt,
    Player& player,
    Bicycle& bicycle,
    Newspaper& newspaper,
    Mailbox& mailbox,
    BatteryManager& batteryManager,
    DeliveryManager& deliveryManager,
    NightEnvironment& nightEnv,
    WeatherManager& weatherManager,
    HorrorManager& horrorManager,
    bool& outQuitRequested
)
{
    (void)mailbox;

    // マウス入力更新
    GetMousePoint(&mouseX, &mouseY);
    isMouseLeft = ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
    mouseClicked = (isMouseLeft && !oldMouseLeft);
    oldMouseLeft = isMouseLeft;

    // マウスクリック判定はメンバー関数 CheckButtonHit / CheckButtonHover を使用

    // キー入力エッジトリガー検出
    bool curEnter = (CheckHitKey(KEY_INPUT_RETURN) != 0 || CheckHitKey(KEY_INPUT_NUMPADENTER) != 0);
    bool enterDown = curEnter && !oldEnterKey;
    oldEnterKey = curEnter;

    bool curSpace = (CheckHitKey(KEY_INPUT_SPACE) != 0);
    bool spaceDown = curSpace && !oldSpaceKey;
    oldSpaceKey = curSpace;

    bool curR = (CheckHitKey(KEY_INPUT_R) != 0);
    bool rDown = curR && !oldRKey;
    oldRKey = curR;

    bool curEsc = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);
    bool escDown = curEsc && !oldEscKey;
    oldEscKey = curEsc;

    bool curUp = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool upDown = curUp && !oldUpKey;
    oldUpKey = curUp;

    bool curDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool downDown = curDown && !oldDownKey;
    oldDownKey = curDown;

    bool curLeft = (CheckHitKey(KEY_INPUT_LEFT) != 0 || CheckHitKey(KEY_INPUT_A) != 0);
    bool leftDown = curLeft && !oldLeftKey;
    oldLeftKey = curLeft;

    bool curRight = (CheckHitKey(KEY_INPUT_RIGHT) != 0 || CheckHitKey(KEY_INPUT_D) != 0);
    bool rightDown = curRight && !oldRightKey;
    oldRightKey = curRight;

    // Hキー（ヘルプ・遊び方ポップアップトグル用エッジトリガー）
    bool curH = (CheckHitKey(KEY_INPUT_H) != 0);
    bool hDown = curH && !oldHKey;
    oldHKey = curH;

    // トランジション進行処理
    if (transitionState == TransitionState::FadeOut)
    {
        float halfDur = transitionDuration * 0.5f;
        transitionTimer += dt;
        transitionAlpha = transitionTimer / halfDur;
        if (transitionAlpha >= 1.0f)
        {
            transitionAlpha = 1.0f;
            currentScene = targetScene;
            transitionState = TransitionState::FadeIn;
            transitionTimer = 0.0f;

            if (currentScene == SceneType::Game)
            {
                SetPaused(false);
                SetMouseDispFlag(FALSE);
                SetMousePoint(640, 360);
                AudioManager::Instance().PlayGameAmbienceBgm();

                // 【最重要仕様】タイトル画面から初めてDay 1を開始した時のみルール説明を自動表示
                if (currentDay == 1 && !hasShownInitialRulePopup)
                {
                    OpenRulePopup(false);
                    hasShownInitialRulePopup = true;
                }
            }
            else if (currentScene == SceneType::Title)
            {
                SetMouseDispFlag(TRUE);
                AudioManager::Instance().PlayTitleBgm();
            }
            else
            {
                SetMouseDispFlag(TRUE);
                AudioManager::Instance().StopBgm();
            }
        }return;
    }
    else if (transitionState == TransitionState::FadeIn)
    {
        float halfDur = transitionDuration * 0.5f;
        transitionTimer += dt;
        transitionAlpha = 1.0f - (transitionTimer / halfDur);
        if (transitionAlpha <= 0.0f)
        {
            transitionAlpha = 0.0f;
            transitionState = TransitionState::None;
            transitionTimer = 0.0f;
        }
    }

    int cx = 640;
    int cy = 360;

    // シーン別処理
    switch (currentScene)
    {
    case SceneType::Title:
        {
            SetMouseDispFlag(TRUE);
            titleTimer += dt;

            // タイトルBGMの確実な再生維持
            AudioManager::Instance().PlayTitleBgm();

            if (audioTestFeedbackTimer > 0.0f)
            {
                audioTestFeedbackTimer -= dt;
            }

            // タイトル画面でのシネマティックカメラ移動
            float camT = titleTimer * 0.05f;
            VECTOR camPos = VGet(-25.0f + std::sin(camT * 0.5f) * 1.5f, 2.2f, -28.0f + std::fmod(camT * 12.0f, 42.0f));
            VECTOR camTarget = VGet(-25.0f, 1.8f, camPos.z + 10.0f);
            SetCameraPositionAndTarget_UpVecY(camPos, camTarget);

            // 夜間環境・天候の背景更新
            nightEnv.Update(bicycle, player.GetPosition());
            weatherManager.Update(dt, player, bicycle, nightEnv);

            if (titleSubMenu == TitleSubMenu::Main)
            {
                // キーボードによるメニュー選択カーソル移動（4項目：0〜3）
                if (upDown)
                {
                    titleMenuSelection = (titleMenuSelection <= 0) ? 3 : titleMenuSelection - 1;
                    PlayMenuAdjust();
                }
                if (downDown)
                {
                    titleMenuSelection = (titleMenuSelection >= 3) ? 0 : titleMenuSelection + 1;
                    PlayMenuAdjust();
                }

                int menuY = 250;
                // ボタンの当たり判定（4項目）
                bool hitStart    = CheckButtonHit(cx - 200, menuY + 20,  cx + 200, menuY + 62);
                bool hitSettings = CheckButtonHit(cx - 200, menuY + 76,  cx + 200, menuY + 118);
                bool hitGuide    = CheckButtonHit(cx - 200, menuY + 132, cx + 200, menuY + 174);
                bool hitQuit     = CheckButtonHit(cx - 200, menuY + 188, cx + 200, menuY + 230);

                if (CheckButtonHover(cx - 200, menuY + 20,  cx + 200, menuY + 62))  titleMenuSelection = 0;
                if (CheckButtonHover(cx - 200, menuY + 76,  cx + 200, menuY + 118)) titleMenuSelection = 1;
                if (CheckButtonHover(cx - 200, menuY + 132, cx + 200, menuY + 174)) titleMenuSelection = 2;
                if (CheckButtonHover(cx - 200, menuY + 188, cx + 200, menuY + 230)) titleMenuSelection = 3;

                // 決定処理（クリックまたはEnter/Space）
                if (((titleMenuSelection == 0 && (enterDown || spaceDown)) || hitStart) && !IsTransitioning())
                {
                    PlayMenuClick();
                    hasShownInitialRulePopup = false;
                    ResetGame(player, bicycle, newspaper, batteryManager, deliveryManager, nightEnv, weatherManager, horrorManager, 1);
                    StartTransition(SceneType::Game, 0.8f);
                }
                else if ((titleMenuSelection == 1 && (enterDown || spaceDown)) || hitSettings)
                {
                    PlayMenuClick();
                    titleSubMenu = TitleSubMenu::Settings;
                    settingsSelection = 0;
                }
                else if ((titleMenuSelection == 2 && (enterDown || spaceDown)) || hitGuide)
                {
                    PlayMenuClick();
                    titleSubMenu = TitleSubMenu::KeyGuide;
                }
                else if (((titleMenuSelection == 3 && (enterDown || spaceDown)) || hitQuit || escDown) && !IsTransitioning())
                {
                    PlayMenuClick();
                    outQuitRequested = true;
                }
            }
            else if (titleSubMenu == TitleSubMenu::Settings)
            {
                HandleSettingsInput(upDown, downDown, leftDown, rightDown, enterDown, spaceDown, escDown, player, false);
            }
            else if (titleSubMenu == TitleSubMenu::KeyGuide)
            {
                int pw = 1100;
                int ph = 500;
                int py = 115;
                int btnY = py + ph - 52;
                bool hitBack = CheckButtonHit(cx - 140, btnY, cx + 140, btnY + 36);

                if (hitBack || escDown || enterDown || spaceDown)
                {
                    PlayMenuClick();
                    titleSubMenu = TitleSubMenu::Main;
                }
            }
        }
        break;

    case SceneType::Game:
        {
            // プレイヤーのマウス感度を常に同期
            player.SetMouseSensitivity(GetCalculatedMouseSensitivity());

            // ルール説明ポップアップ表示中の入力制御（ゲームプレイは完全停止）
            if (showRulePopup)
            {
                SetMouseDispFlag(TRUE);

                // Keep input hit-testing aligned with the exact button rectangle drawn below.
                const int btnX1 = cx - 110;
                const int btnY1 = cy - 360 + 406;
                const int btnX2 = cx + 110;
                const int btnY2 = btnY1 + 40;
                bool hitButton = CheckButtonHit(btnX1, btnY1, btnX2, btnY2);

                // Accept a click anywhere in the popup as an accessible mouse fallback.
                bool popupClicked = mouseClicked && mouseX >= cx - 420 && mouseX <= cx + 420
                    && mouseY >= cy - 270 && mouseY <= cy + 270;

                // Both the first-run help and manual help can be dismissed with Escape.
                bool closeRequested = hitButton || popupClicked || enterDown || escDown;

                if (closeRequested)
                {
                    PlayMenuClick();
                    CloseRulePopup();
                }

                // ポップアップ表示中は以降のゲームプレイ処理・ポーズ切り替えをすべて遮断
                return;
            }

            // Hキー: プレイ中のルール説明ポップアップ手動表示（第4段階仕様）
            if (hDown && !IsTransitioning() && !horrorManager.IsControlRestricted())
            {
                if (isPaused)
                {
                    SetPaused(false);
                }
                PlayMenuClick();
                OpenRulePopup(true);
                return;
            }

            // ESCキーでポーズメニュー切り替え
            if (escDown && !IsTransitioning() && !horrorManager.IsControlRestricted())
            {
                if (isPaused && pauseSubMenu == PauseSubMenu::Settings)
                {
                    PlayMenuClick();
                    pauseSubMenu = PauseSubMenu::Main;
                }
                else
                {
                    SetPaused(!isPaused);
                }
            }

            // ポーズ中のメニュー操作
            if (isPaused)
            {
                SetMouseDispFlag(TRUE);

                if (pauseSubMenu == PauseSubMenu::Settings)
                {
                    HandleSettingsInput(upDown, downDown, leftDown, rightDown, enterDown, spaceDown, escDown, player, true);
                    return;
                }

                // ポーズメインメニュー操作（5項目：0〜4）
                if (upDown)
                {
                    pauseMenuSelection = (pauseMenuSelection <= 0) ? 4 : pauseMenuSelection - 1;
                    PlayMenuAdjust();
                }
                if (downDown)
                {
                    pauseMenuSelection = (pauseMenuSelection >= 4) ? 0 : pauseMenuSelection + 1;
                    PlayMenuAdjust();
                }

                // ポーズメニューのボタン判定（5項目）
                bool hitResume    = CheckButtonHit(cx - 190, cy - 105, cx + 190, cy - 65);
                bool hitSettings  = CheckButtonHit(cx - 190, cy - 55,  cx + 190, cy - 15);
                bool hitRetry     = CheckButtonHit(cx - 190, cy - 5,   cx + 190, cy + 35);
                bool hitTitle     = CheckButtonHit(cx - 190, cy + 45,  cx + 190, cy + 85);
                bool hitQuit      = CheckButtonHit(cx - 190, cy + 95,  cx + 190, cy + 135);

                if (hitResume || (enterDown && pauseMenuSelection == 0))
                {
                    PlayMenuClick();
                    SetPaused(false);
                }
                else if (hitSettings || (enterDown && pauseMenuSelection == 1))
                {
                    PlayMenuClick();
                    pauseSubMenu = PauseSubMenu::Settings;
                    settingsSelection = 0;
                }
                else if (hitRetry || (enterDown && pauseMenuSelection == 2))
                {
                    PlayMenuClick();
                    SetPaused(false);
                    ResetGame(player, bicycle, newspaper, batteryManager, deliveryManager, nightEnv, weatherManager, horrorManager, currentDay);
                    StartTransition(SceneType::Game, 0.6f);
                }
                else if (hitTitle || (enterDown && pauseMenuSelection == 3))
                {
                    PlayMenuClick();
                    SetPaused(false);
                    ResetGame(player, bicycle, newspaper, batteryManager, deliveryManager, nightEnv, weatherManager, horrorManager, 1);
                    StartTransition(SceneType::Title, 0.8f);
                }
                else if (hitQuit || (enterDown && pauseMenuSelection == 4))
                {
                    PlayMenuClick();
                    outQuitRequested = true;
                }

                return;
            }

            // 全配達完了検出 -> クリアシーンへ移行
            if (deliveryManager.IsAllDeliveriesComplete() &&
                deliveryManager.GetState() == DeliveryState::AllComplete)
            {
                if (deliveryManager.GetAllCompleteTimer() < 240 && !IsTransitioning())
                {
                    SetMouseDispFlag(TRUE);
                    StartTransition(SceneType::Clear, 1.2f);
                }
            }

            // 最大恐怖ゲームオーバー演出終了検出 -> ゲームオーバーシーンへ移行
            if (horrorManager.IsGameOver() && !IsTransitioning())
            {
                SetMouseDispFlag(TRUE);
                StartTransition(SceneType::GameOver, 1.0f);
            }
        }
        break;

    case SceneType::Clear:
        {
            SetMouseDispFlag(TRUE);
            clearTimer += dt;

            if (currentDay < 7)
            {
                // Day 1〜6: ENTERキーまたは約3.5秒経過で翌日へスムーズに自動遷移
                bool hitNextNight = mouseClicked && (mouseY > 200);
                if ((hitNextNight || enterDown || spaceDown || clearTimer >= 3.5f) && !IsTransitioning())
                {
                    currentDay++;
                    ResetGame(player, bicycle, newspaper, batteryManager, deliveryManager, nightEnv, weatherManager, horrorManager, currentDay);
                    StartTransition(SceneType::Game, 1.0f);
                }
            }
            else
            {
                // 最終日: タイトルへ戻る / 終了
                bool hitTitle = CheckButtonHit(cx - 180, cy + 60, cx + 180, cy + 100);
                bool hitQuit = CheckButtonHit(cx - 180, cy + 115, cx + 180, cy + 155);

                if ((hitTitle || enterDown || spaceDown) && !IsTransitioning())
                {
                    ResetGame(player, bicycle, newspaper, batteryManager, deliveryManager, nightEnv, weatherManager, horrorManager, 1);
                    StartTransition(SceneType::Title, 1.0f);
                }

                if ((hitQuit || escDown) && !IsTransitioning())
                {
                    outQuitRequested = true;
                }
            }
        }
        break;

    case SceneType::GameOver:
        {
            SetMouseDispFlag(TRUE);
            gameOverTimer += dt;

            // ボタン判定（リトライ・タイトルへ戻る・終了）
            bool hitRetry = CheckButtonHit(cx - 180, cy + 25, cx + 180, cy + 65);
            bool hitTitle = CheckButtonHit(cx - 180, cy + 75, cx + 180, cy + 115);
            bool hitQuit = CheckButtonHit(cx - 180, cy + 125, cx + 180, cy + 165);

            if ((hitRetry || rDown) && !IsTransitioning())
            {
                ResetGame(player, bicycle, newspaper, batteryManager, deliveryManager, nightEnv, weatherManager, horrorManager, currentDay);
                StartTransition(SceneType::Game, 0.8f);
            }

            if ((hitTitle || enterDown || spaceDown) && !IsTransitioning())
            {
                ResetGame(player, bicycle, newspaper, batteryManager, deliveryManager, nightEnv, weatherManager, horrorManager, 1);
                StartTransition(SceneType::Title, 1.0f);
            }

            if ((hitQuit || escDown) && !IsTransitioning())
            {
                outQuitRequested = true;
            }
        }
        break;
    }
}

// シーン毎フレーム描画
void SceneManager::Draw(
    Player& player,
    Bicycle& bicycle,
    Newspaper& newspaper,
    Mailbox& mailbox,
    BatteryManager& batteryManager,
    DeliveryManager& deliveryManager,
    Minimap& minimap,
    NightEnvironment& nightEnv,
    WeatherManager& weatherManager,
    HorrorManager& horrorManager,
    bool mapDebugEnabled
)
{
    switch (currentScene)
    {
    case SceneType::Title:
        {
            // 3D背景の描画
            SetUseZBuffer3D(TRUE);
            SetWriteZBuffer3D(TRUE);
            nightEnv.ApplyLightingAndFog();
            DrawGround();
            MapDraw();
            batteryManager.Draw();
            nightEnv.DrawStreetLights();
            weatherManager.DrawParticles(bicycle, nightEnv);

            // 2D描画への影響を防ぐためフォグとZバッファを無効化
            SetFogEnable(FALSE);
            SetUseZBuffer3D(FALSE);
            SetWriteZBuffer3D(FALSE);

            // 2Dタイトル画面オーバーレイ
            DrawTitleScene(nightEnv);
        }
        break;

    case SceneType::Game:
        {
            // 3Dワールド描画
            SetUseZBuffer3D(TRUE);
            SetWriteZBuffer3D(TRUE);
            nightEnv.ApplyLightingAndFog();
            DrawGround();
            MapDraw();
            batteryManager.Draw();
            nightEnv.DrawStreetLights();
            bicycle.Draw();
            newspaper.Draw();
            mailbox.Draw(deliveryManager);
            batteryManager.Draw();
            weatherManager.DrawParticles(bicycle, nightEnv);
            horrorManager.Draw3D();

            if (mapDebugEnabled)
            {
                MapDrawDebug(deliveryManager.GetCurrentTargetId());
            }

            // 2D描画・UI・ミニマップへの影響を防ぐためフォグとZバッファを無効化
            SetFogEnable(FALSE);
            SetUseZBuffer3D(FALSE);
            SetWriteZBuffer3D(FALSE);

            // 恐怖視覚エフェクト（二重像・焦点ボケ・冷色脱色）を3D画面へ適用
            HorrorUI::Instance().DrawPostHorrorEffects();

            // 2D UI・HUD描画
            if (!horrorManager.IsControlRestricted())
            {
                bicycle.DrawUI();
                newspaper.DrawUI();
                mailbox.DrawUI(deliveryManager);
            }

            HorrorUI::Instance().DrawScreenVignette();
            HorrorUI::Instance().DrawClock();

            minimap.Draw(
                player,
                bicycle,
                deliveryManager,
                newspaper
            );

            // 3D空間内の目的地案内マーカーと距離UIを描画（画面内外対応）
            HorrorUI::Instance().DrawWorldDestinationMarker(
                player.GetPosition(),
                deliveryManager.GetCurrentTargetPosition(),
                deliveryManager.GetCurrentTargetId(),
                deliveryManager.IsAllDeliveriesComplete() || deliveryManager.IsFeedbackActive() || !deliveryManager.HasActiveTarget()
            );

            HorrorUI::Instance().DrawBicycleBattery(
                bicycle.GetBatteryPercent(),
                bicycle.IsHeadlightOn(),
                bicycle.IsRiding(),
                bicycle.IsDebugFastDrain(),
                batteryManager.GetSpareCount(),
                batteryManager.GetMaxSpareCount()
            );

            HorrorUI::Instance().DrawPaperCargo(
                newspaper.GetNewspaperCount(),
                newspaper.IsHolding()
            );

            HorrorUI::Instance().DrawInteractionPrompt();

            bool canReplace = batteryManager.CanReplaceBicycleBattery(player, bicycle);
            bool isAiming = bicycle.CanRide() || newspaper.CanTake() || mailbox.CanDeliver() || (batteryManager.GetAimedBatteryId() != -1) || canReplace;
            HorrorUI::Instance().DrawCrosshair(isAiming);

            // 操作キー案内HUD描画（画面下部）
            HorrorUI::Instance().DrawKeyGuide(
                bicycle.IsRiding(),
                newspaper.IsHolding(),
                isAiming,
                bicycle.IsHeadlightOn(),
                mapDebugEnabled,
                canReplace
            );

            nightEnv.DrawDebugHUD(&bicycle, weatherManager.GetWeatherName(weatherManager.GetCurrentWeather()));
            weatherManager.DrawDebugHUD();
            horrorManager.DrawUI();

            if (mapDebugEnabled)
            {
                HorrorUI::Instance().DrawRetroPanel(18, 304, 340, 436,
                    HorrorUI::COL_BORDER, HorrorUI::COL_PANEL_BG, 220);

                HorrorUI::Instance().DrawJitterString(26, 310, "[ F6: MAP & MISSION DEBUG ]",
                    GetColor(220, 180, 80), HorrorUI::Instance().GetFontSmall());

                char dbgTarget[64];
                int tid = deliveryManager.GetCurrentTargetId();
                if (tid >= 0)
                {
                    snprintf(dbgTarget, sizeof(dbgTarget), "TARGET: HOUSE %d", tid);
                }
                else
                {
                    snprintf(dbgTarget, sizeof(dbgTarget), "TARGET: ALL COMPLETED");
                }
                HorrorUI::Instance().DrawJitterString(26, 330, dbgTarget,
                    HorrorUI::COL_TEXT, HorrorUI::Instance().GetFontSmall());

                char dbgProg[64];
                snprintf(dbgProg, sizeof(dbgProg), "DELIVERY: %02d / %02d HOUSES",
                    deliveryManager.GetCompletedDeliveries(), deliveryManager.GetTotalDeliveries());
                HorrorUI::Instance().DrawJitterString(26, 350, dbgProg,
                    HorrorUI::COL_TEXT_DIM, HorrorUI::Instance().GetFontSmall());

                char dbgDay[64];
                snprintf(dbgDay, sizeof(dbgDay), "DAY %d / 7 (SEED: 0x%05X)",
                    currentDay, deliveryManager.GetCurrentDaySeed());
                HorrorUI::Instance().DrawJitterString(26, 370, dbgDay,
                    GetColor(140, 210, 240), HorrorUI::Instance().GetFontSmall());

                HorrorUI::Instance().DrawJitterString(26, 390, "COLLISION: HOUSES + FENCES + POLES [OK]",
                    GetColor(120, 230, 160), HorrorUI::Instance().GetFontSmall());

                HorrorUI::Instance().DrawJitterString(26, 410, "SAFETY VALIDATION: 100% CLEAR",
                    GetColor(100, 240, 180), HorrorUI::Instance().GetFontSmall());
            }

            // 夜開始告知バナーの描画
            HorrorUI::Instance().DrawDayBanner(
                deliveryManager.GetDayName(),
                deliveryManager.GetDaySubtitle(),
                deliveryManager.GetTotalDeliveries()
            );

            DrawScanlinesAndNoise(12);

            // ポーズメニュー表示
            if (isPaused)
            {
                DrawPauseMenu(deliveryManager);
            }

            // 【最重要仕様】ルール説明ポップアップ表示（ポーズメニューや右側HUDより最前面に表示）
            if (showRulePopup)
            {
                DrawRulePopup();
            }
        }
        break;

    case SceneType::Clear:
        {
            // 早朝・薄明かりの3D背景描画
            SetBackgroundColor(24, 34, 44);
            SetFogEnable(TRUE);
            SetFogColor(24, 34, 44);
            SetFogStartEnd(12.0f, 40.0f);
            SetLightEnable(FALSE);
            SetGlobalAmbientLight(GetColorF(0.20f, 0.22f, 0.28f, 1.0f));

            DrawGround();
            MapDraw();
            batteryManager.Draw();
            nightEnv.DrawStreetLights();
            bicycle.Draw();
            weatherManager.DrawParticles(bicycle, nightEnv);

            // 2D配達完了報告書画面オーバーレイ
            DrawClearScene(deliveryManager, bicycle);
        }
        break;

    case SceneType::GameOver:
        {
            // ゲームオーバー専用画面
            DrawGameOverScene(deliveryManager);
        }
        break;
    }

    // トランジション演出の最前面描画
    if (IsTransitioning())
    {
        DrawTransitionOverlay();
    }
}

// タイトルシーン描画
// 音量スライダーバー描画ヘルパー
void SceneManager::DrawVolumeSlider(int x, int y, int width, int height, int volume, const char* label, bool isSelected, bool& outDecreased, bool& outIncreased)
{
    (void)outDecreased;
    (void)outIncreased;
    (void)width;

    // 項目名ラベル
    unsigned int lblCol = isSelected ? GetColor(255, 210, 160) : GetColor(200, 210, 205);
    if (isSelected)
    {
        DrawString(x - 18, y + 4, ">", GetColor(240, 80, 60));
    }
    DrawString(x, y + 4, label, lblCol);

    int barX = x + 160;
    int barW = 240;
    int btnW = 28;

    // マイナスボタン [ < ]
    bool hLeft = CheckButtonHover(barX, y, barX + btnW, y + height);
    DrawBox(barX, y, barX + btnW, y + height, hLeft ? GetColor(55, 28, 28) : GetColor(32, 35, 38), TRUE);
    DrawBox(barX, y, barX + btnW, y + height, hLeft ? GetColor(200, 80, 70) : GetColor(65, 70, 75), FALSE);
    DrawString(barX + 9, y + 4, "<", hLeft ? GetColor(255, 255, 255) : GetColor(210, 220, 215));

    // バー本体背景
    int sliderX = barX + btnW + 8;
    DrawBox(sliderX, y + 2, sliderX + barW, y + height - 2, GetColor(16, 18, 20), TRUE);
    DrawBox(sliderX, y + 2, sliderX + barW, y + height - 2, isSelected ? GetColor(140, 60, 50) : GetColor(55, 60, 65), FALSE);

    // 10段階のホラーセグメントブロック
    int segCount = 10;
    int activeSegs = (volume + 5) / 10;
    int segW = (barW - 6) / segCount;
    for (int i = 0; i < segCount; ++i)
    {
        int sx = sliderX + 3 + i * segW;
        bool active = (i < activeSegs);
        unsigned int segCol = active
            ? (isSelected ? GetColor(240, 75, 55) : GetColor(190, 65, 50))
            : GetColor(28, 32, 36);
        DrawBox(sx, y + 5, sx + segW - 2, y + height - 5, segCol, TRUE);
    }

    // プラスボタン [ > ]
    int rightBtnX = sliderX + barW + 8;
    bool hRight = CheckButtonHover(rightBtnX, y, rightBtnX + btnW, y + height);
    DrawBox(rightBtnX, y, rightBtnX + btnW, y + height, hRight ? GetColor(55, 28, 28) : GetColor(32, 35, 38), TRUE);
    DrawBox(rightBtnX, y, rightBtnX + btnW, y + height, hRight ? GetColor(200, 80, 70) : GetColor(65, 70, 75), FALSE);
    DrawString(rightBtnX + 9, y + 4, ">", hRight ? GetColor(255, 255, 255) : GetColor(210, 220, 215));

    // パーセンテージ値
    char pctStr[16];
    snprintf(pctStr, sizeof(pctStr), "%3d%%", volume);
    DrawString(rightBtnX + btnW + 12, y + 4, pctStr, isSelected ? GetColor(255, 200, 120) : GetColor(210, 175, 130));
}

// タイトルシーン描画（メイン・設定・ガイドのディスパッチ）
void SceneManager::DrawTitleScene(const NightEnvironment& nightEnv)
{
    (void)nightEnv;

    int cx = 640;

    // 画面全体の半暗転
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 115);
    DrawBox(0, 0, 1280, 720, GetColor(4, 6, 8), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 周辺減光（ホラーヴィネット）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 90);
    DrawBox(0, 0, 1280, 55, GetColor(0, 0, 0), TRUE);
    DrawBox(0, 665, 1280, 720, GetColor(0, 0, 0), TRUE);
    DrawBox(0, 0, 55, 720, GetColor(0, 0, 0), TRUE);
    DrawBox(1225, 0, 1280, 720, GetColor(0, 0, 0), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 上部レトロVHS・カムコーダーインフォメーション
    bool recBlink = (((int)(titleTimer * 2.0f)) % 2) == 0;
    unsigned int recCol = recBlink ? GetColor(230, 45, 40) : GetColor(70, 20, 20);
    DrawString(36, 26, "REC ●", recCol);
    DrawString(125, 26, "TAPE: SP  0:01:24", GetColor(180, 195, 185));

    char timeBuf[64];
    snprintf(timeBuf, sizeof(timeBuf), "1998.08.14 FRI  AM 01:00:%02d", ((int)titleTimer) % 60);
    DrawString(960, 26, timeBuf, GetColor(180, 195, 185));

    // サブメニュー画面ごとの描画ディスパッチ
    if (titleSubMenu == TitleSubMenu::Main)
    {
        int titleY = 110;
        float jitterSine = std::sin(titleTimer * 8.0f);
        int jx = (std::rand() % 4 == 0) ? (int)(jitterSine * 2.0f) : 0;

        const char* mainTitle = "NEWSPAPER DELIVERY";
        int logoW = (fontTitleKanji != -1) ? GetDrawStringWidthToHandle(mainTitle, (int)strlen(mainTitle), fontTitleKanji) : 340;
        int logoX = cx - logoW / 2;

        if (fontTitleKanji != -1)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
            DrawStringToHandle(logoX - 2 + jx, titleY, mainTitle, GetColor(190, 35, 30), fontTitleKanji);
            DrawStringToHandle(logoX + 2 - jx, titleY, mainTitle, GetColor(35, 120, 140), fontTitleKanji);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawStringToHandle(logoX, titleY, mainTitle, GetColor(235, 240, 230), fontTitleKanji);
        }
        else
        {
            DrawString(cx - 150, titleY, mainTitle, GetColor(235, 240, 230));
        }

        // サブタイトル
        const char* subTitle = "- SHINYA HAITATSU -";
        if (fontTitleEnglish != -1)
        {
            int subW = GetDrawStringWidthToHandle(subTitle, (int)strlen(subTitle), fontTitleEnglish);
            DrawStringToHandle(cx - subW / 2, titleY + 70, subTitle, GetColor(185, 75, 65), fontTitleEnglish);
        }

        DrawTitleMainMenu(cx, jitterSine);
    }
    else if (titleSubMenu == TitleSubMenu::Settings)
    {
        DrawTitleSettings(cx);
    }
    else if (titleSubMenu == TitleSubMenu::KeyGuide)
    {
        DrawTitleKeyGuide(cx);
    }

    // CRT走査線・VHSアナログノイズ
    DrawScanlinesAndNoise(24);
}

// タイトルメインメニュー描画
void SceneManager::DrawTitleMainMenu(int cx, float jitterSine)
{
    (void)jitterSine;

    // キャッチコピー
    const char* tagline = "午前1時の静寂、そして路地裏に潜む恐怖。";
    if (fontSubText != -1)
    {
        int tagW = GetDrawStringWidthToHandle(tagline, (int)strlen(tagline), fontSubText);
        DrawStringToHandle(cx - tagW / 2, 215, tagline, GetColor(135, 145, 140), fontSubText);
    }

    // メニューパネル（古びた金属と錆・血痕を思わせるレトロホラー枠）
    int menuY = 250;
    int mw = 480;
    int mh = 246;
    HorrorUI::Instance().DrawRetroPanel(cx - mw / 2, menuY, cx + mw / 2, menuY + mh,
        GetColor(85, 45, 45), GetColor(15, 12, 14), 225);

    // ボタンの描画ヘルパー
    auto DrawMenuRow = [&](int y, const char* label, bool isSelected) {
        bool hovered = CheckButtonHover(cx - 200, y, cx + 200, y + 42);
        bool active = isSelected || hovered;

        unsigned int bgCol = active ? GetColor(45, 20, 22) : GetColor(20, 22, 25);
        unsigned int borderCol = active ? GetColor(185, 60, 50) : GetColor(50, 55, 60);
        unsigned int textCol = active ? GetColor(255, 125, 100) : GetColor(185, 195, 190);

        DrawBox(cx - 200, y, cx + 200, y + 42, bgCol, TRUE);
        DrawBox(cx - 200, y, cx + 200, y + 42, borderCol, FALSE);

        if (active)
        {
            DrawString(cx - 185, y + 13, ">", GetColor(240, 80, 60));
        }

        if (fontMenu != -1)
        {
            int lw = GetDrawStringWidthToHandle(label, (int)strlen(label), fontMenu);
            DrawStringToHandle(cx - lw / 2, y + 8, label, textCol, fontMenu);
        }
        else
        {
            int lw = GetDrawStringWidth(label, (int)strlen(label));
            DrawString(cx - lw / 2, y + 13, label, textCol);
        }
    };

    // 1. 配達開始
    DrawMenuRow(menuY + 20, "配達開始", (titleMenuSelection == 0));

    // 2. 音声・環境設定
    DrawMenuRow(menuY + 74, "音量・操作設定", (titleMenuSelection == 1));

    // 3. 操作・サバイバルガイド
    DrawMenuRow(menuY + 128, "操作説明・サバイバル規則", (titleMenuSelection == 2));

    // 4. ゲーム終了
    DrawMenuRow(menuY + 182, "ゲーム終了", (titleMenuSelection == 3));

    // 画面最下部の簡易操作インフォバー
    int helpY = 665;
    HorrorUI::Instance().DrawRetroPanel(26, helpY, 1254, helpY + 34,
        HorrorUI::COL_BORDER_DIM, HorrorUI::COL_BG, 190);
    HorrorUI::Instance().DrawJitterString(42, helpY + 9,
        "[ 操作方法 ]  WASD / マウス: 移動・視線  |  Enter / クリック: 決定  |  < >: 音量調整  |  Esc: 終了",
        GetColor(150, 165, 160), HorrorUI::Instance().GetFontKeyAction());
}

// 共通設定パネル描画（タイトル・ポーズ共用）
void SceneManager::DrawSettingsPanel(int cx, int setY, const char* headerTitle, bool inPause)
{
    int sw = 680;
    int sh = 450;

    // ポーズ時の暗転オーバーレイ
    if (inPause)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
        DrawBox(0, 0, 1280, 720, GetColor(5, 8, 10), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 設定ウィンドウパネル
    HorrorUI::Instance().DrawRetroPanel(cx - sw / 2, setY, cx + sw / 2, setY + sh,
        GetColor(90, 50, 50), GetColor(16, 14, 18), 245);

    // ヘッダータイトル
    if (fontReportTitle != -1)
    {
        int tw = GetDrawStringWidthToHandle(headerTitle, (int)strlen(headerTitle), fontReportTitle);
        DrawStringToHandle(cx - tw / 2, setY + 14, headerTitle, GetColor(240, 185, 140), fontReportTitle);
    }
    else
    {
        int tw = GetDrawStringWidth(headerTitle, (int)strlen(headerTitle));
        DrawString(cx - tw / 2, setY + 16, headerTitle, GetColor(240, 185, 140));
    }

    DrawLine(cx - sw / 2 + 25, setY + 48, cx + sw / 2 - 25, setY + 48, GetColor(75, 42, 42));

    // 4本のスライダーバー描画
    bool dummyDec = false, dummyInc = false;
    DrawVolumeSlider(cx - 280, setY + 65,  560, 28, masterVolume,           "マスター音量",     (settingsSelection == 0), dummyDec, dummyInc);
    DrawVolumeSlider(cx - 280, setY + 125, 560, 28, bgmVolume,              "BGM・環境音",      (settingsSelection == 1), dummyDec, dummyInc);
    DrawVolumeSlider(cx - 280, setY + 185, 560, 28, sfxVolume,              "効果音 (SE)",       (settingsSelection == 2), dummyDec, dummyInc);
    DrawVolumeSlider(cx - 280, setY + 245, 560, 28, mouseSensitivityLevel, "マウス感度",       (settingsSelection == 3), dummyDec, dummyInc);

    // サウンドテストボタン
    int testBtnY = setY + 315;
    bool hTest = CheckButtonHover(cx - 160, testBtnY, cx + 160, testBtnY + 36);
    bool testActive = (settingsSelection == 4) || hTest;
    bool pulseFlashing = (audioTestFeedbackTimer > 0.0f);

    unsigned int testBg = pulseFlashing ? GetColor(90, 25, 25) : (testActive ? GetColor(48, 22, 24) : GetColor(24, 26, 30));
    unsigned int testBorder = testActive ? GetColor(200, 75, 65) : GetColor(60, 65, 70);
    unsigned int testTxt = pulseFlashing ? GetColor(255, 255, 200) : (testActive ? GetColor(255, 135, 110) : GetColor(190, 200, 195));

    DrawBox(cx - 160, testBtnY, cx + 160, testBtnY + 36, testBg, TRUE);
    DrawBox(cx - 160, testBtnY, cx + 160, testBtnY + 36, testBorder, FALSE);
    const char* testLabel = pulseFlashing ? "[ ! 音声再生中 ! ]" : "テスト音声を再生";
    int tlw = GetDrawStringWidth(testLabel, (int)strlen(testLabel));
    DrawString(cx - tlw / 2, testBtnY + 10, testLabel, testTxt);

    // 戻るボタン
    int backBtnY = setY + 365;
    bool hBack = CheckButtonHover(cx - 160, backBtnY, cx + 160, backBtnY + 36);
    bool backActive = (settingsSelection == 5) || hBack;

    DrawBox(cx - 160, backBtnY, cx + 160, backBtnY + 36, backActive ? GetColor(50, 22, 24) : GetColor(24, 26, 30), TRUE);
    DrawBox(cx - 160, backBtnY, cx + 160, backBtnY + 36, backActive ? GetColor(200, 75, 65) : GetColor(60, 65, 70), FALSE);
    const char* backLabel = inPause ? "ポーズメニューへ戻る [ESC]" : "タイトルへ戻る [ESC]";
    int blw = GetDrawStringWidth(backLabel, (int)strlen(backLabel));
    DrawString(cx - blw / 2, backBtnY + 10, backLabel, backActive ? GetColor(255, 135, 110) : GetColor(190, 200, 195));

    // 操作ガイド注釈
    const char* guide = "矢印キー < > またはドラッグで調整、Enterでテスト、Escで戻る";
    int gw = GetDrawStringWidth(guide, (int)strlen(guide));
    DrawString(cx - gw / 2, setY + sh - 24, guide, GetColor(130, 140, 135));
}

void SceneManager::DrawTitleSettings(int cx)
{
    DrawSettingsPanel(cx, 130, "音量・操作設定", false);
}

// 操作説明・サバイバル規則サブメニュー画面描画
void SceneManager::DrawTitleKeyGuide(int cx)
{
    int pw = 1100;
    int ph = 500;
    int py = 115;

    // レトロマニュアルパネル（幅1100px：cx-550〜cx+550）
    HorrorUI::Instance().DrawRetroPanel(cx - pw / 2, py, cx + pw / 2, py + ph,
        GetColor(90, 50, 50), GetColor(14, 12, 16), 242);

    // ヘッダータイトル
    const char* guideTitle = "配達員必携ハンドブック — 午前1時のサバイバル規則";
    if (fontReportTitle != -1)
    {
        int tw = GetDrawStringWidthToHandle(guideTitle, (int)strlen(guideTitle), fontReportTitle);
        DrawStringToHandle(cx - tw / 2, py + 14, guideTitle, GetColor(240, 190, 130), fontReportTitle);
    }
    else
    {
        int tw = GetDrawStringWidth(guideTitle, (int)strlen(guideTitle));
        DrawString(cx - tw / 2, py + 16, guideTitle, GetColor(240, 190, 130));
    }

    DrawLine(cx - pw / 2 + 25, py + 48, cx + pw / 2 - 25, py + 48, GetColor(75, 42, 42));
    DrawLine(cx - 10, py + 58, cx - 10, py + ph - 65, GetColor(50, 35, 38));

    // 第1列：キーバインド
    int col1X = cx - pw / 2 + 35;
    DrawString(col1X, py + 58, "[ 1. 基本操作・キー設定 ]", GetColor(240, 200, 140));

    auto DrawKeyRow = [&](int y, const char* key, const char* desc) {
        DrawBox(col1X, y, col1X + 130, y + 25, GetColor(30, 34, 38), TRUE);
        DrawBox(col1X, y, col1X + 130, y + 25, GetColor(70, 75, 80), FALSE);
        DrawString(col1X + 10, y + 5, key, GetColor(255, 230, 180));
        DrawString(col1X + 145, y + 5, desc, GetColor(190, 200, 195));
    };

    DrawKeyRow(py + 90,  "W / A / S / D", "移動 / 自転車の操縦");
    DrawKeyRow(py + 122, "マウス",         "視点移動 / ライトを向ける");
    DrawKeyRow(py + 154, "左クリック",     "新聞を配達 / 新聞・アイテムを取る");
    DrawKeyRow(py + 186, "E キー",         "自転車の乗り降り");
    DrawKeyRow(py + 218, "F キー",         "自転車ライトの点灯・消灯");
    DrawKeyRow(py + 250, "R キー",         "予備バッテリーを交換");
    DrawKeyRow(py + 282, "スペース",       "ブレーキ / ベル");
    DrawKeyRow(py + 314, "Shift",          "ダッシュ (スタミナ消費)");
    DrawKeyRow(py + 346, "Esc キー",       "ポーズメニュー");
    DrawKeyRow(py + 378, "< > 矢印",       "音量・感度の調整");
    DrawKeyRow(py + 410, "H / Tab",        "遊び方・操作ガイド切替");

    // 第2列：深夜配達サバイバル指示（枠内に完全に収まる文字幅設計）
    int col2X = cx + 15;
    DrawString(col2X, py + 58, "[ 2. 深夜配達サバイバル指示 ]", GetColor(240, 110, 95));

    auto DrawRule = [&](int y, const char* title, const char* desc, const char* desc2) {
        DrawString(col2X, y, title, GetColor(230, 90, 75));
        DrawString(col2X + 10, y + 18, desc, GetColor(175, 185, 180));
        if (desc2) DrawString(col2X + 10, y + 34, desc2, GetColor(140, 155, 150));
    };

    DrawRule(py + 90,  "1. 街灯の安全な光",
        "・街灯は怪異を遠ざけます (安全範囲 約6.5m)。",
        "・街灯が激しく明滅したら、すぐに離れてください。");

    DrawRule(py + 152, "2. 自転車ライトとバッテリー",
        "・バッテリー残量がある状態の乗車中は襲撃を防げます。",
        "・バッテリーが0%になると無防備になり極めて危険です。");

    DrawRule(py + 214, "3. 恐怖度と怪異",
        "・暗闇にいると恐怖度が上昇し、100%で襲撃されます。",
        "・街灯などの安全な光の方を向いて恐怖度を下げてください。");

    DrawRule(py + 276, "4. 中盤の追跡者 (配達4軒目〜)",
        "・配達が半分進むと、追跡型モンスターが出現します。",
        "・光の中に留まってください。追跡中はプレイヤーを直視します。");

    DrawRule(py + 338, "5. 予備バッテリー (最大2個)",
        "・予備バッテリーは最大2個まで持ち歩けます。",
        "・自転車の近くで [R] を押すとバッテリーが100%回復します。");

    // 戻るボタン
    int btnY = py + ph - 52;
    bool hBack = CheckButtonHover(cx - 140, btnY, cx + 140, btnY + 36);

    DrawBox(cx - 140, btnY, cx + 140, btnY + 36, hBack ? GetColor(50, 22, 24) : GetColor(25, 28, 30), TRUE);
    DrawBox(cx - 140, btnY, cx + 140, btnY + 36, hBack ? GetColor(200, 75, 65) : GetColor(65, 70, 75), FALSE);
    const char* backTxt = "メニューへ戻る [ESC]";
    int bw = GetDrawStringWidth(backTxt, (int)strlen(backTxt));
    DrawString(cx - bw / 2, btnY + 10, backTxt, hBack ? GetColor(255, 135, 110) : GetColor(190, 200, 195));
}

void SceneManager::DrawPauseMenu(const DeliveryManager& deliveryManager)
{
    (void)deliveryManager;

    int cx = 640;
    int cy = 360;

    // ポーズ設定画面が開いている場合は設定パネルを描画
    if (pauseSubMenu == PauseSubMenu::Settings)
    {
        DrawSettingsPanel(cx, 130, "ポーズ設定 - 音量・マウス感度", true);
        return;
    }

    // 暗転オーバーレイ
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
    DrawBox(0, 0, 1280, 720, GetColor(5, 8, 10), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ダイアログパネル（5項目対応）
    int pw = 480;
    int ph = 390;
    HorrorUI::Instance().DrawRetroPanel(cx - pw / 2, cy - ph / 2, cx + pw / 2, cy + ph / 2,
        GetColor(75, 45, 45), GetColor(16, 12, 14), 240);

    // タイトル（現在の日数を明記）
    char pauseTitle[64];
    snprintf(pauseTitle, sizeof(pauseTitle), "一時停止 - DAY %d / 7", currentDay);
    if (fontReportTitle != -1)
    {
        int tw = GetDrawStringWidthToHandle(pauseTitle, (int)strlen(pauseTitle), fontReportTitle);
        DrawStringToHandle(cx - tw / 2, cy - ph / 2 + 18, pauseTitle, GetColor(215, 175, 170), fontReportTitle);
    }
    else
    {
        int tw = GetDrawStringWidth(pauseTitle, (int)strlen(pauseTitle));
        DrawString(cx - tw / 2, cy - ph / 2 + 20, pauseTitle, GetColor(210, 230, 220));
    }

    DrawLine(cx - pw / 2 + 25, cy - ph / 2 + 55, cx + pw / 2 - 25, cy - ph / 2 + 55, GetColor(60, 35, 35));

    // ボタンの描画（マウスホバー＆キー選択両対応）5項目
    bool sel0 = (pauseMenuSelection == 0);
    bool sel1 = (pauseMenuSelection == 1);
    bool sel2 = (pauseMenuSelection == 2);
    bool sel3 = (pauseMenuSelection == 3);
    bool sel4 = (pauseMenuSelection == 4);

    char retryLabel[32];
    snprintf(retryLabel, sizeof(retryLabel), "Day %d をリトライ", currentDay);

    DrawButton(cx - 190, cy - 105, cx + 190, cy - 65, "ゲームを再開",
        sel0 ? GetColor(255, 255, 240) : GetColor(185, 195, 190),
        GetColor(255, 255, 240), fontMenu);

    DrawButton(cx - 190, cy - 55, cx + 190, cy - 15, "音量・マウス設定",
        sel1 ? GetColor(255, 255, 240) : GetColor(185, 195, 190),
        GetColor(255, 255, 240), fontMenu);

    DrawButton(cx - 190, cy - 5, cx + 190, cy + 35, retryLabel,
        sel2 ? GetColor(255, 255, 240) : GetColor(185, 195, 190),
        GetColor(255, 255, 240), fontMenu);

    DrawButton(cx - 190, cy + 45, cx + 190, cy + 85, "タイトルへ戻る",
        sel3 ? GetColor(255, 255, 240) : GetColor(185, 195, 190),
        GetColor(255, 255, 240), fontMenu);

    DrawButton(cx - 190, cy + 95, cx + 190, cy + 135, "ゲーム終了",
        sel4 ? GetColor(255, 255, 240) : GetColor(185, 195, 190),
        GetColor(255, 255, 240), fontMenu);

    // キーボード選択時のカーソル表示
    int arrowY[5] = { cy - 90, cy - 40, cy + 10, cy + 60, cy + 110 };
    DrawString(cx - 208, arrowY[pauseMenuSelection], ">", GetColor(220, 240, 230));

    // ガイドテキスト
    const char* pauseGuide = "[ 操作方法 ] マウスまたは矢印キー / Enter で選択";
    int gw = GetDrawStringWidthToHandle(pauseGuide, (int)strlen(pauseGuide), HorrorUI::Instance().GetFontKeyGuide());
    HorrorUI::Instance().DrawJitterString(cx - gw / 2, cy + ph / 2 - 28, pauseGuide,
        GetColor(120, 135, 130), HorrorUI::Instance().GetFontKeyGuide());

    DrawScanlinesAndNoise(22);
}

// クリアシーン描画
void SceneManager::DrawClearScene(const DeliveryManager& deliveryManager, const Bicycle& bicycle)
{
    (void)bicycle;
    int cx = 640;
    int cy = 360;

    // 完全な黒画面背景
    DrawBox(0, 0, 1280, 720, GetColor(0, 0, 0), TRUE);

    // 上部レトロVHSタイムスタンプ
    char timeBuf[64];
    snprintf(timeBuf, sizeof(timeBuf), "1998.08.%02d  AM 01:00:00", 14 + currentDay - 1);
    DrawString(60, 40, "REC ●", GetColor(180, 40, 40));
    DrawString(145, 40, "TAPE: SP  MIDNIGHT SURVEILLANCE", GetColor(130, 145, 140));
    DrawString(980, 40, timeBuf, GetColor(130, 145, 140));

    // 中央テキストUI
    if (currentDay < 7)
    {
        // 1〜6日目：日ごとのクリア黒画面テキストUI
        char titleStr[64];
        snprintf(titleStr, sizeof(titleStr), "DAY %d クリア", currentDay);

        if (fontReportTitle != -1)
        {
            int tw = GetDrawStringWidthToHandle(titleStr, (int)strlen(titleStr), fontReportTitle);
            DrawStringToHandle(cx - tw / 2, cy - 80, titleStr, GetColor(235, 245, 240), fontReportTitle);
        }
        else
        {
            int tw = GetDrawStringWidth(titleStr, (int)strlen(titleStr));
            DrawString(cx - tw / 2, cy - 70, titleStr, GetColor(235, 245, 240));
        }

        char delivStr[64];
        snprintf(delivStr, sizeof(delivStr), "全 %d 軒への配達完了", deliveryManager.GetTotalDeliveries());
        int dw = GetDrawStringWidth(delivStr, (int)strlen(delivStr));
        DrawString(cx - dw / 2, cy - 15, delivStr, GetColor(215, 150, 90));

        const char* survStr = "夜を生き延びた。";
        int sw = GetDrawStringWidth(survStr, (int)strlen(survStr));
        DrawString(cx - sw / 2, cy + 20, survStr, GetColor(140, 195, 165));

        char nextStr[64];
        snprintf(nextStr, sizeof(nextStr), "翌日（Day %d）の配達ルートを準備中...", currentDay + 1);
        int nw = GetDrawStringWidth(nextStr, (int)strlen(nextStr));
        DrawString(cx - nw / 2, cy + 60, nextStr, GetColor(165, 180, 175));

        // 点滅プロンプト案内
        bool blink = (((int)(clearTimer * 2.5f)) % 2) == 0;
        if (blink)
        {
            const char* prompt = "[ ENTERキーで進む (または自動進行) ]";
            int pw = GetDrawStringWidth(prompt, (int)strlen(prompt));
            DrawString(cx - pw / 2, cy + 125, prompt, GetColor(180, 210, 200));
        }
    }
    else
    {
        // 最終7日目クリア：全7夜生存達成テキストUI
        const char* titleStr = "FINAL DAY クリア";
        if (fontReportTitle != -1)
        {
            int tw = GetDrawStringWidthToHandle(titleStr, (int)strlen(titleStr), fontReportTitle);
            DrawStringToHandle(cx - tw / 2, cy - 100, titleStr, GetColor(245, 220, 150), fontReportTitle);
        }
        else
        {
            int tw = GetDrawStringWidth(titleStr, (int)strlen(titleStr));
            DrawString(cx - tw / 2, cy - 90, titleStr, GetColor(245, 220, 150));
        }

        const char* sub1 = "全7夜 生還達成";
        int s1w = GetDrawStringWidth(sub1, (int)strlen(sub1));
        DrawString(cx - s1w / 2, cy - 35, sub1, GetColor(140, 235, 185));

        const char* sub2 = "夜が明けた。あなたは悪夢の夜を生き延びた。";
        int s2w = GetDrawStringWidth(sub2, (int)strlen(sub2));
        DrawString(cx - s2w / 2, cy + 5, sub2, GetColor(190, 205, 200));

        // ボタン
        DrawButton(cx - 180, cy + 60, cx + 180, cy + 100, "タイトルへ戻る [ENTER]",
            GetColor(35, 75, 65), GetColor(60, 130, 110));
        DrawButton(cx - 180, cy + 115, cx + 180, cy + 155, "ゲーム終了 [ESC]",
            GetColor(50, 30, 35), GetColor(120, 50, 60));
    }

    // 微細なCRT走査線
    DrawScanlinesAndNoise(15);
}

void SceneManager::DrawGameOverScene(const DeliveryManager& deliveryManager)
{
    (void)deliveryManager;

    int cx = 640;
    int cy = 360;

    // 暗黒背景（純黒）
    DrawBox(0, 0, 1280, 720, GetColor(0, 0, 0), TRUE);

    // アナログ砂嵐ノイズ（テレビの砂嵐演出）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 35);
    for (int i = 0; i < 220; ++i)
    {
        int rx = std::rand() % 1280;
        int ry = std::rand() % 720;
        int rw = 2 + std::rand() % 5;
        unsigned int ncol = (std::rand() % 2 == 0) ? GetColor(190, 190, 190) : GetColor(40, 40, 40);
        DrawBox(rx, ry, rx + rw, ry + 1, ncol, TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 中央ダイアログパネル
    int pw = 580;
    int ph = 350;
    HorrorUI::Instance().DrawRetroPanel(cx - pw / 2, cy - ph / 2, cx + pw / 2, cy + ph / 2,
        GetColor(90, 35, 35), GetColor(15, 8, 10), 235);

    // GAME OVER タイトル（赤色・激しいノイズジッター）
    int jx = (std::rand() % 5) - 2;
    int jy = (std::rand() % 3) - 1;

    char overTitle[64];
    snprintf(overTitle, sizeof(overTitle), "GAME OVER - DAY %d", currentDay);
    if (fontTitleKanji != -1)
    {
        int ow = GetDrawStringWidthToHandle(overTitle, (int)strlen(overTitle), fontTitleKanji);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 160);
        DrawStringToHandle(cx - ow / 2 + jx + 2, cy - 145 + jy, overTitle, GetColor(120, 10, 10), fontTitleKanji);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawStringToHandle(cx - ow / 2 + jx, cy - 145 + jy, overTitle, GetColor(220, 30, 25), fontTitleKanji);
    }
    else
    {
        DrawString(cx - 65, cy - 135, overTitle, GetColor(210, 35, 30));
    }

    const char* subOver = "CONSUMED BY DARKNESS";
    if (fontTitleEnglish != -1)
    {
        int sw = GetDrawStringWidthToHandle(subOver, (int)strlen(subOver), fontTitleEnglish);
        DrawStringToHandle(cx - sw / 2, cy - 80, subOver, GetColor(165, 45, 45), fontTitleEnglish);
    }

    // ホラーテキスト（フォントを適用して中央揃え）
    char line1[64];
    snprintf(line1, sizeof(line1), "第 %d 夜を生き延びることはできなかった...", currentDay);
    const char* line2 = "配達員は深夜の街から消息を絶った。";
    if (fontSubText != -1)
    {
        int l1w = GetDrawStringWidthToHandle(line1, (int)strlen(line1), fontSubText);
        int l2w = GetDrawStringWidthToHandle(line2, (int)strlen(line2), fontSubText);
        DrawStringToHandle(cx - l1w / 2, cy - 42, line1, GetColor(185, 120, 120), fontSubText);
        DrawStringToHandle(cx - l2w / 2, cy - 18, line2, GetColor(150, 115, 115), fontSubText);
    }
    else
    {
        int l1w = GetDrawStringWidth(line1, (int)strlen(line1));
        int l2w = GetDrawStringWidth(line2, (int)strlen(line2));
        DrawString(cx - l1w / 2, cy - 40, line1, GetColor(190, 110, 110));
        DrawString(cx - l2w / 2, cy - 15, line2, GetColor(140, 110, 110));
    }

    // メニューボタン（マウスホバー＆キー選択両対応）
    char retryLabel[32];
    snprintf(retryLabel, sizeof(retryLabel), "Day %d をリトライ", currentDay);
    DrawButton(cx - 180, cy + 25, cx + 180, cy + 65, retryLabel,
        GetColor(230, 210, 150), GetColor(255, 245, 190), fontMenu);

    DrawButton(cx - 180, cy + 75, cx + 180, cy + 115, "タイトルへ戻る",
        GetColor(180, 170, 165), GetColor(240, 235, 230), fontMenu);

    DrawButton(cx - 180, cy + 125, cx + 180, cy + 165, "ゲーム終了",
        GetColor(140, 130, 125), GetColor(220, 215, 210), fontMenu);

    DrawScanlinesAndNoise(26);
}

// CRT走査線およびVHSノイズ共通描画
void SceneManager::DrawScanlinesAndNoise(int alpha)
{
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    unsigned int sCol = GetColor(0, 0, 0);
    for (int y = 0; y < 720; y += 4)
    {
        DrawBox(0, y, 1280, y + 2, sCol, TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// トランジション演出描画
void SceneManager::DrawTransitionOverlay()
{
    int alpha = static_cast<int>(transitionAlpha * 255.0f);
    if (alpha > 255) alpha = 255;
    if (alpha < 0) alpha = 0;

    // 全面黒フェード
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    DrawBox(0, 0, 1280, 720, GetColor(2, 3, 4), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // トランジションピーク付近のCRT電源オン/オフ光条およびVHSトラッキング乱れ
    if (transitionAlpha > 0.4f)
    {
        // ランダム水平スライス歪み
        int sliceCount = 3 + (std::rand() % 4);
        for (int i = 0; i < sliceCount; ++i)
        {
            int sy = std::rand() % 700;
            int sh = 2 + std::rand() % 8;
            int sAlpha = (int)(transitionAlpha * 70.0f);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, sAlpha);
            DrawBox(0, sy, 1280, sy + sh, GetColor(180, 200, 190), TRUE);
        }

        // ピーク付近の水平ビームライン
        if (transitionAlpha > 0.88f)
        {
            int beamH = (int)((1.0f - transitionAlpha) * 40.0f) + 2;
            int beamY = 360 - beamH / 2;
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 160);
            DrawBox(0, beamY, 1280, beamY + beamH, GetColor(220, 240, 230), TRUE);
        }

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

// ルール説明ポップアップの描画処理（第2段階仕様）
void SceneManager::DrawRulePopup()
{
    int cx = 640;
    int cy = 360;

    // 1. 背景のゲーム画面を少し暗くするオーバーレイ
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 205);
    DrawBox(0, 0, 1280, 720, GetColor(4, 7, 9), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. 画面中央の暗い半透明背景パネル（適切な余白と既存ホラーデザインを両立）
    int pw = 840;
    int ph = 540;
    int x1 = cx - pw / 2;
    int y1 = cy - ph / 2;
    int x2 = cx + pw / 2;
    int y2 = cy + ph / 2;

    HorrorUI::Instance().DrawRetroPanel(x1, y1, x2, y2,
        GetColor(70, 88, 80), GetColor(12, 16, 15), 245);

    // 3. タイトル表示
    const char* titleText = "新聞配達 — 遊び方";
    if (fontPopupTitle != -1)
    {
        int tw = GetDrawStringWidthToHandle(titleText, (int)strlen(titleText), fontPopupTitle);
        DrawStringToHandle(cx - tw / 2, y1 + 18, titleText, GetColor(235, 245, 240), fontPopupTitle);
    }
    else
    {
        int tw = GetDrawStringWidth(titleText, (int)strlen(titleText));
        DrawString(cx - tw / 2, y1 + 18, titleText, GetColor(235, 245, 240));
    }

    const char* subTitleEn = "午前1時の配達業務マニュアル";
    if (fontPopupSub != -1)
    {
        int stw = GetDrawStringWidthToHandle(subTitleEn, (int)strlen(subTitleEn), fontPopupSub);
        DrawStringToHandle(cx - stw / 2, y1 + 50, subTitleEn, GetColor(140, 160, 150), fontPopupSub);
    }

    // 装飾区切り線
    DrawLine(x1 + 35, y1 + 72, x2 - 35, y1 + 72, GetColor(50, 68, 62));

    // 4. 導入文
    const char* introText = "夜が明ける前に、指定された家へ新聞を届けてください。";
    if (fontPopupBody != -1)
    {
        int iw = GetDrawStringWidthToHandle(introText, (int)strlen(introText), fontPopupBody);
        DrawStringToHandle(cx - iw / 2, y1 + 84, introText, GetColor(190, 220, 210), fontPopupBody);
    }
    else
    {
        int iw = GetDrawStringWidth(introText, (int)strlen(introText));
        DrawString(cx - iw / 2, y1 + 84, introText, GetColor(190, 220, 210));
    }

    // 5. 説明セクション共通描画ラムダ
    auto DrawSection = [&](int secY, int secH, const char* secTitle, const char* bodyText, const char* hintText, unsigned int titleCol) {
        int sx1 = x1 + 35;
        int sx2 = x2 - 35;
        int sy1 = secY;
        int sy2 = secY + secH;

        // セクション枠背景
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
        DrawBox(sx1, sy1, sx2, sy2, GetColor(18, 24, 22), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(sx1, sy1, sx2, sy2, GetColor(45, 60, 55), FALSE);

        // 見出し（明確な文字サイズ差）
        if (fontPopupSection != -1)
        {
            DrawStringToHandle(sx1 + 18, sy1 + 7, secTitle, titleCol, fontPopupSection);
        }
        else
        {
            DrawString(sx1 + 18, sy1 + 7, secTitle, titleCol);
        }

        // 本文（読みやすい白色／薄灰）
        if (fontPopupBody != -1)
        {
            DrawStringToHandle(sx1 + 22, sy1 + 32, bodyText, GetColor(215, 225, 220), fontPopupBody);
        }
        else
        {
            DrawString(sx1 + 22, sy1 + 32, bodyText, GetColor(215, 225, 220));
        }

        // 補足・操作方法
        if (hintText && fontPopupSub != -1)
        {
            DrawStringToHandle(sx1 + 22, sy1 + 54, hintText, GetColor(150, 175, 168), fontPopupSub);
        }
    };

    // 説明1：新聞の配達
    DrawSection(
        y1 + 112, 78,
        "1. 新聞を配達する",
        "自転車のカゴから新聞を取り、指定されたポストへ順番に届けてください。",
        "カゴやポストを見て左クリックで新聞を取る／配達します。",
        GetColor(245, 230, 175)
    );

    // 説明2：暗闇と恐怖
    DrawSection(
        y1 + 198, 78,
        "2. 暗闇を避ける",
        "暗闇にいると恐怖度が上昇します。街灯や自転車のライトを利用して、危険から身を守ってください。",
        "安全な光の中に留まると恐怖度が低下します。",
        GetColor(245, 175, 160)
    );

    // 説明3：バッテリー
    DrawSection(
        y1 + 284, 78,
        "3. バッテリーを管理する",
        "自転車のライトはバッテリーを消費します。街中で予備バッテリーを探し、必要に応じて交換してください。",
        "落ちているバッテリーを見て左クリックで拾い、自転車の近くでRキーを押して交換します。",
        GetColor(170, 230, 205)
    );

    // 補足：ゲームオーバー条件
    const char* warnText = "※ 恐怖度が限界に達するとゲームオーバーになります。";
    if (fontPopupBody != -1)
    {
        int ww = GetDrawStringWidthToHandle(warnText, (int)strlen(warnText), fontPopupBody);
        DrawStringToHandle(cx - ww / 2, y1 + 372, warnText, GetColor(235, 105, 95), fontPopupBody);
    }
    else
    {
        int ww = GetDrawStringWidth(warnText, (int)strlen(warnText));
        DrawString(cx - ww / 2, y1 + 372, warnText, GetColor(235, 105, 95));
    }

    // 6. アクションボタン
    const char* btnLabel = (!rulePopupFromManual) ? "配達を開始" : "ゲームに戻る";
    int btnW = 220;
    int btnH = 40;
    int btnX1 = cx - btnW / 2;
    int btnY1 = y1 + 406;
    int btnX2 = cx + btnW / 2;
    int btnY2 = btnY1 + btnH;

    bool isHover = (mouseX >= btnX1 && mouseX <= btnX2 && mouseY >= btnY1 && mouseY <= btnY2);
    unsigned int btnBorder = isHover ? GetColor(180, 80, 70) : GetColor(70, 95, 85);
    unsigned int btnBg = isHover ? GetColor(32, 18, 20) : GetColor(18, 26, 22);

    HorrorUI::Instance().DrawRetroPanel(btnX1, btnY1, btnX2, btnY2, btnBorder, btnBg, 240);

    unsigned int btnTextCol = isHover ? GetColor(255, 255, 240) : GetColor(210, 230, 220);
    if (fontPopupSection != -1)
    {
        int bw = GetDrawStringWidthToHandle(btnLabel, (int)strlen(btnLabel), fontPopupSection);
        DrawStringToHandle(cx - bw / 2, btnY1 + 9, btnLabel, btnTextCol, fontPopupSection);
    }
    else
    {
        int bw = GetDrawStringWidth(btnLabel, (int)strlen(btnLabel));
        DrawString(cx - bw / 2, btnY1 + 11, btnLabel, btnTextCol);
    }

    if (isHover)
    {
        DrawString(btnX1 + 12, btnY1 + 11, ">", GetColor(230, 70, 60));
        DrawString(btnX2 - 20, btnY1 + 11, "<", GetColor(230, 70, 60));
    }

    // 7. ボタン下ガイド案内
    const char* keyGuideText = (!rulePopupFromManual)
        ? "Enterキーまたはクリックで開始"
        : "Enterキー、Escキー、またはクリックで戻る";

    if (fontPopupSub != -1)
    {
        int kw = GetDrawStringWidthToHandle(keyGuideText, (int)strlen(keyGuideText), fontPopupSub);
        DrawStringToHandle(cx - kw / 2, y1 + 458, keyGuideText, GetColor(150, 175, 165), fontPopupSub);
    }
    else
    {
        int kw = GetDrawStringWidth(keyGuideText, (int)strlen(keyGuideText));
        DrawString(cx - kw / 2, y1 + 458, keyGuideText, GetColor(150, 175, 165));
    }

    // アナログCRT走査線エフェクト
    DrawScanlinesAndNoise(18);
}
