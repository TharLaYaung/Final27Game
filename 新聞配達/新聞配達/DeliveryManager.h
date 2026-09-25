#pragma once

#include "DxLib.h"
#include <vector>

// 配達状態の列挙型
enum class DeliveryState
{
    Active,             // 通常配達受付中
    CompleteFeedback,   // 配達完了メッセージ表示中（約2秒間）
    AllComplete         // 全ての配達が完了した状態
};

// 配達先（郵便ポスト）のデータ構造体
struct DeliveryPoint
{
    int id;             // 配達先固有ID (1, 2, 3...)
    VECTOR position;    // ワールド配置座標
    float angle;        // 向き（Y軸回転角度）
    bool isCompleted;   // 配達完了フラグ
};

// 配達進行・目的地管理クラス
class DeliveryManager
{
public:
    DeliveryManager();
    ~DeliveryManager();

    // 初期化（8箇所の配達先座標設定など）
    void Initialize();

    // 毎フレーム更新（完了メッセージタイマー、誤配送タイマーなど）
    void Update();

    // 正しいポストへの配達成功処理
    void OnDeliverySuccess(int mailboxId);

    // 誤ったポストへの配達試行処理
    void TriggerWrongDelivery();

    // 配達状況アクセサ
    int GetCurrentDelivery() const;
    int GetTotalDeliveries() const;
    int GetCompletedDeliveries() const;
    bool IsAllDeliveriesComplete() const;

    // 現在の目的地の取得
    int GetCurrentTargetId() const;
    VECTOR GetCurrentTargetPosition() const;
    bool HasActiveTarget() const;

    // 状態の取得
    DeliveryState GetState() const;
    bool IsFeedbackActive() const;
    bool IsWrongDeliveryActive() const;
    int GetAllCompleteTimer() const;

    // 全配達先リストの参照
    const std::vector<DeliveryPoint>& GetDeliveryPoints() const;

private:
    int currentDelivery;            // 現在の配達番号 (1〜8)
    int totalDeliveries;            // 全配達必要数 (8)
    int completedDeliveries;        // 完了済み配達数 (0〜8)
    bool allDeliveriesComplete;     // 全配達完了フラグ

    DeliveryState state;            // 現在の配達状態
    int feedbackTimer;              // 配達完了メッセージ表示タイマー（約120フレーム = 2秒）
    int wrongDeliveryTimer;         // 誤配送メッセージ表示タイマー（約120フレーム = 2秒）
    int allCompleteTimer;           // 全配達完了メッセージ表示タイマー

    std::vector<DeliveryPoint> deliveryPoints; // 8箇所の配達先リスト
};
