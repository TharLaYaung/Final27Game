#include "DeliveryManager.h"
#include "Map.h"
#include <cmath>

// コンストラクタ
DeliveryManager::DeliveryManager()
{
    currentDay = 1;
    currentDelivery = 0;
    totalDeliveries = 3;
    completedDeliveries = 0;
    allDeliveriesComplete = false;
    state = DeliveryState::Active;
    feedbackTimer = 0;
    wrongDeliveryTimer = 0;
    allCompleteTimer = 0;
}

// デストラクタ
DeliveryManager::~DeliveryManager()
{
}

// 初期化
void DeliveryManager::Initialize()
{
    deliveryPoints.clear();

    // マップデータ（Map.h）に定義された8棟の各ポスト位置・角度を連携登録
    const std::vector<HouseData>& houses = MapGetHouses();
    for (size_t i = 0; i < houses.size(); i++)
    {
        deliveryPoints.push_back({
            houses[i].id,
            houses[i].mailboxPosition,
            houses[i].mailboxRotationY,
            false
        });
    }

    // 初日（Day 1）の配達スケジュール設定
    SetDay(1);
}

// 毎フレーム更新
void DeliveryManager::Update()
{
    // 配達完了メッセージの表示タイマー処理
    if (state == DeliveryState::CompleteFeedback)
    {
        feedbackTimer--;
        if (feedbackTimer <= 0)
        {
            feedbackTimer = 0;

            // 全配達完了かチェック
            if (completedDeliveries >= totalDeliveries)
            {
                allDeliveriesComplete = true;
                state = DeliveryState::AllComplete;
                allCompleteTimer = 360; // 約6秒間完了通知を表示
            }
            else
            {
                // 次の配達先をアクティブ化（0〜7）
                currentDelivery = completedDeliveries;
                state = DeliveryState::Active;
            }
        }
    }

    // 誤配送警告メッセージのタイマー処理
    if (wrongDeliveryTimer > 0)
    {
        wrongDeliveryTimer--;
    }

    // 全配達完了メッセージタイマー処理
    if (allCompleteTimer > 0)
    {
        allCompleteTimer--;
    }
}

// 正しいポストへの配達成功処理
void DeliveryManager::OnDeliverySuccess(int mailboxId)
{
    // 該当する配達先を完了状態にする
    for (size_t i = 0; i < deliveryPoints.size(); i++)
    {
        if (deliveryPoints[i].id == mailboxId)
        {
            deliveryPoints[i].isCompleted = true;
            break;
        }
    }

    // 完了配達数を加算
    completedDeliveries++;

    // 配達完了フィードバック状態へ移行（約2秒間表示）
    state = DeliveryState::CompleteFeedback;
    feedbackTimer = 120;
}

// 誤ったポストへの配達試行処理
void DeliveryManager::TriggerWrongDelivery()
{
    // 警告メッセージを約2秒間表示
    wrongDeliveryTimer = 120;
}

// 現在の配達番号
int DeliveryManager::GetCurrentDelivery() const
{
    return currentDelivery;
}

// 全配達必要数
int DeliveryManager::GetTotalDeliveries() const
{
    return totalDeliveries;
}

// 完了済み配達数
int DeliveryManager::GetCompletedDeliveries() const
{
    return completedDeliveries;
}

// 全配達完了判定
bool DeliveryManager::IsAllDeliveriesComplete() const
{
    return allDeliveriesComplete;
}

// 7日間シード値の取得（日毎の決定論的シード）
unsigned int DeliveryManager::GetDaySeed(int day)
{
    switch (day)
    {
    case 1: return 0x1001Au;
    case 2: return 0x2002Bu;
    case 3: return 0x3003Cu;
    case 4: return 0x4004Du;
    case 5: return 0x5005Eu;
    case 6: return 0x6006Fu;
    case 7:
    default: return 0x70070u;
    }
}

// 現在日のシード値取得
unsigned int DeliveryManager::GetCurrentDaySeed() const
{
    return GetDaySeed(currentDay);
}

// 7日間ステージ設定（決定論的シードに基づく配達順生成）
void DeliveryManager::SetDay(int day)
{
    if (day < 1) day = 1;
    if (day > 7) day = 7;
    currentDay = day;

    // 日別配達必要軒数（Day 1: 3軒 〜 Day 7: 全8軒）
    int count = 3;
    switch (currentDay)
    {
    case 1: count = 3; break;
    case 2: count = 4; break;
    case 3: count = 5; break;
    case 4: count = 6; break;
    case 5: count = 6; break;
    case 6: count = 7; break;
    case 7:
    default: count = 8; break;
    }

    // 決定論的シードに基づく疑似乱数ジェネレータ
    unsigned int rng = GetDaySeed(currentDay);
    auto NextRand = [&]() -> unsigned int {
        rng = rng * 1664525u + 1013904223u;
        return rng;
    };

    // 全8棟の住宅ID（0〜7）をフィッシャー・イェーツ法で日別シード決定論的シャッフル
    std::vector<int> allHouses = { 0, 1, 2, 3, 4, 5, 6, 7 };
    for (size_t i = allHouses.size() - 1; i > 0; --i)
    {
        size_t j = NextRand() % (i + 1);
        std::swap(allHouses[i], allHouses[j]);
    }

    dayTargetHouses.clear();
    for (int i = 0; i < count; ++i)
    {
        dayTargetHouses.push_back(allHouses[i]);
    }

    totalDeliveries = static_cast<int>(dayTargetHouses.size());
    completedDeliveries = 0;
    currentDelivery = 0;
    allDeliveriesComplete = false;
    state = DeliveryState::Active;
    feedbackTimer = 0;
    wrongDeliveryTimer = 0;
    allCompleteTimer = 0;

    // 配達先完了状態のリセット
    for (size_t i = 0; i < deliveryPoints.size(); ++i)
    {
        deliveryPoints[i].isCompleted = false;
    }
}

// 現在の日数取得
int DeliveryManager::GetCurrentDay() const
{
    return currentDay;
}

// 日数名文字列取得
const char* DeliveryManager::GetDayName() const
{
    switch (currentDay)
    {
    case 1: return "DAY 1 - MONDAY";
    case 2: return "DAY 2 - TUESDAY";
    case 3: return "DAY 3 - WEDNESDAY";
    case 4: return "DAY 4 - THURSDAY";
    case 5: return "DAY 5 - FRIDAY";
    case 6: return "DAY 6 - SATURDAY";
    case 7: return "DAY 7 - SUNDAY (FINAL)";
    default: return "DAY 1 - MONDAY";
    }
}

// 日別サブタイトル取得
const char* DeliveryManager::GetDaySubtitle() const
{
    switch (currentDay)
    {
    case 1: return "The First Night - Orientation";
    case 2: return "Cold Rain & Flickering Lights";
    case 3: return "Creeping Mist & Distant Whispers";
    case 4: return "Midnight Blackouts & Torrential Rain";
    case 5: return "Freezing Sleet & Darkened Streets";
    case 6: return "The Blizzard & Stalking Shadows";
    case 7: return "Final Night - The Supernatural Nightmare";
    default: return "The First Night - Orientation";
    }
}

// 現在の目的地のポストIDを取得（0〜7、未完了目標なしは-1）
int DeliveryManager::GetCurrentTargetId() const
{
    if (state == DeliveryState::Active && !allDeliveriesComplete &&
        currentDelivery >= 0 && currentDelivery < static_cast<int>(dayTargetHouses.size()))
    {
        return dayTargetHouses[currentDelivery];
    }
    return -1; // アクティブな目標なし
}

// 現在の目的地の座標を取得
VECTOR DeliveryManager::GetCurrentTargetPosition() const
{
    int targetId = GetCurrentTargetId();
    if (targetId >= 0)
    {
        for (size_t i = 0; i < deliveryPoints.size(); i++)
        {
            if (deliveryPoints[i].id == targetId)
            {
                return deliveryPoints[i].position;
            }
        }
    }
    return VGet(0.0f, 0.0f, 0.0f);
}

// アクティブな目的地が存在するか
bool DeliveryManager::HasActiveTarget() const
{
    return (state == DeliveryState::Active && !allDeliveriesComplete &&
        currentDelivery >= 0 && currentDelivery < static_cast<int>(dayTargetHouses.size()));
}

// 現在の配達状態を取得
DeliveryState DeliveryManager::GetState() const
{
    return state;
}

// 配達完了フィードバック中か
bool DeliveryManager::IsFeedbackActive() const
{
    return (state == DeliveryState::CompleteFeedback && feedbackTimer > 0);
}

// 誤配送メッセージ表示中か
bool DeliveryManager::IsWrongDeliveryActive() const
{
    return (wrongDeliveryTimer > 0);
}

// 全配達完了タイマーを取得
int DeliveryManager::GetAllCompleteTimer() const
{
    return allCompleteTimer;
}

// 全配達先リストの参照
const std::vector<DeliveryPoint>& DeliveryManager::GetDeliveryPoints() const
{
    return deliveryPoints;
}
