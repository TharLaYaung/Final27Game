#include "DeliveryManager.h"
#include <cmath>

// コンストラクタ
DeliveryManager::DeliveryManager()
{
    currentDelivery = 1;
    totalDeliveries = 8;
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
    currentDelivery = 1;
    totalDeliveries = 8;
    completedDeliveries = 0;
    allDeliveriesComplete = false;
    state = DeliveryState::Active;
    feedbackTimer = 0;
    wrongDeliveryTimer = 0;
    allCompleteTimer = 0;

    deliveryPoints.clear();

    // 8箇所の配達先ポストを住宅街マップ内に配置
    // ID 1: スタート地点正面（直線進んだ場所）
    deliveryPoints.push_back({ 1, VGet(  0.0f, 0.0f,   6.0f), DX_PI_F * 0.75f, false });
    // ID 2: 北東エリア
    deliveryPoints.push_back({ 2, VGet(  6.0f, 0.0f,   9.0f), DX_PI_F * 0.50f, false });
    // ID 3: 北西エリア
    deliveryPoints.push_back({ 3, VGet( -7.0f, 0.0f,  13.0f), DX_PI_F * 1.00f, false });
    // ID 4: 東エリア
    deliveryPoints.push_back({ 4, VGet( 11.0f, 0.0f,   3.0f), DX_PI_F * 0.25f, false });
    // ID 5: 南東エリア
    deliveryPoints.push_back({ 5, VGet(  8.0f, 0.0f,  -7.0f), DX_PI_F * 0.10f, false });
    // ID 6: 南エリア
    deliveryPoints.push_back({ 6, VGet(  0.0f, 0.0f, -13.0f), DX_PI_F * 0.00f, false });
    // ID 7: 南西エリア
    deliveryPoints.push_back({ 7, VGet( -9.0f, 0.0f,  -9.0f), DX_PI_F * 1.75f, false });
    // ID 8: 西エリア（最後の配達先）
    deliveryPoints.push_back({ 8, VGet(-10.0f, 0.0f,   2.0f), DX_PI_F * 1.50f, false });
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
                // 次の配達先をアクティブ化
                currentDelivery = completedDeliveries + 1;
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

// 現在の目的地のポストIDを取得
int DeliveryManager::GetCurrentTargetId() const
{
    if (state == DeliveryState::Active && !allDeliveriesComplete)
    {
        return currentDelivery;
    }
    return 0; // アクティブな目標なし
}

// 現在の目的地の座標を取得
VECTOR DeliveryManager::GetCurrentTargetPosition() const
{
    int targetId = GetCurrentTargetId();
    for (size_t i = 0; i < deliveryPoints.size(); i++)
    {
        if (deliveryPoints[i].id == targetId)
        {
            return deliveryPoints[i].position;
        }
    }
    return VGet(0.0f, 0.0f, 0.0f);
}

// アクティブな目的地が存在するか
bool DeliveryManager::HasActiveTarget() const
{
    return (state == DeliveryState::Active && !allDeliveriesComplete);
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
