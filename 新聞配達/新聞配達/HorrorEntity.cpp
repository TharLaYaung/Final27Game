#include "HorrorEntity.h"
#include <cmath>

// 候補パスからモデルを順次探索して読み込むヘルパー関数
static int LoadModelWithFallback(const char* relPath)
{
    const char* prefixes[] = {
        "",
        "新聞配達/",
        "../新聞配達/",
        "新聞配達/新聞配達/",
        "../../新聞配達/"
    };

    char fullPath[260];
    for (const char* prefix : prefixes)
    {
        snprintf(fullPath, sizeof(fullPath), "%s%s", prefix, relPath);
        int handle = MV1LoadModel(fullPath);
        if (handle != -1)
        {
            return handle;
        }
    }
    return -1;
}

// コンストラクタ
HorrorEntity::HorrorEntity()
    : idleModelHandle(-1)
    , runModelHandle(-1)
    , crawlModelHandle(-1)
    , idleAnimAttach(-1)
    , idleAnimTotal(156.0f)
    , runAnimAttach(-1)
    , runAnimTotal(16.0f)
    , crawlAnimAttach(-1)
    , crawlAnimTotal(20.0f)
    , currentModelType(HorrorModelType::Crawl)
    , animPlayTime(0.0f)
    , position(VGet(0.0f, 0.0f, 0.0f))
    , yawAngle(0.0f)
    , scale(0.01f)
    , isActive(false)
    , lifeTimer(0.0f)
    , speed(0.0f)
    , afterimageTimer(0.0f)
    , afterimagePos(VGet(0.0f, 0.0f, 0.0f))
    , afterimageYaw(0.0f)
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
    // 1. 待機・観察モデル（MonsterIdle.mv1）の読み込み
    idleModelHandle = LoadModelWithFallback("Data/Model/Monster/MonsterIdle.mv1");
    if (idleModelHandle != -1)
    {
        int animNum = MV1GetAnimNum(idleModelHandle);
        if (animNum > 1)
        {
            idleAnimAttach = MV1AttachAnim(idleModelHandle, 1, -1, FALSE);
            idleAnimTotal = MV1GetAnimTotalTime(idleModelHandle, 1);
        }
        else if (animNum == 1)
        {
            idleAnimAttach = MV1AttachAnim(idleModelHandle, 0, -1, FALSE);
            idleAnimTotal = MV1GetAnimTotalTime(idleModelHandle, 0);
        }
        MV1SetScale(idleModelHandle, VGet(scale, scale, scale));

        int matNum = MV1GetMaterialNum(idleModelHandle);
        for (int m = 0; m < matNum; ++m)
        {
            MV1SetMaterialDifColor(idleModelHandle, m, GetColorF(0.85f, 0.85f, 0.85f, 1.0f));
            MV1SetMaterialAmbColor(idleModelHandle, m, GetColorF(0.55f, 0.55f, 0.55f, 1.0f));
            MV1SetMaterialSpcColor(idleModelHandle, m, GetColorF(0.05f, 0.05f, 0.05f, 1.0f));
        }
    }

    // 2. 追跡疾走モデル（MonsterRun.mv1）の読み込み
    runModelHandle = LoadModelWithFallback("Data/Model/Monster/MonsterRun.mv1");
    if (runModelHandle != -1)
    {
        int animNum = MV1GetAnimNum(runModelHandle);
        if (animNum > 1)
        {
            runAnimAttach = MV1AttachAnim(runModelHandle, 1, -1, FALSE);
            runAnimTotal = MV1GetAnimTotalTime(runModelHandle, 1);
        }
        else if (animNum == 1)
        {
            runAnimAttach = MV1AttachAnim(runModelHandle, 0, -1, FALSE);
            runAnimTotal = MV1GetAnimTotalTime(runModelHandle, 0);
        }
        MV1SetScale(runModelHandle, VGet(scale, scale, scale));

        int matNum = MV1GetMaterialNum(runModelHandle);
        for (int m = 0; m < matNum; ++m)
        {
            MV1SetMaterialDifColor(runModelHandle, m, GetColorF(0.85f, 0.85f, 0.85f, 1.0f));
            MV1SetMaterialAmbColor(runModelHandle, m, GetColorF(0.55f, 0.55f, 0.55f, 1.0f));
            MV1SetMaterialSpcColor(runModelHandle, m, GetColorF(0.05f, 0.05f, 0.05f, 1.0f));
        }
    }

    // 3. 這いずり怪異モデル（RunningCrawl.mv1）の読み込み
    crawlModelHandle = LoadModelWithFallback("Data/Model/RunningCrawl.mv1");
    if (crawlModelHandle == -1)
    {
        crawlModelHandle = LoadModelWithFallback("Data/Model/Running Crawl.fbx");
    }

    if (crawlModelHandle != -1)
    {
        int animNum = MV1GetAnimNum(crawlModelHandle);
        if (animNum > 1)
        {
            crawlAnimAttach = MV1AttachAnim(crawlModelHandle, 1, -1, FALSE);
            crawlAnimTotal = MV1GetAnimTotalTime(crawlModelHandle, 1);
        }
        else if (animNum == 1)
        {
            crawlAnimAttach = MV1AttachAnim(crawlModelHandle, 0, -1, FALSE);
            crawlAnimTotal = MV1GetAnimTotalTime(crawlModelHandle, 0);
        }
        MV1SetScale(crawlModelHandle, VGet(scale, scale, scale));
    }

    currentModelType = HorrorModelType::Crawl;
    animPlayTime = 0.0f;
    scale = 0.01f;
    isActive = false;
    lifeTimer = 0.0f;
    speed = 0.0f;

    // 少なくとも1つモデルが読み込めていれば成功
    return (idleModelHandle != -1 || runModelHandle != -1 || crawlModelHandle != -1);
}

// モデル種別の切り替え（位置・向き・スケールは厳格に引き継がれる）
void HorrorEntity::SetModelType(HorrorModelType type)
{
    if (currentModelType == type)
    {
        return;
    }

    currentModelType = type;
}

// 現在のアクティブなモデルハンドル取得
int HorrorEntity::GetActiveModelHandle() const
{
    switch (currentModelType)
    {
    case HorrorModelType::Idle:
        return (idleModelHandle != -1) ? idleModelHandle : crawlModelHandle;
    case HorrorModelType::Run:
        return (runModelHandle != -1) ? runModelHandle : crawlModelHandle;
    case HorrorModelType::Crawl:
    default:
        return (crawlModelHandle != -1) ? crawlModelHandle : runModelHandle;
    }
}

// 現在のアクティブなアニメーションアタッチインデックス取得
int HorrorEntity::GetActiveAnimAttachIndex() const
{
    switch (currentModelType)
    {
    case HorrorModelType::Idle:
        return (idleModelHandle != -1) ? idleAnimAttach : crawlAnimAttach;
    case HorrorModelType::Run:
        return (runModelHandle != -1) ? runAnimAttach : crawlAnimAttach;
    case HorrorModelType::Crawl:
    default:
        return (crawlModelHandle != -1) ? crawlAnimAttach : runAnimAttach;
    }
}

// 現在のアクティブなアニメーション総時間取得
float HorrorEntity::GetActiveAnimTotalTime() const
{
    switch (currentModelType)
    {
    case HorrorModelType::Idle:
        return (idleModelHandle != -1) ? idleAnimTotal : crawlAnimTotal;
    case HorrorModelType::Run:
        return (runModelHandle != -1) ? runAnimTotal : crawlAnimTotal;
    case HorrorModelType::Crawl:
    default:
        return (crawlModelHandle != -1) ? crawlAnimTotal : runAnimTotal;
    }
}

// モデル種別に応じたローカル正面軸の補正オフセット取得
float HorrorEntity::GetModelForwardOffset() const
{
    if (currentModelType == HorrorModelType::Crawl)
    {
        // Crawlモデルはローカル-Zが視覚的正面のため180度補正
        return DX_PI_F;
    }
    // MonsterRunおよびMonsterIdleはローカル+Zが視覚的正面
    return 0.0f;
}

// アニメーション再生時間の更新
void HorrorEntity::AdvanceAnim(float dt, float speedMultiplier)
{
    int activeHandle = GetActiveModelHandle();
    int activeAttach = GetActiveAnimAttachIndex();
    float totalTime = GetActiveAnimTotalTime();

    if (activeHandle != -1 && activeAttach != -1 && totalTime > 0.0f)
    {
        animPlayTime += dt * 30.0f * speedMultiplier;
        if (animPlayTime >= totalTime)
        {
            animPlayTime = std::fmod(animPlayTime, totalTime);
        }
        MV1SetAttachAnimTime(activeHandle, activeAttach, animPlayTime);
    }
}

// 毎フレーム更新処理（自律進行）
void HorrorEntity::Update(float dt, const VECTOR& playerPos)
{
    // 残像タイマーのカウントダウン
    if (afterimageTimer > 0.0f)
    {
        afterimageTimer -= dt;
        if (afterimageTimer < 0.0f)
        {
            afterimageTimer = 0.0f;
        }
    }

    if (!isActive)
    {
        return;
    }

    // アニメーション再生時間の更新
    AdvanceAnim(dt, 1.0f);

    // プレイヤー方向への旋回と前進接近移動（シンプルな追従時）
    float dx = playerPos.x - position.x;
    float dz = playerPos.z - position.z;
    float dist = std::sqrt(dx * dx + dz * dz);

    if (dist > 0.01f)
    {
        float moveAngle = std::atan2(dx, dz);
        yawAngle = moveAngle + GetModelForwardOffset();

        if (speed > 0.0f && dist > 0.8f)
        {
            float step = speed * dt;
            position.x += std::sin(moveAngle) * step;
            position.z += std::cos(moveAngle) * step;
        }
    }

    // 生存時間カウントダウン
    lifeTimer -= dt;
    if (lifeTimer <= 0.0f)
    {
        Despawn();
    }
}

// 3Dモデル描画処理（実体および消失時の薄い残像）
void HorrorEntity::Draw()
{
    int activeHandle = GetActiveModelHandle();
    if (activeHandle == -1)
    {
        return;
    }

    if (isActive)
    {
        MV1SetOpacityRate(activeHandle, 1.0f);
        MV1SetPosition(activeHandle, position);
        MV1SetRotationXYZ(activeHandle, VGet(0.0f, yawAngle, 0.0f));
        MV1SetScale(activeHandle, VGet(scale, scale, scale));
        MV1DrawModel(activeHandle);
    }
    else if (afterimageTimer > 0.0f)
    {
        // 消失直後0.1〜0.3秒間、薄い残像シルエットをフェードアウト描画
        float fade = afterimageTimer / 0.25f;
        MV1SetOpacityRate(activeHandle, fade * 0.45f);
        MV1SetPosition(activeHandle, afterimagePos);
        MV1SetRotationXYZ(activeHandle, VGet(0.0f, afterimageYaw, 0.0f));
        MV1SetScale(activeHandle, VGet(scale, scale, scale));
        MV1DrawModel(activeHandle);
        MV1SetOpacityRate(activeHandle, 1.0f);
    }
}

// 指定位置への出現
void HorrorEntity::Spawn(const VECTOR& pos, float angle, float durationSeconds, float moveSpeed, HorrorModelType modelType)
{
    currentModelType = modelType;
    position = pos;
    yawAngle = angle + GetModelForwardOffset();
    lifeTimer = durationSeconds;
    speed = moveSpeed;
    animPlayTime = 0.0f;
    isActive = true;
}

// ジャンプスケア演出用の直接トランスフォーム設定
void HorrorEntity::SetDirectTransform(const VECTOR& pos, float yaw)
{
    position = pos;
    yawAngle = yaw;
    isActive = true;
}

// 特定のターゲット（カメラやプレイヤー）を正面に見据える
void HorrorEntity::SetLookAtTarget(const VECTOR& targetPos)
{
    float dx = targetPos.x - position.x;
    float dz = targetPos.z - position.z;
    if (std::sqrt(dx * dx + dz * dz) > 0.001f)
    {
        float angleToTarget = std::atan2(dx, dz);
        yawAngle = angleToTarget + GetModelForwardOffset();
    }
}

// 強制消滅（残像情報を保持）
void HorrorEntity::Despawn()
{
    if (isActive)
    {
        afterimageTimer = 0.25f;
        afterimagePos = position;
        afterimageYaw = yawAngle;
    }
    isActive = false;
    lifeTimer = 0.0f;
    speed = 0.0f;
}

// 終了処理
void HorrorEntity::Finalize()
{
    if (idleModelHandle != -1)
    {
        if (idleAnimAttach != -1) MV1DetachAnim(idleModelHandle, idleAnimAttach);
        MV1DeleteModel(idleModelHandle);
        idleModelHandle = -1;
    }
    if (runModelHandle != -1)
    {
        if (runAnimAttach != -1) MV1DetachAnim(runModelHandle, runAnimAttach);
        MV1DeleteModel(runModelHandle);
        runModelHandle = -1;
    }
    if (crawlModelHandle != -1)
    {
        if (crawlAnimAttach != -1) MV1DetachAnim(crawlModelHandle, crawlAnimAttach);
        MV1DeleteModel(crawlModelHandle);
        crawlModelHandle = -1;
    }
    isActive = false;
}
