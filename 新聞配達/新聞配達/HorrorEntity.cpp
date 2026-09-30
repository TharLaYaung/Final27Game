#include "HorrorEntity.h"
#include <cmath>

// コンストラクタ
HorrorEntity::HorrorEntity()
    : modelHandle(-1)
    , animAttachIndex(-1)
    , animTotalTime(20.0f)
    , animPlayTime(0.0f)
    , position(VGet(0.0f, 0.0f, 0.0f))
    , yawAngle(0.0f)
    , scale(0.01f)
    , isActive(false)
    , lifeTimer(0.0f)
    , speed(0.0f)
{
}

// デストラクタ
HorrorEntity::~HorrorEntity()
{
    Finalize();
}

// 初期化処理
bool HorrorEntity::Initialize()
{
    // プロジェクト内に配置済みの怪異モデルを読み込み
    modelHandle = MV1LoadModel("Data/Model/RunningCrawl.mv1");
    if (modelHandle == -1)
    {
        modelHandle = MV1LoadModel("Data/Model/Running Crawl.fbx");
        if (modelHandle == -1)
        {
            return false;
        }
    }

    // 這いずり疾走アニメーション（mixamo.com / index 1）をアタッチ
    int animNum = MV1GetAnimNum(modelHandle);
    if (animNum > 1)
    {
        animAttachIndex = MV1AttachAnim(modelHandle, 1, -1, FALSE);
        animTotalTime = MV1GetAnimTotalTime(modelHandle, 1);
    }
    else if (animNum == 1)
    {
        animAttachIndex = MV1AttachAnim(modelHandle, 0, -1, FALSE);
        animTotalTime = MV1GetAnimTotalTime(modelHandle, 0);
    }

    animPlayTime = 0.0f;
    scale = 0.01f;
    MV1SetScale(modelHandle, VGet(scale, scale, scale));

    isActive = false;
    lifeTimer = 0.0f;
    speed = 0.0f;

    return true;
}

// 毎フレーム更新処理
void HorrorEntity::Update(float dt, const VECTOR& playerPos)
{
    if (!isActive || modelHandle == -1)
    {
        return;
    }

    // アニメーション再生時間の更新
    if (animAttachIndex != -1 && animTotalTime > 0.0f)
    {
        animPlayTime += dt * 30.0f;
        if (animPlayTime >= animTotalTime)
        {
            animPlayTime = std::fmod(animPlayTime, animTotalTime);
        }
        MV1SetAttachAnimTime(modelHandle, animAttachIndex, animPlayTime);
    }

    // プレイヤー方向への旋回と追従接近移動
    float dx = playerPos.x - position.x;
    float dz = playerPos.z - position.z;
    float dist = std::sqrt(dx * dx + dz * dz);

    if (dist > 0.01f)
    {
        yawAngle = std::atan2(dx, dz);
    }

    if (speed > 0.0f && dist > 0.8f)
    {
        float step = speed * dt;
        position.x += std::sin(yawAngle) * step;
        position.z += std::cos(yawAngle) * step;
    }

    // 生存時間カウントダウン
    lifeTimer -= dt;
    if (lifeTimer <= 0.0f)
    {
        Despawn();
    }
}

// 3Dモデル描画処理
void HorrorEntity::Draw()
{
    if (!isActive || modelHandle == -1)
    {
        return;
    }

    MV1SetPosition(modelHandle, position);
    MV1SetRotationXYZ(modelHandle, VGet(0.0f, yawAngle, 0.0f));
    MV1SetScale(modelHandle, VGet(scale, scale, scale));

    MV1DrawModel(modelHandle);
}

// 指定位置への出現
void HorrorEntity::Spawn(const VECTOR& pos, float angle, float durationSeconds, float moveSpeed)
{
    position = pos;
    yawAngle = angle;
    lifeTimer = durationSeconds;
    speed = moveSpeed;
    animPlayTime = 0.0f;
    isActive = true;
}

// 強制消滅
void HorrorEntity::Despawn()
{
    isActive = false;
    lifeTimer = 0.0f;
    speed = 0.0f;
}

// 終了処理
void HorrorEntity::Finalize()
{
    if (modelHandle != -1)
    {
        if (animAttachIndex != -1)
        {
            MV1DetachAnim(modelHandle, animAttachIndex);
            animAttachIndex = -1;
        }
        MV1DeleteModel(modelHandle);
        modelHandle = -1;
    }
    isActive = false;
}
