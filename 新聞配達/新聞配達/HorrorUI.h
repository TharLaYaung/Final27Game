#pragma once

#include "DxLib.h"
#include <string>

// インタラクト案内プロンプトの種別
enum class PromptType
{
    None,               // 表示なし
    RideBicycle,        // 自転車に乗る [ E ] RIDE
    DismountBicycle,    // 自転車から降りる [ E ] DISMOUNT
    TakeNewspaper,      // 新聞を取る [ LMB ] TAKE
    DeliverNewspaper,   // 配達する [ LMB ] DELIVER
    TakeBattery,        // バッテリーを拾う [ LMB ] TAKE BATTERY
    BatteryFull,        // バッテリー満杯 BATTERY FULL / SPARE 2/2
    ReplaceBattery      // バッテリー交換 [ R ] REPLACE BATTERY
};

// ホラーUI総合管理クラス
// レトロなアナログ機器・CRT・低照度ナビゲーション端末風のHUDスタイルを提供
class HorrorUI
{
public:
    static HorrorUI& Instance();

    // 初期化（フォント生成など）
    void Initialize();

    // 毎フレーム更新（フェード、ノイズ、ゲーム内時計など、デルタ秒対応）
    void Update(float dt = 0.01666f);

    // 終了処理（フォントハンドルの破棄）
    void Finalize();

    // 画面周辺のダーク減光（ビネット）描画
    void DrawScreenVignette();

    // 恐怖視覚エフェクト（二重像・焦点ボケ・冷色脱色）の描画
    void DrawPostHorrorEffects();

    // ホラー視覚エフェクトパラメータ設定
    void SetHorrorVisualParams(float fear, float deathClose = 0.0f, float deathBlink = 1.0f, float doubleVision = 0.0f, float desat = 0.0f, float focusBlur = 0.0f);

    // 画面中央の最小限レティクル（照準）描画
    void DrawCrosshair(bool isTargetingInteractable);

    // インタラクト案内のフェード描画
    void DrawInteractionPrompt();

    // ゲーム内時計・日数HUDの描画（画面左上: 例 DAY 1 / 7, 01:00 AM）
    void DrawClock();

    // 夜開始時のステージ告知バナー描画（画面上部中央フェード）
    void DrawDayBanner(const char* dayName, const char* subtitle, int quota);

    // 新聞所持数・ステータスの描画（画面下部: 例 PAPERS 07）
    void DrawPaperCargo(int paperCount, bool isHolding);

    // 操作キー案内HUD描画（ゲーム画面下部）
    void DrawKeyGuide(bool isRiding, bool isHoldingPaper, bool canTakeOrDeliver, bool isHeadlightOn, bool isMapDebugOn, bool canReplaceBattery = false);

    // 操作キー案内HUDの表示/非表示トグル（Hキー対応）
    void ToggleKeyGuide() { showKeyGuide = !showKeyGuide; }
    bool IsKeyGuideVisible() const { return showKeyGuide; }
    void SetKeyGuideVisible(bool visible) { showKeyGuide = visible; }

    // 自転車ヘッドライト・バッテリーHUD描画（ミニマップ直下に配置）
    void DrawBicycleBattery(float batteryPercent, bool isLightOn, bool isRiding, bool isFastDrain, int spareBatteries = 0, int maxSpareBatteries = 2);

    // 恐怖度メーターHUD描画（画面左上、時計直下に配置）
    void DrawFearMeter(float fearPercent, bool inSafeLight);

    // 最大恐怖演出時の画面歪曲・ブラックアウト描画
    void DrawScareDistortion(float intensity);

    // 襲撃・被弾時のカメラレンズ血痕スプラッター描画
    void DrawBloodSplatter(float intensity);

    // ゲームオーバー画面描画
    void DrawGameOver(float fadeAlpha, bool isInteractive);

    // アナログ端末風の枠付きパネル描画
    void DrawRetroPanel(int x1, int y1, int x2, int y2, unsigned int borderColor, unsigned int bgColor, int alpha = 200);

    // ノイズ・ジッター付き文字列描画
    void DrawJitterString(int x, int y, const char* text, unsigned int color, int fontHandle = -1);

    // プロンプト・ステート設定

    // 現在フレームで要求されるプロンプトを設定
    void SetPrompt(PromptType type);

    // 7日間システム設定
    void SetCurrentDay(int day);
    int GetCurrentDay() const;
    void TriggerDayBanner(float duration = 4.5f);

    // ゲーム内時計の設定・進行制御
    void SetGameTime(int hour, int minute);
    void GetGameTime(int& hour, int& minute) const;
    int GetGameHour() const { return clockHour; }
    int GetGameMinute() const { return clockMinute; }
    float GetTotalGameMinutes() const { return totalGameMinutes; }
    float GetTimeScale() const { return gameMinutesPerRealSecond; }
    void SetTimeScale(float scale) { gameMinutesPerRealSecond = scale; }
    void AdvanceGameMinutes(float minutes);
    void SetTimePaused(bool paused) { isTimePaused = paused; }
    bool IsTimePaused() const { return isTimePaused; }

    // ゲーム内時刻に基づくホラー深度レベル取得 (0: 01時台, 1: 02時台, 2: 03時台, 3: 04時台以降)
    int GetHorrorTimeLevel() const;
    const char* GetHorrorTimeLevelName() const;

    // ホラー演出・グリッチAPI

    // UIグリッチを1回トリガー
    void TriggerUIGlitch(int durationFrames = 15);

    // 文字列の歪み・ブレをトリガー
    void TriggerTextDistortion(int durationFrames = 20);

    // 全体的なUI電波干渉度の設定 (0.0f〜1.0f)
    void SetUIInterference(float amount);
    float GetUIInterference() const;

    // ホラー深度レベルの設定 (0: 正常 〜 4: 深刻な故障)
    void SetHorrorIntensity(int intensity);
    int GetHorrorIntensity() const;

    // ホラー演出の有効/無効トグル（デバッグ用F4）
    void ToggleHorrorEffects();
    bool IsHorrorEffectsEnabled() const;

    // カラーパレット定数
    static const unsigned int COL_BG;           // ほぼ黒・暗灰緑 (10, 14, 12)
    static const unsigned int COL_PANEL_BG;     // パネル背景 (14, 18, 16)
    static const unsigned int COL_BORDER;       // 端末フレーム枠 (55, 72, 64)
    static const unsigned int COL_BORDER_DIM;   // 暗い枠線 (32, 42, 38)
    static const unsigned int COL_TEXT;         // 汚れ感のある幽霊的オフホワイト (175, 188, 180)
    static const unsigned int COL_TEXT_DIM;     // 控えめな減衰文字 (95, 108, 102)
    static const unsigned int COL_WARNING;      // 血液・狂気の暗赤色 (185, 45, 40)
    static const unsigned int COL_TARGET;       // 錆びたアンバーオレンジ (185, 90, 42)
    static const unsigned int COL_PLAYER;       // プレイヤー表示 (200, 210, 202)
    static const unsigned int COL_BIKE;         // 自転車表示 (90, 155, 165)

    // フォントハンドル取得
    int GetFontSmall() const { return fontSmall; }
    int GetFontMedium() const { return fontMedium; }
    int GetFontLarge() const { return fontLarge; }
    int GetFontClock() const { return fontClock; }
    int GetFontTitleHorror() const { return fontTitleHorror; }
    int GetFontKeyGuide() const { return fontKeyGuide; }
    int GetFontKeyAction() const { return fontKeyAction; }

private:
    HorrorUI();
    ~HorrorUI();
    HorrorUI(const HorrorUI&) = delete;
    HorrorUI& operator=(const HorrorUI&) = delete;

    // フォントハンドル
    int fontSmall;
    int fontMedium;
    int fontLarge;
    int fontClock;
    int fontTitleHorror;
    int fontKeyGuide;
    int fontKeyAction;

    // ゲーム内時計
    int clockHour;                      // 現在の時 (1〜)
    int clockMinute;                    // 現在の分 (0〜59)
    float totalGameMinutes;             // 01:00 AMからの通算経過ゲーム分数
    float gameMinutesPerRealSecond;     // 実時間1秒あたりのゲーム進行分数（初期値1.0f = 1分/秒）
    bool isTimePaused;                  // ゲーム内時刻の進行停止フラグ

    // プロンプト管理とフェード制御
    PromptType currentPrompt;
    PromptType requestedPrompt;
    float promptAlpha;

    // グリッチ・ホラー演出状態
    int glitchTimer;
    int jitterTimer;
    float interferenceAmount;
    int horrorIntensity;
    bool horrorEffectsEnabled;
    int frameCount;

    // 恐怖度メーター描画用補間値
    float displayedFearPercent;

    // 7日間システム状態
    int currentDay;
    float dayBannerTimer;
    float dayBannerMax;

    // 恐怖視覚エフェクト制御変数
    int ghostScreenGraph;
    int bloodSplatterGraph;
    float currentFearParam;
    float deathClosingProgress;
    float deathBlinkFactor;
    float doubleVisionAmount;
    float desaturationAmount;
    float focusBlurAmount;

    // 操作ガイドHUD表示フラグ
    bool showKeyGuide;
};
