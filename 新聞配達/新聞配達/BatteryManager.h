#pragma once

#include "DxLib.h"
#include <vector>

class Player;
class Bicycle;

// 街中に配置されるバッテリーアイテムのデータ構造体
struct BatteryItem
{
    int id;                 // バッテリー識別ID
    VECTOR position;        // ワールド配置座標
    float rotationY;        // Y軸回転角度
    bool isCollected;       // 回収済みフラグ
};

// 予備バッテリー所持数および街頭バッテリーアイテム統合管理クラス
class BatteryManager
{
public:
    // 予備バッテリーは最大2個まで所持できる
    static const int MAX_SPARE_BATTERIES = 2;

    BatteryManager();
    ~BatteryManager();

    // 初期化（3Dモデル読み込み・スケール自動計測）
    bool Initialize();

    // 毎フレーム更新（プレイヤー注視・回収判定・自転車バッテリー交換判定）
    void Update(Player& player, Bicycle& bicycle);

    // 3Dバッテリーアイテム群の描画
    void Draw();

    // UIプロンプト更新（HorrorUIと連携）
    void UpdateUI(Player& player, Bicycle& bicycle);

    // 終了処理（モデル解放）
    void Finalize();

    // 7日間ステージリセット（日別決定論的シードに基づく配置）
    void Reset(int day);

    // 予備バッテリー所持数取得
    int GetSpareCount() const { return spareBatteries; }

    // 最大所持可能数取得（2個）
    int GetMaxSpareCount() const { return MAX_SPARE_BATTERIES; }

    // 予備バッテリー所持数が上限か
    bool IsFull() const { return spareBatteries >= MAX_SPARE_BATTERIES; }

    // 自転車バッテリー交換が可能か判定
    bool CanReplaceBicycleBattery(const Player& player, const Bicycle& bicycle) const;

    // 自転車バッテリー交換実行
    bool ReplaceBicycleBattery(Bicycle& bicycle);

    // 現在プレイヤーが注視しているバッテリーID（-1はなし）
    int GetAimedBatteryId() const { return aimedBatteryId; }

    // 注視中のバッテリーが回収可能か（満杯でない）
    bool CanCollectAimedBattery() const;

    // 満杯状態でバッテリーを注視中か
    bool IsLookingAtFullBattery() const;

    // 全バッテリーアイテムリストの参照
    const std::vector<BatteryItem>& GetItems() const { return batteryItems; }

private:
    // バッテリー3Dモデルハンドル
    int modelHandle;

    // モデル描画スケール（実寸大約32cmに自動正規化）
    float modelScale;

    // 現在の予備バッテリー所持数（0〜MAX_SPARE_BATTERIES）
    int spareBatteries;

    // 配置されたバッテリーアイテム一覧
    std::vector<BatteryItem> batteryItems;

    // 現在注視中のバッテリーアイテムID (-1はなし)
    int aimedBatteryId;

    // 自転車バッテリー交換可能フラグ
    bool canReplaceBicycle;

    // 前フレームの入力状態（エッジトリガー検出用）
    bool oldLeftClick;
    bool oldRKey;

    // アニメーション用タイマー
    float animTimer;

    // 候補スポーン位置リスト（街中の安全な歩道・道端）
    std::vector<VECTOR> spawnCandidates;

    // 日別バッテリースポーン配置生成
    void SetupDaySpawns(int day);
};
