#include "HorrorManager.h"
#include "Player.h"
#include "Bicycle.h"
#include "NightEnvironment.h"
#include "DeliveryManager.h"
#include "HorrorUI.h"
#include "WorldCollision.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

// コンストラクタ
HorrorManager::HorrorManager()
    : currentDay(1)
    , fearRiseMultiplier(1.0f)
    , spawnChanceBonus(0)
    , entitySpeedMultiplier(1.0f)
    , isChaseUnlocked(false)
    , chaseState(MonsterChaseState::Dormant)
    , lastKnownPlayerPos(VGet(0.0f, 0.0f, 0.0f))
    , lostSightTimer(0.0f)
    , chaseCooldownTimer(0.0f)
    , campingTimer(0.0f)
    , searchStateTimer(0.0f)
    , currentSearchNodeIndex(0)
    , darknessTimeAccumulator(0.0f)
    , cachedBicycleBattery(100.0f)
    , cachedMonsterDist(-1.0f)
    , playerWasVisible(false)
    , lastLOSState(false)
    , lastSafeReason("NONE")
    , currentNavTargetNodeId(-1)
    , fearLevel(0.0f)
    , inSafeLight(true)
    , safeGraceTimer(0.8f)
    , currentCombinedTension(0.0f)
    , gameState(HorrorGameState::Normal)
    , jumpscareState(JumpscareState::None)
    , scareTimer(0.0f)
    , screenFadeAlpha(0.0f)
    , deathFallProgress(0.0f)
    , deathBlinkTimer(0.0f)
    , deathBlinkFactor(1.0f)
    , proceduralBreathTimer(0.0f)
    , swayTimer(0.0f)
    , currentVignetteFactor(0.0f)
    , currentBlurFactor(0.0f)
    , currentSwayFactor(0.0f)
    , focusBlurTimer(15.0f)
    , focusBlurActiveTimer(0.0f)
    , doubleVisionTimer(20.0f)
    , doubleVisionActiveTimer(0.0f)
    , peripheralScareTimer(25.0f)
    , isPeripheralActive(false)
    , streetLightEventTimer(20.0f)
    , headlightGlitchTimer(18.0f)
    , scareCamPos(VGet(0.0f, 0.0f, 0.0f))
    , scareCamForward(VGet(0.0f, 0.0f, 1.0f))
    , spawnCheckTimer(10.0f)
    , lastSpawnId(-1)
    , showDebugHUD(false)
{
}

// デストラクタ
HorrorManager::~HorrorManager()
{
    Finalize();
}

// ナビゲーションノードの初期化（街路・交差点・角地）
void HorrorManager::InitializeNavNodes()
{
    navNodes.clear();

    // 南大通り（Z = -27）
    navNodes.push_back({ 0, VGet(-26.0f, 0.0f, -27.0f) });
    navNodes.push_back({ 1, VGet(-14.0f, 0.0f, -27.0f) });
    navNodes.push_back({ 2, VGet( -2.0f, 0.0f, -27.0f) });
    navNodes.push_back({ 3, VGet( 10.0f, 0.0f, -27.0f) });
    navNodes.push_back({ 4, VGet( 24.0f, 0.0f, -27.0f) });

    // 南南北路地・角地
    navNodes.push_back({ 5, VGet(-23.0f, 0.0f, -18.0f) });
    navNodes.push_back({ 6, VGet(  0.0f, 0.0f, -18.0f) });
    navNodes.push_back({ 7, VGet( 23.0f, 0.0f, -18.0f) });

    // 中央通り（Z = -8）
    navNodes.push_back({ 8, VGet(-26.0f, 0.0f, -8.0f) });
    navNodes.push_back({ 9, VGet(-12.0f, 0.0f, -8.0f) });
    navNodes.push_back({ 10, VGet(  0.0f, 0.0f, -8.0f) });
    navNodes.push_back({ 11, VGet( 12.0f, 0.0f, -8.0f) });
    navNodes.push_back({ 12, VGet( 26.0f, 0.0f, -8.0f) });

    // 中北路地・交差点
    navNodes.push_back({ 13, VGet(-23.0f, 0.0f, 3.0f) });
    navNodes.push_back({ 14, VGet(  0.0f, 0.0f, 3.0f) });
    navNodes.push_back({ 15, VGet( 23.0f, 0.0f, 3.0f) });

    // 北通り（Z = 14）
    navNodes.push_back({ 16, VGet(-26.0f, 0.0f, 14.0f) });
    navNodes.push_back({ 17, VGet(-12.0f, 0.0f, 14.0f) });
    navNodes.push_back({ 18, VGet(  0.0f, 0.0f, 14.0f) });
    navNodes.push_back({ 19, VGet( 12.0f, 0.0f, 14.0f) });
    navNodes.push_back({ 20, VGet( 26.0f, 0.0f, 14.0f) });

    // 北袋小路（邸宅2前、Z = 26〜38）
    navNodes.push_back({ 21, VGet(-6.0f, 0.0f, 26.0f) });
    navNodes.push_back({ 22, VGet( 0.0f, 0.0f, 26.0f) });
    navNodes.push_back({ 23, VGet( 6.0f, 0.0f, 26.0f) });
    navNodes.push_back({ 24, VGet( 0.0f, 0.0f, 37.0f) });

    // 外周側歩道
    navNodes.push_back({ 25, VGet(-31.0f, 0.0f, 0.0f) });
    navNodes.push_back({ 26, VGet( 31.0f, 0.0f, 0.0f) });
    navNodes.push_back({ 27, VGet(-31.0f, 0.0f, -25.0f) });
    navNodes.push_back({ 28, VGet( 31.0f, 0.0f, -25.0f) });
}

// 7日間難易度設定
void HorrorManager::SetDayDifficulty(int day)
{
    if (day < 1) day = 1;
    if (day > 7) day = 7;
    currentDay = day;

    switch (day)
    {
    case 1:
        fearRiseMultiplier = 0.60f;
        spawnChanceBonus = -12;
        entitySpeedMultiplier = 0.70f;
        currentParams = { 4.2f, 1.9f, 2.4f, 32.0f, 8.0f, 22.0f, 1.40f };
        break;
    case 2:
        fearRiseMultiplier = 0.80f;
        spawnChanceBonus = -6;
        entitySpeedMultiplier = 0.85f;
        currentParams = { 4.4f, 2.0f, 2.6f, 28.0f, 9.0f, 23.0f, 1.50f };
        break;
    case 3:
        fearRiseMultiplier = 1.00f;
        spawnChanceBonus = 0;
        entitySpeedMultiplier = 1.00f;
        currentParams = { 4.6f, 2.1f, 2.8f, 24.0f, 10.0f, 24.0f, 1.55f };
        break;
    case 4:
        fearRiseMultiplier = 1.25f;
        spawnChanceBonus = 10;
        entitySpeedMultiplier = 1.15f;
        currentParams = { 4.8f, 2.2f, 3.1f, 20.0f, 11.0f, 25.0f, 1.65f };
        break;
    case 5:
        fearRiseMultiplier = 1.50f;
        spawnChanceBonus = 16;
        entitySpeedMultiplier = 1.30f;
        currentParams = { 5.0f, 2.3f, 3.4f, 16.0f, 12.0f, 26.0f, 1.70f };
        break;
    case 6:
        fearRiseMultiplier = 1.80f;
        spawnChanceBonus = 22;
        entitySpeedMultiplier = 1.45f;
        currentParams = { 5.2f, 2.4f, 3.7f, 14.0f, 14.0f, 27.0f, 1.80f };
        break;
    case 7:
    default:
        fearRiseMultiplier = 2.20f;
        spawnChanceBonus = 30;
        entitySpeedMultiplier = 1.65f;
        currentParams = { 5.4f, 2.5f, 4.0f, 11.0f, 15.0f, 28.0f, 1.85f };
        break;
    }
}

// 状態名取得
const char* HorrorManager::GetChaseStateName() const
{
    switch (chaseState)
    {
    case MonsterChaseState::Dormant:       return "DORMANT";
    case MonsterChaseState::Idle:          return "IDLE";
    case MonsterChaseState::Searching:     return "SEARCHING";
    case MonsterChaseState::Chasing:       return "CHASING";
    case MonsterChaseState::LostPlayer:    return "LOST_PLAYER";
    case MonsterChaseState::AvoidingLight: return "AVOIDING_LIGHT";
    case MonsterChaseState::Despawned:     return "DESPAWNED";
    default:                               return "UNKNOWN";
    }
}

// 初期化処理
bool HorrorManager::Initialize()
{
    fearLevel = 0.0f;
    inSafeLight = true;
    safeGraceTimer = 0.8f;
    currentCombinedTension = 0.0f;
    gameState = HorrorGameState::Normal;
    jumpscareState = JumpscareState::None;
    scareTimer = 0.0f;
    screenFadeAlpha = 0.0f;
    deathFallProgress = 0.0f;
    deathBlinkTimer = 0.0f;
    deathBlinkFactor = 1.0f;
    proceduralBreathTimer = 0.0f;
    swayTimer = 0.0f;
    currentVignetteFactor = 0.0f;
    currentBlurFactor = 0.0f;
    currentSwayFactor = 0.0f;
    focusBlurTimer = 15.0f;
    focusBlurActiveTimer = 0.0f;
    doubleVisionTimer = 20.0f;
    doubleVisionActiveTimer = 0.0f;
    peripheralScareTimer = 25.0f;
    isPeripheralActive = false;
    streetLightEventTimer = 20.0f;
    headlightGlitchTimer = 18.0f;
    scareCamPos = VGet(0.0f, 0.0f, 0.0f);
    scareCamForward = VGet(0.0f, 0.0f, 1.0f);
    spawnCheckTimer = 10.0f;
    lastSpawnId = -1;
    showDebugHUD = false;

    // 追跡システム初期化
    isChaseUnlocked = false;
    chaseState = MonsterChaseState::Dormant;
    lastKnownPlayerPos = VGet(0.0f, 0.0f, 0.0f);
    lostSightTimer = 0.0f;
    chaseCooldownTimer = 0.0f;
    campingTimer = 0.0f;
    searchStateTimer = 0.0f;
    currentSearchNodeIndex = 0;
    darknessTimeAccumulator = 0.0f;
    playerWasVisible = false;
    lastLOSState = false;
    lastSafeReason = "NONE";
    currentNavTargetNodeId = -1;

    // ナビゲーションノードの生成
    InitializeNavNodes();

    // 難易度設定
    SetDayDifficulty(1);

    // 3D怪異モデルの初期化
    entity.Initialize();

    // 雰囲気演出用の固定出現候補地点登録
    spawnPoints.clear();
    spawnPoints.push_back({ 1, VGet(-22.0f, 0.0f, -22.0f), "Southwest Intersection" });
    spawnPoints.push_back({ 2, VGet( 22.0f, 0.0f, -22.0f), "Southeast Crossroad" });
    spawnPoints.push_back({ 3, VGet(-22.0f, 0.0f,  15.0f), "Northwest Corner House" });
    spawnPoints.push_back({ 4, VGet( 22.0f, 0.0f,  15.0f), "Northeast Corner Road" });
    spawnPoints.push_back({ 5, VGet(  0.0f, 0.0f,  35.0f), "North Dead End Path" });
    spawnPoints.push_back({ 6, VGet(-10.0f, 0.0f,  -2.0f), "Mid Narrow Alleyway" });
    spawnPoints.push_back({ 7, VGet( 10.0f, 0.0f,  -2.0f), "Mid East House Wall" });

    return true;
}

// プレイヤーがモンスターの直接攻撃から保護されているか判定
bool HorrorManager::IsPlayerSafeFromMonster(const Player& player, const Bicycle& bicycle, const NightEnvironment& nightEnv, const char** outReason) const
{
    VECTOR playerPos = player.GetPosition();

    // 街灯の安全範囲にはモンスターを侵入させない
    const std::vector<StreetLight>& lights = nightEnv.GetStreetLights();
    for (size_t i = 0; i < lights.size(); ++i)
    {
        if (lights[i].state == StreetLightState::Off || lights[i].currentIntensity < 0.15f)
        {
            continue;
        }

        float dx = playerPos.x - lights[i].position.x;
        float dz = playerPos.z - lights[i].position.z;
        if (dx * dx + dz * dz < (6.5f * 6.5f))
        {
            if (outReason) *outReason = "STREET LIGHT";
            return true;
        }
    }

    // バッテリーが残っている自転車ではプレイヤーを保護する
    // バッテリー切れの場合は自転車の保護を無効にする
    if (bicycle.IsRiding())
    {
        if (bicycle.GetBatteryPercent() > 0.0f && bicycle.IsHeadlightOn() && bicycle.GetHeadlightIntensity() > 0.05f)
        {
            if (outReason) *outReason = "BICYCLE BATTERY";
            return true;
        }
    }

    // 降車時：駐輪された自転車の前方ヘッドライト照射範囲内
    if (bicycle.IsHeadlightOn() && bicycle.GetHeadlightIntensity() > 0.05f && bicycle.GetBatteryPercent() > 0.0f)
    {
        VECTOR lightPos = bicycle.GetHeadlightPosition();
        VECTOR lightDir = bicycle.GetHeadlightDirection();
        float range = bicycle.GetHeadlightRange();

        VECTOR toPlayer = VSub(playerPos, lightPos);
        float dist = VSize(toPlayer);

        if (dist > 0.1f && dist < range)
        {
            VECTOR normToP = VScale(toPlayer, 1.0f / dist);
            float dot = VDot(normToP, lightDir);
            if (dot > 0.743f)
            {
                if (outReason) *outReason = "HEADLIGHT CONE";
                return true;
            }
        }
    }

    if (outReason) *outReason = "NONE";
    return false;
}

// 安全光判定（恐怖度増減用）
bool HorrorManager::CheckSafeLight(const Player& player, const Bicycle& bicycle, const NightEnvironment& nightEnv)
{
    return IsPlayerSafeFromMonster(player, bicycle, nightEnv, nullptr);
}

// 適切なモンスター出現地点の選定
bool HorrorManager::FindValidMonsterSpawn(const Player& player, const NightEnvironment& nightEnv, VECTOR& outSpawnPos)
{
    VECTOR pPos = player.GetPosition();
    VECTOR pFwd = player.GetForward();
    pFwd.y = 0.0f;
    float fwdLen = VSize(pFwd);
    if (fwdLen > 0.001f) pFwd = VScale(pFwd, 1.0f / fwdLen);
    else pFwd = VGet(0.0f, 0.0f, 1.0f);

    float bestScore = -9999.0f;
    VECTOR bestCand = VGet(0.0f, 0.0f, 0.0f);
    bool found = false;

    for (const auto& node : navNodes)
    {
        VECTOR cand = node.pos;
        float dx = cand.x - pPos.x;
        float dz = cand.z - pPos.z;
        float dist = std::sqrt(dx * dx + dz * dz);

        // プレイヤーから16m〜28mの範囲内
        if (dist < 16.0f || dist > 28.0f)
        {
            continue;
        }

        // 建物との衝突判定
        if (WorldCollision::Instance().CheckCollision(cand, 0.5f))
        {
            continue;
        }

        // 街灯の光線範囲外であること
        bool insideLight = false;
        for (const auto& l : nightEnv.GetStreetLights())
        {
            if (l.state != StreetLightState::Off && l.currentIntensity >= 0.15f)
            {
                float lx = cand.x - l.position.x;
                float lz = cand.z - l.position.z;
                if (lx * lx + lz * lz < (7.5f * 7.5f))
                {
                    insideLight = true;
                    break;
                }
            }
        }
        if (insideLight)
        {
            continue;
        }

        // プレイヤーの視界背後・側方を優先（dotが小さいほど背後）
        VECTOR toCand = VScale(VGet(dx, 0.0f, dz), 1.0f / dist);
        float dot = VDot(pFwd, toCand);

        float score = (30.0f - dist) - dot * 15.0f;
        if (score > bestScore)
        {
            bestScore = score;
            bestCand = cand;
            found = true;
        }
    }

    if (found)
    {
        outSpawnPos = bestCand;
        return true;
    }
    return false;
}

// 建物を避けながら目的地へ移動するナビゲーションヘルパー
void HorrorManager::NavigateTowards(const VECTOR& targetPos, float speed, float dt)
{
    // 建物を避けながらプレイヤーの位置へ移動する
    VECTOR pos = entity.GetPosition();

    // 目的地への直線レイキャスト（障害物がないか確認）
    VECTOR fromEye = VGet(pos.x, pos.y + 0.6f, pos.z);
    VECTOR toEye = VGet(targetPos.x, targetPos.y + 0.6f, targetPos.z);
    bool directClear = !WorldCollision::Instance().CheckSweep(fromEye, toEye, 0.35f);

    VECTOR moveTarget = targetPos;
    currentNavTargetNodeId = -1;

    if (!directClear && !navNodes.empty())
    {
        // 建物に遮られている場合は経由可能なナビゲーションノードを選択
        float bestCost = 9999.0f;
        int bestId = -1;
        VECTOR bestNodePos = targetPos;

        for (const auto& node : navNodes)
        {
            VECTOR nEye = VGet(node.pos.x, node.pos.y + 0.6f, node.pos.z);
            if (!WorldCollision::Instance().CheckSweep(fromEye, nEye, 0.35f))
            {
                float d1 = std::sqrt((node.pos.x - pos.x) * (node.pos.x - pos.x) + (node.pos.z - pos.z) * (node.pos.z - pos.z));
                float d2 = std::sqrt((targetPos.x - node.pos.x) * (targetPos.x - node.pos.x) + (targetPos.z - node.pos.z) * (targetPos.z - node.pos.z));
                float cost = d1 * 0.7f + d2;
                if (cost < bestCost)
                {
                    bestCost = cost;
                    bestId = node.id;
                    bestNodePos = node.pos;
                }
            }
        }

        if (bestId != -1)
        {
            moveTarget = bestNodePos;
            currentNavTargetNodeId = bestId;
        }
    }

    // 移動方向ベクトルの算出
    float dx = moveTarget.x - pos.x;
    float dz = moveTarget.z - pos.z;
    float dist = std::sqrt(dx * dx + dz * dz);

    if (dist > 0.05f)
    {
        float invD = 1.0f / dist;
        float dirX = dx * invD;
        float dirZ = dz * invD;

        float step = speed * dt;
        if (step > dist) step = dist;

        pos.x += dirX * step;
        pos.z += dirZ * step;

        // モデルの進行方向回転
        float moveAngle = std::atan2(dirX, dirZ);
        entity.SetYaw(moveAngle + entity.GetModelForwardOffset());
    }

    // 障害物とのめり込み解決（壁抜け防止）
    WorldCollision::Instance().ResolveCollision(pos, 0.40f);
    entity.SetPosition(pos);
}

// モンスター追跡AIの更新
void HorrorManager::UpdateMonsterChase(float dt, Player& player, const Bicycle& bicycle, const NightEnvironment& nightEnv, const DeliveryManager* deliveryManager)
{
    // 配達が半分完了したら追跡システムを有効にする
    if (deliveryManager && deliveryManager->GetTotalDeliveries() > 0)
    {
        if (deliveryManager->GetCompletedDeliveries() >= deliveryManager->GetTotalDeliveries() / 2)
        {
            isChaseUnlocked = true;
        }
    }

    if (!isChaseUnlocked)
    {
        chaseState = MonsterChaseState::Dormant;
        return;
    }

    // ジャンプスケア演出中・ゲームオーバー中は追跡AIを停止
    if (gameState != HorrorGameState::Normal)
    {
        return;
    }

    VECTOR playerPos = player.GetPosition();
    VECTOR monsterPos = entity.GetPosition();

    // プレイヤーの安全保護状態を判定
    const char* reason = "NONE";
    bool playerIsSafe = IsPlayerSafeFromMonster(player, bicycle, nightEnv, &reason);
    lastSafeReason = reason;

    // 暗闇滞在時間の積算
    if (!playerIsSafe)
    {
        darknessTimeAccumulator += dt;
    }
    else
    {
        darknessTimeAccumulator = 0.0f;
    }

    // 視線（Line of Sight）と探知範囲の判定
    bool hasLOS = false;
    bool inRange = false;
    bool inFOV = false;

    if (entity.IsActive())
    {
        VECTOR mEye = VGet(monsterPos.x, monsterPos.y + 1.6f, monsterPos.z);
        VECTOR pEye = VGet(playerPos.x, playerPos.y + 1.4f, playerPos.z);

        // 建物に遮られている場合はプレイヤーを見失う
        bool sweepHit = WorldCollision::Instance().CheckSweep(mEye, pEye, 0.25f);
        hasLOS = !sweepHit;
        lastLOSState = hasLOS;

        float dx = playerPos.x - monsterPos.x;
        float dz = playerPos.z - monsterPos.z;
        float dist = std::sqrt(dx * dx + dz * dz);
        inRange = (dist <= currentParams.detectRange);

        // モンスターの視覚視野角（140度コーン、または至近距離5m以内の気配察知）
        float mYaw = entity.GetYaw() - entity.GetModelForwardOffset();
        VECTOR mFwd = VGet(std::sin(mYaw), 0.0f, std::cos(mYaw));
        float dot = (dist > 0.01f) ? (dx / dist * mFwd.x + dz / dist * mFwd.z) : 1.0f;
        inFOV = (dot >= 0.34f) || (dist <= 5.0f);
    }
    else
    {
        lastLOSState = false;
    }

    bool canSeePlayer = (entity.IsActive() && hasLOS && inRange && inFOV);
    playerWasVisible = canSeePlayer;

    // 状態マシン処理
    switch (chaseState)
    {
    case MonsterChaseState::Dormant:
    {
        // プレイヤーが暗闇にいる場合は追跡を開始する
        if (chaseCooldownTimer <= 0.0f && darknessTimeAccumulator >= 2.5f)
        {
            VECTOR spawnPos;
            if (FindValidMonsterSpawn(player, nightEnv, spawnPos))
            {
                float yaw = std::atan2(playerPos.x - spawnPos.x, playerPos.z - spawnPos.z);
                // 待機モデルで初期出現
                entity.Spawn(spawnPos, yaw, 9999.0f, 0.0f, HorrorModelType::Idle);
                chaseState = MonsterChaseState::Idle;
                searchStateTimer = 2.0f;
            }
        }
        break;
    }

    case MonsterChaseState::Despawned:
    {
        chaseCooldownTimer -= dt;
        if (chaseCooldownTimer <= 0.0f)
        {
            chaseCooldownTimer = 0.0f;
            chaseState = MonsterChaseState::Dormant;
        }
        break;
    }

    case MonsterChaseState::Idle:
    {
        // 待機モデルのアニメーション進行
        entity.SetModelType(HorrorModelType::Idle);
        entity.AdvanceAnim(dt, 1.0f);

        if (canSeePlayer)
        {
            if (!playerIsSafe)
            {
                // 追跡中は走行モデルを使用する
                entity.SetModelType(HorrorModelType::Run);
                chaseState = MonsterChaseState::Chasing;
                // 最後に確認したプレイヤーの位置を記録する
                lastKnownPlayerPos = playerPos;
            }
            else
            {
                // 安全な光の外側でプレイヤーを待つ
                chaseState = MonsterChaseState::AvoidingLight;
                campingTimer = 0.0f;
            }
        }
        else
        {
            searchStateTimer -= dt;
            if (searchStateTimer <= 0.0f)
            {
                // 巡回探索へ移行
                chaseState = MonsterChaseState::Searching;
                if (!navNodes.empty())
                {
                    currentSearchNodeIndex = std::rand() % navNodes.size();
                }
            }
        }
        break;
    }

    case MonsterChaseState::Searching:
    {
        entity.SetModelType(HorrorModelType::Idle);
        entity.AdvanceAnim(dt, 1.1f);

        if (canSeePlayer)
        {
            if (!playerIsSafe)
            {
                // 追跡中は走行モデルを使用する
                entity.SetModelType(HorrorModelType::Run);
                chaseState = MonsterChaseState::Chasing;
                // 最後に確認したプレイヤーの位置を記録する
                lastKnownPlayerPos = playerPos;
            }
            else
            {
                // 安全な光の外側でプレイヤーを待つ
                chaseState = MonsterChaseState::AvoidingLight;
                campingTimer = 0.0f;
            }
            break;
        }

        // 目標ナビゲーションノードへの巡回移動
        if (!navNodes.empty() && currentSearchNodeIndex < static_cast<int>(navNodes.size()))
        {
            VECTOR targetNode = navNodes[currentSearchNodeIndex].pos;
            NavigateTowards(targetNode, currentParams.searchSpeed, dt);

            float d = std::sqrt((targetNode.x - monsterPos.x) * (targetNode.x - monsterPos.x) +
                                (targetNode.z - monsterPos.z) * (targetNode.z - monsterPos.z));
            if (d < 1.5f)
            {
                chaseState = MonsterChaseState::Idle;
                searchStateTimer = 3.5f;
            }
        }
        break;
    }

    case MonsterChaseState::Chasing:
    {
        // 追跡中は走行モデルを使用する
        entity.SetModelType(HorrorModelType::Run);
        entity.AdvanceAnim(dt, 1.4f);

        if (hasLOS)
        {
            // 最後に確認したプレイヤーの位置を記録する
            lastKnownPlayerPos = playerPos;

            // 街灯の安全範囲にはモンスターを侵入させない
            bool lightBarrier = false;
            for (const auto& light : nightEnv.GetStreetLights())
            {
                if (light.state != StreetLightState::Off && light.currentIntensity >= 0.15f)
                {
                    float distToLight = std::sqrt((monsterPos.x - light.position.x) * (monsterPos.x - light.position.x) +
                                                  (monsterPos.z - light.position.z) * (monsterPos.z - light.position.z));
                    if (distToLight <= 7.0f && playerIsSafe)
                    {
                        lightBarrier = true;
                        break;
                    }
                }
            }

            if (lightBarrier)
            {
                // 安全な光の外側でプレイヤーを待つ
                entity.SetModelType(HorrorModelType::Idle);
                chaseState = MonsterChaseState::AvoidingLight;
                campingTimer = 0.0f;
                break;
            }

            // プレイヤーへの疾走移動
            NavigateTowards(playerPos, currentParams.runSpeed, dt);

            // 接触・攻撃判定
            float distToP = std::sqrt((playerPos.x - monsterPos.x) * (playerPos.x - monsterPos.x) +
                                      (playerPos.z - monsterPos.z) * (playerPos.z - monsterPos.z));
            if (distToP <= 1.35f)
            {
                if (!playerIsSafe)
                {
                    // 暗闇の中での接触：ジャンプスケア演出から死亡シーケンスへ移行
                    TriggerMaxFearSequence(player);
                    return;
                }
                else
                {
                    // 安全保護状態（自転車バッテリーまたは街灯内）では直接殺傷しない
                }
            }
        }
        else
        {
            // 建物に遮られている場合はプレイヤーを見失う
            chaseState = MonsterChaseState::LostPlayer;
            lostSightTimer = currentParams.lostSightDuration;
        }
        break;
    }

    case MonsterChaseState::LostPlayer:
    {
        // 見失い中：最後に確認した位置へ急行
        entity.SetModelType(HorrorModelType::Run);
        entity.AdvanceAnim(dt, 1.2f);

        if (canSeePlayer)
        {
            if (!playerIsSafe)
            {
                // 追跡再開
                entity.SetModelType(HorrorModelType::Run);
                chaseState = MonsterChaseState::Chasing;
                lastKnownPlayerPos = playerPos;
                break;
            }
            else
            {
                entity.SetModelType(HorrorModelType::Idle);
                chaseState = MonsterChaseState::AvoidingLight;
                campingTimer = 0.0f;
                break;
            }
        }

        // 最後に確認した地点へ前進
        NavigateTowards(lastKnownPlayerPos, currentParams.runSpeed, dt);

        lostSightTimer -= dt;
        float distToLast = std::sqrt((lastKnownPlayerPos.x - monsterPos.x) * (lastKnownPlayerPos.x - monsterPos.x) +
                                     (lastKnownPlayerPos.z - monsterPos.z) * (lastKnownPlayerPos.z - monsterPos.z));

        // プレイヤーを見失った場合は待機モデルへ切り替える
        if (lostSightTimer <= 0.0f || distToLast <= 1.2f)
        {
            entity.SetModelType(HorrorModelType::Idle);
            chaseState = MonsterChaseState::Idle;
            searchStateTimer = 4.5f;
        }
        break;
    }

    case MonsterChaseState::AvoidingLight:
    {
        // 待機モデルで安全光の外側に立ち、プレイヤーを見据える
        entity.SetModelType(HorrorModelType::Idle);
        entity.AdvanceAnim(dt, 0.8f);
        // 安全な光の外側でプレイヤーを待つ
        entity.SetLookAtTarget(playerPos);

        campingTimer += dt;

        if (!playerIsSafe && canSeePlayer)
        {
            // プレイヤーが安全光から出た：追跡再開
            entity.SetModelType(HorrorModelType::Run);
            chaseState = MonsterChaseState::Chasing;
            lastKnownPlayerPos = playerPos;
            break;
        }

        // 待機限界時間を超えたら霧の中へ退散し、クールダウンに入る
        if (campingTimer >= currentParams.campingMaxDuration)
        {
            entity.Despawn();
            chaseState = MonsterChaseState::Despawned;
            chaseCooldownTimer = currentParams.cooldownDuration;
        }
        break;
    }
    }
}

// 最大恐怖時のジャンプスケア演出開始
void HorrorManager::TriggerMaxFearSequence(Player& player)
{
    if (gameState != HorrorGameState::Normal)
    {
        return;
    }

    // ゲームオーバー演出移行時に時計の進行を停止
    HorrorUI::Instance().SetTimePaused(true);

    fearLevel = 1.0f;
    gameState = HorrorGameState::Jumpscare;
    jumpscareState = JumpscareState::Pause;
    scareTimer = 0.35f;
    screenFadeAlpha = 0.0f;

    deathFallProgress = 0.0f;
    deathBlinkTimer = 0.0f;
    deathBlinkFactor = 1.0f;

    // 現在のカメラ位置・前方を記憶
    scareCamPos = player.GetPosition();
    scareCamForward = player.GetForward();
    scareCamForward.y = 0.0f;
    float len = VSize(scareCamForward);
    if (len > 0.001f) scareCamForward = VScale(scareCamForward, 1.0f / len);
    else scareCamForward = VGet(0.0f, 0.0f, 1.0f);

    // ジャンプスケア時は這いずり怪異または疾走怪異を正面に出現
    entity.SetModelType(HorrorModelType::Run);
    VECTOR facePos = VAdd(scareCamPos, VScale(scareCamForward, 2.8f));
    facePos.y = 0.0f;
    float yaw = std::atan2(-scareCamForward.x, -scareCamForward.z);
    entity.Spawn(facePos, yaw, 10.0f, 0.0f, HorrorModelType::Run);
}

// ジャンプスケア演出の進行更新
void HorrorManager::UpdateJumpscareSequence(float dt, Player& player)
{
    scareTimer -= dt;

    switch (jumpscareState)
    {
    case JumpscareState::Pause:
        player.SetEyeHeightOffset(0.0f);
        player.SetCameraRoll(0.0f);
        HorrorUI::Instance().SetHorrorVisualParams(1.0f, 0.05f, 1.0f, 0.15f, 0.20f, 0.30f);

        if (scareTimer <= 0.0f)
        {
            jumpscareState = JumpscareState::Reveal;
            scareTimer = 0.40f;
            VECTOR revealPos = VAdd(scareCamPos, VScale(scareCamForward, 3.2f));
            revealPos.y = 0.0f;
            float yaw = std::atan2(-scareCamForward.x, -scareCamForward.z);
            entity.SetDirectTransform(revealPos, yaw);
        }
        break;

    case JumpscareState::Reveal:
        entity.AdvanceAnim(dt, 1.5f);
        HorrorUI::Instance().SetHorrorVisualParams(1.0f, 0.15f, 1.0f, 0.35f, 0.35f, 0.50f);

        if (scareTimer <= 0.0f)
        {
            jumpscareState = JumpscareState::Rush;
            scareTimer = 0.38f;
        }
        break;

    case JumpscareState::Rush:
    {
        entity.AdvanceAnim(dt, 2.5f);
        float progress = 1.0f - (scareTimer / 0.38f);
        if (progress > 1.0f) progress = 1.0f;

        float dist = 3.2f * (1.0f - progress) + 0.65f * progress;
        VECTOR rushPos = VAdd(scareCamPos, VScale(scareCamForward, dist));
        rushPos.y = 0.0f;
        float yaw = std::atan2(-scareCamForward.x, -scareCamForward.z);
        entity.SetDirectTransform(rushPos, yaw);

        HorrorUI::Instance().SetHorrorVisualParams(1.0f, 0.30f, 1.0f, 0.55f, 0.40f, 0.80f);

        if (scareTimer <= 0.0f)
        {
            jumpscareState = JumpscareState::Impact;
            scareTimer = 0.22f;
        }
        break;
    }

    case JumpscareState::Impact:
        entity.AdvanceAnim(dt, 3.0f);
        HorrorUI::Instance().SetHorrorVisualParams(1.0f, 0.50f, 1.0f, 0.85f, 0.60f, 1.0f);

        if (scareTimer <= 0.0f)
        {
            jumpscareState = JumpscareState::Fall;
            scareTimer = 0.75f;
            deathFallProgress = 0.0f;
        }
        break;

    case JumpscareState::Fall:
    {
        deathFallProgress += dt / 0.75f;
        if (deathFallProgress > 1.0f) deathFallProgress = 1.0f;

        float dropY = -1.35f * (deathFallProgress * deathFallProgress);
        player.SetEyeHeightOffset(dropY);

        float rollAngle = 0.75f * std::sin(deathFallProgress * DX_PI_F * 0.5f);
        player.SetCameraRoll(rollAngle);

        float shake = (1.0f - deathFallProgress) * 0.04f;
        VECTOR shakeOff = VGet(
            ((std::rand() % 100) / 50.0f - 1.0f) * shake,
            ((std::rand() % 100) / 50.0f - 1.0f) * shake,
            ((std::rand() % 100) / 50.0f - 1.0f) * shake
        );
        player.SetCameraShakeOffset(shakeOff);

        entity.AdvanceAnim(dt, 1.0f);
        HorrorUI::Instance().SetHorrorVisualParams(1.0f, 0.60f + deathFallProgress * 0.15f, 1.0f, 0.70f, 0.50f, 0.90f);

        if (scareTimer <= 0.0f)
        {
            jumpscareState = JumpscareState::GroundImpact;
            scareTimer = 0.20f;
        }
        break;
    }

    case JumpscareState::GroundImpact:
    {
        player.SetEyeHeightOffset(-1.38f);
        player.SetCameraRoll(0.78f);
        player.SetCameraShakeOffset(VGet(0.0f, 0.0f, 0.0f));

        entity.AdvanceAnim(dt, 0.4f);
        HorrorUI::Instance().SetHorrorVisualParams(1.0f, 0.80f, 1.0f, 0.60f, 0.60f, 0.85f);

        if (scareTimer <= 0.0f)
        {
            jumpscareState = JumpscareState::GroundedStare;
            scareTimer = 1.50f;
            deathBlinkTimer = 0.0f;
            deathBlinkFactor = 1.0f;
        }
        break;
    }

    case JumpscareState::GroundedStare:
    {
        entity.AdvanceAnim(dt, 0.2f);

        deathBlinkTimer += dt;
        if (deathBlinkTimer < 0.25f)
        {
            deathBlinkFactor = 1.0f - (deathBlinkTimer / 0.25f);
        }
        else if (deathBlinkTimer < 0.50f)
        {
            deathBlinkFactor = (deathBlinkTimer - 0.25f) / 0.25f;
        }
        else if (deathBlinkTimer < 0.80f)
        {
            deathBlinkFactor = 0.70f + 0.30f * std::sin((deathBlinkTimer - 0.50f) * 10.0f);
        }
        else
        {
            float closeT = (deathBlinkTimer - 0.80f) / 0.70f;
            if (closeT > 1.0f) closeT = 1.0f;
            deathBlinkFactor = 1.0f - closeT;
        }

        HorrorUI::Instance().SetHorrorVisualParams(1.0f, 0.88f, deathBlinkFactor, 0.50f, 0.70f, 0.80f);

        if (scareTimer <= 0.0f)
        {
            jumpscareState = JumpscareState::VisionClosing;
            scareTimer = 2.00f;
        }
        break;
    }

    case JumpscareState::VisionClosing:
    {
        float fade = 1.0f - (scareTimer / 2.00f);
        if (fade > 1.0f) fade = 1.0f;
        screenFadeAlpha = fade;

        float finalClose = 0.88f + fade * 0.12f;
        deathBlinkFactor = 1.0f - fade;
        if (deathBlinkFactor < 0.0f) deathBlinkFactor = 0.0f;

        HorrorUI::Instance().SetHorrorVisualParams(1.0f, finalClose, deathBlinkFactor, 0.30f, 0.90f, 1.0f);

        if (scareTimer <= 0.0f)
        {
            jumpscareState = JumpscareState::GameOver;
            gameState = HorrorGameState::GameOver;
            entity.Despawn();
        }
        break;
    }

    case JumpscareState::GameOver:
        HorrorUI::Instance().SetHorrorVisualParams(1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
        break;

    default:
        break;
    }
}

// 恐怖段階に応じた怪異の自動出現判定（配達前半・未解禁時および追跡非アクティブ時）
void HorrorManager::UpdateAutoSpawns(float dt, const Player& player, const NightEnvironment& nightEnv)
{
    // モンスター追跡中または演出中は通常出現を抑制
    if (chaseState == MonsterChaseState::Chasing ||
        chaseState == MonsterChaseState::LostPlayer ||
        chaseState == MonsterChaseState::AvoidingLight)
    {
        return;
    }

    if (entity.IsActive() || gameState != HorrorGameState::Normal)
    {
        return;
    }

    spawnCheckTimer -= dt;
    if (spawnCheckTimer > 0.0f)
    {
        return;
    }
    spawnCheckTimer = 7.0f + (std::rand() % 6);

    FearStage stage = GetFearStage();
    if (stage == FearStage::Calm)
    {
        return;
    }

    VECTOR playerPos = player.GetPosition();
    VECTOR forward = player.GetForward();
    forward.y = 0.0f;
    float len = VSize(forward);
    if (len > 0.001f) forward = VScale(forward, 1.0f / len);
    else forward = VGet(0.0f, 0.0f, 1.0f);

    int chance = std::rand() % 100;
    int threshold = 0;
    float baseDist = 12.0f;
    float crawlSpeed = 0.0f;
    float duration = 5.0f;

    switch (stage)
    {
    case FearStage::Uneasy:
        threshold = 15 + spawnChanceBonus;
        baseDist = 18.0f;
        duration = 3.5f;
        crawlSpeed = 0.0f;
        break;
    case FearStage::Afraid:
        threshold = 35 + spawnChanceBonus;
        baseDist = 14.0f;
        duration = 5.0f;
        crawlSpeed = 0.5f * entitySpeedMultiplier;
        break;
    case FearStage::Panic:
        threshold = 60 + spawnChanceBonus;
        baseDist = 10.0f;
        duration = 6.5f;
        crawlSpeed = 1.3f * entitySpeedMultiplier;
        break;
    case FearStage::Terror:
        threshold = 85 + spawnChanceBonus;
        baseDist = 7.5f;
        duration = 8.0f;
        crawlSpeed = 2.4f * entitySpeedMultiplier;
        break;
    default:
        break;
    }

    if (threshold < 5) threshold = 5;
    if (chance >= threshold)
    {
        return;
    }

    // 視覚的霧の端点手前に怪異を出現
    float fogStart = nightEnv.GetFogStart();
    float fogEnd = nightEnv.GetFogEnd();
    float targetDist = baseDist;
    if (targetDist > fogEnd - 1.5f)
    {
        targetDist = fogEnd - 1.5f;
    }
    if (targetDist < fogStart + 1.5f)
    {
        targetDist = fogStart + 1.5f;
    }

    VECTOR dynamicPos = VAdd(playerPos, VScale(forward, targetDist));
    dynamicPos.y = 0.0f;

    float yaw = std::atan2(playerPos.x - dynamicPos.x, playerPos.z - dynamicPos.z);
    entity.Spawn(dynamicPos, yaw, duration, crawlSpeed, HorrorModelType::Crawl);
}

// 視界端（周辺視野）での怪異出現・直視時消滅の更新
void HorrorManager::UpdatePeripheralScares(float dt, const Player& player)
{
    if (chaseState == MonsterChaseState::Chasing ||
        chaseState == MonsterChaseState::LostPlayer ||
        chaseState == MonsterChaseState::AvoidingLight)
    {
        return;
    }

    if (isPeripheralActive)
    {
        if (!entity.IsActive())
        {
            isPeripheralActive = false;
            return;
        }

        VECTOR fwd = player.GetForward();
        fwd.y = 0.0f;
        float flen = VSize(fwd);
        if (flen > 0.001f) fwd = VScale(fwd, 1.0f / flen);

        VECTOR pPos = player.GetPosition();
        VECTOR ePos = entity.GetPosition();
        VECTOR toE = VSub(ePos, pPos);
        toE.y = 0.0f;
        float elen = VSize(toE);

        if (elen > 0.001f)
        {
            toE = VScale(toE, 1.0f / elen);
            float dot = VDot(fwd, toE);
            // プレイヤーがカメラを正面に向けたら瞬時に消滅
            if (dot > 0.55f)
            {
                entity.Despawn();
                isPeripheralActive = false;
            }
        }
    }
}

// 毎フレーム更新処理
void HorrorManager::Update(float dt, Player& player, const Bicycle& bicycle, const NightEnvironment& nightEnv, const DeliveryManager* deliveryManager)
{
    // 通常ゲームプレイ中の処理
    if (gameState == HorrorGameState::Normal)
    {
        // ゲーム時間と配達進行による総合緊張度計算
        float totalMins = HorrorUI::Instance().GetTotalGameMinutes();
        float timeTension = totalMins / 180.0f;
        if (timeTension < 0.0f) timeTension = 0.0f;
        if (timeTension > 1.0f) timeTension = 1.0f;

        float deliveryTension = 0.0f;
        if (deliveryManager && deliveryManager->GetTotalDeliveries() > 0)
        {
            deliveryTension = static_cast<float>(deliveryManager->GetCompletedDeliveries()) /
                              static_cast<float>(deliveryManager->GetTotalDeliveries());
            if (deliveryTension < 0.0f) deliveryTension = 0.0f;
            if (deliveryTension > 1.0f) deliveryTension = 1.0f;
        }

        currentCombinedTension = 0.55f * timeTension + 0.45f * deliveryTension;
        if (currentCombinedTension > 1.0f) currentCombinedTension = 1.0f;

        cachedBicycleBattery = bicycle.GetBatteryPercent();
        if (entity.IsActive())
        {
            VECTOR pPos = player.GetPosition();
            VECTOR mPos = entity.GetPosition();
            cachedMonsterDist = std::sqrt((pPos.x - mPos.x) * (pPos.x - mPos.x) + (pPos.z - mPos.z) * (pPos.z - mPos.z));
        }
        else
        {
            cachedMonsterDist = -1.0f;
        }

        // 安全光判定
        const char* safeReason = "NONE";
        inSafeLight = IsPlayerSafeFromMonster(player, bicycle, nightEnv, &safeReason);
        lastSafeReason = safeReason;

        // 恐怖度の増減計算
        if (inSafeLight)
        {
            safeGraceTimer = 0.8f;
            fearLevel -= 0.08f * dt;
            if (fearLevel < 0.0f) fearLevel = 0.0f;

            float nextVignette = currentVignetteFactor - dt * 0.45f;
            currentVignetteFactor = (nextVignette > fearLevel) ? nextVignette : fearLevel;

            float nextBlur = currentBlurFactor - dt * 0.65f;
            currentBlurFactor = (nextBlur > 0.0f) ? nextBlur : 0.0f;

            float nextSway = currentSwayFactor - dt * 0.55f;
            currentSwayFactor = (nextSway > 0.0f) ? nextSway : 0.0f;
        }
        else
        {
            if (safeGraceTimer > 0.0f)
            {
                safeGraceTimer -= dt;
            }
            else
            {
                float tensionDarkFactor = 1.0f + currentCombinedTension * 0.5f;
                float chaseMult = (chaseState == MonsterChaseState::Chasing) ? currentParams.fearMultiplier : 1.0f;
                fearLevel += 0.10f * fearRiseMultiplier * tensionDarkFactor * chaseMult * dt;
            }

            currentVignetteFactor += (fearLevel - currentVignetteFactor) * 0.15f;
            currentBlurFactor += (fearLevel - currentBlurFactor) * 0.10f;
            currentSwayFactor += (fearLevel - currentSwayFactor) * 0.12f;
        }

        // 怪異が接近している場合の恐怖度上昇
        if (entity.IsActive())
        {
            VECTOR playerPos = player.GetPosition();
            VECTOR entityPos = entity.GetPosition();
            float dx = playerPos.x - entityPos.x;
            float dz = playerPos.z - entityPos.z;
            float dist = std::sqrt(dx * dx + dz * dz);

            if (dist < 18.0f)
            {
                float proximityFactor = (1.0f - (dist / 18.0f));
                fearLevel += (0.04f + 0.08f * proximityFactor) * fearRiseMultiplier * dt;
            }
        }

        // 恐怖度100%到達時にゲームオーバー演出へ移行
        if (fearLevel >= 1.0f)
        {
            fearLevel = 1.0f;
            TriggerMaxFearSequence(player);
            return;
        }

        // 手続き的呼吸・カメラ揺れの計算
        proceduralBreathTimer += dt;
        float breathAmpY = 0.0f;
        float breathAmpX = 0.0f;
        float breathFreq = 1.8f;

        if (fearLevel >= 0.80f)
        {
            breathAmpY = 0.032f;
            breathAmpX = 0.016f;
            breathFreq = 4.2f;
        }
        else if (fearLevel >= 0.60f)
        {
            breathAmpY = 0.022f;
            breathAmpX = 0.011f;
            breathFreq = 3.2f;
        }
        else if (fearLevel >= 0.40f)
        {
            breathAmpY = 0.014f;
            breathAmpX = 0.007f;
            breathFreq = 2.4f;
        }
        else if (fearLevel >= 0.20f)
        {
            breathAmpY = 0.007f;
            breathAmpX = 0.003f;
            breathFreq = 1.8f;
        }

        float breathOffY = std::sin(proceduralBreathTimer * breathFreq) * breathAmpY;
        player.SetEyeHeightOffset(breathOffY);

        // カメラ揺動計算
        swayTimer += dt;
        float swayIntensity = 0.0f;
        if (fearLevel >= 0.80f)
        {
            swayIntensity = 0.018f;
        }
        else if (fearLevel >= 0.60f)
        {
            swayIntensity = 0.010f;
        }

        if (swayIntensity > 0.001f)
        {
            float s1 = std::sin(swayTimer * 1.37f);
            float s2 = std::cos(swayTimer * 0.89f);
            float s3 = std::sin(swayTimer * 2.11f);

            VECTOR swayVec = VGet(s1 * swayIntensity, s2 * swayIntensity * 0.6f, 0.0f);
            float swayPitch = s2 * swayIntensity * 0.35f;
            float swayYaw = s3 * swayIntensity * 0.45f;
            player.SetSwayOffset(swayVec, swayPitch, swayYaw);
        }
        else
        {
            player.SetSwayOffset(VGet(0.0f, 0.0f, 0.0f), 0.0f, 0.0f);
        }

        // ホラー視覚パラメータのUI伝達
        float fearNorm = fearLevel;
        float vignette = currentVignetteFactor;
        float blur = currentBlurFactor * 0.6f;
        HorrorUI::Instance().SetHorrorVisualParams(fearNorm, vignette, 1.0f, 0.0f, 0.0f, blur);

        // モンスター追跡AIの更新
        UpdateMonsterChase(dt, player, bicycle, nightEnv, deliveryManager);

        // 前半および追跡非アクティブ時の演出更新
        UpdateAutoSpawns(dt, player, nightEnv);
        UpdatePeripheralScares(dt, player);
    }
    else if (gameState == HorrorGameState::Jumpscare)
    {
        // ジャンプスケア演出の進行
        UpdateJumpscareSequence(dt, player);
    }
}

// 3D怪異モデルの描画
void HorrorManager::Draw3D()
{
    entity.Draw();

    // デバッグ表示時の3D視覚化ガイド描画
    if (showDebugHUD && entity.IsActive())
    {
        VECTOR mPos = entity.GetPosition();
        VECTOR mEye = VGet(mPos.x, mPos.y + 1.6f, mPos.z);

        // 1. 視線レイ（LOS Ray: 障害物なし=緑、遮断=赤）
        unsigned int rayCol = lastLOSState ? GetColor(50, 240, 80) : GetColor(255, 40, 40);
        DrawLine3D(mEye, VGet(lastKnownPlayerPos.x, lastKnownPlayerPos.y + 1.4f, lastKnownPlayerPos.z), rayCol);

        // 2. 最後に確認したプレイヤー位置（オレンジ色球体）
        DrawSphere3D(lastKnownPlayerPos, 0.45f, 8, GetColor(255, 140, 20), GetColor(255, 140, 20), FALSE);

        // 3. ナビゲーションノード（シアン色点）
        for (const auto& node : navNodes)
        {
            DrawSphere3D(node.pos, 0.25f, 6, GetColor(40, 200, 240), GetColor(40, 200, 240), FALSE);
        }

        // 4. モンスター視野コーン（黄色）
        float mYaw = entity.GetYaw() - entity.GetModelForwardOffset();
        float leftYaw = mYaw - 1.22f; // ~70 deg
        float rightYaw = mYaw + 1.22f;
        float fovRange = currentParams.detectRange;
        VECTOR leftDir = VGet(std::sin(leftYaw) * fovRange, 0.0f, std::cos(leftYaw) * fovRange);
        VECTOR rightDir = VGet(std::sin(rightYaw) * fovRange, 0.0f, std::cos(rightYaw) * fovRange);
        DrawLine3D(mPos, VAdd(mPos, leftDir), GetColor(240, 230, 60));
        DrawLine3D(mPos, VAdd(mPos, rightDir), GetColor(240, 230, 60));
    }
}

// 2DホラーUI・ゲームオーバー・デバッグ描画
void HorrorManager::DrawUI()
{
    // 通常プレイ時の恐怖度メーター描画
    if (gameState == HorrorGameState::Normal)
    {
        HorrorUI::Instance().DrawFearMeter(fearLevel * 100.0f, inSafeLight);
    }
    else if (gameState == HorrorGameState::Jumpscare)
    {
        switch (jumpscareState)
        {
        case JumpscareState::Pause:
            HorrorUI::Instance().DrawScareDistortion(0.35f);
            break;
        case JumpscareState::Reveal:
            HorrorUI::Instance().DrawScareDistortion(0.55f);
            break;
        case JumpscareState::Rush:
            HorrorUI::Instance().DrawScareDistortion(0.85f);
            break;
        case JumpscareState::Impact:
            HorrorUI::Instance().DrawScareDistortion(1.0f);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
            DrawBox(0, 0, 1280, 720, GetColor(160, 20, 20), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            break;
        case JumpscareState::Fall:
            HorrorUI::Instance().DrawScareDistortion(0.70f);
            break;
        case JumpscareState::GroundImpact:
            HorrorUI::Instance().DrawScareDistortion(0.50f);
            break;
        case JumpscareState::GroundedStare:
            HorrorUI::Instance().DrawScareDistortion(0.30f);
            break;
        case JumpscareState::VisionClosing:
        {
            int alpha = static_cast<int>(screenFadeAlpha * 255.0f);
            if (alpha > 0)
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
                DrawBox(0, 0, 1280, 720, GetColor(0, 0, 0), TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
            break;
        }
        default:
            break;
        }
    }
    else if (gameState == HorrorGameState::GameOver)
    {
        HorrorUI::Instance().DrawGameOver(1.0f, true);
    }

    // デバッグHUD描画 (F11キー)
    if (showDebugHUD)
    {
        HorrorUI::Instance().DrawRetroPanel(18, 320, 440, 700,
            GetColor(90, 45, 45), HorrorUI::COL_PANEL_BG, 230);

        int lineY = 328;
        HorrorUI::Instance().DrawJitterString(26, lineY, "[ CHASE & HORROR DEBUG (F11) ]",
            GetColor(235, 100, 85), HorrorUI::Instance().GetFontSmall());
        lineY += 20;

        char buf[128];
        snprintf(buf, sizeof(buf), "CHASE UNLOCKED:   %s", isChaseUnlocked ? "YES (>=4/8 Deliveries)" : "NO (Locked)");
        HorrorUI::Instance().DrawJitterString(26, lineY, buf,
            isChaseUnlocked ? GetColor(110, 240, 140) : GetColor(180, 180, 180), HorrorUI::Instance().GetFontSmall());
        lineY += 18;

        snprintf(buf, sizeof(buf), "MONSTER STATE:    %s", GetChaseStateName());
        unsigned int stateCol = (chaseState == MonsterChaseState::Chasing) ? GetColor(255, 60, 60) :
                               (chaseState == MonsterChaseState::AvoidingLight) ? GetColor(255, 200, 60) :
                               GetColor(200, 210, 200);
        HorrorUI::Instance().DrawJitterString(26, lineY, buf, stateCol, HorrorUI::Instance().GetFontSmall());
        lineY += 18;

        const char* modelName = (entity.GetModelType() == HorrorModelType::Run) ? "MonsterRun.mv1" :
                                (entity.GetModelType() == HorrorModelType::Idle) ? "MonsterIdle.mv1" : "RunningCrawl.mv1";
        snprintf(buf, sizeof(buf), "MODEL:            %s", modelName);
        HorrorUI::Instance().DrawJitterString(26, lineY, buf, HorrorUI::COL_TEXT, HorrorUI::Instance().GetFontSmall());
        lineY += 18;

        snprintf(buf, sizeof(buf), "PLAYER VISIBLE:   %s", playerWasVisible ? "YES" : "NO");
        HorrorUI::Instance().DrawJitterString(26, lineY, buf,
            playerWasVisible ? GetColor(255, 100, 100) : GetColor(160, 170, 160), HorrorUI::Instance().GetFontSmall());
        lineY += 18;

        snprintf(buf, sizeof(buf), "LOS:              %s", lastLOSState ? "CLEAR" : "BLOCKED (House/Fence)");
        HorrorUI::Instance().DrawJitterString(26, lineY, buf,
            lastLOSState ? GetColor(110, 240, 140) : GetColor(240, 160, 80), HorrorUI::Instance().GetFontSmall());
        lineY += 18;

        snprintf(buf, sizeof(buf), "LAST KNOWN POS:   (%.1f, %.1f, %.1f)",
            lastKnownPlayerPos.x, lastKnownPlayerPos.y, lastKnownPlayerPos.z);
        HorrorUI::Instance().DrawJitterString(26, lineY, buf, HorrorUI::COL_TEXT_DIM, HorrorUI::Instance().GetFontSmall());
        lineY += 18;

        snprintf(buf, sizeof(buf), "LOST SIGHT TIMER: %.1fs", lostSightTimer);
        HorrorUI::Instance().DrawJitterString(26, lineY, buf, HorrorUI::COL_TEXT, HorrorUI::Instance().GetFontSmall());
        lineY += 18;

        if (cachedMonsterDist >= 0.0f)
        {
            snprintf(buf, sizeof(buf), "MONSTER DISTANCE: %.1fm", cachedMonsterDist);
        }
        else
        {
            snprintf(buf, sizeof(buf), "MONSTER DISTANCE: N/A (Inactive)");
        }
        HorrorUI::Instance().DrawJitterString(26, lineY, buf, HorrorUI::COL_TEXT, HorrorUI::Instance().GetFontSmall());
        lineY += 18;

        snprintf(buf, sizeof(buf), "BICYCLE BATTERY:  %.0f%%", cachedBicycleBattery);
        HorrorUI::Instance().DrawJitterString(26, lineY, buf, HorrorUI::COL_TEXT, HorrorUI::Instance().GetFontSmall());
        lineY += 18;

        snprintf(buf, sizeof(buf), "PLAYER SAFE:      %s", inSafeLight ? "YES" : "NO (Danger Darkness)");
        HorrorUI::Instance().DrawJitterString(26, lineY, buf,
            inSafeLight ? GetColor(110, 230, 160) : GetColor(255, 80, 80), HorrorUI::Instance().GetFontSmall());
        lineY += 18;

        snprintf(buf, sizeof(buf), "SAFE SOURCE:      %s", lastSafeReason);
        HorrorUI::Instance().DrawJitterString(26, lineY, buf, GetColor(245, 230, 140), HorrorUI::Instance().GetFontSmall());
        lineY += 18;

        snprintf(buf, sizeof(buf), "NAV NODE TARGET:  #%d", currentNavTargetNodeId);
        HorrorUI::Instance().DrawJitterString(26, lineY, buf, HorrorUI::COL_TEXT_DIM, HorrorUI::Instance().GetFontSmall());
        lineY += 18;

        snprintf(buf, sizeof(buf), "CHASE COOLDOWN:   %.1fs", chaseCooldownTimer);
        HorrorUI::Instance().DrawJitterString(26, lineY, buf, HorrorUI::COL_TEXT, HorrorUI::Instance().GetFontSmall());
        lineY += 18;

        snprintf(buf, sizeof(buf), "FEAR: %.1f%%  TENSION: %.1f%%  SPEED: %.1fm/s",
            fearLevel * 100.0f, currentCombinedTension * 100.0f, currentParams.runSpeed);
        HorrorUI::Instance().DrawJitterString(26, lineY, buf, GetColor(230, 160, 100), HorrorUI::Instance().GetFontSmall());
    }
}

// リトライ処理（恐怖度・プレイヤー位置・怪異状態のリセット）
void HorrorManager::RetryGame(Player& player)
{
    fearLevel = 0.0f;
    inSafeLight = true;
    safeGraceTimer = 3.0f;
    currentCombinedTension = 0.0f;
    gameState = HorrorGameState::Normal;
    jumpscareState = JumpscareState::None;
    scareTimer = 0.0f;
    screenFadeAlpha = 0.0f;
    deathFallProgress = 0.0f;
    deathBlinkTimer = 0.0f;
    deathBlinkFactor = 1.0f;
    currentVignetteFactor = 0.0f;
    currentBlurFactor = 0.0f;
    currentSwayFactor = 0.0f;
    focusBlurActiveTimer = 0.0f;
    doubleVisionActiveTimer = 0.0f;
    isPeripheralActive = false;

    // 追跡状態のリセット
    isChaseUnlocked = false;
    chaseState = MonsterChaseState::Dormant;
    chaseCooldownTimer = 0.0f;
    campingTimer = 0.0f;
    lostSightTimer = 0.0f;
    darknessTimeAccumulator = 0.0f;

    entity.Despawn();

    // 視覚パラメータのリセット
    HorrorUI::Instance().SetHorrorVisualParams(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);

    // 午前1:00にリセットし時計の進行を再開
    HorrorUI::Instance().SetGameTime(1, 0);
    HorrorUI::Instance().SetTimePaused(false);

    // カメラ各種オフセットのリセット
    player.SetCameraRoll(0.0f);
    player.SetEyeHeightOffset(0.0f);
    player.SetSwayOffset(VGet(0.0f, 0.0f, 0.0f), 0.0f, 0.0f);
    player.SetCameraShakeOffset(VGet(0.0f, 0.0f, 0.0f));

    // プレイヤー初期位置へ復元
    player.SetPosition(VGet(0.0f, 1.7f, -30.0f));
    player.SetPitch(0.0f);
    player.SetYaw(0.0f);
}

// 恐怖度段階取得
FearStage HorrorManager::GetFearStage() const
{
    if (fearLevel >= 1.0f) return FearStage::Maximum;
    if (fearLevel >= 0.80f) return FearStage::Terror;
    if (fearLevel >= 0.60f) return FearStage::Panic;
    if (fearLevel >= 0.40f) return FearStage::Afraid;
    if (fearLevel >= 0.20f) return FearStage::Uneasy;
    return FearStage::Calm;
}

// 操作制限判定
bool HorrorManager::IsControlRestricted() const
{
    return (gameState == HorrorGameState::Jumpscare || gameState == HorrorGameState::GameOver);
}

// デバッグ用恐怖度設定
void HorrorManager::DebugSetFear(float level)
{
    fearLevel = level;
    if (fearLevel < 0.0f) fearLevel = 0.0f;
    if (fearLevel > 1.0f) fearLevel = 1.0f;
}

// デバッグ用怪異出現
void HorrorManager::DebugTriggerSpawn(const Player& player)
{
    VECTOR forward = player.GetForward();
    forward.y = 0.0f;
    float len = VSize(forward);
    if (len > 0.001f) forward = VScale(forward, 1.0f / len);
    else forward = VGet(0.0f, 0.0f, 1.0f);

    VECTOR playerPos = player.GetPosition();
    VECTOR spawnPos = VAdd(playerPos, VScale(forward, 7.5f));
    spawnPos.y = 0.0f;

    float yaw = std::atan2(playerPos.x - spawnPos.x, playerPos.z - spawnPos.z);
    entity.Spawn(spawnPos, yaw, 6.0f, 1.8f, HorrorModelType::Idle);
}

// デバッグ用視界端怪異出現
void HorrorManager::DebugTriggerPeripheralScare(const Player& player)
{
    VECTOR fwd = player.GetForward();
    fwd.y = 0.0f;
    float len = VSize(fwd);
    if (len > 0.001f) fwd = VScale(fwd, 1.0f / len);
    else fwd = VGet(0.0f, 0.0f, 1.0f);

    float sideAngle = 1.15f;
    VECTOR dir;
    dir.x = fwd.x * std::cos(sideAngle) - fwd.z * std::sin(sideAngle);
    dir.y = 0.0f;
    dir.z = fwd.x * std::sin(sideAngle) + fwd.z * std::cos(sideAngle);

    VECTOR pPos = player.GetPosition();
    VECTOR spawnPos = VAdd(pPos, VScale(dir, 7.2f));
    spawnPos.y = 0.0f;

    float yaw = std::atan2(pPos.x - spawnPos.x, pPos.z - spawnPos.z);
    entity.Spawn(spawnPos, yaw, 5.0f, 0.0f, HorrorModelType::Idle);
    isPeripheralActive = true;
}

// デバッグ用街灯明滅イベント
void HorrorManager::DebugTriggerStreetLightEvent(NightEnvironment& nightEnv, const Player& player)
{
    VECTOR pPos = player.GetPosition();
    int nearestId = -1;
    float nearestDist = 999.0f;
    for (const auto& light : nightEnv.GetStreetLights())
    {
        float dx = light.position.x - pPos.x;
        float dz = light.position.z - pPos.z;
        float dist = std::sqrt(dx * dx + dz * dz);
        if (dist < nearestDist && dist < 20.0f)
        {
            nearestDist = dist;
            nearestId = light.id;
        }
    }
    if (nearestId != -1)
    {
        nightEnv.TriggerStreetLightFlicker(nearestId);
    }
}

// デバッグ用即時ゲームオーバートリガー
void HorrorManager::DebugTriggerMaxFearGameOver(Player& player)
{
    fearLevel = 1.0f;
    TriggerMaxFearSequence(player);
}

// 終了処理
void HorrorManager::Finalize()
{
    entity.Finalize();
}
