#pragma once

#include "DxLib.h"
#include "HorrorEntity.h"
#include <vector>

class Player;
class Bicycle;
class NightEnvironment;
class DeliveryManager;

// 恐怖度段階定義
enum class FearStage
{
    Calm,       // 平穏 (0〜20%): ほぼ安全、怪異は出現しない
    Uneasy,     // 警戒 (20〜40%): 暗闇の違和感、極めて稀な遠景視認
    Afraid,     // 恐怖 (40〜60%): 霧の奥に怪異のシルエットが立ち止まる
    Panic,      // 恐慌 (60〜80%): 路地裏や街角から怪異が接近
    Terror,     // 狂乱 (80〜99%): 至近距離での徘徊・街灯明滅時の急接近
    Maximum     // 極限 (100%): ゲームオーバー演出へ強制移行
};

// ゲームオーバー・ジャンプスケア状態マシン
enum class HorrorGameState
{
    Normal,         // 通常ゲームプレイ中
    Jumpscare,      // カメラ突進ジャンプスケア演出中
    GameOver        // ゲームオーバー画面（リトライ待機）
};

// ジャンプスケア内部シーケンス状態
enum class JumpscareState
{
    None,           // 非演出中
    Pause,          // フェーズ1: 前兆微小硬直（暗転・静寂・操作不能化、0.35s）
    Reveal,         // フェーズ2: 前方暗闇に怪異シルエット出現（0.40s）
    Rush,           // フェーズ3: カメラ正面への急加速突進（0.38s）
    Impact,         // フェーズ3後半: 激突・暗赤色フラッシュ・強烈な衝撃（0.22s）
    Fall,           // フェーズ4: 倒れ込み・カメラ高度急降下＆ロール傾斜（0.75s）
    GroundImpact,   // フェーズ4後半: 地面アスファルト激突の鈍い衝撃（0.20s）
    GroundedStare,  // フェーズ5: 地面横倒し視点・眼前に直立する怪異の直視（1.50s）
    VisionClosing,  // フェーズ6: 視界狭窄・最後のまばたき・暗転（2.00s）
    GameOver        // フェーズ7: ゲームオーバー待機
};

// ホラー固定出現候補地点
struct HorrorSpawnPoint
{
    int id;
    VECTOR position;
    const char* locationName;
};

// 恐怖システム・怪異出現・ゲームオーバー統合管理クラス
class HorrorManager
{
public:
    HorrorManager();
    ~HorrorManager();

    // 初期化処理
    bool Initialize();

    // 毎フレーム更新（安全光判定、恐怖度増減、怪異出現判定、ジャンプスケア演出進行、時刻・配達連携）
    void Update(float dt, Player& player, const Bicycle& bicycle, const NightEnvironment& nightEnv, const DeliveryManager* deliveryManager = nullptr);

    // 3D怪異モデルの描画
    void Draw3D();

    // 2DホラーUI・ゲームオーバー・デバッグ描画
    void DrawUI();

    // 終了処理
    void Finalize();

    // リトライ処理（恐怖度・プレイヤー位置・怪異状態のリセット）
    void RetryGame(Player& player);

    // プレイヤー操作制限フラグの取得（演出中およびゲームオーバー中）
    bool IsControlRestricted() const;

    // ジャンプスケア演出中かどうかの取得
    bool IsJumpscareActive() const { return gameState == HorrorGameState::Jumpscare; }

    // ゲームオーバー画面表示中かどうかの取得
    bool IsGameOver() const { return gameState == HorrorGameState::GameOver; }

    // 現在の恐怖度取得 (0.0f〜1.0f)
    float GetFearLevel() const { return fearLevel; }

    // 現在の恐怖段階取得
    FearStage GetFearStage() const;

    // 安全光内にいるかどうかの取得
    bool IsInSafeLight() const { return inSafeLight; }

    // 怪異モデル管理インスタンスへの参照取得
    const HorrorEntity& GetEntity() const { return entity; }

    // 時刻と配達進捗を統合した総合緊張度 (0.0f〜1.0f)
    float GetCombinedTension() const { return currentCombinedTension; }

    // 7日間難易度設定
    void SetDayDifficulty(int day);
    int GetCurrentDay() const { return currentDay; }
    float GetFearRiseMultiplier() const { return fearRiseMultiplier; }

    // デバッグHUDのON/OFF切り替え (F11)
    void ToggleDebugHUD() { showDebugHUD = !showDebugHUD; }

    // デバッグ用恐怖度強制設定
    void DebugSetFear(float level);

    // デバッグ用怪異強制出現
    void DebugTriggerSpawn(const Player& player);

    // デバッグ用視界端（周辺視野）怪異出現テスト
    void DebugTriggerPeripheralScare(const Player& player);

    // デバッグ用街灯怪異明滅・消灯イベントテスト
    void DebugTriggerStreetLightEvent(NightEnvironment& nightEnv, const Player& player);

    // デバッグ用最大恐怖ゲームオーバー即時トリガー
    void DebugTriggerMaxFearGameOver(Player& player);

private:
    // 安全光判定（自転車ヘッドライト照射・街灯直下）
    bool CheckSafeLight(const Player& player, const Bicycle& bicycle, const NightEnvironment& nightEnv);

    // 恐怖段階に応じた怪異の自動出現判定（濃霧視程に合わせた動的距離計算）
    void UpdateAutoSpawns(float dt, const Player& player, const NightEnvironment& nightEnv);

    // 視界端（周辺視野）での怪異出現・直視時消滅の更新
    void UpdatePeripheralScares(float dt, const Player& player);

    // 最大恐怖時のジャンプスケア演出開始
    void TriggerMaxFearSequence(Player& player);

    // ジャンプスケア演出の進行更新
    void UpdateJumpscareSequence(float dt, Player& player);

    // 怪異モデル実体
    HorrorEntity entity;

    // 7日間難易度スケーリングパラメータ
    int currentDay;
    float fearRiseMultiplier;
    int spawnChanceBonus;
    float entitySpeedMultiplier;

    // 恐怖度・環境緊張度パラメータ
    float fearLevel;
    bool inSafeLight;
    float safeGraceTimer;
    float currentCombinedTension;       // 時刻と配達進捗を統合した総合緊張度 (0.0f〜1.0f)

    // ゲーム状態マシン
    HorrorGameState gameState;
    JumpscareState jumpscareState;
    float scareTimer;
    float screenFadeAlpha;

    // 死亡シネマティック演出パラメータ
    float deathFallProgress;
    float deathBlinkTimer;
    float deathBlinkFactor;

    // 恐怖視覚・カメラ手続き的エフェクト変数
    float proceduralBreathTimer;
    float swayTimer;
    float currentVignetteFactor;
    float currentBlurFactor;
    float currentSwayFactor;

    // 焦点ボケ・二重像・周辺視野・街灯イベントタイマー
    float focusBlurTimer;
    float focusBlurActiveTimer;
    float doubleVisionTimer;
    float doubleVisionActiveTimer;
    float peripheralScareTimer;
    bool isPeripheralActive;
    float streetLightEventTimer;
    float headlightGlitchTimer;

    // ジャンプスケア用座標記憶
    VECTOR scareCamPos;
    VECTOR scareCamForward;

    // 出現制御
    std::vector<HorrorSpawnPoint> spawnPoints;
    float spawnCheckTimer;
    int lastSpawnId;

    // デバッグ表示フラグ
    bool showDebugHUD;
};
