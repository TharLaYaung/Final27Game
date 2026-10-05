#include "HorrorManager.h"
#include "Player.h"
#include "Bicycle.h"
#include "NightEnvironment.h"
#include "HorrorUI.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

// コンストラクタ
HorrorManager::HorrorManager()
    : fearLevel(0.0f)
    , inSafeLight(true)
    , safeGraceTimer(0.8f)
    , gameState(HorrorGameState::Normal)
    , jumpscareState(JumpscareState::None)
    , scareTimer(0.0f)
    , screenFadeAlpha(0.0f)
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

// 初期化処理
bool HorrorManager::Initialize()
{
    fearLevel = 0.0f;
    inSafeLight = true;
    safeGraceTimer = 0.8f;
    gameState = HorrorGameState::Normal;
    jumpscareState = JumpscareState::None;
    scareTimer = 0.0f;
    screenFadeAlpha = 0.0f;
    scareCamPos = VGet(0.0f, 0.0f, 0.0f);
    scareCamForward = VGet(0.0f, 0.0f, 1.0f);
    spawnCheckTimer = 10.0f;
    lastSpawnId = -1;
    showDebugHUD = false;

    // 既存の怪異3Dモデルの初期化
    entity.Initialize();

    // マップ上の制御された出現候補地点の登録（路地・電柱陰・行き止まり・交差点）
    spawnPoints.clear();
    spawnPoints.push_back({ 0, VGet(  0.0f, 0.0f,  36.0f), "North Dead End (House 2 Area)" });
    spawnPoints.push_back({ 1, VGet(-22.0f, 0.0f,   8.0f), "North-West Intersection (Flicker Lamp)" });
    spawnPoints.push_back({ 2, VGet( 22.0f, 0.0f,  -5.0f), "East Street Foggy Corner" });
    spawnPoints.push_back({ 3, VGet( -7.0f, 0.0f,  25.0f), "Between Houses 1 & 2 Alley" });
    spawnPoints.push_back({ 4, VGet( 18.0f, 0.0f, -27.0f), "South-East Road End" });
    spawnPoints.push_back({ 5, VGet(-22.0f, 0.0f, -20.0f), "South-West Corner (House 7)" });
    spawnPoints.push_back({ 6, VGet(  6.0f, 0.0f,  -7.0f), "Central Alley Shadow (House 4)" });
    spawnPoints.push_back({ 7, VGet( 22.0f, 0.0f,  20.0f), "North-East Foggy Dead End" });

    return true;
}

// 恐怖段階の判定取得
FearStage HorrorManager::GetFearStage() const
{
    if (fearLevel >= 1.0f) return FearStage::Maximum;
    if (fearLevel >= 0.80f) return FearStage::Terror;
    if (fearLevel >= 0.60f) return FearStage::Panic;
    if (fearLevel >= 0.40f) return FearStage::Afraid;
    if (fearLevel >= 0.20f) return FearStage::Uneasy;
    return FearStage::Calm;
}

// プレイヤー操作制限フラグ（ゲームオーバー演出中は操作不可）
bool HorrorManager::IsControlRestricted() const
{
    return (gameState != HorrorGameState::Normal);
}

// 安全光判定（自転車ヘッドライト照射・街灯直下）
bool HorrorManager::CheckSafeLight(const Player& player, const Bicycle& bicycle, const NightEnvironment& nightEnv)
{
    VECTOR playerPos = player.GetPosition();

    // 自転車乗車中かつヘッドライト点灯時は常に安全光内
    if (bicycle.IsRiding() && bicycle.IsHeadlightOn() && bicycle.GetHeadlightIntensity() > 0.05f)
    {
        return true;
    }

    // 駐輪中の自転車ヘッドライトのコーン判定
    if (bicycle.IsHeadlightOn() && bicycle.GetHeadlightIntensity() > 0.05f)
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
            // スポットライト照射角42度 (cos(42度) 約0.743)
            if (dot > 0.743f)
            {
                return true;
            }
        }
    }

    // 街灯ポイントライトの直下判定（半径6.5mの光の輪）
    const std::vector<StreetLight>& lights = nightEnv.GetStreetLights();
    for (size_t i = 0; i < lights.size(); ++i)
    {
        if (lights[i].state == StreetLightState::Off || lights[i].currentIntensity < 0.15f)
        {
            continue;
        }

        float dx = playerPos.x - lights[i].position.x;
        float dz = playerPos.z - lights[i].position.z;
        float distSq = dx * dx + dz * dz;

        if (distSq < (6.5f * 6.5f))
        {
            return true;
        }
    }

    return false;
}

// 最大恐怖時のジャンプスケア演出開始
void HorrorManager::TriggerMaxFearSequence(Player& player)
{
    if (gameState != HorrorGameState::Normal)
    {
        return;
    }

    fearLevel = 1.0f;
    gameState = HorrorGameState::Jumpscare;
    jumpscareState = JumpscareState::Pause;
    scareTimer = 0.35f;
    screenFadeAlpha = 0.0f;

    // プレイヤーの現在カメラ位置・視線方向を固定記録
    scareCamPos = player.GetPosition();
    scareCamForward = player.GetForward();

    // 視線方向の正規化
    float len = VSize(scareCamForward);
    if (len > 0.001f)
    {
        scareCamForward = VScale(scareCamForward, 1.0f / len);
    }
    else
    {
        scareCamForward = VGet(0.0f, 0.0f, 1.0f);
    }
}

// ジャンプスケア演出の進行更新
void HorrorManager::UpdateJumpscareSequence(float dt, Player& player)
{
    // 怪異モデル（RunningCrawl）の顔面・頭部ローカルオフセット
    const float HEAD_HEIGHT_OFFSET = 0.47f;
    const float HEAD_FORWARD_OFFSET = 0.56f;
    const float START_DISTANCE = 4.20f;
    const float IMPACT_DISTANCE = 0.45f;

    // 頭部目標位置を元に怪異モデルを配置し、視覚的正面をカメラに向けるラムダ
    auto PlaceModelForHead = [&](const VECTOR& targetHead)
    {
        float dx = scareCamPos.x - targetHead.x;
        float dz = scareCamPos.z - targetHead.z;
        float angleToCam = std::atan2(dx, dz);
        float yaw = angleToCam + HorrorEntity::MODEL_FORWARD_OFFSET;

        VECTOR modelPos;
        modelPos.x = targetHead.x - std::sin(angleToCam) * HEAD_FORWARD_OFFSET;
        modelPos.y = targetHead.y - HEAD_HEIGHT_OFFSET;
        modelPos.z = targetHead.z - std::cos(angleToCam) * HEAD_FORWARD_OFFSET;

        entity.SetDirectTransform(modelPos, yaw);
    };

    switch (jumpscareState)
    {
    case JumpscareState::Pause:
        // フェーズ1: 前兆微小硬直（暗転・静寂・操作不能化、0.35秒）
        player.SetCameraShakeOffset(VGet(0.0f, 0.0f, 0.0f));
        scareTimer -= dt;
        if (scareTimer <= 0.0f)
        {
            jumpscareState = JumpscareState::Reveal;
            scareTimer = 0.40f;
            VECTOR headPos = VAdd(scareCamPos, VScale(scareCamForward, START_DISTANCE));
            PlaceModelForHead(headPos);
        }
        break;

    case JumpscareState::Reveal:
        // フェーズ2: 前方暗闇に出現（0.40秒）
        {
            VECTOR headPos = VAdd(scareCamPos, VScale(scareCamForward, START_DISTANCE));
            PlaceModelForHead(headPos);
            entity.AdvanceAnim(dt, 1.5f);

            // 前兆微小振動
            float shakeX = ((rand() % 100) / 50.0f - 1.0f) * 0.015f;
            float shakeY = ((rand() % 100) / 50.0f - 1.0f) * 0.015f;
            player.SetCameraShakeOffset(VGet(shakeX, shakeY, 0.0f));

            scareTimer -= dt;
            if (scareTimer <= 0.0f)
            {
                jumpscareState = JumpscareState::Rush;
                scareTimer = 0.42f;
            }
        }
        break;

    case JumpscareState::Rush:
        // フェーズ3: カメラ正面への急加速突進（0.42秒、イーズイン加速）
        {
            const float RUSH_DURATION = 0.42f;
            float t = 1.0f - (scareTimer / RUSH_DURATION);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            // 加速カーブ（二次イーズイン）
            float curve = t * t;
            float dist = START_DISTANCE - curve * (START_DISTANCE - IMPACT_DISTANCE);

            VECTOR headPos = VAdd(scareCamPos, VScale(scareCamForward, dist));
            PlaceModelForHead(headPos);
            entity.AdvanceAnim(dt, 2.5f);

            // 突進加速に連動したカメラ揺れ増大
            float shakeMag = 0.02f + curve * 0.08f;
            float shakeX = ((rand() % 100) / 50.0f - 1.0f) * shakeMag;
            float shakeY = ((rand() % 100) / 50.0f - 1.0f) * shakeMag;
            player.SetCameraShakeOffset(VGet(shakeX, shakeY, 0.0f));

            scareTimer -= dt;
            if (scareTimer <= 0.0f)
            {
                jumpscareState = JumpscareState::Impact;
                scareTimer = 0.28f;
            }
        }
        break;

    case JumpscareState::Impact:
        // フェーズ4: 画面激突・顔面大迫力停止（0.28秒、激震）
        {
            VECTOR headPos = VAdd(scareCamPos, VScale(scareCamForward, IMPACT_DISTANCE));
            PlaceModelForHead(headPos);
            entity.AdvanceAnim(dt, 3.0f);

            // 激突インパクト時の最大カメラ揺れ
            float shakeX = ((rand() % 100) / 50.0f - 1.0f) * 0.14f;
            float shakeY = ((rand() % 100) / 50.0f - 1.0f) * 0.14f;
            player.SetCameraShakeOffset(VGet(shakeX, shakeY, 0.0f));

            scareTimer -= dt;
            if (scareTimer <= 0.0f)
            {
                jumpscareState = JumpscareState::FadeOut;
                scareTimer = 0.40f;
            }
        }
        break;

    case JumpscareState::FadeOut:
        // フェーズ5: 暗転フェードアウト（0.40秒）
        {
            VECTOR headPos = VAdd(scareCamPos, VScale(scareCamForward, IMPACT_DISTANCE));
            PlaceModelForHead(headPos);

            const float FADE_DURATION = 0.40f;
            screenFadeAlpha = 1.0f - (scareTimer / FADE_DURATION);
            if (screenFadeAlpha > 1.0f) screenFadeAlpha = 1.0f;

            float dampen = (scareTimer > 0.0f) ? (scareTimer / FADE_DURATION) : 0.0f;
            float shakeX = ((rand() % 100) / 50.0f - 1.0f) * 0.05f * dampen;
            float shakeY = ((rand() % 100) / 50.0f - 1.0f) * 0.05f * dampen;
            player.SetCameraShakeOffset(VGet(shakeX, shakeY, 0.0f));

            scareTimer -= dt;
            if (scareTimer <= 0.0f)
            {
                gameState = HorrorGameState::GameOver;
                jumpscareState = JumpscareState::None;
                screenFadeAlpha = 1.0f;
                player.SetCameraShakeOffset(VGet(0.0f, 0.0f, 0.0f));
                entity.Despawn();
            }
        }
        break;

    default:
        break;
    }
}

// 恐怖段階に応じた怪異の自動出現判定
void HorrorManager::UpdateAutoSpawns(float dt, const Player& player)
{
    if (entity.IsActive() || gameState != HorrorGameState::Normal)
    {
        return;
    }

    spawnCheckTimer -= dt;
    if (spawnCheckTimer > 0.0f)
    {
        return;
    }

    // 次回チェック間隔（10〜16秒）
    spawnCheckTimer = 10.0f + static_cast<float>(GetRand(60)) / 10.0f;

    FearStage stage = GetFearStage();
    if (stage == FearStage::Calm)
    {
        return;
    }

    // 各恐怖段階における出現確率と距離パラメータ
    int spawnChance = 0;
    float minDist = 0.0f;
    float maxDist = 0.0f;
    float duration = 4.0f;
    float crawlSpeed = 0.0f;

    switch (stage)
    {
    case FearStage::Uneasy:
        spawnChance = 15;
        minDist = 25.0f;
        maxDist = 38.0f;
        duration = 3.0f;
        crawlSpeed = 0.0f;
        break;

    case FearStage::Afraid:
        spawnChance = 40;
        minDist = 20.0f;
        maxDist = 32.0f;
        duration = 5.0f;
        crawlSpeed = 0.0f;
        break;

    case FearStage::Panic:
        spawnChance = 65;
        minDist = 14.0f;
        maxDist = 24.0f;
        duration = 4.5f;
        crawlSpeed = 1.6f;
        break;

    case FearStage::Terror:
        spawnChance = 85;
        minDist = 9.0f;
        maxDist = 18.0f;
        duration = 4.0f;
        crawlSpeed = 3.0f;
        break;

    default:
        break;
    }

    if (GetRand(99) >= spawnChance)
    {
        return;
    }

    VECTOR playerPos = player.GetPosition();
    std::vector<int> candidates;

    for (size_t i = 0; i < spawnPoints.size(); ++i)
    {
        if (static_cast<int>(i) == lastSpawnId) continue;

        float dx = spawnPoints[i].position.x - playerPos.x;
        float dz = spawnPoints[i].position.z - playerPos.z;
        float d = std::sqrt(dx * dx + dz * dz);

        if (d >= minDist && d <= maxDist)
        {
            candidates.push_back(static_cast<int>(i));
        }
    }

    if (!candidates.empty())
    {
        int chosenIndex = candidates[GetRand(static_cast<int>(candidates.size()) - 1)];
        lastSpawnId = chosenIndex;

        const VECTOR& p = spawnPoints[chosenIndex].position;
        float yaw = std::atan2(playerPos.x - p.x, playerPos.z - p.z);
        entity.Spawn(p, yaw, duration, crawlSpeed);
    }
}

// 毎フレーム更新処理
void HorrorManager::Update(float dt, Player& player, const Bicycle& bicycle, const NightEnvironment& nightEnv)
{
    // 通常ゲームプレイ中の恐怖度更新
    if (gameState == HorrorGameState::Normal)
    {
        inSafeLight = CheckSafeLight(player, bicycle, nightEnv);

        if (inSafeLight)
        {
            // 安全光内：猶予タイマーリセットおよび恐怖度の減少回復
            safeGraceTimer = 0.8f;
            fearLevel -= 0.08f * dt;
            if (fearLevel < 0.0f)
            {
                fearLevel = 0.0f;
            }
        }
        else
        {
            // 暗闇内：猶予タイマー（0.8秒）消費後に恐怖度が高速上昇（毎秒10%）
            if (safeGraceTimer > 0.0f)
            {
                safeGraceTimer -= dt;
            }
            else
            {
                fearLevel += 0.10f * dt;
            }
        }

        // 怪異が付近を徘徊・接近している場合の追加恐怖度上昇
        if (entity.IsActive())
        {
            VECTOR playerPos = player.GetPosition();
            VECTOR entityPos = entity.GetPosition();
            float dx = playerPos.x - entityPos.x;
            float dz = playerPos.z - entityPos.z;
            float dist = std::sqrt(dx * dx + dz * dz);

            if (dist < 18.0f)
            {
                // 至近距離ほど急速に恐怖度が加速
                float proximityFactor = (1.0f - (dist / 18.0f));
                fearLevel += (0.04f + 0.08f * proximityFactor) * dt;
            }
        }

        // 100%到達時にジャンプスケア演出へ移行
        if (fearLevel >= 1.0f)
        {
            fearLevel = 1.0f;
            TriggerMaxFearSequence(player);
        }

        // 怪異実体の更新および自動出現チェック
        entity.Update(dt, player.GetPosition());
        UpdateAutoSpawns(dt, player);
    }
    else if (gameState == HorrorGameState::Jumpscare)
    {
        // ジャンプスケア演出の進行更新
        UpdateJumpscareSequence(dt, player);
    }
    else if (gameState == HorrorGameState::GameOver)
    {
        // ゲームオーバー待機画面：Rキーでリトライ
        if (CheckHitKey(KEY_INPUT_R))
        {
            RetryGame(player);
        }
    }
}

// 3D怪異モデルの描画
void HorrorManager::Draw3D()
{
    entity.Draw();
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
        // ジャンプスケア各フェーズに応じた視覚エフェクト
        switch (jumpscareState)
        {
        case JumpscareState::Pause:
            HorrorUI::Instance().DrawScareDistortion(0.35f);
            break;

        case JumpscareState::Reveal:
            HorrorUI::Instance().DrawScareDistortion(0.55f);
            break;

        case JumpscareState::Rush:
            {
                float progress = 1.0f - (scareTimer / 0.42f);
                if (progress < 0.0f) progress = 0.0f;
                if (progress > 1.0f) progress = 1.0f;
                HorrorUI::Instance().DrawScareDistortion(0.60f + progress * 0.40f);
            }
            break;

        case JumpscareState::Impact:
            HorrorUI::Instance().DrawScareDistortion(1.0f);
            // 激突時の一瞬の暗赤色フラッシュ
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
            DrawBox(0, 0, 1280, 720, GetColor(160, 20, 20), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            break;

        case JumpscareState::FadeOut:
            HorrorUI::Instance().DrawGameOver(screenFadeAlpha, false);
            break;

        default:
            break;
        }

        HorrorUI::Instance().DrawFearMeter(100.0f, false);
    }
    else if (gameState == HorrorGameState::GameOver)
    {
        // ゲームオーバーダイアログ画面
        HorrorUI::Instance().DrawGameOver(1.0f, true);
    }

    // デバッグHUD描画 (F11)
    if (showDebugHUD)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 215);
        DrawBox(18, 430, 360, 600, GetColor(10, 14, 16), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(18, 430, 360, 600, GetColor(120, 60, 60), FALSE);

        DrawString(26, 436, "[ FEAR & ENTITY DEBUG (F11) ]", GetColor(255, 120, 100));

        char buf[128];
        snprintf(buf, sizeof(buf), "MODEL:    Data/Model/RunningCrawl.mv1");
        DrawString(26, 458, buf, GetColor(210, 220, 230));

        const char* stageName = "CALM";
        switch (GetFearStage())
        {
        case FearStage::Uneasy:  stageName = "UNEASY"; break;
        case FearStage::Afraid:  stageName = "AFRAID"; break;
        case FearStage::Panic:   stageName = "PANIC"; break;
        case FearStage::Terror:  stageName = "TERROR"; break;
        case FearStage::Maximum: stageName = "MAXIMUM"; break;
        default: break;
        }

        snprintf(buf, sizeof(buf), "FEAR:     %.1f%% [%s]", fearLevel * 100.0f, stageName);
        DrawString(26, 478, buf, GetColor(255, 180, 80));

        snprintf(buf, sizeof(buf), "LIGHT:    %s (Grace: %.1fs)", inSafeLight ? "SAFE LIGHT" : "DARK DANGER", safeGraceTimer);
        DrawString(26, 498, buf, inSafeLight ? GetColor(120, 210, 160) : GetColor(230, 110, 90));

        if (entity.IsActive())
        {
            VECTOR ep = entity.GetPosition();
            snprintf(buf, sizeof(buf), "ENTITY:   ACTIVE (Life: %.1fs)", entity.GetRemainingTime());
            DrawString(26, 518, buf, GetColor(255, 80, 80));
            snprintf(buf, sizeof(buf), "POS:      (%.1f, %.1f)", ep.x, ep.z);
            DrawString(26, 536, buf, GetColor(200, 200, 200));
        }
        else
        {
            DrawString(26, 518, "ENTITY:   INACTIVE", GetColor(140, 150, 150));
            snprintf(buf, sizeof(buf), "NEXT CHK: %.1fs", spawnCheckTimer);
            DrawString(26, 536, buf, GetColor(160, 180, 180));
        }

        DrawString(26, 560, "KEYS: [1:25%] [2:50%] [3:75%] [4:95%]", GetColor(170, 190, 180));
        DrawString(26, 578, "      [5/J:JUMPSCARE OVER] [6:SPAWN]", GetColor(170, 190, 180));
    }
}

// リトライ処理（恐怖度・プレイヤー位置・怪異状態のリセット）
void HorrorManager::RetryGame(Player& player)
{
    fearLevel = 0.0f;
    inSafeLight = true;
    safeGraceTimer = 3.0f;
    gameState = HorrorGameState::Normal;
    jumpscareState = JumpscareState::None;
    scareTimer = 0.0f;
    screenFadeAlpha = 0.0f;
    entity.Despawn();

    // カメラシェイクのリセット
    player.SetCameraShakeOffset(VGet(0.0f, 0.0f, 0.0f));

    // プレイヤー位置を南通りスタート地点へ復帰
    player.SetPosition(VGet(0.0f, 1.7f, -30.0f));
    player.SetPitch(0.0f);
    player.SetYaw(0.0f);
}

// デバッグ用恐怖度強制設定
void HorrorManager::DebugSetFear(float level)
{
    fearLevel = level;
    if (fearLevel < 0.0f) fearLevel = 0.0f;
    if (fearLevel > 1.0f) fearLevel = 1.0f;
}

// デバッグ用怪異強制出現（プレイヤーの前方16m）
void HorrorManager::DebugTriggerSpawn(const Player& player)
{
    VECTOR forward = player.GetForward();
    forward.y = 0.0f;
    float len = VSize(forward);
    if (len > 0.001f) forward = VScale(forward, 1.0f / len);
    else forward = VGet(0.0f, 0.0f, 1.0f);

    VECTOR playerPos = player.GetPosition();
    VECTOR spawnPos = VAdd(playerPos, VScale(forward, 16.0f));
    spawnPos.y = 0.0f;

    float yaw = std::atan2(playerPos.x - spawnPos.x, playerPos.z - spawnPos.z);
    entity.Spawn(spawnPos, yaw, 6.0f, 1.8f);
}

// デバッグ用最大恐怖ゲームオーバー即時トリガー
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
