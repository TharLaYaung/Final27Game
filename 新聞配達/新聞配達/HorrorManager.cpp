#include "HorrorManager.h"
#include "DeliveryManager.h"
#include "Player.h"
#include "Bicycle.h"
#include "NightEnvironment.h"
#include "HorrorUI.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

// コンストラクタ
HorrorManager::HorrorManager()
    : currentDay(1)
    , fearRiseMultiplier(1.0f)
    , spawnChanceBonus(0)
    , entitySpeedMultiplier(1.0f)
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

// 7日間難易度設定
void HorrorManager::SetDayDifficulty(int day)
{
    if (day < 1) day = 1;
    if (day > 7) day = 7;
    currentDay = day;

    switch (currentDay)
    {
    case 1:
        // Day 1（チュートリアル夜）: 恐怖上昇は穏やか、怪異はほぼ出現しない
        fearRiseMultiplier = 0.60f;
        spawnChanceBonus = -12;
        entitySpeedMultiplier = 0.70f;
        break;
    case 2:
        // Day 2（小雨）: わずかに警戒
        fearRiseMultiplier = 0.80f;
        spawnChanceBonus = -6;
        entitySpeedMultiplier = 0.85f;
        break;
    case 3:
        // Day 3（濃霧）: 標準恐怖
        fearRiseMultiplier = 1.00f;
        spawnChanceBonus = 0;
        entitySpeedMultiplier = 1.00f;
        break;
    case 4:
        // Day 4（豪雨・停電）: 暗闇での恐怖蓄積が加速
        fearRiseMultiplier = 1.25f;
        spawnChanceBonus = 10;
        entitySpeedMultiplier = 1.15f;
        break;
    case 5:
        // Day 5（降雪）: 怪異の接近が敏捷化
        fearRiseMultiplier = 1.50f;
        spawnChanceBonus = 16;
        entitySpeedMultiplier = 1.30f;
        break;
    case 6:
        // Day 6（猛吹雪）: 非常に危険な夜
        fearRiseMultiplier = 1.80f;
        spawnChanceBonus = 22;
        entitySpeedMultiplier = 1.45f;
        break;
    case 7:
    default:
        // Day 7（最終夜）: 極限の恐怖と容赦ない怪異の追跡
        fearRiseMultiplier = 2.20f;
        spawnChanceBonus = 30;
        entitySpeedMultiplier = 1.65f;
        break;
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

    // 初日難易度設定
    SetDayDifficulty(1);

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

    // ゲームオーバー演出移行時に時計の進行を停止する
    HorrorUI::Instance().SetTimePaused(true);

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

// ジャンプスケア演出の進行更新（シネマティック死亡シーケンス: 倒れ込み・ロール・直視・最後のまばたき）
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
        player.SetCameraRoll(0.0f);
        player.SetEyeHeightOffset(0.0f);
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
                scareTimer = 0.38f;
            }
        }
        break;

    case JumpscareState::Rush:
        // フェーズ3: カメラ正面への急加速突進（0.38秒、イーズイン加速）
        {
            const float RUSH_DURATION = 0.38f;
            float t = 1.0f - (scareTimer / RUSH_DURATION);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            float curve = t * t;
            float dist = START_DISTANCE - curve * (START_DISTANCE - IMPACT_DISTANCE);

            VECTOR headPos = VAdd(scareCamPos, VScale(scareCamForward, dist));
            PlaceModelForHead(headPos);
            entity.AdvanceAnim(dt, 2.5f);

            // 突進加速に連動したカメラ揺れ増大
            float shakeMag = 0.02f + curve * 0.09f;
            float shakeX = ((rand() % 100) / 50.0f - 1.0f) * shakeMag;
            float shakeY = ((rand() % 100) / 50.0f - 1.0f) * shakeMag;
            player.SetCameraShakeOffset(VGet(shakeX, shakeY, 0.0f));

            scareTimer -= dt;
            if (scareTimer <= 0.0f)
            {
                jumpscareState = JumpscareState::Impact;
                scareTimer = 0.22f;
            }
        }
        break;

    case JumpscareState::Impact:
        // フェーズ3後半: 画面激突・顔面大迫力停止・激震（0.22秒）
        {
            VECTOR headPos = VAdd(scareCamPos, VScale(scareCamForward, IMPACT_DISTANCE));
            PlaceModelForHead(headPos);
            entity.AdvanceAnim(dt, 3.0f);

            // 激突インパクト時の最大カメラ揺れ
            float shakeX = ((rand() % 100) / 50.0f - 1.0f) * 0.16f;
            float shakeY = ((rand() % 100) / 50.0f - 1.0f) * 0.16f;
            player.SetCameraShakeOffset(VGet(shakeX, shakeY, 0.0f));

            scareTimer -= dt;
            if (scareTimer <= 0.0f)
            {
                jumpscareState = JumpscareState::Fall;
                scareTimer = 0.75f;
            }
        }
        break;

    case JumpscareState::Fall:
        // フェーズ4: 倒れ込み・カメラ高度急降下 (1.7m -> 0.28m) ＆ ロール傾斜 (0 -> 66度)（0.75秒）
        {
            const float FALL_DURATION = 0.75f;
            float t = 1.0f - (scareTimer / FALL_DURATION);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            // 加速落下カーブ
            float fallCurve = t * t;
            float dropHeight = -(1.7f - 0.28f) * fallCurve;
            float rollAngle = 1.15f * fallCurve; // 約66度の横倒れ傾斜

            player.SetEyeHeightOffset(dropHeight);
            player.SetCameraRoll(rollAngle);

            // 怪異モデルを倒れ込んだプレイヤーの前方アスファルト上に配置
            VECTOR headPos = VAdd(scareCamPos, VScale(scareCamForward, 1.55f));
            headPos.y = 0.0f;
            PlaceModelForHead(headPos);
            entity.AdvanceAnim(dt, 1.0f);

            // 落下中の微小揺れ減衰
            float shakeMag = 0.04f * (1.0f - t);
            float shakeX = ((rand() % 100) / 50.0f - 1.0f) * shakeMag;
            float shakeY = ((rand() % 100) / 50.0f - 1.0f) * shakeMag;
            player.SetCameraShakeOffset(VGet(shakeX, shakeY, 0.0f));

            scareTimer -= dt;
            if (scareTimer <= 0.0f)
            {
                jumpscareState = JumpscareState::GroundImpact;
                scareTimer = 0.20f;
            }
        }
        break;

    case JumpscareState::GroundImpact:
        // フェーズ4後半: 地面衝突の一撃衝撃・鈍い鈍痛（0.20秒）
        {
            player.SetEyeHeightOffset(-(1.7f - 0.28f));
            player.SetCameraRoll(1.15f);

            // 地面激突の一撃ショック
            float thudX = ((rand() % 100) / 50.0f - 1.0f) * 0.05f;
            float thudY = -0.06f; // 下向きの強烈な底打ち
            player.SetCameraShakeOffset(VGet(thudX, thudY, 0.0f));

            VECTOR headPos = VAdd(scareCamPos, VScale(scareCamForward, 1.55f));
            headPos.y = 0.0f;
            PlaceModelForHead(headPos);

            scareTimer -= dt;
            if (scareTimer <= 0.0f)
            {
                jumpscareState = JumpscareState::GroundedStare;
                scareTimer = 1.50f;
            }
        }
        break;

    case JumpscareState::GroundedStare:
        // フェーズ5: 地面横倒し視点・眼前に直立する怪異の直視・静かな恐怖（1.50秒）
        {
            player.SetEyeHeightOffset(-(1.7f - 0.28f));
            player.SetCameraRoll(1.15f);

            // 地面に倒れ込んだ状態での微弱な荒い呼吸揺れ
            float breathe = std::sin((1.50f - scareTimer) * 2.8f) * 0.007f;
            player.SetCameraShakeOffset(VGet(0.0f, breathe, 0.0f));

            // 怪異モデルを眼前に配置し、顔面をカメラへ直視させる
            VECTOR headPos = VAdd(scareCamPos, VScale(scareCamForward, 1.55f));
            headPos.y = 0.0f;
            PlaceModelForHead(headPos);
            entity.AdvanceAnim(dt, 0.4f);

            scareTimer -= dt;
            if (scareTimer <= 0.0f)
            {
                jumpscareState = JumpscareState::VisionClosing;
                scareTimer = 2.00f;
            }
        }
        break;

    case JumpscareState::VisionClosing:
        // フェーズ6: 視界狭窄・最後のまばたき・暗転（2.00秒）
        {
            const float CLOSE_DURATION = 2.00f;
            float t = 1.0f - (scareTimer / CLOSE_DURATION);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            deathFallProgress = t;

            // 最後のまばたき（視界が急速に細まった後、一瞬僅かに見開き、そして永遠に閉じる）
            if (t < 0.40f)
            {
                deathBlinkFactor = 1.0f - (t / 0.40f) * 0.85f;
            }
            else if (t < 0.70f)
            {
                float flutter = std::sin((t - 0.40f) / 0.30f * DX_PI_F);
                deathBlinkFactor = 0.15f + flutter * 0.42f;
            }
            else
            {
                deathBlinkFactor = 0.0f;
            }

            // ホラーUIへ視覚パラメータ伝達
            HorrorUI::Instance().SetHorrorVisualParams(1.0f, deathFallProgress, deathBlinkFactor, 0.0f, 0.85f, 0.6f);

            player.SetEyeHeightOffset(-(1.7f - 0.28f));
            player.SetCameraRoll(1.15f);
            player.SetCameraShakeOffset(VGet(0.0f, 0.0f, 0.0f));

            VECTOR headPos = VAdd(scareCamPos, VScale(scareCamForward, 1.55f));
            headPos.y = 0.0f;
            PlaceModelForHead(headPos);
            entity.AdvanceAnim(dt, 0.2f);

            scareTimer -= dt;
            if (scareTimer <= 0.0f)
            {
                gameState = HorrorGameState::GameOver;
                jumpscareState = JumpscareState::GameOver;
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

// 恐怖段階に応じた怪異の自動出現判定（濃霧視程・時間帯・配達進捗連携）
void HorrorManager::UpdateAutoSpawns(float dt, const Player& player, const NightEnvironment& nightEnv)
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

    int timeLevel = HorrorUI::Instance().GetHorrorTimeLevel();

    // 時間帯に応じた次回チェック間隔（深夜3時以降や高緊張時は出現頻度上昇）
    float baseInterval = 12.0f;
    if (timeLevel == 0) baseInterval = 16.0f;       // 01時台: 序盤・怪異出現チェック間隔を長めに設定
    else if (timeLevel == 1) baseInterval = 12.0f;  // 02時台: 緊張上昇
    else if (timeLevel == 2) baseInterval = 8.5f;   // 03時台: 深夜3時の魔窟・出現頻度高
    else baseInterval = 7.0f;                       // 04時台以降: 極限状態

    spawnCheckTimer = baseInterval + static_cast<float>(GetRand(40)) / 10.0f;

    FearStage stage = GetFearStage();
    if (stage == FearStage::Calm)
    {
        return;
    }

    // 現在の環境の濃霧終端距離を取得し、怪異出現位置を霧の視認限界に連動させる
    float fogEnd = nightEnv.GetFogEnd();

    // 各恐怖段階における出現確率と距離パラメータ（濃霧により完全遮蔽されないよう動的調整）
    int spawnChance = 0;
    float minDist = 0.0f;
    float maxDist = 0.0f;
    float duration = 4.0f;
    float crawlSpeed = 0.0f;

    switch (stage)
    {
    case FearStage::Uneasy:
        // 警戒段階：濃霧の向こうにかすかな輪郭が見える境界距離（fogEndの72%〜105%）
        spawnChance = 15;
        minDist = fogEnd * 0.72f;
        maxDist = fogEnd * 1.05f;
        duration = 3.5f;
        crawlSpeed = 0.0f;
        break;

    case FearStage::Afraid:
        // 恐怖段階：霧の中から人型シルエットが佇みこちらを注視（fogEndの50%〜80%）
        spawnChance = 40;
        minDist = fogEnd * 0.50f;
        maxDist = fogEnd * 0.80f;
        duration = 5.0f;
        crawlSpeed = 0.0f;
        break;

    case FearStage::Panic:
        // 恐慌段階：霧を突き破って徘徊・接近（4.5m〜fogEndの70%）
        spawnChance = 65;
        minDist = 4.5f;
        maxDist = (fogEnd * 0.70f < 9.5f) ? fogEnd * 0.70f : 9.5f;
        if (maxDist < minDist + 1.0f) maxDist = minDist + 1.0f;
        duration = 4.5f;
        crawlSpeed = 1.6f;
        break;

    case FearStage::Terror:
        // 狂乱段階：濃霧の至近距離からの急襲（3.5m〜6.5m）
        spawnChance = 85;
        minDist = 3.5f;
        maxDist = 6.5f;
        duration = 4.0f;
        crawlSpeed = 3.0f;
        break;

    default:
        break;
    }

    // 時間帯および配達完了進捗による統合出現確率補正
    int tensionBonus = static_cast<int>(currentCombinedTension * 16.0f) + (timeLevel * 4);
    spawnChance += spawnChanceBonus + tensionBonus;

    // 01時台は序盤のため出現確率を抑制
    if (timeLevel == 0)
    {
        spawnChance -= 10;
    }

    if (spawnChance < 2) spawnChance = 2;
    if (spawnChance > 95) spawnChance = 95;

    // 時間帯に応じた怪異の移動速度スケーリング
    float timeSpeedBonus = (timeLevel >= 2) ? (1.0f + 0.15f * (timeLevel - 1)) : 1.0f;
    crawlSpeed *= entitySpeedMultiplier * timeSpeedBonus;

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
    else
    {
        // 固定地点が視認適正距離内に存在しない場合、プレイヤー前方視界の濃霧境界に動的出現
        VECTOR fwd = player.GetForward();
        fwd.y = 0.0f;
        float len = VSize(fwd);
        if (len > 0.001f) fwd = VScale(fwd, 1.0f / len);
        else fwd = VGet(0.0f, 0.0f, 1.0f);

        // 正面からわずかに左右にずらして配置
        float angleOffset = (GetRand(1) == 0 ? 0.32f : -0.32f);
        VECTOR dir;
        dir.x = fwd.x * std::cos(angleOffset) - fwd.z * std::sin(angleOffset);
        dir.y = 0.0f;
        dir.z = fwd.x * std::sin(angleOffset) + fwd.z * std::cos(angleOffset);

        float targetDist = (minDist + maxDist) * 0.5f;
        VECTOR dynamicPos = VAdd(playerPos, VScale(dir, targetDist));
        dynamicPos.y = 0.0f;

        float yaw = std::atan2(playerPos.x - dynamicPos.x, playerPos.z - dynamicPos.z);
        entity.Spawn(dynamicPos, yaw, duration, crawlSpeed);
    }
}

// 毎フレーム更新処理（時刻・配達進捗連携）
// 毎フレーム更新処理（時刻・配達進捗連携・手続き的カメラ呼吸・恐怖視覚演出）
void HorrorManager::Update(float dt, Player& player, const Bicycle& bicycle, const NightEnvironment& nightEnv, const DeliveryManager* deliveryManager)
{
    // 通常ゲームプレイ中の恐怖度更新
    if (gameState == HorrorGameState::Normal)
    {
        // ゲーム内時刻と配達進捗の統合緊張度計算 (0.0f〜1.0f)
        float totalMins = HorrorUI::Instance().GetTotalGameMinutes();
        float timeTension = totalMins / 180.0f;
        if (timeTension < 0.0f) timeTension = 0.0f;
        if (timeTension > 1.0f) timeTension = 1.0f;

        // 配達完了進捗度
        float deliveryTension = 0.0f;
        if (deliveryManager && deliveryManager->GetTotalDeliveries() > 0)
        {
            deliveryTension = static_cast<float>(deliveryManager->GetCompletedDeliveries()) /
                              static_cast<float>(deliveryManager->GetTotalDeliveries());
            if (deliveryTension < 0.0f) deliveryTension = 0.0f;
            if (deliveryTension > 1.0f) deliveryTension = 1.0f;
        }

        // 時刻(55%)と配達進捗(45%)を組み合わせた総合緊張度
        currentCombinedTension = 0.55f * timeTension + 0.45f * deliveryTension;
        if (currentCombinedTension > 1.0f) currentCombinedTension = 1.0f;

        inSafeLight = CheckSafeLight(player, bicycle, nightEnv);

        if (inSafeLight)
        {
            // 安全光内：常に安全。猶予タイマーリセットおよび恐怖度の減少回復
            safeGraceTimer = 0.8f;
            fearLevel -= 0.08f * dt;
            if (fearLevel < 0.0f) fearLevel = 0.0f;

            // 安全光復帰時の視覚エフェクト滑らかな減衰（急激な消滅を防止）
            float nextVignette = currentVignetteFactor - dt * 0.45f;
            currentVignetteFactor = (nextVignette > fearLevel) ? nextVignette : fearLevel;

            float nextBlur = currentBlurFactor - dt * 0.65f;
            currentBlurFactor = (nextBlur > 0.0f) ? nextBlur : 0.0f;

            float nextSway = currentSwayFactor - dt * 0.55f;
            currentSwayFactor = (nextSway > 0.0f) ? nextSway : 0.0f;
        }
        else
        {
            // 暗闇内：猶予タイマー（0.8秒）消費後に恐怖度が上昇
            // 深夜が進むほど、配達が進むほど、暗闇の恐怖蓄積が加速（最大+50%）
            if (safeGraceTimer > 0.0f)
            {
                safeGraceTimer -= dt;
            }
            else
            {
                float tensionDarkFactor = 1.0f + currentCombinedTension * 0.5f;
                fearLevel += 0.10f * fearRiseMultiplier * tensionDarkFactor * dt;
            }

            // 暗闇内でのエフェクト追従
            currentVignetteFactor += (fearLevel - currentVignetteFactor) * 0.15f;
            currentBlurFactor += (fearLevel - currentBlurFactor) * 0.10f;
            currentSwayFactor += (fearLevel - currentSwayFactor) * 0.12f;
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
                // 至近距離ほど急速に恐怖度が加速（日別倍率適用）
                float proximityFactor = (1.0f - (dist / 18.0f));
                fearLevel += (0.04f + 0.08f * proximityFactor) * fearRiseMultiplier * dt;
            }
        }

        // 100%到達時にジャンプスケア演出へ移行
        if (fearLevel >= 1.0f)
        {
            fearLevel = 1.0f;
            TriggerMaxFearSequence(player);
            return;
        }

        // 手続き的呼吸・カメラ微動の更新
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
            breathAmpY = 0.020f;
            breathAmpX = 0.010f;
            breathFreq = 3.2f;
        }
        else if (fearLevel >= 0.40f)
        {
            breathAmpY = 0.012f;
            breathAmpX = 0.005f;
            breathFreq = 2.4f;
        }
        else if (fearLevel >= 0.20f)
        {
            breathAmpY = 0.005f;
            breathAmpX = 0.002f;
            breathFreq = 1.8f;
        }

        float breathY = std::sin(proceduralBreathTimer * breathFreq) * breathAmpY;
        float breathX = std::cos(proceduralBreathTimer * breathFreq * 0.5f) * breathAmpX;

        // 恐怖カメラ揺れ（PANIC / TERROR時のみ不規則な動揺、操作性は阻害しない）
        float swayPitch = 0.0f;
        float swayYaw = 0.0f;
        if (fearLevel >= 0.60f)
        {
            swayTimer += dt;
            float swayScale = (fearLevel - 0.60f) / 0.40f;
            swayPitch = std::sin(swayTimer * 1.25f) * 0.014f * swayScale;
            swayYaw = std::cos(swayTimer * 0.85f) * 0.018f * swayScale;
        }

        VECTOR breathOffset = VGet(breathX, breathY, 0.0f);
        player.SetSwayOffset(breathOffset, swayPitch, swayYaw);

        // 焦点ボケ・視覚不安定タイマー更新（PANIC以上）
        if (fearLevel >= 0.60f)
        {
            focusBlurTimer -= dt;
            if (focusBlurTimer <= 0.0f)
            {
                focusBlurActiveTimer = 0.8f + static_cast<float>(rand() % 10) / 10.0f;
                focusBlurTimer = 14.0f + static_cast<float>(rand() % 12);
            }
        }
        if (focusBlurActiveTimer > 0.0f)
        {
            focusBlurActiveTimer -= dt;
            if (focusBlurActiveTimer < 0.0f) focusBlurActiveTimer = 0.0f;
        }

        // 二重像（幽体ゴースト）タイマー更新（TERROR以上）
        if (fearLevel >= 0.80f)
        {
            doubleVisionTimer -= dt;
            if (doubleVisionTimer <= 0.0f)
            {
                doubleVisionActiveTimer = 0.6f + static_cast<float>(rand() % 8) / 10.0f;
                doubleVisionTimer = 18.0f + static_cast<float>(rand() % 14);
            }
        }
        if (doubleVisionActiveTimer > 0.0f)
        {
            doubleVisionActiveTimer -= dt;
            if (doubleVisionActiveTimer < 0.0f) doubleVisionActiveTimer = 0.0f;
        }

        // 周辺視野（視界端）怪異出現の更新
        UpdatePeripheralScares(dt, player);

        // 街灯怪異明滅・消灯イベントの更新（PANIC以上）
        if (fearLevel >= 0.60f)
        {
            streetLightEventTimer -= dt;
            if (streetLightEventTimer <= 0.0f)
            {
                VECTOR pPos = player.GetPosition();
                int nearestId = -1;
                float nearestDist = 999.0f;
                for (const auto& light : nightEnv.GetStreetLights())
                {
                    float dx = light.position.x - pPos.x;
                    float dz = light.position.z - pPos.z;
                    float dist = std::sqrt(dx * dx + dz * dz);
                    if (dist < nearestDist && dist < 14.0f)
                    {
                        nearestDist = dist;
                        nearestId = light.id;
                    }
                }
                if (nearestId != -1)
                {
                    const_cast<NightEnvironment&>(nightEnv).TriggerStreetLightFlicker(nearestId);
                }
                streetLightEventTimer = 22.0f + static_cast<float>(rand() % 16);
            }
        }

        // 自転車ヘッドライト怪異明滅（PANIC以上）
        if (fearLevel >= 0.60f)
        {
            headlightGlitchTimer -= dt;
            if (headlightGlitchTimer <= 0.0f)
            {
                if (bicycle.IsHeadlightOn())
                {
                    const_cast<Bicycle&>(bicycle).TriggerHeadlightFlicker();
                }
                headlightGlitchTimer = 20.0f + static_cast<float>(rand() % 15);
            }
        }

        // ホラーUIへ視覚パラメータ伝達
        float dvAmount = (doubleVisionActiveTimer > 0.0f) ? (doubleVisionActiveTimer / 1.0f) : 0.0f;
        float fbAmount = (focusBlurActiveTimer > 0.0f) ? (focusBlurActiveTimer / 1.5f) : 0.0f;
        float desatAmount = (fearLevel > 0.50f) ? ((fearLevel - 0.50f) * 0.70f) : 0.0f;
        HorrorUI::Instance().SetHorrorVisualParams(currentVignetteFactor, 0.0f, 1.0f, dvAmount, desatAmount, fbAmount);

        // ミニマップ電波干渉度の設定
        float mmInterf = (fearLevel > 0.20f) ? ((fearLevel - 0.20f) * 0.85f) : 0.0f;
        HorrorUI::Instance().SetUIInterference(mmInterf);

        // 怪異実体の更新および自動出現チェック
        entity.Update(dt, player.GetPosition());
        UpdateAutoSpawns(dt, player, nightEnv);
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

// 視界端（周辺視野）での怪異出現・直視時消滅の更新
void HorrorManager::UpdatePeripheralScares(float dt, const Player& player)
{
    FearStage stage = GetFearStage();
    if (stage != FearStage::Panic && stage != FearStage::Terror)
    {
        return;
    }

    if (!entity.IsActive() && !inSafeLight)
    {
        peripheralScareTimer -= dt;
        if (peripheralScareTimer <= 0.0f)
        {
            VECTOR fwd = player.GetForward();
            fwd.y = 0.0f;
            float len = VSize(fwd);
            if (len > 0.001f) fwd = VScale(fwd, 1.0f / len);
            else fwd = VGet(0.0f, 0.0f, 1.0f);

            // プレイヤー視界の左右端（約66度）へ配置
            float sideAngle = (GetRand(1) == 0 ? 1.15f : -1.15f);
            VECTOR dir;
            dir.x = fwd.x * std::cos(sideAngle) - fwd.z * std::sin(sideAngle);
            dir.y = 0.0f;
            dir.z = fwd.x * std::sin(sideAngle) + fwd.z * std::cos(sideAngle);

            VECTOR pPos = player.GetPosition();
            VECTOR spawnPos = VAdd(pPos, VScale(dir, 7.2f));
            spawnPos.y = 0.0f;

            float yaw = std::atan2(pPos.x - spawnPos.x, pPos.z - spawnPos.z);
            entity.Spawn(spawnPos, yaw, 3.5f, 0.0f);
            isPeripheralActive = true;
            peripheralScareTimer = 24.0f + static_cast<float>(rand() % 16);
        }
    }

    if (isPeripheralActive)
    {
        if (!entity.IsActive())
        {
            isPeripheralActive = false;
        }
        else
        {
            // プレイヤーが怪異の方向を直視したか判定
            VECTOR pPos = player.GetPosition();
            VECTOR ePos = entity.GetPosition();
            float dx = ePos.x - pPos.x;
            float dz = ePos.z - pPos.z;
            float dist = std::sqrt(dx * dx + dz * dz);
            if (dist > 0.1f)
            {
                dx /= dist;
                dz /= dist;
                VECTOR fwd = player.GetForward();
                float dot = fwd.x * dx + fwd.z * dz;
                if (dot > 0.82f) // 視界中央35度以内に入った瞬間に消失
                {
                    entity.Despawn();
                    isPeripheralActive = false;
                }
            }
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
            HorrorUI::Instance().DrawScareDistortion(0.85f);
            break;

        case JumpscareState::Impact:
            HorrorUI::Instance().DrawScareDistortion(1.0f);
            // 激突時の一瞬の暗赤色フラッシュ
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
            DrawBox(0, 0, 1280, 720, GetColor(160, 20, 20), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            break;

        case JumpscareState::Fall:
            HorrorUI::Instance().DrawScareDistortion(0.70f);
            break;

        case JumpscareState::GroundImpact:
            // 地面激突の暗色ショックパルス
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
            DrawBox(0, 0, 1280, 720, GetColor(10, 16, 24), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            HorrorUI::Instance().DrawScareDistortion(0.85f);
            break;

        case JumpscareState::GroundedStare:
            HorrorUI::Instance().DrawScareDistortion(0.60f);
            break;

        case JumpscareState::VisionClosing:
            // 視界狭窄・まばたきはDrawScreenVignetteで描画される
            break;

        case JumpscareState::GameOver:
            HorrorUI::Instance().DrawGameOver(1.0f, true);
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
        HorrorUI::Instance().DrawRetroPanel(18, 350, 400, 680,
            GetColor(90, 45, 45), HorrorUI::COL_PANEL_BG, 230);

        int lineY = 358;
        HorrorUI::Instance().DrawJitterString(26, lineY, "[ FEAR & TIME DEBUG (F11) ]",
            GetColor(235, 100, 85), HorrorUI::Instance().GetFontSmall());
        lineY += 22;

        char buf[128];
        int gh = 1, gm = 0;
        HorrorUI::Instance().GetGameTime(gh, gm);
        snprintf(buf, sizeof(buf), "GAME TIME:          %02d:%02d AM", gh, gm);
        HorrorUI::Instance().DrawJitterString(26, lineY, buf, GetColor(245, 230, 150), HorrorUI::Instance().GetFontSmall());
        lineY += 19;

        snprintf(buf, sizeof(buf), "TOTAL GAME MINUTES: %.1f", HorrorUI::Instance().GetTotalGameMinutes());
        HorrorUI::Instance().DrawJitterString(26, lineY, buf, HorrorUI::COL_TEXT, HorrorUI::Instance().GetFontSmall());
        lineY += 19;

        snprintf(buf, sizeof(buf), "TIME SCALE:         %.1f min/sec", HorrorUI::Instance().GetTimeScale());
        HorrorUI::Instance().DrawJitterString(26, lineY, buf, HorrorUI::COL_TEXT, HorrorUI::Instance().GetFontSmall());
        lineY += 19;

        snprintf(buf, sizeof(buf), "HORROR TIME LEVEL:  %d (%s)",
            HorrorUI::Instance().GetHorrorTimeLevel(),
            HorrorUI::Instance().GetHorrorTimeLevelName());
        HorrorUI::Instance().DrawJitterString(26, lineY, buf, GetColor(245, 130, 90), HorrorUI::Instance().GetFontSmall());
        lineY += 19;

        snprintf(buf, sizeof(buf), "COMBINED TENSION:   %.1f%% (Time+Delivery)", currentCombinedTension * 100.0f);
        HorrorUI::Instance().DrawJitterString(26, lineY, buf, GetColor(210, 175, 130), HorrorUI::Instance().GetFontSmall());
        lineY += 22;

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
        HorrorUI::Instance().DrawJitterString(26, lineY, buf,
            GetColor(235, 150, 70), HorrorUI::Instance().GetFontSmall());
        lineY += 19;

        snprintf(buf, sizeof(buf), "LIGHT:    %s (Grace: %.1fs)", inSafeLight ? "SAFE LIGHT" : "DARK DANGER", safeGraceTimer);
        HorrorUI::Instance().DrawJitterString(26, lineY, buf,
            inSafeLight ? GetColor(110, 195, 145) : GetColor(220, 85, 75), HorrorUI::Instance().GetFontSmall());
        lineY += 19;

        if (entity.IsActive())
        {
            VECTOR ep = entity.GetPosition();
            snprintf(buf, sizeof(buf), "ENTITY:   ACTIVE (Life: %.1fs)", entity.GetRemainingTime());
            HorrorUI::Instance().DrawJitterString(26, lineY, buf,
                GetColor(240, 70, 70), HorrorUI::Instance().GetFontSmall());
            lineY += 18;
            snprintf(buf, sizeof(buf), "POS:      (%.1f, %.1f)", ep.x, ep.z);
            HorrorUI::Instance().DrawJitterString(26, lineY, buf,
                HorrorUI::COL_TEXT, HorrorUI::Instance().GetFontSmall());
            lineY += 22;
        }
        else
        {
            HorrorUI::Instance().DrawJitterString(26, lineY, "ENTITY:   INACTIVE",
                HorrorUI::COL_TEXT_DIM, HorrorUI::Instance().GetFontSmall());
            lineY += 18;
            snprintf(buf, sizeof(buf), "NEXT CHK: %.1fs", spawnCheckTimer);
            HorrorUI::Instance().DrawJitterString(26, lineY, buf,
                HorrorUI::COL_TEXT_DIM, HorrorUI::Instance().GetFontSmall());
            lineY += 22;
        }

        HorrorUI::Instance().DrawJitterString(26, lineY, "KEYS: [1..4: Fear] [5/J: GameOver] [6: Spawn]",
            GetColor(140, 160, 150), HorrorUI::Instance().GetFontSmall());
        lineY += 18;
        HorrorUI::Instance().DrawJitterString(26, lineY, "      [7: +10m] [8: +1h] [9: Periph] [0: Lamp]",
            GetColor(160, 205, 180), HorrorUI::Instance().GetFontSmall());
        lineY += 18;
        HorrorUI::Instance().DrawJitterString(26, lineY, "      [R: Retry Game]    [F11: Close HUD]",
            GetColor(130, 175, 160), HorrorUI::Instance().GetFontSmall());
    }
}

// リトライ処理（恐怖度・プレイヤー位置・怪異状態・時刻のリセット）
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
    entity.Despawn();

    // 視覚パラメータのリセット
    HorrorUI::Instance().SetHorrorVisualParams(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);

    // ゲーム内時刻を午前1時にリセットし時計の進行を再開
    HorrorUI::Instance().SetGameTime(1, 0);
    HorrorUI::Instance().SetTimePaused(false);

    // カメラ傾き・倒れ込み・揺れ・シェイクのリセット
    player.SetCameraRoll(0.0f);
    player.SetEyeHeightOffset(0.0f);
    player.SetSwayOffset(VGet(0.0f, 0.0f, 0.0f), 0.0f, 0.0f);
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

// デバッグ用怪異強制出現（濃霧でもシルエットが確認できるよう前方7.5m地点に出現）
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
    entity.Spawn(spawnPos, yaw, 6.0f, 1.8f);
}

// デバッグ用視界端（周辺視野）怪異出現テスト
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
    entity.Spawn(spawnPos, yaw, 5.0f, 0.0f);
    isPeripheralActive = true;
}

// デバッグ用街灯怪異明滅・消灯イベントテスト
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
