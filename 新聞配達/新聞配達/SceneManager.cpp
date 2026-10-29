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
    , masterVolume(80)
    , bgmVolume(70)
    , sfxVolume(85)
    , audioTestFeedbackTimer(0.0f)
    , soundMenuSelect(-1)
    , soundMenuAdjust(-1)
    , soundTestPulse(-1)
    , isPaused(false)
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
    masterVolume = 80;
    bgmVolume = 70;
    sfxVolume = 85;
    audioTestFeedbackTimer = 0.0f;
    isPaused = false;
    pauseMenuSelection = 0;

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

    // クリーピーホラーフォント生成
    // 大見出しホラーロゴ（Chiller優先、フォールバックでMS Gothicエッジ）
    fontTitleKanji = CreateFontToHandle("Chiller", 68, 4, DX_FONTTYPE_ANTIALIASING);
    if (fontTitleKanji == -1) fontTitleKanji = CreateFontToHandle("MS Gothic", 56, 6, DX_FONTTYPE_ANTIALIASING_EDGE_8X8);

    // サブタイトル
    fontTitleEnglish = CreateFontToHandle("Chiller", 28, 2, DX_FONTTYPE_ANTIALIASING);
    if (fontTitleEnglish == -1) fontTitleEnglish = CreateFontToHandle("MS Gothic", 22, 3, DX_FONTTYPE_NORMAL);

    // 説明文・キャッチコピー（Chillerホラーフォント）
    fontSubText = CreateFontToHandle("Chiller", 22, 2, DX_FONTTYPE_ANTIALIASING);
    if (fontSubText == -1) fontSubText = CreateFontToHandle("MS Gothic", 16, 2, DX_FONTTYPE_NORMAL);

    // ボタンメニュー用（Chillerホラーフォント）
    fontMenu = CreateFontToHandle("Chiller", 26, 3, DX_FONTTYPE_ANTIALIASING);
    if (fontMenu == -1) fontMenu = CreateFontToHandle("MS Gothic", 19, 2, DX_FONTTYPE_NORMAL);

    // レポート見出し
    fontReportTitle = CreateFontToHandle("Chiller", 34, 3, DX_FONTTYPE_ANTIALIASING);
    if (fontReportTitle == -1) fontReportTitle = CreateFontToHandle("MS Gothic", 24, 3, DX_FONTTYPE_NORMAL);

    // レポート本文（報告書タイプライター調）
    fontReportBody = CreateFontToHandle("Consolas", 18, 2, DX_FONTTYPE_NORMAL);
    if (fontReportBody == -1) fontReportBody = CreateFontToHandle("MS Gothic", 18, 2, DX_FONTTYPE_NORMAL);

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
            nightEnv.Update(bicycle);
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
                // 設定画面でのキーボードカーソル移動
                if (upDown)
                {
                    settingsSelection = (settingsSelection <= 0) ? 4 : settingsSelection - 1;
                    PlayMenuAdjust();
                }
                if (downDown)
                {
                    settingsSelection = (settingsSelection >= 4) ? 0 : settingsSelection + 1;
                    PlayMenuAdjust();
                }

                int setY = 160;
                int barW = 240;
                int btnW = 28;
                int barX = cx - 280 + 160;
                int sliderX = barX + btnW + 8;
                int rightBtnX = sliderX + barW + 8;

                // スライダー操作のラムダ
                auto HandleSlider = [&](int row, int& vol, int y) {
                    bool hitLeftBtn = CheckButtonHit(barX, y, barX + btnW, y + 28);
                    bool hitRightBtn = CheckButtonHit(rightBtnX, y, rightBtnX + btnW, y + 28);
                    bool hitBar = CheckButtonHit(sliderX, y + 2, sliderX + barW, y + 26);

                    bool dec = hitLeftBtn || (settingsSelection == row && leftDown);
                    bool inc = hitRightBtn || (settingsSelection == row && rightDown);

                    if (dec)
                    {
                        vol = (vol <= 5) ? 0 : vol - 5;
                        PlayMenuAdjust();
                    }
                    if (inc)
                    {
                        vol = (vol >= 95) ? 100 : vol + 5;
                        PlayMenuAdjust();
                    }
                    if (hitBar && mouseClicked)
                    {
                        int relX = mouseX - (barX + btnW + 10);
                        float rate = (float)relX / (float)barW;
                        rate = (rate < 0.0f) ? 0.0f : (rate > 1.0f ? 1.0f : rate);
                        vol = (int)(rate * 100.0f);
                        PlayMenuAdjust();
                    }
                };

                HandleSlider(0, masterVolume, setY + 45);
                HandleSlider(1, bgmVolume,    setY + 110);
                HandleSlider(2, sfxVolume,    setY + 175);

                bool hitTest = CheckButtonHit(cx - 150, setY + 245, cx + 150, setY + 285);
                bool hitBack = CheckButtonHit(cx - 150, setY + 305, cx + 150, setY + 345);

                if (hitTest) settingsSelection = 3;
                if (hitBack) settingsSelection = 4;

                if ((settingsSelection == 3 && (enterDown || spaceDown)) || hitTest)
                {
                    PlayTestAudioPulse();
                    audioTestFeedbackTimer = 0.8f;
                }
                if ((settingsSelection == 4 && (enterDown || spaceDown)) || hitBack || escDown)
                {
                    PlayMenuClick();
                    titleSubMenu = TitleSubMenu::Main;
                }
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
            // ESCキーでポーズメニュー切り替え
            if (escDown && !IsTransitioning() && !horrorManager.IsControlRestricted())
            {
                SetPaused(!isPaused);
            }

            // ポーズ中のメニュー操作
            if (isPaused)
            {
                SetMouseDispFlag(TRUE);

                if (upDown)
                {
                    pauseMenuSelection = (pauseMenuSelection + 3) % 4;
                }
                if (downDown)
                {
                    pauseMenuSelection = (pauseMenuSelection + 1) % 4;
                }

                // ポーズメニューのボタン判定
                bool hitResume = CheckButtonHit(cx - 180, cy - 80, cx + 180, cy - 38);
                bool hitRetry = CheckButtonHit(cx - 180, cy - 26, cx + 180, cy + 16);
                bool hitTitle = CheckButtonHit(cx - 180, cy + 28, cx + 180, cy + 70);
                bool hitQuit = CheckButtonHit(cx - 180, cy + 82, cx + 180, cy + 124);

                if (hitResume || (enterDown && pauseMenuSelection == 0))
                {
                    SetPaused(false);
                }
                else if (hitRetry || (enterDown && pauseMenuSelection == 1))
                {
                    SetPaused(false);
                    ResetGame(player, bicycle, newspaper, batteryManager, deliveryManager, nightEnv, weatherManager, horrorManager, currentDay);
                    StartTransition(SceneType::Game, 0.6f);
                }
                else if (hitTitle || (enterDown && pauseMenuSelection == 2))
                {
                    SetPaused(false);
                    ResetGame(player, bicycle, newspaper, batteryManager, deliveryManager, nightEnv, weatherManager, horrorManager, 1);
                    StartTransition(SceneType::Title, 0.8f);
                }
                else if (hitQuit || (enterDown && pauseMenuSelection == 3))
                {
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

            nightEnv.DrawDebugHUD();
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

            // ポーズメニュー表示（最前面）
            if (isPaused)
            {
                DrawPauseMenu(deliveryManager);
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
    const char* tagline = "The silence of 1:00 AM, and the horrors lurking in the alley.";
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
    DrawMenuRow(menuY + 20, "START DELIVERY", (titleMenuSelection == 0));

    // 2. 音声・環境設定
    DrawMenuRow(menuY + 74, "AUDIO SETTINGS", (titleMenuSelection == 1));

    // 3. 操作・サバイバルガイド
    DrawMenuRow(menuY + 128, "CONTROLS & SURVIVAL GUIDE", (titleMenuSelection == 2));

    // 4. ゲーム終了
    DrawMenuRow(menuY + 182, "QUIT GAME", (titleMenuSelection == 3));

    // 画面最下部の簡易操作インフォバー
    int helpY = 665;
    HorrorUI::Instance().DrawRetroPanel(26, helpY, 1254, helpY + 34,
        HorrorUI::COL_BORDER_DIM, HorrorUI::COL_BG, 190);
    HorrorUI::Instance().DrawJitterString(42, helpY + 9,
        "[ CONTROLS ]  WASD / Mouse: Navigate  |  ENTER / Click: Select  |  < >: Adjust Volume in Settings  |  ESC: Quit",
        GetColor(150, 165, 160), HorrorUI::Instance().GetFontKeyGuide());
}

void SceneManager::DrawTitleSettings(int cx)
{
    int setY = 160;
    int sw = 680;
    int sh = 420;

    // 設定ウィンドウパネル
    HorrorUI::Instance().DrawRetroPanel(cx - sw / 2, setY, cx + sw / 2, setY + sh,
        GetColor(90, 50, 50), GetColor(16, 14, 18), 240);

    // ヘッダータイトル
    const char* setTitle = "AUDIO & ENVIRONMENT SETTINGS";
    if (fontReportTitle != -1)
    {
        int tw = GetDrawStringWidthToHandle(setTitle, (int)strlen(setTitle), fontReportTitle);
        DrawStringToHandle(cx - tw / 2, setY + 14, setTitle, GetColor(240, 185, 140), fontReportTitle);
    }
    else
    {
        DrawString(cx - 130, setY + 16, setTitle, GetColor(240, 185, 140));
    }

    DrawLine(cx - sw / 2 + 25, setY + 48, cx + sw / 2 - 25, setY + 48, GetColor(75, 42, 42));

    // 3本のスライダーバー描画
    bool dummyDec = false, dummyInc = false;
    DrawVolumeSlider(cx - 280, setY + 65,  560, 28, masterVolume, "MASTER VOLUME", (settingsSelection == 0), dummyDec, dummyInc);
    DrawVolumeSlider(cx - 280, setY + 125, 560, 28, bgmVolume,    "BGM / AMBIENCE", (settingsSelection == 1), dummyDec, dummyInc);
    DrawVolumeSlider(cx - 280, setY + 185, 560, 28, sfxVolume,    "SFX / EFFECTS",  (settingsSelection == 2), dummyDec, dummyInc);

    // サウンドテストボタン
    int testBtnY = setY + 250;
    bool hTest = CheckButtonHover(cx - 150, testBtnY, cx + 150, testBtnY + 38);
    bool testActive = (settingsSelection == 3) || hTest;
    bool pulseFlashing = (audioTestFeedbackTimer > 0.0f);

    unsigned int testBg = pulseFlashing ? GetColor(90, 25, 25) : (testActive ? GetColor(48, 22, 24) : GetColor(24, 26, 30));
    unsigned int testBorder = testActive ? GetColor(200, 75, 65) : GetColor(60, 65, 70);
    unsigned int testTxt = pulseFlashing ? GetColor(255, 255, 200) : (testActive ? GetColor(255, 135, 110) : GetColor(190, 200, 195));

    DrawBox(cx - 150, testBtnY, cx + 150, testBtnY + 38, testBg, TRUE);
    DrawBox(cx - 150, testBtnY, cx + 150, testBtnY + 38, testBorder, FALSE);
    const char* testLabel = pulseFlashing ? "[ ! AUDIO PULSE PLAYING ! ]" : "TEST AUDIO PULSE";
    int tlw = GetDrawStringWidth(testLabel, (int)strlen(testLabel));
    DrawString(cx - tlw / 2, testBtnY + 11, testLabel, testTxt);

    // 戻るボタン
    int backBtnY = setY + 305;
    bool hBack = CheckButtonHover(cx - 150, backBtnY, cx + 150, backBtnY + 38);
    bool backActive = (settingsSelection == 4) || hBack;

    DrawBox(cx - 150, backBtnY, cx + 150, backBtnY + 38, backActive ? GetColor(50, 22, 24) : GetColor(24, 26, 30), TRUE);
    DrawBox(cx - 150, backBtnY, cx + 150, backBtnY + 38, backActive ? GetColor(200, 75, 65) : GetColor(60, 65, 70), FALSE);
    const char* backLabel = "BACK TO MENU [ESC]";
    int blw = GetDrawStringWidth(backLabel, (int)strlen(backLabel));
    DrawString(cx - blw / 2, backBtnY + 11, backLabel, backActive ? GetColor(255, 135, 110) : GetColor(190, 200, 195));

    // ガイド注釈
    const char* guide = "Use Arrow Keys < > to Adjust, ENTER to Test, ESC to Return";
    int gw = GetDrawStringWidth(guide, (int)strlen(guide));
    DrawString(cx - gw / 2, setY + sh - 28, guide, GetColor(120, 135, 130));
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
    const char* guideTitle = "FIELD COURIER HANDBOOK - 1:00 AM PROTOCOL";
    if (fontReportTitle != -1)
    {
        int tw = GetDrawStringWidthToHandle(guideTitle, (int)strlen(guideTitle), fontReportTitle);
        DrawStringToHandle(cx - tw / 2, py + 14, guideTitle, GetColor(240, 190, 130), fontReportTitle);
    }
    else
    {
        DrawString(cx - 180, py + 16, guideTitle, GetColor(240, 190, 130));
    }

    DrawLine(cx - pw / 2 + 25, py + 48, cx + pw / 2 - 25, py + 48, GetColor(75, 42, 42));
    DrawLine(cx - 10, py + 58, cx - 10, py + ph - 65, GetColor(50, 35, 38));

    // 第1列：キーバインド
    int col1X = cx - pw / 2 + 35;
    DrawString(col1X, py + 58, "[ 1. KEY BINDINGS & CONTROLS ]", GetColor(240, 200, 140));

    auto DrawKeyRow = [&](int y, const char* key, const char* desc) {
        DrawBox(col1X, y, col1X + 130, y + 25, GetColor(30, 34, 38), TRUE);
        DrawBox(col1X, y, col1X + 130, y + 25, GetColor(70, 75, 80), FALSE);
        DrawString(col1X + 10, y + 5, key, GetColor(255, 230, 180));
        DrawString(col1X + 145, y + 5, desc, GetColor(190, 200, 195));
    };

    DrawKeyRow(py + 90,  "W / A / S / D", "Move / Steer Bicycle");
    DrawKeyRow(py + 122, "MOUSE",         "Look / Aim Flashlight");
    DrawKeyRow(py + 154, "LEFT CLICK",    "Deliver Paper / Take Item");
    DrawKeyRow(py + 186, "E KEY",         "Ride / Dismount Bicycle");
    DrawKeyRow(py + 218, "F KEY",         "Bicycle Headlight ON/OFF");
    DrawKeyRow(py + 250, "R KEY",         "Replace Battery from Spare");
    DrawKeyRow(py + 282, "SPACE",         "Bicycle Bell / Brake");
    DrawKeyRow(py + 314, "SHIFT",         "Sprint (Consumes Stamina)");
    DrawKeyRow(py + 346, "ESC KEY",       "Pause Menu / Dispatch Log");
    DrawKeyRow(py + 378, "< > ARROWS",    "Adjust Volume in Settings");
    DrawKeyRow(py + 410, "H / TAB",       "Toggle Control Guide");

    // 第2列：深夜配達サバイバル指示（枠内に完全に収まる文字幅設計）
    int col2X = cx + 15;
    DrawString(col2X, py + 58, "[ 2. MIDNIGHT SURVIVAL DIRECTIVES ]", GetColor(240, 110, 95));

    auto DrawRule = [&](int y, const char* title, const char* desc, const char* desc2) {
        DrawString(col2X, y, title, GetColor(230, 90, 75));
        DrawString(col2X + 10, y + 18, desc, GetColor(175, 185, 180));
        if (desc2) DrawString(col2X + 10, y + 34, desc2, GetColor(140, 155, 150));
    };

    DrawRule(py + 90,  "1. SAFE LIGHT (STREET LAMPS)",
        "- Street lights repel entities (safety radius 6.5m).",
        "- If a lamp flickers, prepare to leave immediately.");

    DrawRule(py + 152, "2. BICYCLE HEADLIGHT & BATTERY",
        "- Riding with battery > 0% shields against attacks.",
        "- Battery at 0% leaves you exposed to fatal death.");

    DrawRule(py + 214, "3. FEAR & THE CRAWL MONSTER",
        "- Fear rises in darkness; at 100%, Crawl strikes.",
        "- Face street lights to lower your Fear Meter.");

    DrawRule(py + 276, "4. HALFWAY HUNT (DELIVERY 4/8)",
        "- Halfway delivered, Chasing Monster begins hunt.",
        "- Stay in light; monster faces you when chasing.");

    DrawRule(py + 338, "5. SPARE BATTERIES LIMIT (MAX 2)",
        "- You can carry up to 2 spare batteries.",
        "- Press [R] near bicycle to reload 100% battery.");

    // 戻るボタン
    int btnY = py + ph - 52;
    bool hBack = CheckButtonHover(cx - 140, btnY, cx + 140, btnY + 36);

    DrawBox(cx - 140, btnY, cx + 140, btnY + 36, hBack ? GetColor(50, 22, 24) : GetColor(25, 28, 30), TRUE);
    DrawBox(cx - 140, btnY, cx + 140, btnY + 36, hBack ? GetColor(200, 75, 65) : GetColor(65, 70, 75), FALSE);
    const char* backTxt = "RETURN TO MENU [ESC]";
    int bw = GetDrawStringWidth(backTxt, (int)strlen(backTxt));
    DrawString(cx - bw / 2, btnY + 10, backTxt, hBack ? GetColor(255, 135, 110) : GetColor(190, 200, 195));
}

void SceneManager::DrawPauseMenu(const DeliveryManager& deliveryManager)
{
    (void)deliveryManager;

    int cx = 640;
    int cy = 360;

    // 暗転オーバーレイ
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
    DrawBox(0, 0, 1280, 720, GetColor(5, 8, 10), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ダイアログパネル
    int pw = 460;
    int ph = 350;
    HorrorUI::Instance().DrawRetroPanel(cx - pw / 2, cy - ph / 2, cx + pw / 2, cy + ph / 2,
        GetColor(75, 45, 45), GetColor(16, 12, 14), 240);

    // タイトル（現在の日数を明記）
    char pauseTitle[64];
    snprintf(pauseTitle, sizeof(pauseTitle), "PAUSE MENU - DAY %d / 7", currentDay);
    if (fontReportTitle != -1)
    {
        int tw = GetDrawStringWidthToHandle(pauseTitle, (int)strlen(pauseTitle), fontReportTitle);
        DrawStringToHandle(cx - tw / 2, cy - ph / 2 + 18, pauseTitle, GetColor(215, 175, 170), fontReportTitle);
    }
    else
    {
        DrawString(cx - 65, cy - ph / 2 + 20, pauseTitle, GetColor(210, 230, 220));
    }

    DrawLine(cx - pw / 2 + 25, cy - ph / 2 + 55, cx + pw / 2 - 25, cy - ph / 2 + 55, GetColor(60, 35, 35));

    // ボタンの描画（マウスホバー＆キー選択両対応）
    bool sel0 = (pauseMenuSelection == 0);
    bool sel1 = (pauseMenuSelection == 1);
    bool sel2 = (pauseMenuSelection == 2);
    bool sel3 = (pauseMenuSelection == 3);

    char retryLabel[32];
    snprintf(retryLabel, sizeof(retryLabel), "RETRY DAY %d", currentDay);

    DrawButton(cx - 180, cy - 80, cx + 180, cy - 38, "RESUME GAME",
        sel0 ? GetColor(255, 255, 240) : GetColor(185, 195, 190),
        GetColor(255, 255, 240), fontMenu);

    DrawButton(cx - 180, cy - 26, cx + 180, cy + 16, retryLabel,
        sel1 ? GetColor(255, 255, 240) : GetColor(185, 195, 190),
        GetColor(255, 255, 240), fontMenu);

    DrawButton(cx - 180, cy + 28, cx + 180, cy + 70, "RETURN TO TITLE",
        sel2 ? GetColor(255, 255, 240) : GetColor(185, 195, 190),
        GetColor(255, 255, 240), fontMenu);

    DrawButton(cx - 180, cy + 82, cx + 180, cy + 124, "QUIT GAME",
        sel3 ? GetColor(255, 255, 240) : GetColor(185, 195, 190),
        GetColor(255, 255, 240), fontMenu);

    // キーボード選択時のカーソル表示
    int arrowY[4] = { cy - 65, cy - 11, cy + 43, cy + 97 };
    DrawString(cx - 198, arrowY[pauseMenuSelection], ">", GetColor(220, 240, 230));

    // ガイドテキスト
    const char* pauseGuide = "[ CONTROLS ] Click with Mouse or use Arrow Keys / ENTER";
    int gw = GetDrawStringWidthToHandle(pauseGuide, (int)strlen(pauseGuide), HorrorUI::Instance().GetFontKeyGuide());
    HorrorUI::Instance().DrawJitterString(cx - gw / 2, cy + ph / 2 - 28, pauseGuide,
        GetColor(120, 140, 135), HorrorUI::Instance().GetFontKeyGuide());

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
        snprintf(titleStr, sizeof(titleStr), "DAY %d COMPLETED", currentDay);

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
        snprintf(delivStr, sizeof(delivStr), "ALL %d HOUSES DELIVERED", deliveryManager.GetTotalDeliveries());
        int dw = GetDrawStringWidth(delivStr, (int)strlen(delivStr));
        DrawString(cx - dw / 2, cy - 15, delivStr, GetColor(215, 150, 90));

        const char* survStr = "SURVIVED THE NIGHT.";
        int sw = GetDrawStringWidth(survStr, (int)strlen(survStr));
        DrawString(cx - sw / 2, cy + 20, survStr, GetColor(140, 195, 165));

        char nextStr[64];
        snprintf(nextStr, sizeof(nextStr), "Preparing Day %d Route...", currentDay + 1);
        int nw = GetDrawStringWidth(nextStr, (int)strlen(nextStr));
        DrawString(cx - nw / 2, cy + 60, nextStr, GetColor(165, 180, 175));

        // 点滅プロンプト案内
        bool blink = (((int)(clearTimer * 2.5f)) % 2) == 0;
        if (blink)
        {
            const char* prompt = "[ PRESS ENTER OR WAIT TO PROCEED ]";
            int pw = GetDrawStringWidth(prompt, (int)strlen(prompt));
            DrawString(cx - pw / 2, cy + 125, prompt, GetColor(180, 210, 200));
        }
    }
    else
    {
        // 最終7日目クリア：全7夜生存達成テキストUI
        const char* titleStr = "FINAL DAY COMPLETED";
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

        const char* sub1 = "ALL 7 NIGHTS SURVIVED";
        int s1w = GetDrawStringWidth(sub1, (int)strlen(sub1));
        DrawString(cx - s1w / 2, cy - 35, sub1, GetColor(140, 235, 185));

        const char* sub2 = "The dawn finally breaks. You made it through the nightmare alive.";
        int s2w = GetDrawStringWidth(sub2, (int)strlen(sub2));
        DrawString(cx - s2w / 2, cy + 5, sub2, GetColor(190, 205, 200));

        // ボタン
        DrawButton(cx - 180, cy + 60, cx + 180, cy + 100, "RETURN TO TITLE [ENTER]",
            GetColor(35, 75, 65), GetColor(60, 130, 110));
        DrawButton(cx - 180, cy + 115, cx + 180, cy + 155, "QUIT GAME [ESC]",
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
    snprintf(line1, sizeof(line1), "Failed to survive Night %d of 7...", currentDay);
    const char* line2 = "The delivery courier went missing in the midnight streets.";
    if (fontSubText != -1)
    {
        int l1w = GetDrawStringWidthToHandle(line1, (int)strlen(line1), fontSubText);
        int l2w = GetDrawStringWidthToHandle(line2, (int)strlen(line2), fontSubText);
        DrawStringToHandle(cx - l1w / 2, cy - 42, line1, GetColor(185, 120, 120), fontSubText);
        DrawStringToHandle(cx - l2w / 2, cy - 18, line2, GetColor(150, 115, 115), fontSubText);
    }
    else
    {
        DrawString(cx - 105, cy - 40, line1, GetColor(190, 110, 110));
        DrawString(cx - 165, cy - 15, line2, GetColor(140, 110, 110));
    }

    // メニューボタン（マウスホバー＆キー選択両対応）
    char retryLabel[32];
    snprintf(retryLabel, sizeof(retryLabel), "RETRY DAY %d", currentDay);
    DrawButton(cx - 180, cy + 25, cx + 180, cy + 65, retryLabel,
        GetColor(230, 210, 150), GetColor(255, 245, 190), fontMenu);

    DrawButton(cx - 180, cy + 75, cx + 180, cy + 115, "RETURN TO TITLE",
        GetColor(180, 170, 165), GetColor(240, 235, 230), fontMenu);

    DrawButton(cx - 180, cy + 125, cx + 180, cy + 165, "QUIT GAME",
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
