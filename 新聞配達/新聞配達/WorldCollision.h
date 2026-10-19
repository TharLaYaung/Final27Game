#pragma once

#include "DxLib.h"
#include <vector>
#include <string>

// 静的コリジョン直方体構造体（水平面OBB + 高さY範囲）
struct CollisionBox
{
    int id;                     // 識別用ID
    std::string tag;            // 種別名（"HouseBody", "Fence", "Boundary"等）
    VECTOR center;              // 中心座標 (X, Y, Z)
    float halfWidth;            // ローカルX軸半幅 (m)
    float halfHeight;           // Y軸半高 (m)
    float halfDepth;            // ローカルZ軸半幅 (m)
    float rotationY;            // Y軸回転角度（ラジアン）
    bool enabled;               // 有効フラグ
};

// 静的コリジョン円柱構造体（垂直円柱: 電柱・郵便ポスト等）
struct CollisionCylinder
{
    int id;                     // 識別用ID
    std::string tag;            // 種別名（"LampPole", "Mailbox"等）
    VECTOR center;              // 基準底面/中心座標 (X, Y, Z)
    float radius;               // 半径 (m)
    float minY;                 // 底面Y座標
    float maxY;                 // 上面Y座標
    bool enabled;               // 有効フラグ
};

// 街全体の静的ワールドコリジョン管理クラス
class WorldCollision
{
public:
    static WorldCollision& Instance();

    // コリジョン情報の全消去
    void Clear();

    // 直方体（OBB / AABB）コリジョンの追加
    void AddBox(int id, const std::string& tag, const VECTOR& center, float halfWidth, float halfHeight, float halfDepth, float rotationY = 0.0f);

    // 垂直円柱コリジョンの追加
    void AddCylinder(int id, const std::string& tag, const VECTOR& center, float radius, float minY, float maxY);

    // 球（プレイヤー・自転車）と障害物との衝突判定
    bool CheckCollision(const VECTOR& pos, float radius) const;

    // 線分・カプセルスイープ衝突判定（高速移動時のすり抜け防止）
    bool CheckSweep(const VECTOR& from, const VECTOR& to, float radius) const;

    // めり込み解消・滑らかスライド補正
    // 接触した障害物の法線方向へposを押し戻す
    bool ResolveCollision(VECTOR& pos, float radius) const;

    // デバッグ描画（F6有効時に全コリジョン枠を3Dワイヤーフレーム描画）
    void DrawDebug() const;

    // 登録されたコリジョン一覧の取得
    const std::vector<CollisionBox>& GetBoxes() const { return boxes; }
    const std::vector<CollisionCylinder>& GetCylinders() const { return cylinders; }

private:
    WorldCollision();
    ~WorldCollision();

    // 球とOBBの衝突判定・法線・押し戻し量計算
    static bool TestSphereOBB(const VECTOR& spherePos, float sphereRadius, const CollisionBox& box, VECTOR* outNormal = nullptr, float* outPenetration = nullptr);

    // 球と円柱の衝突判定・法線・押し戻し量計算
    static bool TestSphereCylinder(const VECTOR& spherePos, float sphereRadius, const CollisionCylinder& cyl, VECTOR* outNormal = nullptr, float* outPenetration = nullptr);

    std::vector<CollisionBox> boxes;
    std::vector<CollisionCylinder> cylinders;
};
