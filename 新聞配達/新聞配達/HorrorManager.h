#pragma once

#include "DxLib.h"
#include "HorrorEntity.h"
#include <vector>

class Player;
class Bicycle;
class NightEnvironment;

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

// ゲームオーバー状態マシン
enum class HorrorGameState
{
    Normal,         // 通常ゲームプレイ中
    MaxFearScare,   // 最大恐怖演出（暗闇から怪異が突進、視界歪曲）
    GameOverFade,   // 暗転フェードアウト
    GameOver        // ゲームオーバー画面（リトライ待機）
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

    // 毎フレーム更新（安全光判定、恐怖度増減、怪異出現判定、ゲームオーバー進行）
    void Update(float dt, Player& player, const Bicycle& bicycle, const NightEnvironment& nightEnv);

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

    // デバッグHUDのON/OFF切り替え (F11)
    void ToggleDebugHUD() { showDebugHUD = !showDebugHUD; }

    // デバッグ用恐怖度強制設定
    void DebugSetFear(float level);

    // デバッグ用怪異強制出現
    void DebugTriggerSpawn(const Player& player);

    // デバッグ用最大恐怖ゲームオーバー即時トリガー
    void DebugTriggerMaxFearGameOver(const Player& player);

private:
    // 安全光判定（自転車ヘッドライト照射・街灯直下）
    bool CheckSafeLight(const Player& player, const Bicycle& bicycle, const NightEnvironment& nightEnv);

    // 恐怖段階に応じた怪異の自動出現判定
    void UpdateAutoSpawns(float dt, const Player& player);

    // 最大恐怖時のゲームオーバー突進演出開始
    void TriggerMaxFearSequence(const Player& player);

    // 怪異モデル実体
    HorrorEntity entity;

    // 恐怖度パラメータ
    float fearLevel;
    bool inSafeLight;
    float safeGraceTimer;

    // ゲーム状態マシン
    HorrorGameState gameState;
    float stateTimer;
    float screenFadeAlpha;

    // 出現制御
    std::vector<HorrorSpawnPoint> spawnPoints;
    float spawnCheckTimer;
    int lastSpawnId;

    // デバッグ表示フラグ
    bool showDebugHUD;
};
