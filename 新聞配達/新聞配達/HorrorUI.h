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
    DeliverNewspaper    // 配達する [ LMB ] DELIVER
};

// ホラーUI総合管理クラス
// レトロなアナログ機器・CRT・低照度ナビゲーション端末風のHUDスタイルを提供
class HorrorUI
{
public:
    static HorrorUI& Instance();

    // 初期化（フォント生成など）
    void Initialize();

    // 毎フレーム更新（フェード、ノイズ、ゲーム内時計など）
    void Update();

    // 終了処理（フォントハンドルの破棄）
    void Finalize();

    // ==========================================
    // 描画ヘルパー
    // ==========================================

    // 画面周辺のダーク減光（ビネット）描画
    void DrawScreenVignette();

    // 画面中央の最小限レティクル（照準）描画
    void DrawCrosshair(bool isTargetingInteractable);

    // インタラクト案内のフェード描画
    void DrawInteractionPrompt();

    // ゲーム内時計の描画（画面左上: 例 03:17 AM）
    void DrawClock();

    // 新聞所持数・ステータスの描画（画面下部: 例 PAPERS 07）
    void DrawPaperCargo(int paperCount, bool isHolding);

    // アナログ端末風の枠付きパネル描画
    void DrawRetroPanel(int x1, int y1, int x2, int y2, unsigned int borderColor, unsigned int bgColor, int alpha = 200);

    // ノイズ・ジッター付き文字列描画
    void DrawJitterString(int x, int y, const char* text, unsigned int color, int fontHandle = -1);

    // ==========================================
    // プロンプト・ステート設定
    // ==========================================

    // 現在フレームで要求されるプロンプトを設定
    void SetPrompt(PromptType type);

    // ゲーム内時計の設定
    void SetGameTime(int hour, int minute);
    void GetGameTime(int& hour, int& minute) const;

    // ==========================================
    // ホラー演出・グリッチAPI
    // ==========================================

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

    // ==========================================
    // カラーパレット定数
    // ==========================================
    static const unsigned int COL_BG;           // ほぼ黒・暗灰緑 (14, 18, 16)
    static const unsigned int COL_PANEL_BG;     // パネル背景 (18, 24, 20)
    static const unsigned int COL_BORDER;       // 端末フレーム枠 (65, 80, 72)
    static const unsigned int COL_BORDER_DIM;   // 暗い枠線 (40, 50, 45)
    static const unsigned int COL_TEXT;         // 汚れ感のあるオフホワイト (185, 195, 188)
    static const unsigned int COL_TEXT_DIM;     // 控えめな減衰文字 (105, 115, 110)
    static const unsigned int COL_WARNING;      // 警告・誤配 (175, 75, 65)
    static const unsigned int COL_TARGET;       // 配達先目標 (190, 95, 50)
    static const unsigned int COL_PLAYER;       // プレイヤー表示 (210, 215, 205)
    static const unsigned int COL_BIKE;         // 自転車表示 (110, 165, 175)

    // フォントハンドル取得
    int GetFontSmall() const { return fontSmall; }
    int GetFontMedium() const { return fontMedium; }
    int GetFontLarge() const { return fontLarge; }
    int GetFontClock() const { return fontClock; }

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

    // ゲーム内時計
    int clockHour;
    int clockMinute;
    int clockSecondTimer;

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
};
