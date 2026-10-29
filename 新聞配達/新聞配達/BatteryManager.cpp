#include "AudioManager.h"
#include "BatteryManager.h"
#include "Player.h"
#include "Bicycle.h"
#include "HorrorUI.h"
#include "DeliveryManager.h"
#include <cmath>
#include <algorithm>

BatteryManager::BatteryManager()
{
    modelHandle = -1;
    modelScale = 1.0f;
    spareBatteries = 0;
    aimedBatteryId = -1;
    canReplaceBicycle = false;
    oldLeftClick = false;
    oldRKey = false;
    animTimer = 0.0f;

    // 街中の安全かつ自然な歩道・道端スポーン候補地（12箇所）
    spawnCandidates = {
        VGet(-4.5f, 0.05f, -27.8f),   // 候補0: 南通り街灯1付近の道端
        VGet(-22.5f, 0.05f, -25.5f),  // 候補1: 南西交差点・住宅7付近の歩道
        VGet(-24.5f, 0.05f, -1.0f),   // 候補2: 西通り・住宅0前の道路脇
        VGet(-21.5f, 0.05f, 13.0f),   // 候補3: 北西交差点・街灯3付近
        VGet(-8.0f, 0.05f, 17.0f),    // 候補4: 北横通り・住宅1付近
        VGet(2.2f, 0.05f, 36.5f),     // 候補5: 北路地行き止まり・街灯5付近
        VGet(12.0f, 0.05f, 17.0f),    // 候補6: 北横通り東側・住宅3前
        VGet(21.5f, 0.05f, 13.0f),    // 候補7: 北東交差点・街灯6付近
        VGet(24.5f, 0.05f, -1.0f),    // 候補8: 東通り・住宅5前の道路脇
        VGet(22.5f, 0.05f, -25.5f),   // 候補9: 南東交差点・住宅6前
        VGet(-4.5f, 0.05f, -8.0f),    // 候補10: 中央横通り・住宅4付近
        VGet(8.0f, 0.05f, -28.0f)     // 候補11: 南通り東側の路肩
    };
}

BatteryManager::~BatteryManager()
{
    Finalize();
}

// 初期化（3Dモデル読み込み・実寸大スケール自動正規化）
bool BatteryManager::Initialize()
{
    const char* candidatePaths[] = {
        "Data/Model/battery.mv1",
        "新聞配達/Data/Model/battery.mv1",
        "新聞配達/新聞配達/Data/Model/battery.mv1",
        "../新聞配達/Data/Model/battery.mv1",
        "../Data/Model/battery.mv1"
    };

    modelHandle = -1;
    for (const char* path : candidatePaths)
    {
        modelHandle = MV1LoadModel(path);
        if (modelHandle != -1)
        {
            break;
        }
    }

    if (modelHandle == -1)
    {
        return false;
    }

    // メッシュのバウンディングボックスを取得して実寸大（約32cm）に自動スケール
    VECTOR minP = MV1GetMeshMinPosition(modelHandle, 0);
    VECTOR maxP = MV1GetMeshMaxPosition(modelHandle, 0);
    float sizeX = maxP.x - minP.x;
    float sizeY = maxP.y - minP.y;
    float sizeZ = maxP.z - minP.z;
    float maxDim = sizeX;
    if (sizeY > maxDim) maxDim = sizeY;
    if (sizeZ > maxDim) maxDim = sizeZ;

    if (maxDim > 0.0001f)
    {
        modelScale = 0.32f / maxDim;
    }
    else
    {
        modelScale = 1.0f;
    }

    // 夜間視認性を高めるマテリアル発光・環境光設定
    int matNum = MV1GetMaterialNum(modelHandle);
    for (int m = 0; m < matNum; ++m)
    {
        MV1SetMaterialDifColor(modelHandle, m, GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
        MV1SetMaterialAmbColor(modelHandle, m, GetColorF(0.85f, 0.85f, 0.85f, 1.0f));
        MV1SetMaterialEmiColor(modelHandle, m, GetColorF(0.20f, 0.28f, 0.22f, 1.0f));
    }

    // 初日（Day 1）の配置初期化
    Reset(1);

    return true;
}

// 7日間ステージリセット（日別決定論的シードに基づく配置生成）
void BatteryManager::Reset(int day)
{
    if (day < 1) day = 1;
    if (day > 7) day = 7;

    // ゲーム開始時は予備バッテリー0個で開始
    spareBatteries = 0;
    aimedBatteryId = -1;
    canReplaceBicycle = false;
    oldLeftClick = false;
    oldRKey = false;
    animTimer = 0.0f;

    SetupDaySpawns(day);
}

// 日別バッテリースポーン配置生成
void BatteryManager::SetupDaySpawns(int day)
{
    batteryItems.clear();

    // 日別固定シードを取得（同日リプレイ時の完全同一配置を保証）
    unsigned int seed = DeliveryManager::GetDaySeed(day);
    unsigned int rng = seed ^ 0xBA77E12Cu;
    auto NextRand = [&]() -> unsigned int {
        rng = rng * 1664525u + 1013904223u;
        return rng;
    };

    // 候補地インデックスリスト
    std::vector<int> candidateIndices(spawnCandidates.size());
    for (size_t i = 0; i < candidateIndices.size(); ++i)
    {
        candidateIndices[i] = static_cast<int>(i);
    }

    // フィッシャー・イェーツ法による決定論的シャッフル
    for (size_t i = candidateIndices.size() - 1; i > 0; --i)
    {
        size_t j = NextRand() % (i + 1);
        std::swap(candidateIndices[i], candidateIndices[j]);
    }

    // 日毎に配置するバッテリー数（Day 1〜2: 3個, Day 3〜7: 4個）
    int spawnCount = (day <= 2) ? 3 : 4;
    if (spawnCount > static_cast<int>(spawnCandidates.size()))
    {
        spawnCount = static_cast<int>(spawnCandidates.size());
    }

    for (int i = 0; i < spawnCount; ++i)
    {
        int candIdx = candidateIndices[i];
        float rot = static_cast<float>(NextRand() % 360) * DX_PI_F / 180.0f;

        BatteryItem item;
        item.id = i;
        item.position = spawnCandidates[candIdx];
        item.rotationY = rot;
        item.isCollected = false;

        batteryItems.push_back(item);
    }
}

// 自転車バッテリー交換が可能か判定
bool BatteryManager::CanReplaceBicycleBattery(const Player& player, const Bicycle& bicycle) const
{
    // 予備バッテリーを1個以上所持していなければ交換不可
    if (spareBatteries <= 0)
    {
        return false;
    }

    // すでに100%満充電の場合は交換不可（無駄な消費を防止）
    if (bicycle.GetBatteryPercent() >= 99.5f)
    {
        return false;
    }

    // 自転車に乗車中、または自転車から3.2m以内の至近距離にいる場合のみ交換可能
    if (bicycle.IsRiding())
    {
        return true;
    }

    VECTOR playerPos = player.GetPosition();
    VECTOR bikePos = bicycle.GetPosition();
    float dx = playerPos.x - bikePos.x;
    float dy = playerPos.y - bikePos.y;
    float dz = playerPos.z - bikePos.z;
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    return (dist <= 3.2f);
}

// 自転車バッテリー交換実行
bool BatteryManager::ReplaceBicycleBattery(Bicycle& bicycle)
{
    // 交換可能条件を満たさない場合は何もしない
    if (spareBatteries <= 0 || bicycle.GetBatteryPercent() >= 99.5f)
    {
        return false;
    }

    // バッテリー交換時に予備バッテリーを1個消費する
    spareBatteries--;

    // 自転車バッテリーを満充電（100%）に復帰
    bicycle.ReplaceBattery();
    AudioManager::Instance().PlayBatteryReload();

    return true;
}

// 注視中のバッテリーが回収可能か（所持枠に空きがある）
bool BatteryManager::CanCollectAimedBattery() const
{
    return (aimedBatteryId != -1 && spareBatteries < MAX_SPARE_BATTERIES);
}

// 満杯状態でバッテリーを注視中か
bool BatteryManager::IsLookingAtFullBattery() const
{
    return (aimedBatteryId != -1 && spareBatteries >= MAX_SPARE_BATTERIES);
}

// 毎フレーム更新
void BatteryManager::Update(Player& player, Bicycle& bicycle)
{
    animTimer += 0.01666f;

    VECTOR playerPos = player.GetPosition();
    VECTOR playerFwd = player.GetForward();

    aimedBatteryId = -1;

    // 未回収バッテリーアイテムとの距離および注視判定
    float bestDot = 0.88f;
    for (size_t i = 0; i < batteryItems.size(); ++i)
    {
        if (batteryItems[i].isCollected)
        {
            continue;
        }

        VECTOR itemCenter = VGet(
            batteryItems[i].position.x,
            batteryItems[i].position.y + 0.16f,
            batteryItems[i].position.z
        );

        VECTOR toItem = VSub(itemCenter, playerPos);
        float dist = VSize(toItem);

        // 3.0メートル以内
        if (dist <= 3.0f && dist > 0.001f)
        {
            VECTOR dir = VNorm(toItem);
            float dot = VDot(playerFwd, dir);

            if (dot >= bestDot)
            {
                bestDot = dot;
                aimedBatteryId = batteryItems[i].id;
            }
        }
    }

    // 左クリックのエッジトリガー検出（回収操作）
    bool currentLeftClick = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;
    bool leftClickDown = currentLeftClick && !oldLeftClick;
    oldLeftClick = currentLeftClick;

    if (aimedBatteryId != -1 && leftClickDown)
    {
        if (spareBatteries < MAX_SPARE_BATTERIES)
        {
            // 予備バッテリーは最大2個まで所持できる
            spareBatteries++;
            AudioManager::Instance().PlayBatteryPickup();

            // アイテムを回収済みに設定し、マップ上から非表示にする
            for (size_t i = 0; i < batteryItems.size(); ++i)
            {
                if (batteryItems[i].id == aimedBatteryId)
                {
                    batteryItems[i].isCollected = true;
                    break;
                }
            }
            aimedBatteryId = -1;
        }
        else
        {
            // 所持数が上限の場合はバッテリーを回収しない
            // 空きができた場合は残っているバッテリーを後から回収できる
        }
    }

    // 自転車バッテリー交換の判定と入力
    canReplaceBicycle = CanReplaceBicycleBattery(player, bicycle);

    bool currentRKey = (CheckHitKey(KEY_INPUT_R) != 0);
    bool rKeyDown = currentRKey && !oldRKey;
    oldRKey = currentRKey;

    if (canReplaceBicycle && rKeyDown)
    {
        ReplaceBicycleBattery(bicycle);
    }

    // HorrorUIのプロンプト更新
    UpdateUI(player, bicycle);
}

// UIプロンプト更新
void BatteryManager::UpdateUI(Player& player, Bicycle& bicycle)
{
    (void)player;
    (void)bicycle;

    // バッテリーアイテム注視時のプロンプト設定
    if (aimedBatteryId != -1)
    {
        if (spareBatteries >= MAX_SPARE_BATTERIES)
        {
            // 満杯時の表示: BATTERY FULL / SPARE 2/2
            HorrorUI::Instance().SetPrompt(PromptType::BatteryFull);
        }
        else
        {
            // 回収可能時の表示: [ LMB ] TAKE BATTERY
            HorrorUI::Instance().SetPrompt(PromptType::TakeBattery);
        }
    }
    else if (canReplaceBicycle)
    {
        // 自転車バッテリー交換可能時の表示: [ R ] REPLACE BATTERY
        HorrorUI::Instance().SetPrompt(PromptType::ReplaceBattery);
    }
}

// 3Dバッテリーアイテム群の描画
void BatteryManager::Draw()
{
    for (size_t i = 0; i < batteryItems.size(); ++i)
    {
        if (batteryItems[i].isCollected)
        {
            continue;
        }

        // 浮遊アニメーションと微細な回転
        float hoverY = std::sin(animTimer * 2.5f + static_cast<float>(i)) * 0.03f;
        VECTOR drawPos = VGet(
            batteryItems[i].position.x,
            batteryItems[i].position.y + 0.12f + hoverY,
            batteryItems[i].position.z
        );

        float rotY = batteryItems[i].rotationY + animTimer * 0.8f;

        if (modelHandle != -1)
        {
            MV1SetScale(modelHandle, VGet(modelScale, modelScale, modelScale));
            MV1SetPosition(modelHandle, drawPos);
            MV1SetRotationXYZ(modelHandle, VGet(0.0f, rotY, 0.0f));
            MV1DrawModel(modelHandle);
        }
        else
        {
            // フォールバック描画（モデル読み込み不可時でも直方体バッテリーとして表示）
            DrawCube3D(
                VGet(drawPos.x - 0.08f, drawPos.y - 0.05f, drawPos.z - 0.12f),
                VGet(drawPos.x + 0.08f, drawPos.y + 0.15f, drawPos.z + 0.12f),
                GetColor(40, 45, 50),
                GetColor(70, 75, 80),
                TRUE
            );
        }

        // 深夜の暗闇・霧中でも発見しやすい淡い緑色グロー発光球
        SetUseLighting(FALSE);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 55);
        unsigned int glowCol = GetColor(60, 180, 110);
        DrawSphere3D(drawPos, 0.22f, 6, glowCol, glowCol, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        SetUseLighting(TRUE);
    }
}

// 終了処理
void BatteryManager::Finalize()
{
    if (modelHandle != -1)
    {
        MV1DeleteModel(modelHandle);
        modelHandle = -1;
    }
    batteryItems.clear();
}
