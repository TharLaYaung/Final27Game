#include "DeliveryManager.h"
#include "Map.h"
#include <cmath>

// コンストラクタ
DeliveryManager::DeliveryManager()
{
    currentDelivery = 0;
    totalDeliveries = NEIGHBORHOOD_HOUSE_COUNT;
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
    currentDelivery = 0;
    totalDeliveries = NEIGHBORHOOD_HOUSE_COUNT;
    completedDeliveries = 0;
    allDeliveriesComplete = false;
    state = DeliveryState::Active;
    feedbackTimer = 0;
    wrongDeliveryTimer = 0;
    allCompleteTimer = 0;

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

// 現在の目的地のポストIDを取得（0〜7、未完了目標なしは-1）
int DeliveryManager::GetCurrentTargetId() const
{
    if (state == DeliveryState::Active && !allDeliveriesComplete && currentDelivery >= 0 && currentDelivery < totalDeliveries)
    {
        return currentDelivery;
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
    return (state == DeliveryState::Active && !allDeliveriesComplete && currentDelivery >= 0 && currentDelivery < totalDeliveries);
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
