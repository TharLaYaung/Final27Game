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
#include <algorithm>
#include <cstdint>

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

    // 音声形式の情報（非圧縮16ビット、毎秒44.1キロサンプル、モノラル）
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

    // 音声データ部分
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
    , loadingTimer(0.0f)
    , loadingDuration(2.1f)
    , loadingProgress(0.0f)
    , pendingLoadingDay(1)
    , loadingAppliedReset(false)
    , loadingIsRetry(false)
    , loadingAnomalyAudioTriggered(false)
    , loadingCompleteAudioTriggered(false)
    , lastMenuHoverSelection(0)
    , lastPauseHoverSelection(0)
    , titleLampAudioCooldown(0.8f)
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
    , fontMinchoSub(-1)
    , fontCamcorder(-1)
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
    , texRadialVignette(-1)
    , texFilmGrain(-1)
    , texFogLayer(-1)
    , texScreenSnap(-1)
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
    loadingTimer = 0.0f;
    loadingDuration = 2.1f;
    loadingProgress = 0.0f;
    pendingLoadingDay = 1;
    loadingAppliedReset = false;
    loadingIsRetry = false;
    loadingAnomalyAudioTriggered = false;
    loadingCompleteAudioTriggered = false;
    lastMenuHoverSelection = 0;
    lastPauseHoverSelection = 0;
    titleLampAudioCooldown = 0.8f;

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

    // 大見出しホラーロゴ（Chiller優先：英語演出用メインタイトル）
    fontTitleKanji = CreateFontToHandle("Chiller", 70, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    if (fontTitleKanji == -1) fontTitleKanji = CreateFontToHandle("MS Gothic", 56, 6, DX_FONTTYPE_ANTIALIASING_EDGE_8X8);

    // サブタイトル（英語演出用）
    fontTitleEnglish = CreateFontToHandle("Chiller", 28, 2, DX_FONTTYPE_ANTIALIASING_8X8);
    if (fontTitleEnglish == -1) fontTitleEnglish = CreateFontToHandle("MS Gothic", 22, 3, DX_FONTTYPE_ANTIALIASING_8X8);

    // 説明文用日本語フォント
    fontSubText = CreateJPFont("Meiryo", "Yu Gothic", 16, 2, DX_FONTTYPE_ANTIALIASING_8X8);

    // ボタンメニュー用日本語フォント（タイトル、ポーズ、ゲームオーバー等のボタン項目）
    fontMenu = CreateJPFont("Meiryo", "Yu Gothic", 18, 3, DX_FONTTYPE_ANTIALIASING_8X8);

    // レポート・設定・ポーズ・ハンドブック見出し用（Jホラー調 游明朝）
    fontReportTitle = CreateJPFont("Yu Mincho Demibold", "Yu Mincho", 24, 3, DX_FONTTYPE_ANTIALIASING_8X8);
    if (fontReportTitle == -1) fontReportTitle = CreateJPFont("MS Mincho", "Meiryo", 23, 3, DX_FONTTYPE_ANTIALIASING_8X8);

    // 不気味なJホラー明朝本文用（キャッチコピー・クリア/ゲームオーバー物語文向け）
    fontMinchoSub = CreateJPFont("Yu Mincho", "MS Mincho", 18, 3, DX_FONTTYPE_ANTIALIASING_8X8);

    // 1998 VHSカムコーダー・数値用等幅フォント
    fontCamcorder = CreateFontToHandle("Consolas", 15, 3, DX_FONTTYPE_ANTIALIASING_8X8);
    if (fontCamcorder == -1) fontCamcorder = CreateFontToHandle("Courier New", 15, 3, DX_FONTTYPE_ANTIALIASING_8X8);

    // レポート本文用日本語フォント
    fontReportBody = CreateJPFont("Meiryo", "Yu Gothic", 15, 2, DX_FONTTYPE_ANTIALIASING_8X8);

    // ルール説明ポップアップ用日本語フォント
    fontPopupTitle = CreateJPFont("Yu Mincho Demibold", "Yu Mincho", 26, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    if (fontPopupTitle == -1) fontPopupTitle = CreateJPFont("MS Mincho", "Meiryo", 24, 4, DX_FONTTYPE_ANTIALIASING_8X8);
    fontPopupSection = CreateJPFont("Meiryo", "Yu Gothic", 17, 3, DX_FONTTYPE_ANTIALIASING_8X8);
    fontPopupBody = CreateJPFont("Meiryo", "Yu Gothic", 15, 2, DX_FONTTYPE_ANTIALIASING_8X8);
    fontPopupSub = CreateJPFont("Meiryo", "Yu Gothic", 13, 2, DX_FONTTYPE_ANTIALIASING_8X8);

    showRulePopup = false;
    rulePopupFromManual = false;
    hasShownInitialRulePopup = false;
    oldHKey = false;

    CreateTitleAndLoadingTextures();
    AudioManager::Instance().PlayTitleBgm();
    AudioManager::Instance().SetTitleWindLevel(0.32f);
    AudioManager::Instance().SetRainAmbience(0.28f);
    return true;
}

// 終了処理
void SceneManager::Finalize()
{
    ReleaseTitleAndLoadingTextures();
    if (fontTitleKanji != -1) { DeleteFontToHandle(fontTitleKanji); fontTitleKanji = -1; }
    if (fontTitleEnglish != -1) { DeleteFontToHandle(fontTitleEnglish); fontTitleEnglish = -1; }
    if (fontSubText != -1) { DeleteFontToHandle(fontSubText); fontSubText = -1; }
    if (fontMenu != -1) { DeleteFontToHandle(fontMenu); fontMenu = -1; }
    if (fontReportTitle != -1) { DeleteFontToHandle(fontReportTitle); fontReportTitle = -1; }
    if (fontReportBody != -1) { DeleteFontToHandle(fontReportBody); fontReportBody = -1; }
    if (fontMinchoSub != -1) { DeleteFontToHandle(fontMinchoSub); fontMinchoSub = -1; }
    if (fontCamcorder != -1) { DeleteFontToHandle(fontCamcorder); fontCamcorder = -1; }

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

    // ポーズ中はゲーム内時刻の進行を一時停止し音響ダッキングを反映
    HorrorUI::Instance().SetTimePaused(paused);
    AudioManager::Instance().SetPausedDucking(paused);

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
// 手動表示かどうかを指定（プレイ中にHキーで開いた場合は真、開始時の自動表示は偽）
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

void SceneManager::StartLoadingToGame(int day, bool isRetry, float duration)
{
    if (IsTransitioning())
    {
        return;
    }

    if (day < 1) day = 1;
    if (day > 5) day = 5;

    pendingLoadingDay = day;
    currentDay = day;
    loadingIsRetry = isRetry;
    loadingTimer = 0.0f;
    loadingDuration = (duration > 0.4f) ? duration : 2.1f;
    loadingProgress = 0.0f;
    loadingAppliedReset = false;
    loadingAnomalyAudioTriggered = false;
    loadingCompleteAudioTriggered = false;

    AudioManager::Instance().PlayTapeInsert();
    StartTransition(SceneType::Loading, 0.55f);
}

// 7日間進行管理
void SceneManager::SetCurrentDay(int day)
{
    if (day < 1) day = 1;
    if (day > 5) day = 5;
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
    if (day > 5) day = 5;
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
    float batteryMultipliers[5] = { 1.0f, 1.25f, 1.50f, 1.75f, 2.00f };
    bicycle.SetBatteryDrainMultiplier(batteryMultipliers[currentDay - 1]);

    // 新聞のリセット（本日の必要部数）
    newspaper.Reset(deliveryManager.GetTotalDeliveries());

    // 深夜環境・街灯損壊プリセット
    nightEnv.ResetManualPresetOverride();
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
    unsigned int borderCol = isHover ? GetColor(185, 65, 56) : HorrorUI::COL_BORDER;
    unsigned int bgCol = isHover ? GetColor(34, 15, 18) : HorrorUI::COL_PANEL_BG;
    int alpha = isHover ? 240 : 195;

    HorrorUI::Instance().DrawRetroPanel(x1, y1, x2, y2, borderCol, bgCol, alpha);

    // 左端ステータスアクセントバー
    DrawBox(x1 + 3, y1 + 6, x1 + (isHover ? 6 : 5), y2 - 6,
        isHover ? GetColor(225, 68, 55) : HorrorUI::COL_BORDER_DIM, TRUE);

    // ホバー時の微小暗赤グラデーション帯
    if (isHover)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 34);
        DrawBox(x1 + 6, y1 + 3, x2 - 4, y2 - 3, GetColor(185, 40, 35), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    int useFont = (fontHandle != -1) ? fontHandle : fontMenu;
    unsigned int textCol = isHover ? hoverCol : baseCol;
    int strW = 0;
    int jx = 0;
    int jy = 0;
    if (isHover && (std::rand() % 14 == 0))
    {
        jx = (std::rand() % 3) - 1;
        jy = (std::rand() % 3) - 1;
    }

    if (useFont != -1)
    {
        strW = GetDrawStringWidthToHandle(label, (int)strlen(label), useFont);
        int tx = (x1 + x2 - strW) / 2 + jx;
        int ty = (y1 + y2 - 19) / 2 + jy;

        // ホバー時の色収差シャドウ
        if (isHover)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
            DrawStringToHandle(tx + 1, ty, label, GetColor(195, 42, 36), useFont);
            DrawStringToHandle(tx - 1, ty, label, GetColor(55, 130, 145), useFont);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        HorrorUI::Instance().DrawShadowString(tx, ty, label, textCol, useFont);
    }
    else
    {
        strW = GetDrawStringWidth(label, (int)strlen(label));
        int tx = (x1 + x2 - strW) / 2 + jx;
        int ty = (y1 + y2 - 16) / 2 + jy;
        HorrorUI::Instance().DrawShadowString(tx, ty, label, textCol, -1);
    }

    if (isHover)
    {
        int arrowF = (fontCamcorder != -1) ? fontCamcorder : useFont;
        int ay = (y1 + y2 - 16) / 2;
        HorrorUI::Instance().DrawShadowString(x1 + 12, ay, ">", GetColor(235, 65, 55), arrowF);
        HorrorUI::Instance().DrawShadowString(x2 - 22, ay, "<", GetColor(235, 65, 55), arrowF);
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

    // Hキー（遊び方表示の切替を押した瞬間に一度だけ行う）
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
                AudioManager::Instance().SetTitleWindLevel(0.32f);
                AudioManager::Instance().SetRainAmbience(0.28f);
            }
            else if (currentScene == SceneType::Menu)
            {
                SetMouseDispFlag(TRUE);
                horrorManager.Reset();
                lastMenuHoverSelection = titleMenuSelection;
                AudioManager::Instance().PlayTitleBgm();
                AudioManager::Instance().SetTitleWindLevel(0.14f);
                AudioManager::Instance().SetRainAmbience(0.12f);
            }
            else if (currentScene == SceneType::Loading)
            {
                SetMouseDispFlag(TRUE);
                horrorManager.Reset();
                AudioManager::Instance().PlayLoadingBgm();
            }
            else if (currentScene == SceneType::Clear)
            {
                SetMouseDispFlag(TRUE);
                AudioManager::Instance().PlayClearWindAmbience();
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
            AudioManager::Instance().PlayTitleBgm();
            AudioManager::Instance().SetTitleWindLevel(0.32f);
            AudioManager::Instance().SetRainAmbience(0.28f);

            if (audioTestFeedbackTimer > 0.0f)
            {
                audioTestFeedbackTimer -= dt;
            }

            UpdateTitleCamera(dt, bicycle, nightEnv, weatherManager, player);

            if ((mouseClicked || enterDown || spaceDown) && !IsTransitioning())
            {
                AudioManager::Instance().PlayTitleConfirm();
                titleSubMenu = TitleSubMenu::Main;
                titleMenuSelection = 0;
                lastMenuHoverSelection = 0;
                StartTransition(SceneType::Menu, 0.55f);
            }
            else if (escDown && !IsTransitioning())
            {
                PlayMenuClick();
                outQuitRequested = true;
            }
        }
        break;

    case SceneType::Menu:
        {
            SetMouseDispFlag(TRUE);
            titleTimer += dt;

            // タイトルBGMの確実な再生維持
            AudioManager::Instance().PlayTitleBgm();

            if (audioTestFeedbackTimer > 0.0f)
            {
                audioTestFeedbackTimer -= dt;
            }

            UpdateMenuCamera(dt, bicycle, nightEnv, weatherManager, player);

            if (titleSubMenu == TitleSubMenu::Main)
            {
                // キーボードによるメニュー選択カーソル移動（4項目：0〜3）
                if (upDown)
                {
                    titleMenuSelection = (titleMenuSelection <= 0) ? 3 : titleMenuSelection - 1;
                    lastMenuHoverSelection = titleMenuSelection;
                    PlayMenuAdjust();
                }
                if (downDown)
                {
                    titleMenuSelection = (titleMenuSelection >= 3) ? 0 : titleMenuSelection + 1;
                    lastMenuHoverSelection = titleMenuSelection;
                    PlayMenuAdjust();
                }

                int menuY = 250;
                // ボタンの当たり判定（4項目・描画位置と完全一致）
                bool hitStart    = CheckButtonHit(cx - 200, menuY + 20,  cx + 200, menuY + 62);
                bool hitSettings = CheckButtonHit(cx - 200, menuY + 74,  cx + 200, menuY + 116);
                bool hitGuide    = CheckButtonHit(cx - 200, menuY + 128, cx + 200, menuY + 170);
                bool hitQuit     = CheckButtonHit(cx - 200, menuY + 182, cx + 200, menuY + 224);

                if (CheckButtonHover(cx - 200, menuY + 20,  cx + 200, menuY + 62))  titleMenuSelection = 0;
                if (CheckButtonHover(cx - 200, menuY + 74,  cx + 200, menuY + 116)) titleMenuSelection = 1;
                if (CheckButtonHover(cx - 200, menuY + 128, cx + 200, menuY + 170)) titleMenuSelection = 2;
                if (CheckButtonHover(cx - 200, menuY + 182, cx + 200, menuY + 224)) titleMenuSelection = 3;
                if (titleMenuSelection != lastMenuHoverSelection)
                {
                    AudioManager::Instance().PlayMenuHover();
                    lastMenuHoverSelection = titleMenuSelection;
                }

                // 決定処理（クリック、Enterキー、Spaceキー）
                if (((titleMenuSelection == 0 && (enterDown || spaceDown)) || hitStart) && !IsTransitioning())
                {
                    PlayMenuClick();
                    hasShownInitialRulePopup = false;
                    StartLoadingToGame(1, false, 2.1f);
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

    case SceneType::Loading:
        {
            SetMouseDispFlag(TRUE);
            loadingTimer += dt;
            AudioManager::Instance().PlayLoadingBgm();

            float norm = (loadingDuration > 0.01f) ? (loadingTimer / loadingDuration) : 1.0f;
            if (norm < 0.0f) norm = 0.0f;
            if (norm > 1.0f) norm = 1.0f;

            // VHSテープシーク風の段階的イージング
            float eased = norm;
            if (norm < 0.35f)
            {
                eased = (norm / 0.35f) * 0.32f;
            }
            else if (norm < 0.72f)
            {
                float mid = (norm - 0.35f) / 0.37f;
                eased = 0.32f + mid * 0.44f;
            }
            else
            {
                float tail = (norm - 0.72f) / 0.28f;
                eased = 0.76f + tail * 0.24f;
            }
            loadingProgress = eased;

            // 中間タイミングで実ステージ・環境・配達ルート・怪異を再構築
            if (norm >= 0.35f && !loadingAppliedReset)
            {
                loadingAppliedReset = true;
                ResetGame(player, bicycle, newspaper, batteryManager, deliveryManager, nightEnv, weatherManager, horrorManager, pendingLoadingDay);
            }

            // 77%〜87%帯の信号干渉に同期したVHS磁気スタティックSE再生
            if (loadingProgress >= 0.77f && !loadingAnomalyAudioTriggered)
            {
                loadingAnomalyAudioTriggered = true;
                AudioManager::Instance().PlayLoadingStaticBurst();
            }

            if (norm >= 1.0f && !IsTransitioning())
            {
                loadingProgress = 1.0f;
                if (!loadingCompleteAudioTriggered)
                {
                    loadingCompleteAudioTriggered = true;
                    AudioManager::Instance().PlayLoadingComplete();
                }
                StartTransition(SceneType::Game, 0.55f);
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

                // 入力判定の範囲を、下に描画するボタンの位置と一致させる
                const int btnX1 = cx - 110;
                const int btnY1 = cy - 360 + 406;
                const int btnX2 = cx + 110;
                const int btnY2 = btnY1 + 40;
                bool hitButton = CheckButtonHit(btnX1, btnY1, btnX2, btnY2);

                // 操作しやすいよう、ポップアップ内のどこを押しても受け付ける
                bool popupClicked = mouseClicked && mouseX >= cx - 420 && mouseX <= cx + 420
                    && mouseY >= cy - 270 && mouseY <= cy + 270;

                // 初回表示と手動表示のどちらも、エスケープキーで閉じられる
                bool closeRequested = hitButton || popupClicked || enterDown || escDown;

                if (closeRequested)
                {
                    PlayMenuClick();
                    CloseRulePopup();
                }

                // ポップアップ表示中は以降のゲームプレイ処理・ポーズ切り替えをすべて遮断
                return;
            }

            // Hキー：プレイ中にルール説明を手動表示（第4段階の仕様）
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

            // ESCキーで一時停止メニューを切り替え
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
                    lastPauseHoverSelection = pauseMenuSelection;
                    PlayMenuAdjust();
                }
                if (downDown)
                {
                    pauseMenuSelection = (pauseMenuSelection >= 4) ? 0 : pauseMenuSelection + 1;
                    lastPauseHoverSelection = pauseMenuSelection;
                    PlayMenuAdjust();
                }

                // ポーズメニューのボタン判定（5項目）
                bool hitResume    = CheckButtonHit(cx - 190, cy - 105, cx + 190, cy - 65);
                bool hitSettings  = CheckButtonHit(cx - 190, cy - 55,  cx + 190, cy - 15);
                bool hitRetry     = CheckButtonHit(cx - 190, cy - 5,   cx + 190, cy + 35);
                bool hitTitle     = CheckButtonHit(cx - 190, cy + 45,  cx + 190, cy + 85);
                bool hitQuit      = CheckButtonHit(cx - 190, cy + 95,  cx + 190, cy + 135);

                if (CheckButtonHover(cx - 190, cy - 105, cx + 190, cy - 65)) pauseMenuSelection = 0;
                if (CheckButtonHover(cx - 190, cy - 55,  cx + 190, cy - 15)) pauseMenuSelection = 1;
                if (CheckButtonHover(cx - 190, cy - 5,   cx + 190, cy + 35)) pauseMenuSelection = 2;
                if (CheckButtonHover(cx - 190, cy + 45,  cx + 190, cy + 85)) pauseMenuSelection = 3;
                if (CheckButtonHover(cx - 190, cy + 95,  cx + 190, cy + 135)) pauseMenuSelection = 4;
                if (pauseMenuSelection != lastPauseHoverSelection)
                {
                    AudioManager::Instance().PlayMenuHover();
                    lastPauseHoverSelection = pauseMenuSelection;
                }

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
                    StartLoadingToGame(currentDay, true, 1.35f);
                }
                else if (hitTitle || (enterDown && pauseMenuSelection == 3))
                {
                    PlayMenuClick();
                    SetPaused(false);
                    titleSubMenu = TitleSubMenu::Main;
                    titleMenuSelection = 0;
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
                    AudioManager::Instance().SetHorrorTension(0.0f);
                    AudioManager::Instance().PlayGameClearSting();
                    SetMouseDispFlag(TRUE);
                    StartTransition(SceneType::Clear, 1.2f);
                }
            }

            // 最大恐怖ゲームオーバー演出終了検出 -> ゲームオーバーシーンへ移行
            if (horrorManager.IsGameOver() && !IsTransitioning())
            {
                AudioManager::Instance().SetHorrorTension(0.0f);
                AudioManager::Instance().PlayGameOverSting();
                SetMouseDispFlag(TRUE);
                StartTransition(SceneType::GameOver, 1.0f);
            }
        }
        break;

    case SceneType::Clear:
        {
            SetMouseDispFlag(TRUE);
            clearTimer += dt;

            if (currentDay < 5)
            {
                // 1〜4日目：Enterキーまたは約3.5秒後にローディングを経て翌日へ移行
                bool hitNextNight = mouseClicked && (mouseY > 200);
                if ((hitNextNight || enterDown || spaceDown || clearTimer >= 3.5f) && !IsTransitioning())
                {
                    StartLoadingToGame(currentDay + 1, false, 1.9f);
                }
            }
            else
            {
                // 最終日: タイトルへ戻る / 終了
                bool hitTitle = CheckButtonHit(cx - 180, cy + 60, cx + 180, cy + 100);
                bool hitQuit = CheckButtonHit(cx - 180, cy + 115, cx + 180, cy + 155);

                if ((hitTitle || enterDown || spaceDown) && !IsTransitioning())
                {
                    titleSubMenu = TitleSubMenu::Main;
                    titleMenuSelection = 0;
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
                StartLoadingToGame(currentDay, true, 1.35f);
            }

            if ((hitTitle || enterDown || spaceDown) && !IsTransitioning())
            {
                titleSubMenu = TitleSubMenu::Main;
                titleMenuSelection = 0;
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
            // 3D背景の描画（深夜住宅街の路地裏＋街灯＋自転車・ポスト＋霧・天候）
            SetUseZBuffer3D(TRUE);
            SetWriteZBuffer3D(TRUE);
            nightEnv.ApplyLightingAndFog();
            DrawGround();
            MapDraw();
            batteryManager.Draw();
            nightEnv.DrawStreetLights();
            bicycle.Draw();
            mailbox.Draw(deliveryManager);
            weatherManager.DrawParticles(bicycle, nightEnv);

            // 2D描画への影響を防ぐためフォグとZバッファを無効化
            SetFogEnable(FALSE);
            SetUseZBuffer3D(FALSE);
            SetWriteZBuffer3D(FALSE);

            // タイトル専用オーバーレイ描画
            DrawTitleSplashScreen(nightEnv);
        }
        break;

    case SceneType::Menu:
        {
            // 3D背景の描画（従来のメニュー選択画面の空気感をそのまま維持）
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

            // メニュー選択画面オーバーレイ（メイン・設定・操作説明）
            DrawMenuScene(nightEnv);
        }
        break;

    case SceneType::Loading:
        {
            SetFogEnable(FALSE);
            SetUseZBuffer3D(FALSE);
            SetWriteZBuffer3D(FALSE);
            DrawLoadingScene();
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
                snprintf(dbgDay, sizeof(dbgDay), "DAY %d / 5 (SEED: 0x%05X)",
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

    int labelF = (fontMenu != -1) ? fontMenu : fontSubText;
    int monoF = (fontCamcorder != -1) ? fontCamcorder : fontSubText;

    // 項目名ラベル（シャドウ＆選択アクセント付き）
    unsigned int lblCol = isSelected ? GetColor(250, 212, 162) : GetColor(198, 210, 204);
    if (isSelected)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 36);
        DrawBox(x - 24, y - 4, x + 530, y + height + 4, GetColor(175, 42, 36), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        HorrorUI::Instance().DrawShadowString(x - 18, y + 4, ">", GetColor(240, 75, 58), monoF);
    }
    HorrorUI::Instance().DrawShadowString(x, y + 4, label, lblCol, labelF);

    int barX = x + 160;
    int barW = 240;
    int btnW = 28;

    // マイナスボタン [ < ]
    bool hLeft = CheckButtonHover(barX, y, barX + btnW, y + height);
    DrawBox(barX, y, barX + btnW, y + height, hLeft ? GetColor(62, 26, 26) : GetColor(22, 26, 28), TRUE);
    DrawBox(barX, y, barX + btnW, y + height, hLeft ? GetColor(215, 78, 68) : GetColor(65, 78, 72), FALSE);
    HorrorUI::Instance().DrawShadowString(barX + 9, y + 5, "<", hLeft ? GetColor(255, 255, 255) : GetColor(210, 220, 215), monoF);

    // バー本体背景（深い陥没グルーブ）
    int sliderX = barX + btnW + 8;
    DrawBox(sliderX, y + 2, sliderX + barW, y + height - 2, GetColor(10, 12, 14), TRUE);
    DrawBox(sliderX, y + 2, sliderX + barW, y + height - 2, isSelected ? GetColor(165, 62, 52) : GetColor(58, 70, 66), FALSE);

    // 10段階のホラーセグメントブロック（上部ベベル発光ライン付き）
    int segCount = 10;
    int activeSegs = (volume + 5) / 10;
    int segW = (barW - 6) / segCount;
    for (int i = 0; i < segCount; ++i)
    {
        int sx = sliderX + 3 + i * segW;
        bool active = (i < activeSegs);
        if (active)
        {
            unsigned int segCol = isSelected ? GetColor(228, 68, 48) : GetColor(180, 56, 44);
            unsigned int hiCol = isSelected ? GetColor(255, 148, 120) : GetColor(225, 112, 95);
            DrawBox(sx, y + 5, sx + segW - 2, y + height - 5, segCol, TRUE);
            DrawLine(sx, y + 5, sx + segW - 2, y + 5, hiCol);
        }
        else
        {
            DrawBox(sx, y + 5, sx + segW - 2, y + height - 5, GetColor(22, 26, 28), TRUE);
        }
    }

    // プラスボタン [ > ]
    int rightBtnX = sliderX + barW + 8;
    bool hRight = CheckButtonHover(rightBtnX, y, rightBtnX + btnW, y + height);
    DrawBox(rightBtnX, y, rightBtnX + btnW, y + height, hRight ? GetColor(62, 26, 26) : GetColor(22, 26, 28), TRUE);
    DrawBox(rightBtnX, y, rightBtnX + btnW, y + height, hRight ? GetColor(215, 78, 68) : GetColor(65, 78, 72), FALSE);
    HorrorUI::Instance().DrawShadowString(rightBtnX + 9, y + 5, ">", hRight ? GetColor(255, 255, 255) : GetColor(210, 220, 215), monoF);

    // パーセンテージ値（等幅フォント）
    char pctStr[16];
    snprintf(pctStr, sizeof(pctStr), "%3d%%", volume);
    HorrorUI::Instance().DrawShadowString(rightBtnX + btnW + 12, y + 5, pctStr, isSelected ? GetColor(255, 205, 125) : GetColor(212, 178, 132), monoF);
}

// タイトル・メニュー・ローディング演出用カメラおよびプロシージャル光学素材
void SceneManager::CreateTitleAndLoadingTextures()
{
    ReleaseTitleAndLoadingTextures();

    // 1. 滑らかな35mm光学レンズ周辺減光テクスチャ (MULブレンド用: alpha=255固定)
    {
        const int vw = 320;
        const int vh = 180;
        int soft = MakeARGB8ColorSoftImage(vw, vh);
        if (soft != -1)
        {
            for (int y = 0; y < vh; ++y)
            {
                float ny = (static_cast<float>(y) + 0.5f) / static_cast<float>(vh) * 2.0f - 1.0f;
                for (int x = 0; x < vw; ++x)
                {
                    float nx = (static_cast<float>(x) + 0.5f) / static_cast<float>(vw) * 2.0f - 1.0f;
                    float dist = std::sqrt(nx * nx * 0.86f + ny * ny * 1.18f);
                    float edge = std::clamp((dist - 0.34f) / 0.96f, 0.0f, 1.0f);
                    float smooth = edge * edge * (3.0f - 2.0f * edge);
                    float keep = 1.0f - smooth * 0.91f;
                    int r = std::clamp(static_cast<int>(246.0f * keep), 14, 255);
                    int g = std::clamp(static_cast<int>(252.0f * keep), 17, 255);
                    int b = std::clamp(static_cast<int>(255.0f * keep), 20, 255);
                    DrawPixelSoftImage(soft, x, y, r, g, b, 255);
                }
            }
            texRadialVignette = CreateGraphFromSoftImage(soft);
            DeleteSoftImage(soft);
        }
    }

    // 2. 1998 VHSアナログイン＆微細縦スクラッチ (ADDブレンド用微細輝度値)
    {
        const int gw = 256;
        const int gh = 256;
        int soft = MakeARGB8ColorSoftImage(gw, gh);
        if (soft != -1)
        {
            auto hash2 = [](uint32_t x, uint32_t y) -> uint32_t {
                uint32_t h = x * 374761393u + y * 668265263u + 1442695041u;
                h = (h ^ (h >> 13)) * 1274126177u;
                return h ^ (h >> 16);
            };
            for (int y = 0; y < gh; ++y)
            {
                for (int x = 0; x < gw; ++x)
                {
                    uint32_t h = hash2(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
                    float n = static_cast<float>(h & 0xFFu) / 255.0f;
                    int grain = static_cast<int>(n * 18.0f);
                    if ((h & 0x3FFu) == 0u) grain += 24;
                    if ((x == 47 || x == 183) && ((y + x) % 5 != 0)) grain += 9;
                    int r = std::clamp(grain, 0, 56);
                    int g = std::clamp(grain + 1, 0, 58);
                    int b = std::clamp(grain + 2, 0, 62);
                    DrawPixelSoftImage(soft, x, y, r, g, b, 255);
                }
            }
            texFilmGrain = CreateGraphFromSoftImage(soft);
            DeleteSoftImage(soft);
        }
    }

    // 3. 多層ローリング深夜霧レイヤーテクスチャ (ADDブレンド用ソフトクラウド)
    {
        const int fw = 256;
        const int fh = 256;
        int soft = MakeARGB8ColorSoftImage(fw, fh);
        if (soft != -1)
        {
            auto hashWrap = [](int x, int y, int period) -> float {
                int wx = ((x % period) + period) % period;
                int wy = ((y % period) + period) % period;
                uint32_t h = static_cast<uint32_t>(wx) * 1597334677u ^ static_cast<uint32_t>(wy) * 3812015801u;
                h = (h ^ (h >> 13)) * 1274126177u;
                return static_cast<float>((h ^ (h >> 16)) & 0xFFu) / 255.0f;
            };
            auto smoothNoise = [&](float u, float v, int grid) -> float {
                float gx = u * static_cast<float>(grid);
                float gy = v * static_cast<float>(grid);
                int ix = static_cast<int>(std::floor(gx));
                int iy = static_cast<int>(std::floor(gy));
                float fx = gx - ix;
                float fy = gy - iy;
                float ux = fx * fx * (3.0f - 2.0f * fx);
                float uy = fy * fy * (3.0f - 2.0f * fy);
                float n00 = hashWrap(ix, iy, grid);
                float n10 = hashWrap(ix + 1, iy, grid);
                float n01 = hashWrap(ix, iy + 1, grid);
                float n11 = hashWrap(ix + 1, iy + 1, grid);
                float nx0 = n00 + (n10 - n00) * ux;
                float nx1 = n01 + (n11 - n01) * ux;
                return nx0 + (nx1 - nx0) * uy;
            };
            for (int y = 0; y < fh; ++y)
            {
                float v = static_cast<float>(y) / static_cast<float>(fh);
                for (int x = 0; x < fw; ++x)
                {
                    float u = static_cast<float>(x) / static_cast<float>(fw);
                    float f = smoothNoise(u, v, 4) * 0.52f + smoothNoise(u, v, 8) * 0.30f + smoothNoise(u, v, 16) * 0.18f;
                    float mist = std::clamp((f - 0.32f) / 0.55f, 0.0f, 1.0f);
                    int val = static_cast<int>(mist * mist * 220.0f);
                    DrawPixelSoftImage(soft, x, y, val, val, val, 255);
                }
            }
            texFogLayer = CreateGraphFromSoftImage(soft);
            DeleteSoftImage(soft);
        }
    }

    // 4. VHSトラッキング乱れ・色収差キャプチャ用オフスクリーン (alphaなし)
    texScreenSnap = MakeScreen(1280, 720, FALSE);
}

void SceneManager::ReleaseTitleAndLoadingTextures()
{
    if (texRadialVignette != -1) { DeleteGraph(texRadialVignette); texRadialVignette = -1; }
    if (texFilmGrain != -1) { DeleteGraph(texFilmGrain); texFilmGrain = -1; }
    if (texFogLayer != -1) { DeleteGraph(texFogLayer); texFogLayer = -1; }
    if (texScreenSnap != -1) { DeleteGraph(texScreenSnap); texScreenSnap = -1; }
}

void SceneManager::DrawOpticalVignette() const
{
    if (texRadialVignette == -1) return;
    SetDrawMode(DX_DRAWMODE_BILINEAR);
    SetDrawBlendMode(DX_BLENDMODE_MUL, 255);
    DrawExtendGraph(0, 0, 1280, 720, texRadialVignette, FALSE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetDrawMode(DX_DRAWMODE_NEAREST);
}

void SceneManager::DrawAnalogFilmGrain(float timerSeed, int addAlpha) const
{
    if (texFilmGrain == -1) return;
    int step = static_cast<int>(timerSeed * 24.0f);
    int ox = ((step * 73) % 256);
    int oy = ((step * 137) % 256);
    SetDrawBlendMode(DX_BLENDMODE_ADD, addAlpha);
    SetDrawBright(210, 222, 228);
    for (int y = -oy; y < 720; y += 256)
    {
        for (int x = -ox; x < 1280; x += 256)
        {
            DrawGraph(x, y, texFilmGrain, FALSE);
        }
    }
    SetDrawBright(255, 255, 255);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void SceneManager::DrawDriftingFogOverlay(float timerSeed, int alpha) const
{
    if (texFogLayer == -1) return;
    SetDrawMode(DX_DRAWMODE_BILINEAR);
    SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);

    // レイヤー1: 冷たい深夜藍色の濃霧（右方向へ緩やかに流動）
    int shiftX1 = static_cast<int>(std::fmod(timerSeed * 22.0f, 512.0f));
    int shiftY1 = static_cast<int>(std::sin(timerSeed * 0.35f) * 24.0f);
    SetDrawBright(36, 48, 58);
    for (int y = -256 + shiftY1; y < 720; y += 512)
    {
        for (int x = -shiftX1; x < 1280; x += 512)
        {
            DrawExtendGraph(x, y, x + 512, y + 512, texFogLayer, FALSE);
        }
    }

    // レイヤー2: 街灯に滲むほのかな琥珀霧（逆方向へ流動）
    int shiftX2 = static_cast<int>(std::fmod(timerSeed * 14.0f, 640.0f));
    SetDrawBright(42, 34, 28);
    for (int x = -640 + shiftX2; x < 1280; x += 640)
    {
        DrawExtendGraph(x, 120, x + 640, 680, texFogLayer, FALSE);
    }

    SetDrawBright(255, 255, 255);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetDrawMode(DX_DRAWMODE_NEAREST);
}

void SceneManager::ApplyVhsTrackingEffect(float timerSeed, float anomalyStrength) const
{
    if (texScreenSnap == -1) return;

    float cycle = std::fmod(timerSeed, 4.4f);
    float burst = (cycle < 0.16f) ? (1.0f - cycle / 0.16f) * 0.45f : 0.0f;
    float lvl = (std::max)(anomalyStrength, burst);
    if (lvl <= 0.04f) return;

    GetDrawScreenGraph(0, 0, 1280, 720, texScreenSnap);

    // 水平VHSトラッキングズレ帯
    int bands = 2 + static_cast<int>(lvl * 4.0f);
    for (int i = 0; i < bands; ++i)
    {
        unsigned int h = static_cast<unsigned int>(timerSeed * 60.0f) * 2654435761u + static_cast<unsigned int>(i) * 97u;
        int sy = static_cast<int>(h % 680u);
        int sh = 5 + static_cast<int>((h >> 8) % 16u);
        int dx = ((i % 2 == 0) ? 1 : -1) * (4 + static_cast<int>(lvl * 16.0f));
        DrawRectGraph(dx, sy, 0, sy, 1280, sh, texScreenSnap, FALSE, FALSE);
    }

    // 微細なRGB色収差滲み
    int rgbShift = 2 + static_cast<int>(lvl * 4.0f);
    SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(16.0f + lvl * 24.0f));
    SetDrawBright(255, 45, 45);
    DrawGraph(-rgbShift, 0, texScreenSnap, FALSE);
    SetDrawBright(50, 190, 240);
    DrawGraph(rgbShift, 0, texScreenSnap, FALSE);
    SetDrawBright(255, 255, 255);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void SceneManager::UpdateTitleCamera(float dt, Bicycle& bicycle, NightEnvironment& nightEnv, WeatherManager& weatherManager, Player& player)
{
    // ブロック塀に挟まれた暗い路地奥と明滅する街灯を捉える1998カムコーダー構図
    float breatheX = std::sin(titleTimer * 0.85f) * 0.055f + std::sin(titleTimer * 0.24f) * 0.18f;
    float breatheY = std::cos(titleTimer * 1.15f) * 0.024f;
    float breatheZ = std::sin(titleTimer * 0.18f) * 0.28f;
    VECTOR camPos = VGet(-0.45f + breatheX, 1.32f + breatheY, 10.6f + breatheZ);
    VECTOR camTarget = VGet(0.42f + std::sin(titleTimer * 0.31f) * 0.09f, 1.38f + breatheY * 0.6f, 23.0f);
    SetCameraPositionAndTarget_UpVecY(camPos, camTarget);

    // 路地入口の最寄り街灯 (ID: 4) を周期的に不気味に明滅させ電気火花音を同期
    if (titleLampAudioCooldown > 0.0f)
    {
        titleLampAudioCooldown -= dt;
    }
    if (std::fmod(titleTimer, 4.6f) < dt)
    {
        nightEnv.TriggerStreetLightFlicker(4);
        if (titleTimer > 0.4f && titleLampAudioCooldown <= 0.0f)
        {
            AudioManager::Instance().PlayLampFlickerBuzz(-55);
            titleLampAudioCooldown = 2.0f;
        }
    }

    nightEnv.Update(bicycle, camPos);
    weatherManager.Update(dt, player, bicycle, nightEnv);
}

void SceneManager::UpdateMenuCamera(float dt, Bicycle& bicycle, NightEnvironment& nightEnv, WeatherManager& weatherManager, Player& player)
{
    float swayX = std::sin(titleTimer * 0.22f) * 1.35f;
    float sweepZ = -21.5f + std::sin(titleTimer * 0.14f) * 2.4f;
    VECTOR camPos = VGet(-1.4f + swayX, 1.95f, sweepZ);
    VECTOR camTarget = VGet(-2.8f + std::cos(titleTimer * 0.16f) * 0.75f, 1.75f, sweepZ + 14.5f);
    SetCameraPositionAndTarget_UpVecY(camPos, camTarget);

    nightEnv.Update(bicycle, camPos);
    (void)player;
    weatherManager.Update(dt, player, bicycle, nightEnv);
}

void SceneManager::DrawTitleLogoHero(int cx, int titleY, float jitterSine)
{
    int jx = (std::rand() % 5 == 0) ? (int)(jitterSine * 2.0f) : 0;
    const char* mainTitle = "NEWSPAPER DELIVERY";
    int logoW = (fontTitleKanji != -1) ? GetDrawStringWidthToHandle(mainTitle, (int)strlen(mainTitle), fontTitleKanji) : 340;
    int logoX = cx - logoW / 2;

    if (fontTitleKanji != -1)
    {
        // ロゴ背後の幽かな血赤ノイズオーラ＋重厚シャドウ＋VHS色収差
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 65);
        DrawStringToHandle(logoX - 1, titleY + 3, mainTitle, GetColor(125, 16, 16), fontTitleKanji);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210);
        DrawStringToHandle(logoX + 2, titleY + 2, mainTitle, GetColor(4, 5, 6), fontTitleKanji);
        DrawStringToHandle(logoX - 3 + jx, titleY, mainTitle, GetColor(200, 36, 32), fontTitleKanji);
        DrawStringToHandle(logoX + 3 - jx, titleY, mainTitle, GetColor(40, 138, 155), fontTitleKanji);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawStringToHandle(logoX, titleY, mainTitle, GetColor(238, 242, 232), fontTitleKanji);
    }
    else
    {
        HorrorUI::Instance().DrawShadowString(cx - 150, titleY, mainTitle, GetColor(235, 240, 230), -1);
    }

    // サブタイトル
    const char* subTitle = "- SHINYA HAITATSU -";
    if (fontTitleEnglish != -1)
    {
        int subW = GetDrawStringWidthToHandle(subTitle, (int)strlen(subTitle), fontTitleEnglish);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 105);
        DrawStringToHandle(cx - subW / 2 + 1, titleY + 73, subTitle, GetColor(75, 10, 10), fontTitleEnglish);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        HorrorUI::Instance().DrawShadowString(cx - subW / 2, titleY + 72, subTitle, GetColor(205, 76, 66), fontTitleEnglish);
    }
}

void SceneManager::DrawTitleSplashScreen(const NightEnvironment& nightEnv)
{
    (void)nightEnv;
    int cx = 640;

    // 1. 濃密な霧の質感・35mm光学周辺減光・フィルムグレインの重畳
    DrawDriftingFogOverlay(titleTimer, 20);
    DrawOpticalVignette();
    DrawAnalogFilmGrain(titleTimer, 20);

    // 2. 上部1998 VHSカムコーダー枠＆ライブタイムコード
    bool recBlink = (((int)(titleTimer * 2.0f)) % 2) == 0;
    int sec = static_cast<int>(titleTimer) % 60;
    int frameSub = static_cast<int>(titleTimer * 24.0f) % 24;
    char timeBuf[64];
    snprintf(timeBuf, sizeof(timeBuf), "1998.08.14 FRI  AM 01:00:%02d:%02d", sec, frameSub);
    DrawVhsCamcorderHeader("TAPE: SP  CH-03", timeBuf, recBlink, titleTimer);

    // 3. タイトルロゴ（路地裏の怪異が中央下に透けて見える構図に合わせてやや上寄り配置）
    float jitterSine = std::sin(titleTimer * 8.0f);
    DrawTitleLogoHero(cx, 128, jitterSine);

    // Jホラー調キャッチコピー（枠なしの重厚シャドウで霧に溶け込む表現）
    const char* tagline = "午前1時の静寂、そして路地裏に潜む恐怖。";
    int tagF = (fontMinchoSub != -1) ? fontMinchoSub : fontSubText;
    if (tagF != -1)
    {
        int tagW = GetDrawStringWidthToHandle(tagline, (int)strlen(tagline), tagF);
        HorrorUI::Instance().DrawShadowString(cx - tagW / 2 + 1, 250, tagline, GetColor(6, 8, 10), tagF);
        HorrorUI::Instance().DrawShadowString(cx - tagW / 2, 248, tagline, GetColor(196, 208, 202), tagF);
    }

    // 漸減グラデーション状の細い血赤スクラッチ線
    for (int dx = -220; dx < 220; dx += 4)
    {
        float falloff = 1.0f - (static_cast<float>(std::abs(dx)) / 220.0f);
        int la = static_cast<int>(150.0f * falloff * falloff);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, la);
        DrawLine(cx + dx, 286, cx + dx + 4, 286, GetColor(168, 36, 32));
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 4. 枠なし・呼吸する入力待機プロンプト（深い暗影路上に直接浮かぶシネマティック演出）
    float pulse = 0.50f + 0.50f * std::sin(titleTimer * 2.6f);

    int monoF = (fontCamcorder != -1) ? fontCamcorder : fontSubText;
    const char* enPrompt = "PRESS ENTER OR CLICK TO CONTINUE";
    int ew = (monoF != -1) ? GetDrawStringWidthToHandle(enPrompt, (int)strlen(enPrompt), monoF) : GetDrawStringWidth(enPrompt, (int)strlen(enPrompt));
    int glowAlpha = static_cast<int>(155.0f + 100.0f * pulse);
    HorrorUI::Instance().DrawShadowString(cx - ew / 2 + 1, 505, enPrompt, GetColor(8, 6, 8), monoF);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, glowAlpha);
    HorrorUI::Instance().DrawShadowString(cx - ew / 2, 503, enPrompt, GetColor(240, 96, 78), monoF);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    const char* jpPrompt = "画面をクリック または Enter キーでメニューへ";
    int promptF = (fontMenu != -1) ? fontMenu : fontSubText;
    int jw = (promptF != -1) ? GetDrawStringWidthToHandle(jpPrompt, (int)strlen(jpPrompt), promptF) : GetDrawStringWidth(jpPrompt, (int)strlen(jpPrompt));
    HorrorUI::Instance().DrawShadowString(cx - jw / 2 + 1, 531, jpPrompt, GetColor(4, 6, 8), promptF);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(190.0f + 65.0f * pulse));
    HorrorUI::Instance().DrawShadowString(cx - jw / 2, 529, jpPrompt, GetColor(218, 226, 220), promptF);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 5. 下部フッター（ボックスなしの透過アーカイブ記録表記）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 175);
    HorrorUI::Instance().DrawShadowString(40, 676,
        "1998.08.14 MIDNIGHT RECORDING  //  KIRI-NO-MACHI SECTOR",
        GetColor(130, 144, 138), HorrorUI::Instance().GetFontKeyAction());
    const char* hintRight = "[ Enter / Click ]: メニュー   [ Esc ]: 終了";
    int keyF = HorrorUI::Instance().GetFontKeyAction();
    int hw = (keyF != -1) ? GetDrawStringWidthToHandle(hintRight, (int)strlen(hintRight), keyF) : GetDrawStringWidth(hintRight, (int)strlen(hintRight));
    HorrorUI::Instance().DrawShadowString(1240 - hw, 676, hintRight, GetColor(154, 168, 162), keyF);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 6. 実時間VHSトラッキングノイズ・微細走査線
    ApplyVhsTrackingEffect(titleTimer, 0.0f);
    DrawScanlinesAndNoise(18);
}

void SceneManager::DrawMenuScene(const NightEnvironment& nightEnv)
{
    (void)nightEnv;

    int cx = 640;

    // 画面全体の暗転＋35mm光学ヴィネット（従来のメニュー選択画面構成・視認性をそのまま維持）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 185);
    DrawBox(0, 0, 1280, 720, GetColor(3, 5, 7), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    DrawOpticalVignette();

    // 上部レトロVHS・カムコーダーインフォメーション＆ビューファインダーコーナー枠
    bool recBlink = (((int)(titleTimer * 2.0f)) % 2) == 0;
    char timeBuf[64];
    snprintf(timeBuf, sizeof(timeBuf), "1998.08.14 FRI  AM 01:00:%02d", ((int)titleTimer) % 60);
    DrawVhsCamcorderHeader("TAPE: SP  0:01:24", timeBuf, recBlink, titleTimer);

    // サブメニュー画面ごとの描画ディスパッチ（従来のメニュー構成をそのまま維持）
    if (titleSubMenu == TitleSubMenu::Main)
    {
        float jitterSine = std::sin(titleTimer * 8.0f);
        DrawTitleLogoHero(cx, 104, jitterSine);
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
    DrawScanlinesAndNoise(20);
}

void SceneManager::DrawLoadingScene()
{
    int cx = 640;

    // 1. 深い夜霧の背景＋浮遊霧クラウド＋フィルムグレイン
    DrawBox(0, 0, 1280, 720, GetColor(3, 4, 6), TRUE);
    DrawDriftingFogOverlay(loadingTimer, 34);
    DrawAnalogFilmGrain(loadingTimer, 22);

    bool anomalyWindow = (loadingProgress >= 0.77f && loadingProgress <= 0.87f);
    bool recBlink = (((int)(loadingTimer * (anomalyWindow ? 10.0f : 3.5f))) % 2) == 0;
    char tapeBuf[64];
    snprintf(tapeBuf, sizeof(tapeBuf), "TAPE: SEEK >>  NIGHT 0%d / 05", pendingLoadingDay);
    char dateBuf[64];
    snprintf(dateBuf, sizeof(dateBuf), "1998.08.%02d  AM 01:00:00", 14 + pendingLoadingDay - 1);
    DrawVhsCamcorderHeader(tapeBuf, dateBuf, recBlink, loadingTimer);

    // 2. 重厚感のあるスモークガラス＆ヘアライン金属VHSカセットデッキ窓
    int deckX1 = cx - 232;
    int deckY1 = 98;
    int deckX2 = cx + 232;
    int deckY2 = 238;
    DrawBox(deckX1 + 5, deckY1 + 6, deckX2 + 5, deckY2 + 6, GetColor(1, 2, 3), TRUE);

    // 金属筐体ベゼル＋凹み窓
    DrawBox(deckX1, deckY1, deckX2, deckY2, GetColor(14, 16, 18), TRUE);
    DrawBox(deckX1, deckY1, deckX2, deckY2, GetColor(66, 54, 52), FALSE);
    DrawLine(deckX1 + 1, deckY1 + 1, deckX2 - 1, deckY1 + 1, GetColor(46, 52, 50));

    // スモークガラス窓内部
    int winX1 = deckX1 + 16;
    int winY1 = deckY1 + 30;
    int winX2 = deckX2 - 16;
    int winY2 = deckY2 - 12;
    DrawBox(winX1, winY1, winX2, winY2, GetColor(6, 7, 9), TRUE);
    DrawBox(winX1, winY1, winX2, winY2, GetColor(36, 40, 42), FALSE);

    // ステータスLED＋テープカウンタ
    int monoF = (fontCamcorder != -1) ? fontCamcorder : fontSubText;
    DrawCircle(deckX1 + 22, deckY1 + 15, 4, recBlink ? GetColor(225, 48, 42) : GetColor(75, 22, 22), TRUE);
    const char* deckLabel = loadingIsRetry ? "DECK REWIND // TAPE-01" : "VHS-HQ SEEK // CH-03";
    HorrorUI::Instance().DrawShadowString(deckX1 + 34, deckY1 + 8, deckLabel, GetColor(186, 198, 190), monoF);
    char ftBuf[32];
    snprintf(ftBuf, sizeof(ftBuf), "%04d FT", 40 + static_cast<int>(loadingProgress * 280.0f));
    int fw = (monoF != -1) ? GetDrawStringWidthToHandle(ftBuf, (int)strlen(ftBuf), monoF) : GetDrawStringWidth(ftBuf, (int)strlen(ftBuf));
    HorrorUI::Instance().DrawShadowString(deckX2 - 18 - fw, deckY1 + 8, ftBuf, GetColor(218, 112, 86), monoF);

    // 物理連動する2基の磁気テープリール（進行度に応じて左が減り右へ巻き取られる）
    int reelY = (winY1 + winY2) / 2 - 2;
    int leftReelX = cx - 106;
    int rightReelX = cx + 106;
    int leftTapeR = static_cast<int>(39.0f - loadingProgress * 10.0f);
    int rightTapeR = static_cast<int>(29.0f + loadingProgress * 10.0f);

    // 下部テープガイドピン＆張り渡された磁気テープリボン
    DrawCircle(winX1 + 28, winY2 - 10, 4, GetColor(95, 102, 104), TRUE);
    DrawCircle(winX2 - 28, winY2 - 10, 4, GetColor(95, 102, 104), TRUE);
    DrawLine(leftReelX - leftTapeR + 2, reelY + 12, winX1 + 28, winY2 - 10, GetColor(52, 46, 44));
    DrawLine(winX1 + 28, winY2 - 6, winX2 - 28, winY2 - 6, GetColor(68, 62, 60));
    DrawLine(winX2 - 28, winY2 - 10, rightReelX + rightTapeR - 2, reelY + 12, GetColor(52, 46, 44));

    auto DrawDetailedTapeReel = [&](int rx, int ry, int outerR, float dirSign, float phaseOff) {
        // リール下の奥行きドロップシャドウ
        DrawCircle(rx + 2, ry + 3, outerR + 1, GetColor(2, 3, 4), TRUE);
        // 巻かれた暗褐色〜黒の光沢磁気テープ層
        DrawCircle(rx, ry, outerR, GetColor(20, 18, 19), TRUE);
        for (int tr = 24; tr < outerR; tr += 4)
        {
            DrawCircle(rx, ry, tr, GetColor(30, 28, 30), FALSE);
        }
        DrawCircle(rx, ry, outerR, GetColor(56, 52, 54), FALSE);

        // 内側の金属フランジと放射歯
        DrawCircle(rx, ry, 23, GetColor(34, 38, 40), TRUE);
        DrawCircle(rx, ry, 23, GetColor(84, 92, 94), FALSE);
        DrawCircle(rx, ry, 19, GetColor(12, 14, 15), TRUE);
        for (int k = 0; k < 6; ++k)
        {
            float a = loadingTimer * 5.8f * dirSign + phaseOff + k * (DX_PI_F / 3.0f);
            float aNext = a + 0.16f;
            int x1 = rx + static_cast<int>(std::cos(a) * 10.0f);
            int y1 = ry + static_cast<int>(std::sin(a) * 10.0f);
            int x2 = rx + static_cast<int>(std::cos(a) * 19.0f);
            int y2 = ry + static_cast<int>(std::sin(a) * 19.0f);
            int x3 = rx + static_cast<int>(std::cos(aNext) * 19.0f);
            int y3 = ry + static_cast<int>(std::sin(aNext) * 19.0f);
            DrawTriangle(rx, ry, x2, y2, x3, y3, GetColor(58, 64, 66), TRUE);
            DrawLine(x1, y1, x2, y2, GetColor(156, 62, 54));
        }
        DrawCircle(rx, ry, 10, GetColor(52, 56, 58), TRUE);
        DrawCircle(rx, ry, 10, GetColor(115, 124, 122), FALSE);
        DrawCircle(rx, ry, 4, GetColor(14, 16, 17), TRUE);
    };
    DrawDetailedTapeReel(leftReelX, reelY, leftTapeR, 1.0f, 0.0f);
    DrawDetailedTapeReel(rightReelX, reelY, rightTapeR, 1.0f, 0.45f);

    // 窓表面の微細なスモークガラス反射ハイライト帯
    SetDrawBlendMode(DX_BLENDMODE_ADD, 14);
    DrawTriangle(winX1 + 20, winY1, winX1 + 115, winY1, winX1 + 45, winY2, GetColor(170, 190, 200), TRUE);
    DrawTriangle(winX1 + 115, winY1, winX1 + 150, winY1, winX1 + 80, winY2, GetColor(120, 140, 150), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 3. 風化した深夜配達指令日報＆高精細シークバー
    int panelY = 254;
    DrawBox(cx - 385, panelY + 6, cx + 395, panelY + 344, GetColor(1, 2, 3), TRUE);

    HorrorUI::Instance().DrawRetroPanel(cx - 390, panelY, cx + 390, panelY + 338,
        anomalyWindow ? GetColor(175, 52, 46) : GetColor(96, 48, 46), GetColor(9, 10, 12), 240);

    static const char* kDayTitles[5] = {
        "NIGHT 01 / 05 — 月曜深夜：不穏な配達開始",
        "NIGHT 02 / 05 — 火曜深夜：霧の深まる路地",
        "NIGHT 03 / 05 — 水曜深夜：霧と囁きの境界",
        "NIGHT 04 / 05 — 木曜深夜：消えゆく街灯",
        "NIGHT 05 / 05 — 金曜深夜：最悪の深夜配達"
    };
    int dayIdx = (pendingLoadingDay >= 1 && pendingLoadingDay <= 5) ? (pendingLoadingDay - 1) : 0;

    // タイトル見出し
    int titleF = (fontReportTitle != -1) ? fontReportTitle : fontMenu;
    int tw = (titleF != -1) ? GetDrawStringWidthToHandle(kDayTitles[dayIdx], (int)strlen(kDayTitles[dayIdx]), titleF)
                            : GetDrawStringWidth(kDayTitles[dayIdx], (int)strlen(kDayTitles[dayIdx]));
    HorrorUI::Instance().DrawShadowString(cx - tw / 2, panelY + 18, kDayTitles[dayIdx], GetColor(232, 194, 138), titleF);

    DrawLine(cx - 340, panelY + 56, cx + 340, panelY + 56, GetColor(82, 42, 40));

    // ライブEVP/テープ音声オシロスコープ波形（読み込み段階と連動）
    int waveBaseY = panelY + 82;
    const int waveSamples = 56;
    int waveSpan = 660;
    int waveStartX = cx - waveSpan / 2;
    for (int i = 0; i < waveSamples; ++i)
    {
        float xNorm = static_cast<float>(i) / static_cast<float>(waveSamples - 1);
        float s1 = std::sin(xNorm * 28.0f + loadingTimer * 18.0f);
        float s2 = std::cos(xNorm * 61.0f - loadingTimer * 29.0f);
        float spike = anomalyWindow ? (2.2f + std::abs(std::sin(loadingTimer * 44.0f + i))) : 1.0f;
        float amp = (std::abs(s1 * 0.6f + s2 * 0.4f) * (xNorm <= loadingProgress ? 10.0f : 2.5f)) * spike;
        int wx = waveStartX + (i * waveSpan) / (waveSamples - 1);
        int wyTop = waveBaseY - static_cast<int>(amp);
        int wyBot = waveBaseY + static_cast<int>(amp);
        unsigned int wCol = (xNorm <= loadingProgress)
            ? (anomalyWindow ? GetColor(235, 68, 56) : GetColor(168, 62, 54))
            : GetColor(38, 44, 46);
        DrawLine(wx, wyTop, wx, wyBot, wCol);
    }

    // 進行段階メッセージ
    const char* statusMsg = "住宅街マップ・道路コリジョンを構築中...";
    if (anomalyWindow)
    {
        statusMsg = "微弱な信号干渉を検出 — トラッキング補正中...";
    }
    else if (loadingProgress >= 0.96f)
    {
        statusMsg = "準備完了 — 深夜の住宅街へ移動します";
    }
    else if (loadingProgress >= 0.70f)
    {
        statusMsg = "空間ホラー音響・VHS収録システムを接続中...";
    }
    else if (loadingProgress >= 0.35f)
    {
        statusMsg = "街灯照明・濃霧・ポスト配置を同期中...";
    }

    int bodyF = (fontMenu != -1) ? fontMenu : fontSubText;
    int stW = (bodyF != -1) ? GetDrawStringWidthToHandle(statusMsg, (int)strlen(statusMsg), bodyF)
                            : GetDrawStringWidth(statusMsg, (int)strlen(statusMsg));
    HorrorUI::Instance().DrawShadowString(cx - stW / 2, panelY + 104, statusMsg,
        anomalyWindow ? GetColor(245, 130, 112) : GetColor(202, 212, 206), bodyF);

    int pct = std::clamp(static_cast<int>(loadingProgress * 100.0f + 0.5f), 0, 100);
    HorrorUI::Instance().DrawShadowString(cx - 330, panelY + 136,
        anomalyWindow ? "SIGNAL: UNSTABLE  |  SHIFT: AM 01:00" : "TARGET: 05 HOUSES  |  SHIFT: AM 01:00",
        anomalyWindow ? GetColor(220, 96, 86) : GetColor(142, 158, 152), monoF);

    char pctBuf[32];
    snprintf(pctBuf, sizeof(pctBuf), "%3d%%", pct);
    int pw = (monoF != -1) ? GetDrawStringWidthToHandle(pctBuf, (int)strlen(pctBuf), monoF) : GetDrawStringWidth(pctBuf, (int)strlen(pctBuf));
    HorrorUI::Instance().DrawShadowString(cx + 330 - pw, panelY + 136, pctBuf, GetColor(245, 118, 92), monoF);

    // 精密アナログシークバー（発光ヘッド点付き）
    int barX1 = cx - 330;
    int barY1 = panelY + 162;
    int barW = 660;
    int barH = 12;
    DrawBox(barX1, barY1, barX1 + barW, barY1 + barH, GetColor(14, 17, 19), TRUE);
    DrawBox(barX1, barY1, barX1 + barW, barY1 + barH, GetColor(72, 42, 40), FALSE);

    int fillW = static_cast<int>(loadingProgress * static_cast<float>(barW - 4));
    if (fillW > 0)
    {
        DrawBox(barX1 + 2, barY1 + 2, barX1 + 2 + fillW, barY1 + barH - 2, GetColor(168, 42, 36), TRUE);
        DrawLine(barX1 + 2, barY1 + 3, barX1 + 2 + fillW, barY1 + 3, GetColor(225, 86, 68));
        // 先端シークヘッド輝き
        int headX = barX1 + 2 + fillW;
        SetDrawBlendMode(DX_BLENDMODE_ADD, 130);
        DrawCircle(headX, barY1 + barH / 2, 7, GetColor(255, 110, 75), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawCircle(headX, barY1 + barH / 2, 3, GetColor(255, 210, 175), TRUE);
    }

    // 深夜巡回メモ（日別サバイバル記録：余白を広く確保した2行構成）
    HorrorUI::Instance().DrawRetroPanel(cx - 338, panelY + 196, cx + 338, panelY + 312,
        GetColor(48, 60, 56), GetColor(13, 16, 17), 228);

    static const char* kTipHeaders[5] = {
        "[ 巡回メモ 01 ]  街灯と自転車ライト",
        "[ 巡回メモ 02 ]  予備バッテリーの管理",
        "[ 巡回メモ 03 ]  霧の奥の物音と追跡者",
        "[ 巡回メモ 04 ]  街灯の明滅サイン",
        "[ 最終夜メモ ]   生還のための鉄則"
    };
    static const char* kTipLine1[5] = {
        "・街灯の下や自転車ライト点灯中は恐怖度が抑えられます。",
        "・ライトの電力は夜間を通じて徐々に消耗します。",
        "・配達4軒目を過ぎると追跡者が街を徘徊し始めます。",
        "・街灯が激しく明滅したら異常発生の合図です。",
        "・多くの街灯が消えた最終夜です。視界は極めて悪化しています。"
    };
    static const char* kTipLine2[5] = {
        "・暗闇での停止は避け、安全な光を頼りに移動してください。",
        "・街角の予備バッテリーを拾い、[R] キーで交換しましょう。",
        "・霧の奥の泣き声や足音に注意し、光の下へ退避してください。",
        "・消灯する前に速やかに別の街灯か自転車へ退避しましょう。",
        "・GPSマップと耳元の異音に集中し、全5軒へ届け切ってください。"
    };

    int secF = (fontPopupSection != -1) ? fontPopupSection : fontSubText;
    int subF = (fontReportBody != -1) ? fontReportBody : fontSubText;
    HorrorUI::Instance().DrawShadowString(cx - 316, panelY + 210, kTipHeaders[dayIdx], GetColor(232, 184, 126), secF);
    HorrorUI::Instance().DrawShadowString(cx - 312, panelY + 244, kTipLine1[dayIdx], GetColor(192, 204, 198), subF);
    HorrorUI::Instance().DrawShadowString(cx - 312, panelY + 272, kTipLine2[dayIdx], GetColor(162, 176, 170), subF);

    // 4. 仕上げ光学周辺減光＆VHS異常トラッキング乱れ＋走査線
    DrawOpticalVignette();
    ApplyVhsTrackingEffect(loadingTimer, anomalyWindow ? 0.55f : 0.0f);
    DrawScanlinesAndNoise(18);
}

// タイトルメインメニュー描画
void SceneManager::DrawTitleMainMenu(int cx, float jitterSine)
{
    (void)jitterSine;

    // キャッチコピー（Jホラー調 游明朝＋微細グロウ）
    const char* tagline = "午前1時の静寂、そして路地裏に潜む恐怖。";
    int tagF = (fontMinchoSub != -1) ? fontMinchoSub : fontSubText;
    if (tagF != -1)
    {
        int tagW = GetDrawStringWidthToHandle(tagline, (int)strlen(tagline), tagF);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 38);
        DrawBox(cx - tagW / 2 - 28, 211, cx + tagW / 2 + 28, 237, GetColor(4, 6, 7), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        HorrorUI::Instance().DrawShadowString(cx - tagW / 2, 214, tagline, GetColor(168, 178, 172), tagF);
    }

    // メニューパネル（古びた金属と錆・血痕を思わせるレトロホラー枠）
    int menuY = 250;
    int mw = 480;
    int mh = 246;
    HorrorUI::Instance().DrawRetroPanel(cx - mw / 2, menuY, cx + mw / 2, menuY + mh,
        GetColor(105, 48, 46), GetColor(12, 10, 12), 232);

    // ボタンの描画ヘルパー
    auto DrawMenuRow = [&](int y, const char* label, bool isSelected) {
        bool hovered = CheckButtonHover(cx - 200, y, cx + 200, y + 42);
        bool active = isSelected || hovered;

        unsigned int bgCol = active ? GetColor(42, 16, 18) : GetColor(16, 19, 21);
        unsigned int borderCol = active ? GetColor(198, 62, 52) : GetColor(52, 62, 58);
        unsigned int textCol = active ? GetColor(255, 138, 112) : GetColor(192, 202, 196);

        HorrorUI::Instance().DrawRetroPanel(cx - 200, y, cx + 200, y + 42, borderCol, bgCol, active ? 235 : 190);

        if (active)
        {
            DrawBox(cx - 197, y + 6, cx - 194, y + 36, GetColor(225, 62, 50), TRUE);
            int arrowF = (fontCamcorder != -1) ? fontCamcorder : fontMenu;
            HorrorUI::Instance().DrawShadowString(cx - 184, y + 13, ">", GetColor(242, 78, 60), arrowF);
        }

        if (fontMenu != -1)
        {
            int lw = GetDrawStringWidthToHandle(label, (int)strlen(label), fontMenu);
            if (active)
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 135);
                DrawStringToHandle(cx - lw / 2 + 1, y + 11, label, GetColor(185, 36, 32), fontMenu);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
            HorrorUI::Instance().DrawShadowString(cx - lw / 2, y + 11, label, textCol, fontMenu);
        }
        else
        {
            int lw = GetDrawStringWidth(label, (int)strlen(label));
            HorrorUI::Instance().DrawShadowString(cx - lw / 2, y + 13, label, textCol, -1);
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
        HorrorUI::COL_BORDER, HorrorUI::COL_BG, 200);
    HorrorUI::Instance().DrawJitterString(42, helpY + 9,
        "[ 操作方法 ]  WASD / マウス: 移動・視線  |  Enter / クリック: 決定  |  < >: 音量調整  |  Esc: 終了",
        GetColor(168, 182, 176), HorrorUI::Instance().GetFontKeyAction());
}

// 共通設定パネル描画（タイトル・ポーズ共用）
void SceneManager::DrawSettingsPanel(int cx, int setY, const char* headerTitle, bool inPause)
{
    int sw = 680;
    int sh = 450;

    // ポーズ時の暗転オーバーレイ
    if (inPause)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 195);
        DrawBox(0, 0, 1280, 720, GetColor(4, 6, 8), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 設定ウィンドウパネル
    HorrorUI::Instance().DrawRetroPanel(cx - sw / 2, setY, cx + sw / 2, setY + sh,
        GetColor(110, 52, 48), GetColor(12, 12, 15), 245);

    // ヘッダータイトル
    if (fontReportTitle != -1)
    {
        int tw = GetDrawStringWidthToHandle(headerTitle, (int)strlen(headerTitle), fontReportTitle);
        HorrorUI::Instance().DrawShadowString(cx - tw / 2, setY + 13, headerTitle, GetColor(242, 190, 142), fontReportTitle);
    }
    else
    {
        int tw = GetDrawStringWidth(headerTitle, (int)strlen(headerTitle));
        HorrorUI::Instance().DrawShadowString(cx - tw / 2, setY + 16, headerTitle, GetColor(240, 185, 140), -1);
    }

    DrawLine(cx - sw / 2 + 25, setY + 48, cx + sw / 2 - 25, setY + 48, GetColor(85, 45, 42));

    // 4本のスライダーバー描画
    bool dummyDec = false, dummyInc = false;
    DrawVolumeSlider(cx - 280, setY + 65,  560, 28, masterVolume,           "マスター音量",     (settingsSelection == 0), dummyDec, dummyInc);
    DrawVolumeSlider(cx - 280, setY + 125, 560, 28, bgmVolume,              "BGM・環境音",      (settingsSelection == 1), dummyDec, dummyInc);
    DrawVolumeSlider(cx - 280, setY + 185, 560, 28, sfxVolume,              "効果音 (SE)",       (settingsSelection == 2), dummyDec, dummyInc);
    DrawVolumeSlider(cx - 280, setY + 245, 560, 28, mouseSensitivityLevel, "マウス感度",       (settingsSelection == 3), dummyDec, dummyInc);

    // サウンドテストボタン
    int btnF = (fontMenu != -1) ? fontMenu : fontSubText;
    int testBtnY = setY + 315;
    bool hTest = CheckButtonHover(cx - 160, testBtnY, cx + 160, testBtnY + 36);
    bool testActive = (settingsSelection == 4) || hTest;
    bool pulseFlashing = (audioTestFeedbackTimer > 0.0f);

    unsigned int testBg = pulseFlashing ? GetColor(92, 24, 24) : (testActive ? GetColor(46, 18, 20) : GetColor(18, 21, 24));
    unsigned int testBorder = testActive ? GetColor(205, 72, 62) : GetColor(65, 75, 72);
    unsigned int testTxt = pulseFlashing ? GetColor(255, 248, 195) : (testActive ? GetColor(255, 142, 118) : GetColor(198, 208, 202));

    HorrorUI::Instance().DrawRetroPanel(cx - 160, testBtnY, cx + 160, testBtnY + 36, testBorder, testBg, 235);
    const char* testLabel = pulseFlashing ? "[ ! 音声再生中 ! ]" : "テスト音声を再生";
    int tlw = (btnF != -1) ? GetDrawStringWidthToHandle(testLabel, (int)strlen(testLabel), btnF) : GetDrawStringWidth(testLabel, (int)strlen(testLabel));
    HorrorUI::Instance().DrawShadowString(cx - tlw / 2, testBtnY + 9, testLabel, testTxt, btnF);

    // 戻るボタン
    int backBtnY = setY + 365;
    bool hBack = CheckButtonHover(cx - 160, backBtnY, cx + 160, backBtnY + 36);
    bool backActive = (settingsSelection == 5) || hBack;

    HorrorUI::Instance().DrawRetroPanel(cx - 160, backBtnY, cx + 160, backBtnY + 36,
        backActive ? GetColor(205, 72, 62) : GetColor(65, 75, 72),
        backActive ? GetColor(46, 18, 20) : GetColor(18, 21, 24), 235);
    const char* backLabel = inPause ? "ポーズメニューへ戻る [ESC]" : "タイトルへ戻る [ESC]";
    int blw = (btnF != -1) ? GetDrawStringWidthToHandle(backLabel, (int)strlen(backLabel), btnF) : GetDrawStringWidth(backLabel, (int)strlen(backLabel));
    HorrorUI::Instance().DrawShadowString(cx - blw / 2, backBtnY + 9, backLabel, backActive ? GetColor(255, 142, 118) : GetColor(198, 208, 202), btnF);

    // 操作ガイド注釈
    const char* guide = "矢印キー < > またはドラッグで調整、Enterでテスト、Escで戻る";
    int guideF = (fontReportBody != -1) ? fontReportBody : fontSubText;
    int gw = (guideF != -1) ? GetDrawStringWidthToHandle(guide, (int)strlen(guide), guideF) : GetDrawStringWidth(guide, (int)strlen(guide));
    HorrorUI::Instance().DrawShadowString(cx - gw / 2, setY + sh - 26, guide, GetColor(145, 158, 152), guideF);
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
        GetColor(108, 52, 48), GetColor(11, 10, 14), 244);

    // ヘッダータイトル（Jホラー明朝＋陰影）
    const char* guideTitle = "配達員必携ハンドブック — 午前1時のサバイバル規則";
    if (fontReportTitle != -1)
    {
        int tw = GetDrawStringWidthToHandle(guideTitle, (int)strlen(guideTitle), fontReportTitle);
        HorrorUI::Instance().DrawShadowString(cx - tw / 2, py + 12, guideTitle, GetColor(242, 195, 135), fontReportTitle);
    }
    else
    {
        int tw = GetDrawStringWidth(guideTitle, (int)strlen(guideTitle));
        HorrorUI::Instance().DrawShadowString(cx - tw / 2, py + 16, guideTitle, GetColor(240, 190, 130), -1);
    }

    DrawLine(cx - pw / 2 + 25, py + 47, cx + pw / 2 - 25, py + 47, GetColor(92, 46, 44));
    DrawLine(cx - 10, py + 58, cx - 10, py + ph - 65, GetColor(62, 38, 40));

    int secF = (fontPopupSection != -1) ? fontPopupSection : fontSubText;
    int bodyF = (fontReportBody != -1) ? fontReportBody : fontSubText;
    int subF = (fontPopupSub != -1) ? fontPopupSub : fontReportBody;
    int keyF = (fontPopupSub != -1) ? fontPopupSub : fontReportBody;

    // 第1列：キーバインド
    int col1X = cx - pw / 2 + 35;
    HorrorUI::Instance().DrawShadowString(col1X, py + 56, "[ 1. 基本操作・キー設定 ]", GetColor(240, 200, 140), secF);

    auto DrawKeyRow = [&](int y, const char* key, const char* desc) {
        DrawBox(col1X, y, col1X + 132, y + 25, GetColor(19, 23, 25), TRUE);
        DrawBox(col1X, y, col1X + 132, y + 25, GetColor(68, 78, 74), FALSE);
        DrawLine(col1X + 1, y + 1, col1X + 131, y + 1, GetColor(90, 102, 98));
        HorrorUI::Instance().DrawShadowString(col1X + 10, y + 5, key, GetColor(250, 228, 175), keyF);
        HorrorUI::Instance().DrawShadowString(col1X + 146, y + 4, desc, GetColor(198, 208, 202), bodyF);
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
    HorrorUI::Instance().DrawShadowString(col2X, py + 56, "[ 2. 深夜配達サバイバル指示 ]", GetColor(238, 105, 92), secF);

    auto DrawRule = [&](int y, const char* title, const char* desc, const char* desc2) {
        DrawBox(col2X - 4, y + 2, col2X - 1, y + 48, GetColor(160, 48, 44), TRUE);
        HorrorUI::Instance().DrawShadowString(col2X + 4, y - 1, title, GetColor(235, 95, 80), bodyF);
        HorrorUI::Instance().DrawShadowString(col2X + 12, y + 18, desc, GetColor(188, 198, 192), subF);
        if (desc2) HorrorUI::Instance().DrawShadowString(col2X + 12, y + 34, desc2, GetColor(148, 164, 158), subF);
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
    int btnF = (fontMenu != -1) ? fontMenu : fontSubText;

    HorrorUI::Instance().DrawRetroPanel(cx - 140, btnY, cx + 140, btnY + 36,
        hBack ? GetColor(205, 72, 62) : GetColor(68, 76, 72),
        hBack ? GetColor(48, 18, 20) : GetColor(20, 23, 25), 235);
    const char* backTxt = "メニューへ戻る [ESC]";
    int bw = (btnF != -1) ? GetDrawStringWidthToHandle(backTxt, (int)strlen(backTxt), btnF) : GetDrawStringWidth(backTxt, (int)strlen(backTxt));
    HorrorUI::Instance().DrawShadowString(cx - bw / 2, btnY + 9, backTxt, hBack ? GetColor(255, 142, 118) : GetColor(198, 208, 202), btnF);
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
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 195);
    DrawBox(0, 0, 1280, 720, GetColor(4, 6, 8), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ダイアログパネル（5項目対応）
    int pw = 480;
    int ph = 390;
    HorrorUI::Instance().DrawRetroPanel(cx - pw / 2, cy - ph / 2, cx + pw / 2, cy + ph / 2,
        GetColor(105, 48, 46), GetColor(12, 10, 12), 242);

    // タイトル（現在の日数を明記）
    char pauseTitle[64];
    snprintf(pauseTitle, sizeof(pauseTitle), "一時停止 - DAY %d / 5", currentDay);
    if (fontReportTitle != -1)
    {
        int tw = GetDrawStringWidthToHandle(pauseTitle, (int)strlen(pauseTitle), fontReportTitle);
        HorrorUI::Instance().DrawShadowString(cx - tw / 2, cy - ph / 2 + 16, pauseTitle, GetColor(225, 182, 175), fontReportTitle);
    }
    else
    {
        int tw = GetDrawStringWidth(pauseTitle, (int)strlen(pauseTitle));
        HorrorUI::Instance().DrawShadowString(cx - tw / 2, cy - ph / 2 + 20, pauseTitle, GetColor(210, 230, 220), -1);
    }

    DrawLine(cx - pw / 2 + 25, cy - ph / 2 + 54, cx + pw / 2 - 25, cy - ph / 2 + 54, GetColor(82, 42, 40));

    // ボタンの描画（マウスホバー＆キー選択両対応）5項目
    bool sel0 = (pauseMenuSelection == 0);
    bool sel1 = (pauseMenuSelection == 1);
    bool sel2 = (pauseMenuSelection == 2);
    bool sel3 = (pauseMenuSelection == 3);
    bool sel4 = (pauseMenuSelection == 4);

    char retryLabel[32];
    snprintf(retryLabel, sizeof(retryLabel), "Day %d をリトライ", currentDay);

    DrawButton(cx - 190, cy - 105, cx + 190, cy - 65, "ゲームを再開",
        sel0 ? GetColor(255, 245, 220) : GetColor(192, 202, 196),
        GetColor(255, 248, 225), fontMenu);

    DrawButton(cx - 190, cy - 55, cx + 190, cy - 15, "音量・マウス設定",
        sel1 ? GetColor(255, 245, 220) : GetColor(192, 202, 196),
        GetColor(255, 248, 225), fontMenu);

    DrawButton(cx - 190, cy - 5, cx + 190, cy + 35, retryLabel,
        sel2 ? GetColor(255, 245, 220) : GetColor(192, 202, 196),
        GetColor(255, 248, 225), fontMenu);

    DrawButton(cx - 190, cy + 45, cx + 190, cy + 85, "タイトルへ戻る",
        sel3 ? GetColor(255, 245, 220) : GetColor(192, 202, 196),
        GetColor(255, 248, 225), fontMenu);

    DrawButton(cx - 190, cy + 95, cx + 190, cy + 135, "ゲーム終了",
        sel4 ? GetColor(255, 245, 220) : GetColor(192, 202, 196),
        GetColor(255, 248, 225), fontMenu);

    // キーボード選択時のカーソル表示（等幅フォント＆シャドウ）
    int arrowY[5] = { cy - 93, cy - 43, cy + 7, cy + 57, cy + 107 };
    int arrowF = (fontCamcorder != -1) ? fontCamcorder : fontMenu;
    HorrorUI::Instance().DrawShadowString(cx - 210, arrowY[pauseMenuSelection], ">", GetColor(235, 78, 65), arrowF);

    // ガイドテキスト（日本語テキストのため日本語フォントを使用）
    const char* pauseGuide = "[ 操作方法 ] マウスまたは矢印キー / Enter で選択";
    int guideFont = HorrorUI::Instance().GetFontKeyAction();
    int gw = (guideFont != -1)
        ? GetDrawStringWidthToHandle(pauseGuide, (int)strlen(pauseGuide), guideFont)
        : GetDrawStringWidth(pauseGuide, (int)strlen(pauseGuide));
    HorrorUI::Instance().DrawJitterString(cx - gw / 2, cy + ph / 2 - 28, pauseGuide,
        GetColor(145, 160, 154), guideFont);

    DrawScanlinesAndNoise(20);
}

// クリアシーン描画
void SceneManager::DrawClearScene(const DeliveryManager& deliveryManager, const Bicycle& bicycle)
{
    (void)bicycle;
    int cx = 640;
    int cy = 360;

    // 暗転背景＋薄明かりの霧ヴィネット
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 225);
    DrawBox(0, 0, 1280, 720, GetColor(2, 4, 5), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 上部レトロVHSタイムスタンプ＆ビューファインダー枠
    char timeBuf[64];
    snprintf(timeBuf, sizeof(timeBuf), "1998.08.%02d  AM 01:00:00", 14 + currentDay - 1);
    DrawVhsCamcorderHeader("TAPE: SP  MIDNIGHT SURVEILLANCE", timeBuf, true, clearTimer);

    int subF = (fontMinchoSub != -1) ? fontMinchoSub : fontMenu;
    int bodyF = (fontReportBody != -1) ? fontReportBody : fontSubText;

    // 中央テキストUI
    if (currentDay < 5)
    {
        // 日ごとのクリア報告パネル（薄い枠＋深夜レポート風）
        HorrorUI::Instance().DrawRetroPanel(cx - 320, cy - 115, cx + 320, cy + 160,
            GetColor(68, 92, 84), GetColor(8, 12, 11), 215);

        char titleStr[64];
        snprintf(titleStr, sizeof(titleStr), "DAY %d クリア", currentDay);

        if (fontReportTitle != -1)
        {
            int tw = GetDrawStringWidthToHandle(titleStr, (int)strlen(titleStr), fontReportTitle);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 75);
            DrawStringToHandle(cx - tw / 2, cy - 84, titleStr, GetColor(125, 175, 155), fontReportTitle);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            HorrorUI::Instance().DrawShadowString(cx - tw / 2, cy - 86, titleStr, GetColor(235, 245, 240), fontReportTitle);
        }
        else
        {
            int tw = GetDrawStringWidth(titleStr, (int)strlen(titleStr));
            HorrorUI::Instance().DrawShadowString(cx - tw / 2, cy - 70, titleStr, GetColor(235, 245, 240), -1);
        }

        DrawLine(cx - 180, cy - 42, cx + 180, cy - 42, GetColor(55, 76, 68));

        char delivStr[64];
        snprintf(delivStr, sizeof(delivStr), "全 %d 軒への配達完了", deliveryManager.GetTotalDeliveries());
        int dw = (subF != -1) ? GetDrawStringWidthToHandle(delivStr, (int)strlen(delivStr), subF) : GetDrawStringWidth(delivStr, (int)strlen(delivStr));
        HorrorUI::Instance().DrawShadowString(cx - dw / 2, cy - 22, delivStr, GetColor(225, 160, 95), subF);

        const char* survStr = "夜を生き延びた。";
        int sw = (subF != -1) ? GetDrawStringWidthToHandle(survStr, (int)strlen(survStr), subF) : GetDrawStringWidth(survStr, (int)strlen(survStr));
        HorrorUI::Instance().DrawShadowString(cx - sw / 2, cy + 16, survStr, GetColor(145, 210, 175), subF);

        char nextStr[64];
        snprintf(nextStr, sizeof(nextStr), "翌日（Day %d）の配達ルートを準備中...", currentDay + 1);
        int nw = (bodyF != -1) ? GetDrawStringWidthToHandle(nextStr, (int)strlen(nextStr), bodyF) : GetDrawStringWidth(nextStr, (int)strlen(nextStr));
        HorrorUI::Instance().DrawShadowString(cx - nw / 2, cy + 60, nextStr, GetColor(175, 190, 184), bodyF);

        // 点滅プロンプト案内
        bool blink = (((int)(clearTimer * 2.5f)) % 2) == 0;
        if (blink)
        {
            const char* prompt = "[ ENTERキーで進む (または自動進行) ]";
            int pw = (bodyF != -1) ? GetDrawStringWidthToHandle(prompt, (int)strlen(prompt), bodyF) : GetDrawStringWidth(prompt, (int)strlen(prompt));
            HorrorUI::Instance().DrawShadowString(cx - pw / 2, cy + 115, prompt, GetColor(185, 215, 205), bodyF);
        }
    }
    else
    {
        // 最終日クリア：全夜生存達成レポートパネル
        HorrorUI::Instance().DrawRetroPanel(cx - 340, cy - 135, cx + 340, cy + 180,
            GetColor(145, 118, 72), GetColor(10, 12, 12), 225);

        const char* titleStr = "FINAL DAY クリア";
        if (fontReportTitle != -1)
        {
            int tw = GetDrawStringWidthToHandle(titleStr, (int)strlen(titleStr), fontReportTitle);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 85);
            DrawStringToHandle(cx - tw / 2, cy - 102, titleStr, GetColor(195, 145, 65), fontReportTitle);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            HorrorUI::Instance().DrawShadowString(cx - tw / 2, cy - 104, titleStr, GetColor(248, 224, 152), fontReportTitle);
        }
        else
        {
            int tw = GetDrawStringWidth(titleStr, (int)strlen(titleStr));
            HorrorUI::Instance().DrawShadowString(cx - tw / 2, cy - 90, titleStr, GetColor(245, 220, 150), -1);
        }

        DrawLine(cx - 200, cy - 60, cx + 200, cy - 60, GetColor(105, 86, 56));

        const char* sub1 = "全7夜 生還達成";
        int s1w = (subF != -1) ? GetDrawStringWidthToHandle(sub1, (int)strlen(sub1), subF) : GetDrawStringWidth(sub1, (int)strlen(sub1));
        HorrorUI::Instance().DrawShadowString(cx - s1w / 2, cy - 38, sub1, GetColor(145, 235, 185), subF);

        const char* sub2 = "夜が明けた。あなたは悪夢の夜を生き延びた。";
        int s2w = (bodyF != -1) ? GetDrawStringWidthToHandle(sub2, (int)strlen(sub2), bodyF) : GetDrawStringWidth(sub2, (int)strlen(sub2));
        HorrorUI::Instance().DrawShadowString(cx - s2w / 2, cy + 4, sub2, GetColor(200, 214, 208), bodyF);

        // 高視認性ホラー端末ボタン（文字色が暗すぎて潰れていた点を解消）
        DrawButton(cx - 180, cy + 60, cx + 180, cy + 100, "タイトルへ戻る [ENTER]",
            GetColor(185, 225, 202), GetColor(245, 255, 240), fontMenu);
        DrawButton(cx - 180, cy + 115, cx + 180, cy + 155, "ゲーム終了 [ESC]",
            GetColor(198, 178, 178), GetColor(255, 152, 138), fontMenu);
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
    DrawBox(0, 0, 1280, 720, GetColor(2, 1, 2), TRUE);

    // 暗赤の水平干渉帯＆アナログ砂嵐ノイズ（テレビの砂嵐演出）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 24);
    for (int b = 0; b < 3; ++b)
    {
        int by = std::rand() % 720;
        int bh = 2 + std::rand() % 10;
        DrawBox(0, by, 1280, by + bh, GetColor(145, 22, 22), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 42);
    for (int i = 0; i < 240; ++i)
    {
        int rx = std::rand() % 1280;
        int ry = std::rand() % 720;
        int rw = 2 + std::rand() % 6;
        unsigned int ncol = (std::rand() % 2 == 0) ? GetColor(195, 195, 195) : GetColor(45, 38, 38);
        DrawBox(rx, ry, rx + rw, ry + 1, ncol, TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ビューファインダー風コーナー枠
    DrawVhsCamcorderHeader("SIGNAL LOST", "PLAYBACK ERROR", false, gameOverTimer);

    // 中央ダイアログパネル
    int pw = 580;
    int ph = 350;
    HorrorUI::Instance().DrawRetroPanel(cx - pw / 2, cy - ph / 2, cx + pw / 2, cy + ph / 2,
        GetColor(120, 38, 36), GetColor(14, 7, 9), 238);

    // ゲーム終了の見出し（赤色・重厚血赤シャドウ・色収差＆激しい揺らぎ）
    int jx = (std::rand() % 5) - 2;
    int jy = (std::rand() % 3) - 1;

    char overTitle[64];
    snprintf(overTitle, sizeof(overTitle), "GAME OVER - DAY %d", currentDay);
    if (fontTitleKanji != -1)
    {
        int ow = GetDrawStringWidthToHandle(overTitle, (int)strlen(overTitle), fontTitleKanji);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 175);
        DrawStringToHandle(cx - ow / 2 + jx + 3, cy - 148 + jy + 2, overTitle, GetColor(95, 6, 8), fontTitleKanji);
        DrawStringToHandle(cx - ow / 2 - 2 + jx, cy - 148 + jy, overTitle, GetColor(45, 115, 125), fontTitleKanji);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawStringToHandle(cx - ow / 2 + jx, cy - 148 + jy, overTitle, GetColor(225, 34, 28), fontTitleKanji);
    }
    else
    {
        HorrorUI::Instance().DrawShadowString(cx - 65, cy - 135, overTitle, GetColor(210, 35, 30), -1);
    }

    const char* subOver = "CONSUMED BY DARKNESS";
    if (fontTitleEnglish != -1)
    {
        int sw = GetDrawStringWidthToHandle(subOver, (int)strlen(subOver), fontTitleEnglish);
        HorrorUI::Instance().DrawShadowString(cx - sw / 2, cy - 82, subOver, GetColor(180, 52, 48), fontTitleEnglish);
    }

    // ホラーテキスト（Jホラー明朝フォント＋シャドウで中央揃え）
    char line1[64];
    snprintf(line1, sizeof(line1), "第 %d 夜を生き延びることはできなかった...", currentDay);
    const char* line2 = "配達員は深夜の街から消息を絶った。";
    int msgF1 = (fontMinchoSub != -1) ? fontMinchoSub : fontSubText;
    int msgF2 = (fontSubText != -1) ? fontSubText : fontReportBody;
    if (msgF1 != -1)
    {
        int l1w = GetDrawStringWidthToHandle(line1, (int)strlen(line1), msgF1);
        HorrorUI::Instance().DrawShadowString(cx - l1w / 2, cy - 44, line1, GetColor(205, 125, 122), msgF1);
    }
    if (msgF2 != -1)
    {
        int l2w = GetDrawStringWidthToHandle(line2, (int)strlen(line2), msgF2);
        HorrorUI::Instance().DrawShadowString(cx - l2w / 2, cy - 16, line2, GetColor(160, 122, 122), msgF2);
    }

    // メニューボタン（マウスホバー＆キー選択両対応）
    char retryLabel[32];
    snprintf(retryLabel, sizeof(retryLabel), "Day %d をリトライ", currentDay);
    DrawButton(cx - 180, cy + 25, cx + 180, cy + 65, retryLabel,
        GetColor(235, 212, 152), GetColor(255, 245, 195), fontMenu);

    DrawButton(cx - 180, cy + 75, cx + 180, cy + 115, "タイトルへ戻る",
        GetColor(188, 178, 174), GetColor(245, 238, 232), fontMenu);

    DrawButton(cx - 180, cy + 125, cx + 180, cy + 165, "ゲーム終了",
        GetColor(155, 142, 138), GetColor(235, 185, 178), fontMenu);

    DrawScanlinesAndNoise(24);
}

// 1998 VHSカムコーダー上部オーバーレイ＆ビューファインダー枠描画
void SceneManager::DrawVhsCamcorderHeader(const char* tapeText, const char* dateText, bool recLit, float animSeed) const
{
    int monoF = (fontCamcorder != -1) ? fontCamcorder : fontSubText;

    // ビューファインダー外周四隅L字ブラケット（1998監視映像フレーム）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 110);
    unsigned int vfCol = GetColor(115, 128, 122);
    const int m = 18;
    const int len = 24;
    DrawLine(m, m, m + len, m, vfCol);
    DrawLine(m, m, m, m + len, vfCol);
    DrawLine(1280 - m - len, m, 1280 - m, m, vfCol);
    DrawLine(1280 - m, m, 1280 - m, m + len, vfCol);
    DrawLine(m, 720 - m - len, m, 720 - m, vfCol);
    DrawLine(m, 720 - m, m + len, 720 - m, vfCol);
    DrawLine(1280 - m - len, 720 - m, 1280 - m, 720 - m, vfCol);
    DrawLine(1280 - m, 720 - m - len, 1280 - m, 720 - m, vfCol);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 左上 REC ● ランプ＋ソフト赤いハロー
    unsigned int recCol = recLit ? GetColor(235, 48, 42) : GetColor(72, 22, 22);
    if (recLit)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 65);
        DrawCircle(77, 34, 8, GetColor(235, 35, 30), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    HorrorUI::Instance().DrawShadowString(36, 26, "REC ●", recCol, monoF);

    if (tapeText)
    {
        HorrorUI::Instance().DrawShadowString(125, 26, tapeText, GetColor(186, 200, 192), monoF);
    }

    if (dateText)
    {
        int dw = (monoF != -1) ? GetDrawStringWidthToHandle(dateText, (int)strlen(dateText), monoF) : GetDrawStringWidth(dateText, (int)strlen(dateText));
        HorrorUI::Instance().DrawShadowString(1244 - dw, 26, dateText, GetColor(186, 200, 192), monoF);
    }
    (void)animSeed;
}

// CRT走査線およびVHSトラッキングノイズ共通描画
void SceneManager::DrawScanlinesAndNoise(int alpha)
{
    // 極細CRT走査線
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    unsigned int sCol = GetColor(0, 0, 0);
    for (int y = 0; y < 720; y += 3)
    {
        DrawLine(0, y, 1280, y, sCol);
    }

    // ゆるやかに垂直移動する微細なVHSテープ帯揺らぎ
    int rollY = (static_cast<int>(titleTimer * 68.0f + loadingTimer * 96.0f + clearTimer * 68.0f + gameOverTimer * 68.0f)) % 720;
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, (std::max)(4, alpha / 3));
    DrawBox(0, rollY, 1280, rollY + 14, GetColor(160, 175, 168), TRUE);
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
        GetColor(82, 102, 92), GetColor(10, 14, 13), 246);

    // 3. タイトル表示（Jホラー明朝＋陰影）
    const char* titleText = "新聞配達 — 遊び方";
    if (fontPopupTitle != -1)
    {
        int tw = GetDrawStringWidthToHandle(titleText, (int)strlen(titleText), fontPopupTitle);
        HorrorUI::Instance().DrawShadowString(cx - tw / 2, y1 + 16, titleText, GetColor(238, 246, 240), fontPopupTitle);
    }
    else
    {
        int tw = GetDrawStringWidth(titleText, (int)strlen(titleText));
        HorrorUI::Instance().DrawShadowString(cx - tw / 2, y1 + 18, titleText, GetColor(235, 245, 240), -1);
    }

    const char* subTitleEn = "午前1時の配達業務マニュアル";
    if (fontPopupSub != -1)
    {
        int stw = GetDrawStringWidthToHandle(subTitleEn, (int)strlen(subTitleEn), fontPopupSub);
        HorrorUI::Instance().DrawShadowString(cx - stw / 2, y1 + 50, subTitleEn, GetColor(152, 174, 164), fontPopupSub);
    }

    // 装飾区切り線
    DrawLine(x1 + 35, y1 + 72, x2 - 35, y1 + 72, GetColor(62, 82, 74));

    // 4. 導入文
    const char* introText = "夜が明ける前に、指定された家へ新聞を届けてください。";
    if (fontPopupBody != -1)
    {
        int iw = GetDrawStringWidthToHandle(introText, (int)strlen(introText), fontPopupBody);
        HorrorUI::Instance().DrawShadowString(cx - iw / 2, y1 + 83, introText, GetColor(198, 224, 214), fontPopupBody);
    }
    else
    {
        int iw = GetDrawStringWidth(introText, (int)strlen(introText));
        HorrorUI::Instance().DrawShadowString(cx - iw / 2, y1 + 84, introText, GetColor(190, 220, 210), -1);
    }

    // 5. 説明セクション共通描画ラムダ（左端カラーアクセント付き）
    auto DrawSection = [&](int secY, int secH, const char* secTitle, const char* bodyText, const char* hintText, unsigned int titleCol) {
        int sx1 = x1 + 35;
        int sx2 = x2 - 35;
        int sy1 = secY;
        int sy2 = secY + secH;

        // セクション枠背景
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 145);
        DrawBox(sx1, sy1, sx2, sy2, GetColor(14, 19, 18), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(sx1, sy1, sx2, sy2, GetColor(52, 68, 62), FALSE);
        DrawBox(sx1 + 1, sy1 + 4, sx1 + 4, sy2 - 4, titleCol, TRUE);

        // 見出し（明確な文字サイズ差＋シャドウ）
        if (fontPopupSection != -1)
        {
            HorrorUI::Instance().DrawShadowString(sx1 + 18, sy1 + 7, secTitle, titleCol, fontPopupSection);
        }
        else
        {
            HorrorUI::Instance().DrawShadowString(sx1 + 18, sy1 + 7, secTitle, titleCol, -1);
        }

        // 本文（読みやすい白色／薄灰）
        if (fontPopupBody != -1)
        {
            HorrorUI::Instance().DrawShadowString(sx1 + 22, sy1 + 32, bodyText, GetColor(220, 230, 224), fontPopupBody);
        }
        else
        {
            HorrorUI::Instance().DrawShadowString(sx1 + 22, sy1 + 32, bodyText, GetColor(215, 225, 220), -1);
        }

        // 補足・操作方法
        if (hintText && fontPopupSub != -1)
        {
            HorrorUI::Instance().DrawShadowString(sx1 + 22, sy1 + 54, hintText, GetColor(158, 182, 174), fontPopupSub);
        }
    };

    // 説明1：新聞の配達
    DrawSection(
        y1 + 112, 78,
        "1. 新聞を配達する",
        "自転車のカゴから新聞を取り、指定されたポストへ順番に届けてください。",
        "カゴやポストを見て左クリックで新聞を取る／配達します。",
        GetColor(245, 225, 165)
    );

    // 説明2：暗闇と恐怖
    DrawSection(
        y1 + 198, 78,
        "2. 暗闇を避ける",
        "暗闇にいると恐怖度が上昇します。街灯や自転車のライトを利用して、危険から身を守ってください。",
        "安全な光の中に留まると恐怖度が低下します。",
        GetColor(238, 120, 108)
    );

    // 説明3：バッテリー
    DrawSection(
        y1 + 284, 78,
        "3. バッテリーを管理する",
        "自転車のライトはバッテリーを消費します。街中で予備バッテリーを探し、必要に応じて交換してください。",
        "落ちているバッテリーを見て左クリックで拾い、自転車の近くでRキーを押して交換します。",
        GetColor(150, 225, 195)
    );

    // 補足：ゲームオーバー条件
    const char* warnText = "※ 恐怖度が限界に達するとゲームオーバーになります。";
    if (fontPopupBody != -1)
    {
        int ww = GetDrawStringWidthToHandle(warnText, (int)strlen(warnText), fontPopupBody);
        HorrorUI::Instance().DrawShadowString(cx - ww / 2, y1 + 372, warnText, GetColor(238, 95, 88), fontPopupBody);
    }
    else
    {
        int ww = GetDrawStringWidth(warnText, (int)strlen(warnText));
        HorrorUI::Instance().DrawShadowString(cx - ww / 2, y1 + 372, warnText, GetColor(235, 105, 95), -1);
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
    unsigned int btnBorder = isHover ? GetColor(195, 72, 62) : GetColor(78, 105, 94);
    unsigned int btnBg = isHover ? GetColor(38, 16, 18) : GetColor(16, 24, 21);

    HorrorUI::Instance().DrawRetroPanel(btnX1, btnY1, btnX2, btnY2, btnBorder, btnBg, 240);

    unsigned int btnTextCol = isHover ? GetColor(255, 248, 225) : GetColor(215, 232, 222);
    if (fontPopupSection != -1)
    {
        int bw = GetDrawStringWidthToHandle(btnLabel, (int)strlen(btnLabel), fontPopupSection);
        HorrorUI::Instance().DrawShadowString(cx - bw / 2, btnY1 + 10, btnLabel, btnTextCol, fontPopupSection);
    }
    else
    {
        int bw = GetDrawStringWidth(btnLabel, (int)strlen(btnLabel));
        HorrorUI::Instance().DrawShadowString(cx - bw / 2, btnY1 + 11, btnLabel, btnTextCol, -1);
    }

    if (isHover)
    {
        int arrowF = (fontCamcorder != -1) ? fontCamcorder : fontPopupSection;
        HorrorUI::Instance().DrawShadowString(btnX1 + 12, btnY1 + 12, ">", GetColor(235, 68, 58), arrowF);
        HorrorUI::Instance().DrawShadowString(btnX2 - 22, btnY1 + 12, "<", GetColor(235, 68, 58), arrowF);
    }

    // 7. ボタン下ガイド案内
    const char* keyGuideText = (!rulePopupFromManual)
        ? "Enterキーまたはクリックで開始"
        : "Enterキー、Escキー、またはクリックで戻る";

    if (fontPopupSub != -1)
    {
        int kw = GetDrawStringWidthToHandle(keyGuideText, (int)strlen(keyGuideText), fontPopupSub);
        HorrorUI::Instance().DrawShadowString(cx - kw / 2, y1 + 458, keyGuideText, GetColor(158, 182, 172), fontPopupSub);
    }
    else
    {
        int kw = GetDrawStringWidth(keyGuideText, (int)strlen(keyGuideText));
        HorrorUI::Instance().DrawShadowString(cx - kw / 2, y1 + 458, keyGuideText, GetColor(150, 175, 165), -1);
    }

    // アナログCRT走査線エフェクト
    DrawScanlinesAndNoise(16);
}
