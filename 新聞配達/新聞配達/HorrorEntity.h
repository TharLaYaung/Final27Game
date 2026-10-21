#pragma once

#include "DxLib.h"

// 怪異モデル種別
enum class HorrorModelType
{
    Idle,           // 待機・観察・見失い探索用（MonsterIdle.mv1）
    Run,            // 追跡疾走用（MonsterRun.mv1）
    Crawl           // 既存の這いずり怪異演出用（RunningCrawl.mv1）
};

// 深夜の徘徊・追跡怪異実体管理クラス
class HorrorEntity
{
public:
    static constexpr float MODEL_FORWARD_OFFSET = DX_PI_F;

    HorrorEntity();
    ~HorrorEntity();

    // 3Dモデル読み込みとアニメーション設定
    bool Initialize();

    // 毎フレーム更新（アニメーション進行、接近移動、生存時間減算）
    void Update(float dt, const VECTOR& playerPos);

    // 3Dモデル描画
    void Draw();

    // 終了処理
    void Finalize();

    // 指定位置への出現
    void Spawn(const VECTOR& pos, float angle, float durationSeconds, float moveSpeed = 0.0f, HorrorModelType modelType = HorrorModelType::Crawl);

    // 強制消滅
    void Despawn();

    // モデル種別の切り替え（ワールド座標・向き・スケールを完全に維持）
    void SetModelType(HorrorModelType type);
    HorrorModelType GetModelType() const { return currentModelType; }

    // ジャンプスケア演出用または追跡AI用の直接トランスフォーム設定
    void SetDirectTransform(const VECTOR& pos, float yaw);
    void SetPosition(const VECTOR& pos) { position = pos; }
    void SetYaw(float yaw) { yawAngle = yaw; }
    void SetActive(bool active) { isActive = active; }

    // アニメーション時間の強制進行
    void AdvanceAnim(float dt, float speedMultiplier = 1.0f);

    // 特定のターゲット（カメラやプレイヤー）を正面に見据える
    void SetLookAtTarget(const VECTOR& targetPos);

    // モデル種別に応じたローカル正面軸の補正オフセット取得
    float GetModelForwardOffset() const;

    // 現在のアクティブなモデルハンドルおよびアニメーション情報の取得
    int GetActiveModelHandle() const;
    int GetActiveAnimAttachIndex() const;
    float GetActiveAnimTotalTime() const;

    // 状態取得
    bool IsActive() const { return isActive; }
    VECTOR GetPosition() const { return position; }
    float GetYaw() const { return yawAngle; }
    float GetRemainingTime() const { return lifeTimer; }
    float GetScale() const { return scale; }
    float GetAfterimageTimer() const { return afterimageTimer; }

private:
    // モデルハンドル群
    int idleModelHandle;
    int runModelHandle;
    int crawlModelHandle;

    // アニメーション情報
    int idleAnimAttach;
    float idleAnimTotal;
    int runAnimAttach;
    float runAnimTotal;
    int crawlAnimAttach;
    float crawlAnimTotal;

    // 現在のモデル種別と再生時間
    HorrorModelType currentModelType;
    float animPlayTime;

    // トランスフォーム
    VECTOR position;
    float yawAngle;
    float scale;

    // 挙動制御
    bool isActive;
    float lifeTimer;
    float speed;

    // 残像（アフターイメージ）管理
    float afterimageTimer;
    VECTOR afterimagePos;
    float afterimageYaw;
};
