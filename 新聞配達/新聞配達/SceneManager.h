#pragma once

#include "DxLib.h"

// 前方宣言
class Player;
class Bicycle;
class Newspaper;
class Mailbox;
class DeliveryManager;
class Minimap;
class NightEnvironment;
class WeatherManager;
class HorrorManager;

// シーン種別列挙型
enum class SceneType
{
    Title,      // タイトルシーン（午前三時の静まり返る住宅街・VHS風アナログUI）
    Game,       // ゲーム本編シーン（配達・自転車・天候・恐怖システム）
    Clear,      // 配達完了クリアシーン（全8件配達達成・夜明けの生還レポート）
    GameOver    // ゲームオーバーシーン（暗闇に呑まれた配達員・リトライ選択）
};

// トランジション（画面遷移）状態列挙型
enum class TransitionState
{
    None,       // 通常状態（トランジションなし）
    FadeOut,    // 暗転フェードアウト（グリッチノイズを伴う画面消失）
    FadeIn      // 明転フェードイン（新シーンの表示開始）
};

// シーン統合管理クラス
class SceneManager
{
public:
    SceneManager();
    ~SceneManager();

    // 初期化（フォント生成など）
    bool Initialize();

    // 終了処理（フォント解放など）
    void Finalize();

    // シーン毎フレーム更新
    void Update(
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
    );

    // シーン毎フレーム描画
    void Draw(
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
    );

    // シーン遷移の開始
    void StartTransition(SceneType target, float duration = 0.8f);

    // ゲーム状態の全リセット（リトライ・タイトル復帰用）
    void ResetGame(
        Player& player,
        Bicycle& bicycle,
        Newspaper& newspaper,
        DeliveryManager& deliveryManager,
        NightEnvironment& nightEnv,
        WeatherManager& weatherManager,
        HorrorManager& horrorManager
    );

    // 現在のシーン取得
    SceneType GetCurrentScene() const { return currentScene; }

    // トランジション中かどうか
    bool IsTransitioning() const { return transitionState != TransitionState::None; }

    // ポーズ中かどうか
    bool IsPaused() const { return isPaused; }

    // ポーズ状態の切り替え
    void SetPaused(bool paused);

private:
    SceneType currentScene;
    SceneType targetScene;
    TransitionState transitionState;
    float transitionTimer;
    float transitionDuration;
    float transitionAlpha;

    // ポーズメニュー状態
    bool isPaused;
    int pauseMenuSelection;

    // 各シーンのアニメーションタイマー
    float titleTimer;
    float clearTimer;
    float gameOverTimer;

    // マウス入力状態
    int mouseX;
    int mouseY;
    bool isMouseLeft;
    bool oldMouseLeft;
    bool mouseClicked;

    // フォントハンドル
    int fontTitleKanji;
    int fontTitleEnglish;
    int fontSubText;
    int fontMenu;
    int fontReportTitle;
    int fontReportBody;

    // キー入力エッジトリガー
    bool oldEnterKey;
    bool oldSpaceKey;
    bool oldRKey;
    bool oldEscKey;
    bool oldUpKey;
    bool oldDownKey;

    // マウスホバー・クリック対応ボタン描画ヘルパー
    bool DrawButton(int x1, int y1, int x2, int y2, const char* label, unsigned int baseCol, unsigned int hoverCol, int fontHandle = -1);

    // トランジション演出描画
    void DrawTransitionOverlay();

    // タイトルシーン描画
    void DrawTitleScene(const NightEnvironment& nightEnv);

    // クリアシーン描画
    void DrawClearScene(const DeliveryManager& deliveryManager);

    // ゲームオーバーシーン描画
    void DrawGameOverScene();

    // ポーズメニュー描画
    void DrawPauseMenu();

    // CRT走査線およびVHSノイズ共通描画
    void DrawScanlinesAndNoise(int alpha = 20);
};
