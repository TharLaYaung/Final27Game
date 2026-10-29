#include "AudioManager.h"
#pragma once

#include "DxLib.h"

// 前方宣言
class Player;
class Bicycle;
class Newspaper;
class Mailbox;
class BatteryManager;
class DeliveryManager;
class Minimap;
class NightEnvironment;
class WeatherManager;
class HorrorManager;

// シーン種別列挙型
enum class SceneType
{
    Title,      // タイトルシーン（深夜の静寂・不気味なVHS演出・ホラーメニュー）
    Game,       // ゲーム本編シーン（配達・自転車・怪異・恐怖システム）
    Clear,      // 配達クリアシーン（全配達達成・朝方の生還レポート）
    GameOver    // ゲームオーバーシーン（暗闇に呑まれた配達員・リトライ選択）
};

// タイトルサブメニュー状態列挙型
enum class TitleSubMenu
{
    Main,       // メインメニュー（配達開始・日付選択・設定・操作ガイド・終了）
    Settings,   // 音量調整・オーディオ設定（マスター・BGM・効果音）
    KeyGuide    // 操作説明・サバイバル規則ハンドブック
};

// トランジション（画面遷移）状態列挙型
enum class TransitionState
{
    None,       // 通常状態（トランジションなし）
    FadeOut,    // 暗転フェードアウト（グリッチノイズを伴う暗転）
    FadeIn      // 明転フェードイン（新シーンの表示開始）
};

// シーン統合管理クラス
class SceneManager
{
public:
    SceneManager();
    ~SceneManager();

    // 初期化（フォント・プロシージャル音声生成など）
    bool Initialize();

    // 終了処理（リソース解放）
    void Finalize();

    // 毎フレーム更新
    void Update(
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
    );

    // 毎フレーム描画
    void Draw(
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
    );

    // シーン遷移の開始
    void StartTransition(SceneType target, float duration = 0.8f);

    // ゲーム状態の全リセット（初回開始・リトライ・タイトル復帰用）
    void ResetGame(
        Player& player,
        Bicycle& bicycle,
        Newspaper& newspaper,
        BatteryManager& batteryManager,
        DeliveryManager& deliveryManager,
        NightEnvironment& nightEnv,
        WeatherManager& weatherManager,
        HorrorManager& horrorManager,
        int day = 1
    );

    // 7日間ステージ進行
    int GetCurrentDay() const { return currentDay; }
    void SetCurrentDay(int day);

    // 現在のシーン取得
    SceneType GetCurrentScene() const { return currentScene; }

    // トランジション中か
    bool IsTransitioning() const { return transitionState != TransitionState::None; }

    // ポーズ中か
    bool IsPaused() const { return isPaused; }

    // ポーズ状態の切り替え
    void SetPaused(bool paused);

    // タイトルサブメニュー制御
    TitleSubMenu GetTitleSubMenu() const { return titleSubMenu; }
    void SetTitleSubMenu(TitleSubMenu sub) { titleSubMenu = sub; }

    // 音量システム・ゲッター＆セッター
    int GetMasterVolume() const { return masterVolume; }
    int GetBgmVolume() const { return bgmVolume; }
    int GetSfxVolume() const { return sfxVolume; }

    void SetMasterVolume(int vol);
    void SetBgmVolume(int vol);
    void SetSfxVolume(int vol);

    float GetMasterVolumeRate() const { return masterVolume / 100.0f; }
    float GetBgmVolumeRate() const { return (masterVolume / 100.0f) * (bgmVolume / 100.0f); }
    float GetSfxVolumeRate() const { return (masterVolume / 100.0f) * (sfxVolume / 100.0f); }

    // サウンドエフェクト再生
    void PlayMenuClick();
    void PlayMenuAdjust();
    void PlayTestAudioPulse();

private:
    SceneType currentScene;
    SceneType targetScene;
    TransitionState transitionState;
    float transitionTimer;
    float transitionDuration;
    float transitionAlpha;

    // 7日間進行管理
    int currentDay;
    int titleSelectedDay;

    // タイトルメニュー状態管理
    TitleSubMenu titleSubMenu;
    int titleMenuSelection;
    int settingsSelection;

    // 音量システム（0〜100%）
    int masterVolume;
    int bgmVolume;
    int sfxVolume;
    float audioTestFeedbackTimer;

    // プロシージャルUI効果音ハンドル
    int soundMenuSelect;
    int soundMenuAdjust;
    int soundTestPulse;

    // ポーズメニュー管理
    bool isPaused;
    int pauseMenuSelection;

    // 各シーンのアニメーションタイマー
    float titleTimer;
    float clearTimer;
    float gameOverTimer;

    // マウス入力情報
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
    bool oldLeftKey;
    bool oldRightKey;

    // マウスホバー・クリック判定ヘルパー
    bool CheckButtonHover(int x1, int y1, int x2, int y2) const {
        return (mouseX >= x1 && mouseX <= x2 && mouseY >= y1 && mouseY <= y2);
    }
    bool CheckButtonHit(int x1, int y1, int x2, int y2) const {
        return (mouseX >= x1 && mouseX <= x2 && mouseY >= y1 && mouseY <= y2 && mouseClicked);
    }

    // マウスホバー・クリック対応ボタン描画ヘルパー
    bool DrawButton(int x1, int y1, int x2, int y2, const char* label, unsigned int baseCol, unsigned int hoverCol, int fontHandle = -1);

    // 音量スライダーバー描画ヘルパー
    void DrawVolumeSlider(int x, int y, int width, int height, int volume, const char* label, bool isSelected, bool& outDecreased, bool& outIncreased);

    // トランジション演出描画
    void DrawTransitionOverlay();

    // タイトルシーン描画（メイン・設定・キーガイド）
    void DrawTitleScene(const NightEnvironment& nightEnv);
    void DrawTitleMainMenu(int cx, float jitterSine);
    void DrawTitleSettings(int cx);
    void DrawTitleKeyGuide(int cx);

    // クリアシーン描画
    void DrawClearScene(const DeliveryManager& deliveryManager, const Bicycle& bicycle);

    // ゲームオーバーシーン描画
    void DrawGameOverScene(const DeliveryManager& deliveryManager);

    // ポーズメニュー描画
    void DrawPauseMenu(const DeliveryManager& deliveryManager);

    // CRT・VHSノイズ演出描画
    void DrawScanlinesAndNoise(int alpha = 20);
};
