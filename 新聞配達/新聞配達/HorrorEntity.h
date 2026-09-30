#pragma once

#include "DxLib.h"

// 深夜の徘徊実体（這いずる怪異モデル）管理クラス
class HorrorEntity
{
public:
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
    void Spawn(const VECTOR& pos, float angle, float durationSeconds, float moveSpeed = 0.0f);

    // 強制消滅
    void Despawn();

    // 状態取得
    bool IsActive() const { return isActive; }
    VECTOR GetPosition() const { return position; }
    float GetRemainingTime() const { return lifeTimer; }
    float GetScale() const { return scale; }

private:
    // モデルハンドル
    int modelHandle;

    // アニメーション関連
    int animAttachIndex;
    float animTotalTime;
    float animPlayTime;

    // トランスフォーム
    VECTOR position;
    float yawAngle;
    float scale;

    // 挙動制御
    bool isActive;
    float lifeTimer;
    float speed;
};
