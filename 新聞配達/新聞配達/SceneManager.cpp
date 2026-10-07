#include "SceneManager.h"
#include "Player.h"
#include "Bicycle.h"
#include "Newspaper.h"
#include "Mailbox.h"
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

// コンストラクタ
SceneManager::SceneManager()
    : currentScene(SceneType::Title)
    , targetScene(SceneType::Title)
    , transitionState(TransitionState::None)
    , transitionTimer(0.0f)
    , transitionDuration(0.8f)
    , transitionAlpha(0.0f)
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

    isPaused = false;
    pauseMenuSelection = 0;

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
}

// ポーズ状態の切り替え
void SceneManager::SetPaused(bool paused)
{
    isPaused = paused;
    pauseMenuSelection = 0;

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

// ゲーム状態の全リセット
void SceneManager::ResetGame(
    Player& player,
    Bicycle& bicycle,
    Newspaper& newspaper,
    DeliveryManager& deliveryManager,
    NightEnvironment& nightEnv,
    WeatherManager& weatherManager,
    HorrorManager& horrorManager
)
{
    // プレイヤー位置・視線のリセット
    player.SetPosition(VGet(0.0f, 1.7f, -30.0f));
    player.SetYaw(0.0f);
    player.SetPitch(0.0f);
    player.SetCameraShakeOffset(VGet(0.0f, 0.0f, 0.0f));

    // 自転車のリセット
    bicycle.Reset();

    // 新聞のリセット
    newspaper.Reset();

    // 配達状況のリセット
    deliveryManager.Initialize();

    // 深夜環境プリセットのリセット
    nightEnv.SetPreset(EnvironmentPreset::NightDeliveryStyle);

    // 動的天候のリセット
    weatherManager.Initialize();

    // 恐怖・怪異システムのリセット
    horrorManager.Initialize();

    // UI時計を午前3時00分にリセット
    HorrorUI::Instance().SetGameTime(3, 0);

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

    // ボタン判定ラムダ式
    auto CheckButtonHit = [&](int x1, int y1, int x2, int y2) -> bool {
        return (mouseX >= x1 && mouseX <= x2 && mouseY >= y1 && mouseY <= y2 && mouseClicked);
    };

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
            }
            else
            {
                SetMouseDispFlag(TRUE);
            }
        }
        return;
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

            // タイトル画面でのシネマティックカメラ移動
            float camT = titleTimer * 0.05f;
            VECTOR camPos = VGet(-25.0f + std::sin(camT * 0.5f) * 1.5f, 2.2f, -28.0f + std::fmod(camT * 12.0f, 42.0f));
            VECTOR camTarget = VGet(-25.0f, 1.8f, camPos.z + 10.0f);
            SetCameraPositionAndTarget_UpVecY(camPos, camTarget);

            // 夜間環境・天候の背景更新
            nightEnv.Update(bicycle);
            weatherManager.Update(dt, player, bicycle, nightEnv);

            // ボタン判定（配達開始・終了）
            bool hitStart = CheckButtonHit(cx - 180, 420, cx + 180, 465);
            bool hitQuit = CheckButtonHit(cx - 180, 480, cx + 180, 525);

            if ((hitStart || enterDown || spaceDown) && !IsTransitioning())
            {
                ResetGame(player, bicycle, newspaper, deliveryManager, nightEnv, weatherManager, horrorManager);
                StartTransition(SceneType::Game, 0.8f);
            }

            if ((hitQuit || escDown) && !IsTransitioning())
            {
                outQuitRequested = true;
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
                    ResetGame(player, bicycle, newspaper, deliveryManager, nightEnv, weatherManager, horrorManager);
                    StartTransition(SceneType::Game, 0.6f);
                }
                else if (hitTitle || (enterDown && pauseMenuSelection == 2))
                {
                    SetPaused(false);
                    ResetGame(player, bicycle, newspaper, deliveryManager, nightEnv, weatherManager, horrorManager);
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

            // 夜明けのシネマティック街路カメラ移動
            float dawnT = clearTimer * 0.04f;
            VECTOR camPos = VGet(-18.0f + std::sin(dawnT) * 1.2f, 3.5f, 6.0f + std::cos(dawnT * 0.5f) * 2.0f);
            VECTOR camTarget = VGet(-25.0f, 1.8f, 12.0f);
            SetCameraPositionAndTarget_UpVecY(camPos, camTarget);

            // 夜明け環境の更新
            weatherManager.Update(dt, player, bicycle, nightEnv);

            // ボタン判定（タイトルへ戻る・終了）
            bool hitTitle = CheckButtonHit(cx - 180, cy + 155, cx + 180, cy + 195);
            bool hitQuit = CheckButtonHit(cx - 180, cy + 205, cx + 180, cy + 245);

            if (clearTimer > 1.5f)
            {
                if ((hitTitle || enterDown || spaceDown) && !IsTransitioning())
                {
                    ResetGame(player, bicycle, newspaper, deliveryManager, nightEnv, weatherManager, horrorManager);
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
                ResetGame(player, bicycle, newspaper, deliveryManager, nightEnv, weatherManager, horrorManager);
                StartTransition(SceneType::Game, 0.8f);
            }

            if ((hitTitle || enterDown || spaceDown) && !IsTransitioning())
            {
                ResetGame(player, bicycle, newspaper, deliveryManager, nightEnv, weatherManager, horrorManager);
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
            nightEnv.ApplyLightingAndFog();
            DrawGround();
            MapDraw();
            nightEnv.DrawStreetLights();
            weatherManager.DrawParticles(bicycle, nightEnv);

            // 2Dタイトル画面オーバーレイ
            DrawTitleScene(nightEnv);
        }
        break;

    case SceneType::Game:
        {
            // 3Dワールド描画
            nightEnv.ApplyLightingAndFog();
            DrawGround();
            MapDraw();
            nightEnv.DrawStreetLights();
            bicycle.Draw();
            newspaper.Draw();
            mailbox.Draw(deliveryManager);
            weatherManager.DrawParticles(bicycle, nightEnv);
            horrorManager.Draw3D();

            if (mapDebugEnabled)
            {
                MapDrawDebug(deliveryManager.GetCurrentTargetId());
            }

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
                bicycle.IsDebugFastDrain()
            );

            HorrorUI::Instance().DrawPaperCargo(
                newspaper.GetNewspaperCount(),
                newspaper.IsHolding()
            );

            HorrorUI::Instance().DrawInteractionPrompt();

            bool isAiming = bicycle.CanRide() || newspaper.CanTake() || mailbox.CanDeliver();
            HorrorUI::Instance().DrawCrosshair(isAiming);

            // 操作キー案内HUD描画（画面下部）
            HorrorUI::Instance().DrawKeyGuide(
                bicycle.IsRiding(),
                newspaper.IsHolding(),
                isAiming,
                bicycle.IsHeadlightOn(),
                mapDebugEnabled
            );

            nightEnv.DrawDebugHUD();
            weatherManager.DrawDebugHUD();
            horrorManager.DrawUI();

            if (mapDebugEnabled)
            {
                HorrorUI::Instance().DrawRetroPanel(18, 312, 310, 390,
                    HorrorUI::COL_BORDER, HorrorUI::COL_PANEL_BG, 220);

                HorrorUI::Instance().DrawJitterString(26, 318, "[ F6: MAP DEBUG ON ]",
                    GetColor(220, 180, 80), HorrorUI::Instance().GetFontSmall());

                char dbgTarget[64];
                int tid = deliveryManager.GetCurrentTargetId();
                if (tid >= 0)
                {
                    snprintf(dbgTarget, sizeof(dbgTarget), "CURRENT TARGET: HOUSE %d", tid);
                }
                else
                {
                    snprintf(dbgTarget, sizeof(dbgTarget), "CURRENT TARGET: ALL COMPLETED");
                }
                HorrorUI::Instance().DrawJitterString(26, 340, dbgTarget,
                    HorrorUI::COL_TEXT, HorrorUI::Instance().GetFontSmall());

                char dbgProg[64];
                snprintf(dbgProg, sizeof(dbgProg), "DELIVERY: %02d / %02d HOUSES",
                    deliveryManager.GetCompletedDeliveries(), deliveryManager.GetTotalDeliveries());
                HorrorUI::Instance().DrawJitterString(26, 362, dbgProg,
                    HorrorUI::COL_TEXT_DIM, HorrorUI::Instance().GetFontSmall());
            }

            DrawScanlinesAndNoise(12);

            // ポーズメニュー表示（最前面）
            if (isPaused)
            {
                DrawPauseMenu();
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
            nightEnv.DrawStreetLights();
            bicycle.Draw();
            weatherManager.DrawParticles(bicycle, nightEnv);

            // 2D配達完了報告書画面オーバーレイ
            DrawClearScene(deliveryManager);
        }
        break;

    case SceneType::GameOver:
        {
            // ゲームオーバー専用画面
            DrawGameOverScene();
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
void SceneManager::DrawTitleScene(const NightEnvironment& nightEnv)
{
    (void)nightEnv;

    int cx = 640;

    // 画面全体の薄い暗減光
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 110);
    DrawBox(0, 0, 1280, 720, GetColor(4, 6, 8), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 上部レトロVHSインフォメーション
    bool playBlink = (((int)(titleTimer * 2.0f)) % 2) == 0;
    unsigned int playCol = playBlink ? GetColor(110, 215, 160) : GetColor(60, 120, 90);
    DrawString(32, 28, "PLAY >", playCol);
    DrawString(120, 28, "SP   -0:03:00", GetColor(180, 195, 185));

    char timeBuf[64];
    snprintf(timeBuf, sizeof(timeBuf), "1998.08.14  AM 03:00:%02d", ((int)titleTimer) % 60);
    DrawString(990, 28, timeBuf, GetColor(180, 195, 185));

    // タイトルロゴの描画（色収差ジッター付き）
    int titleY = 165;
    float jitterSine = std::sin(titleTimer * 8.0f);
    int jx = (std::rand() % 3 == 0) ? (int)(jitterSine * 2.0f) : 0;

    const char* mainTitle = "NIGHT DELIVERY";
    int logoW = (fontTitleKanji != -1) ? GetDrawStringWidthToHandle(mainTitle, (int)strlen(mainTitle), fontTitleKanji) : 260;
    int logoX = cx - logoW / 2;

    // 赤・シアンの色収差シャドウとメインホラーロゴ
    if (fontTitleKanji != -1)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
        DrawStringToHandle(logoX - 2 + jx, titleY, mainTitle, GetColor(190, 35, 30), fontTitleKanji);
        DrawStringToHandle(logoX + 2 - jx, titleY, mainTitle, GetColor(35, 120, 140), fontTitleKanji);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawStringToHandle(logoX, titleY, mainTitle, GetColor(230, 235, 225), fontTitleKanji);
    }
    else
    {
        DrawString(cx - 70, titleY, mainTitle, GetColor(225, 230, 220));
    }

    // サブタイトル
    const char* subTitle = "- SHINYA HAITATSU -";
    if (fontTitleEnglish != -1)
    {
        int subW = GetDrawStringWidthToHandle(subTitle, (int)strlen(subTitle), fontTitleEnglish);
        DrawStringToHandle(cx - subW / 2, titleY + 74, subTitle, GetColor(165, 80, 75), fontTitleEnglish);
    }

    // キャッチコピー
    const char* tagline = "The silence of 3:00 AM, and the horrors in the alley.";
    if (fontSubText != -1)
    {
        int tagW = GetDrawStringWidthToHandle(tagline, (int)strlen(tagline), fontSubText);
        DrawStringToHandle(cx - tagW / 2, titleY + 115, tagline, GetColor(125, 140, 135), fontSubText);
    }

    // メニューボックス（レトロCRTパネル）
    int menuY = 405;
    int mw = 440;
    int mh = 135;
    HorrorUI::Instance().DrawRetroPanel(cx - mw / 2, menuY, cx + mw / 2, menuY + mh,
        HorrorUI::COL_BORDER, HorrorUI::COL_PANEL_BG, 200);

    // ボタンの描画（マウスホバー＆クリック対応）
    DrawButton(cx - 180, 420, cx + 180, 465, "START DELIVERY",
        GetColor(210, 220, 215), GetColor(255, 255, 240), fontMenu);

    DrawButton(cx - 180, 480, cx + 180, 525, "QUIT GAME",
        GetColor(150, 160, 155), GetColor(230, 235, 230), fontMenu);

    // 画面下部の操作ヘルプバー
    int helpY = 665;
    HorrorUI::Instance().DrawRetroPanel(26, helpY, 1254, helpY + 32,
        HorrorUI::COL_BORDER_DIM, HorrorUI::COL_BG, 180);
    HorrorUI::Instance().DrawJitterString(42, helpY + 8,
        "[ CONTROLS ]  WASD: Move  |  Mouse: Look  |  E: Ride / Dismount  |  LMB: Take / Deliver  |  F: Light  |  ESC: Pause",
        GetColor(150, 165, 160), HorrorUI::Instance().GetFontKeyGuide());

    // CRT走査線・ノイズ
    DrawScanlinesAndNoise(24);
}

// ポーズメニュー描画
void SceneManager::DrawPauseMenu()
{
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

    // タイトル
    const char* pauseTitle = "PAUSE MENU";
    if (fontReportTitle != -1)
    {
        int tw = GetDrawStringWidthToHandle(pauseTitle, (int)strlen(pauseTitle), fontReportTitle);
        DrawStringToHandle(cx - tw / 2, cy - ph / 2 + 18, pauseTitle, GetColor(215, 175, 170), fontReportTitle);
    }
    else
    {
        DrawString(cx - 45, cy - ph / 2 + 20, pauseTitle, GetColor(210, 230, 220));
    }

    DrawLine(cx - pw / 2 + 25, cy - ph / 2 + 55, cx + pw / 2 - 25, cy - ph / 2 + 55, GetColor(60, 35, 35));

    // ボタンの描画（マウスホバー＆キー選択両対応）
    bool sel0 = (pauseMenuSelection == 0);
    bool sel1 = (pauseMenuSelection == 1);
    bool sel2 = (pauseMenuSelection == 2);
    bool sel3 = (pauseMenuSelection == 3);

    DrawButton(cx - 180, cy - 80, cx + 180, cy - 38, "RESUME GAME",
        sel0 ? GetColor(255, 255, 240) : GetColor(185, 195, 190),
        GetColor(255, 255, 240), fontMenu);

    DrawButton(cx - 180, cy - 26, cx + 180, cy + 16, "RETRY DELIVERY",
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
void SceneManager::DrawClearScene(const DeliveryManager& deliveryManager)
{
    (void)deliveryManager;

    int cx = 640;
    int cy = 360;

    // 画面全体の薄明かりビネット
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
    DrawBox(0, 0, 1280, 720, GetColor(6, 12, 18), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // クリア報告パネル
    int pw = 720;
    int ph = 500;
    HorrorUI::Instance().DrawRetroPanel(cx - pw / 2, cy - ph / 2, cx + pw / 2, cy + ph / 2,
        GetColor(75, 110, 100), GetColor(12, 20, 24), 225);

    // ヘッダーバー
    int topY = cy - ph / 2 + 25;
    if (fontReportTitle != -1)
    {
        DrawStringToHandle(cx - 145, topY, "DELIVERY LOG REPORT", GetColor(180, 230, 205), fontReportTitle);
    }
    DrawString(cx + 120, topY + 6, "REC ■  04:35 AM", GetColor(120, 160, 140));

    DrawLine(cx - pw / 2 + 30, topY + 40, cx + pw / 2 - 30, topY + 40, GetColor(50, 80, 70));

    // タイプライター調テキスト表示（時間経過で順次フェードイン）
    int lineY = topY + 60;
    int lineGap = 32;

    if (clearTimer > 0.3f)
    {
        DrawStringToHandle(cx - 300, lineY, "■ SECTOR:       Midnight Residential Area (8 Houses)", GetColor(195, 205, 200), fontReportBody);
    }
    if (clearTimer > 0.8f)
    {
        DrawStringToHandle(cx - 300, lineY + lineGap, "■ DELIVERIES:   08 / 08 Houses Successfully Delivered", GetColor(120, 220, 160), fontReportBody);
    }
    if (clearTimer > 1.3f)
    {
        DrawStringToHandle(cx - 300, lineY + lineGap * 2, "■ ANOMALIES:    None Reported (Survived)", GetColor(200, 215, 200), fontReportBody);
    }
    if (clearTimer > 1.8f)
    {
        DrawStringToHandle(cx - 300, lineY + lineGap * 3, "\"The eastern sky begins to pale with the cold light of dawn...\"", GetColor(150, 185, 175), fontReportBody);
        DrawStringToHandle(cx - 300, lineY + lineGap * 4, "\"Another night survived. Morning has finally come.\"", GetColor(170, 205, 195), fontReportBody);
    }

    // 生還スタンプバナー
    if (clearTimer > 2.3f)
    {
        int stampY = lineY + lineGap * 5 + 6;
        int sw = 560;
        int sh = 40;
        DrawBox(cx - sw / 2, stampY, cx + sw / 2, stampY + sh, GetColor(18, 45, 35), TRUE);
        DrawBox(cx - sw / 2, stampY, cx + sw / 2, stampY + sh, GetColor(90, 205, 150), FALSE);

        DrawStringToHandle(cx - 200, stampY + 9, "★ ALL DELIVERIES COMPLETED - SURVIVED ★",
            GetColor(110, 240, 180), fontReportBody);
    }

    // 案内ボタン（マウス選択＆キー対応）
    if (clearTimer > 2.8f)
    {
        DrawButton(cx - 180, cy + 145, cx + 180, cy + 185, "RETURN TO TITLE",
            GetColor(210, 225, 220), GetColor(255, 255, 240), fontMenu);

        DrawButton(cx - 180, cy + 195, cx + 180, cy + 235, "QUIT GAME",
            GetColor(140, 155, 150), GetColor(230, 235, 230), fontMenu);
    }

    DrawScanlinesAndNoise(18);
}

// ゲームオーバーシーン描画
void SceneManager::DrawGameOverScene()
{
    int cx = 640;
    int cy = 360;

    // 暗黒背景
    DrawBox(0, 0, 1280, 720, GetColor(4, 3, 5), TRUE);

    // アナログ砂嵐ノイズ（テレビの砂嵐演出）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 45);
    for (int i = 0; i < 220; ++i)
    {
        int rx = std::rand() % 1280;
        int ry = std::rand() % 720;
        int rw = 2 + std::rand() % 5;
        unsigned int ncol = (std::rand() % 2 == 0) ? GetColor(190, 190, 190) : GetColor(40, 40, 40);
        DrawBox(rx, ry, rx + rw, ry + 1, ncol, TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 心拍のような赤色フラッシュ・ダーク赤ビネットパルス
    float pulse = std::sin(gameOverTimer * 3.2f);
    int pulseAlpha = 80 + (int)(pulse * 35.0f);
    if (pulseAlpha < 40) pulseAlpha = 40;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, pulseAlpha);
    for (int i = 0; i < 8; ++i)
    {
        int inset = i * 25;
        DrawBox(0, inset, 1280, inset + 25, GetColor(140, 15, 15), TRUE);
        DrawBox(0, 720 - inset - 25, 1280, 720 - inset, GetColor(140, 15, 15), TRUE);
        DrawBox(inset, 0, inset + 25, 720, GetColor(140, 15, 15), TRUE);
        DrawBox(1280 - inset - 25, 0, 1280 - inset, 720, GetColor(140, 15, 15), TRUE);
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

    const char* overTitle = "GAME OVER";
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
        DrawString(cx - 50, cy - 135, overTitle, GetColor(210, 35, 30));
    }

    const char* subOver = "CONSUMED BY DARKNESS";
    if (fontTitleEnglish != -1)
    {
        int sw = GetDrawStringWidthToHandle(subOver, (int)strlen(subOver), fontTitleEnglish);
        DrawStringToHandle(cx - sw / 2, cy - 80, subOver, GetColor(165, 45, 45), fontTitleEnglish);
    }

    // ホラーテキスト（フォントを適用して中央揃え）
    const char* line1 = "Consciousness faded into the dark...";
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
    DrawButton(cx - 180, cy + 25, cx + 180, cy + 65, "RETRY DELIVERY",
        GetColor(230, 210, 150), GetColor(255, 245, 190), fontMenu);

    DrawButton(cx - 180, cy + 75, cx + 180, cy + 115, "RETURN TO TITLE",
        GetColor(180, 170, 165), GetColor(240, 235, 230), fontMenu);

    DrawButton(cx - 180, cy + 125, cx + 180, cy + 165, "QUIT GAME",
        GetColor(130, 125, 125), GetColor(210, 205, 205), fontMenu);

    DrawScanlinesAndNoise(30);
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
