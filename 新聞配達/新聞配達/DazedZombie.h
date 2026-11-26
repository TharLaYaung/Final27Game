#pragma once

#include "DxLib.h"
#include <vector>

// 徘徊・錯乱ゾンビの状態定義
enum class DazedZombieState
{
    WaitingSpawn,   // 次のランダム出現待ち（非表示）
    SittingDazed,   // 座り込み茫然状態（Sitting Dazed (1).mv1）
    Scream,         // プレイヤー接近時の威嚇絶叫状態（Zombie Scream (1).mv1）
    InjuredRun,     // 負傷走行でプレイヤーへ突進追跡（Injured Run.mv1）
    Vanishing       // 突如として霧消・消失演出中
};

// マップ上にランダム出現し、接近で絶叫・負傷走行追跡ののち突然消失して別地点へ再出現する怪異
class DazedZombie
{
public:
    static constexpr float MODEL_FORWARD_OFFSET = DX_PI_F;

    DazedZombie();
    ~DazedZombie();

    // 3種類のモデル・アニメーションおよびポイントライトの読み込み・初期化
    bool Initialize();

    // 状態・トランスフォーム・ライトの完全リセット（リトライ・新日開始時）
    void Reset();

    // 毎フレーム更新（状態遷移、アニメーション進行、接近検知、追跡移動、消失・再出現）
    // 恐怖度への加算量を outFearDelta に返す
    void Update(float dt, const VECTOR& playerPos, float& outFearDelta);

    // 3Dモデル描画
    void Draw();

    // 終了処理（モデル・ライト解放）
    void Finalize();

    // デバッグ・外部参照用状態取得
    bool IsActive() const;
    bool IsVisible() const;
    DazedZombieState GetState() const { return state; }
    const char* GetStateName() const;
    VECTOR GetPosition() const { return position; }
    float GetYaw() const { return yawAngle; }
    int GetLastSpawnIndex() const { return lastSpawnIndex; }

    // シネマティック演出・ゲームオーバー時のライト一時消灯
    void SetLightsSuppressed(bool suppressed);

private:
    // 現在のステートに対応するモデルハンドルとアニメーション情報を取得
    int GetActiveModelHandle() const;
    int GetActiveAnimAttachIndex() const;
    float GetActiveAnimTotalTime() const;

    // モデルマテリアル設定ヘルパー（夜間照明・霧下での視認性調整）
    void ConfigureModelMaterials(int handle);

    // マップ上の有効な道路・路地候補から前回と異なるランダム地点を選定して出現
    bool SpawnAtRandomLocation(const VECTOR& playerPos);

    // ターゲット座標の方向へモデル正面を向ける
    void LookAtTarget(const VECTOR& targetPos, float blendRate = 1.0f);

    // 現在のアクティブモデルのループアニメーションを進める
    void AdvanceLoopingAnim(float dt, float speedMult = 1.0f);

    // 3Dモデルハンドル（Dazed / Scream / InjuredRun）
    int dazedModelHandle;
    int screamModelHandle;
    int runModelHandle;

    // アニメーションアタッチインデックス・総時間
    int dazedAnimAttach;
    float dazedAnimTotal;
    int screamAnimAttach;
    float screamAnimTotal;
    int runAnimAttach;
    float runAnimTotal;

    // 自律補助ポイントライトハンドル
    int lightHandle;
    bool lightsSuppressed;

    // 状態・タイマー
    DazedZombieState state;
    float animPlayTime;
    float stateTimer;
    float vanishOpacity;

    // トランスフォーム
    VECTOR position;
    float yawAngle;
    float scale;

    // 出現候補地点リストおよび前回出現インデックス
    std::vector<VECTOR> spawnCandidates;
    int lastSpawnIndex;

    // 怪異周辺VFX（瘴気ウィスプ・絶叫衝撃波・消失霧散パーティクル）
    struct VfxParticle
    {
        VECTOR pos;
        VECTOR vel;
        float size;
        float age;
        float maxAge;
        bool isEmber;
        bool active;
    };
    static constexpr int MAX_VFX_PARTICLES = 56;
    VfxParticle vfxParticles[MAX_VFX_PARTICLES];
    float vfxEmitTimer;
    bool shockwaveActive;
    VECTOR shockwaveCenter;
    float shockwaveAge;
    int vfxWispTex;
    int vfxRingTex;

    void EnsureVfxTextures();
    void FinalizeVfxTextures();
    void ClearVfx();
    void EmitVfxParticle(const VECTOR& p, const VECTOR& v, float size, float maxAge, bool isEmber);
    void EmitScreamAndVanishBurst(bool isVanish);
    void UpdateVfx(float dt);
    void DrawVfx();
};
