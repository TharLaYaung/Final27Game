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
    Settings,   // 音量調整・オーディオ設定（マスター・BGM・効果音・マウス感度）
    KeyGuide    // 操作説明・サバイバル規則ハンドブック
};

// ポーズサブメニュー状態列挙型
enum class PauseSubMenu
{
    Main,       // ポーズメイン（再開・設定・リトライ・タイトル・終了）
    Settings    // ゲーム内設定（音量・マウス感度）
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
    void SetSceneImmediate(SceneType scene) { currentScene = scene; targetScene = scene; transitionState = TransitionState::None; }

    // トランジション中か
    bool IsTransitioning() const { return transitionState != TransitionState::None; }

    // ポーズ中か（ルール説明ポップアップ表示中もゲームプレイを停止するためポーズ扱いとする）
    bool IsPaused() const { return isPaused || showRulePopup; }

    // ポーズ状態の切り替え
    void SetPaused(bool paused);

    // ルール説明ポップアップの開閉および状態取得
    void OpenRulePopup(bool fromManual = false);
    void CloseRulePopup();
    bool IsRulePopupVisible() const { return showRulePopup; }

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

    // マウス感度設定（1〜100、デフォルト50）
    int GetMouseSensitivityLevel() const { return mouseSensitivityLevel; }
    void SetMouseSensitivityLevel(int lvl);
    float GetCalculatedMouseSensitivity() const { return (float)mouseSensitivityLevel * 0.00006f; }

    // ポーズサブメニュー制御
    PauseSubMenu GetPauseSubMenu() const { return pauseSubMenu; }
    void SetPauseSubMenu(PauseSubMenu sub) { pauseSubMenu = sub; }

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

    // マウス感度（1〜100、デフォルト50）
    int mouseSensitivityLevel;

    // プロシージャルUI効果音ハンドル
    int soundMenuSelect;
    int soundMenuAdjust;
    int soundTestPulse;

    // ポーズメニュー管理
    bool isPaused;
    PauseSubMenu pauseSubMenu;
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

    // 共通設定パネル（タイトル・ポーズ共用）
    void DrawSettingsPanel(int cx, int setY, const char* headerTitle, bool inPause);
    void HandleSettingsInput(bool upDown, bool downDown, bool leftDown, bool rightDown, bool enterDown, bool spaceDown, bool escDown, Player& player, bool inPause);

    // クリアシーン描画
    void DrawClearScene(const DeliveryManager& deliveryManager, const Bicycle& bicycle);

    // ゲームオーバーシーン描画
    void DrawGameOverScene(const DeliveryManager& deliveryManager);

    // ポーズメニュー描画
    void DrawPauseMenu(const DeliveryManager& deliveryManager);

    // ルール説明ポップアップ管理
    bool showRulePopup;              // ポップアップ表示中フラグ
    bool rulePopupFromManual;        // 手動ヘルプキー(H)で開かれたか
    bool hasShownInitialRulePopup;   // 初回Day1開始時の自動表示済みフラグ
    bool oldHKey;                    // Hキー前フレーム状態

    // ルール説明ポップアップ用日本語フォントハンドル
    int fontPopupTitle;
    int fontPopupSection;
    int fontPopupBody;
    int fontPopupSub;

    // ルール説明ポップアップ描画
    void DrawRulePopup();

    // CRT・VHSノイズ演出描画
    void DrawScanlinesAndNoise(int alpha = 20);
};
