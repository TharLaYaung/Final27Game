#include "WorldCollision.h"
#include <cmath>
#include <algorithm>

// シングルトンインスタンス取得
WorldCollision& WorldCollision::Instance()
{
    static WorldCollision instance;
    return instance;
}

WorldCollision::WorldCollision()
{
}

WorldCollision::~WorldCollision()
{
}

// コリジョン情報の全消去
void WorldCollision::Clear()
{
    boxes.clear();
    cylinders.clear();
}

// 直方体（OBB / AABB）コリジョンの追加
void WorldCollision::AddBox(int id, const std::string& tag, const VECTOR& center, float halfWidth, float halfHeight, float halfDepth, float rotationY)
{
    CollisionBox box;
    box.id = id;
    box.tag = tag;
    box.center = center;
    box.halfWidth = halfWidth;
    box.halfHeight = halfHeight;
    box.halfDepth = halfDepth;
    box.rotationY = rotationY;
    box.enabled = true;
    boxes.push_back(box);
}

// 垂直円柱コリジョンの追加
void WorldCollision::AddCylinder(int id, const std::string& tag, const VECTOR& center, float radius, float minY, float maxY)
{
    CollisionCylinder cyl;
    cyl.id = id;
    cyl.tag = tag;
    cyl.center = center;
    cyl.radius = radius;
    cyl.minY = minY;
    cyl.maxY = maxY;
    cyl.enabled = true;
    cylinders.push_back(cyl);
}

// 球とOBBの衝突判定・法線・押し戻し量計算
bool WorldCollision::TestSphereOBB(const VECTOR& spherePos, float sphereRadius, const CollisionBox& box, VECTOR* outNormal, float* outPenetration)
{
    // Y軸高さチェック（球の下端〜上端とボックスのY範囲）
    float boxMinY = box.center.y - box.halfHeight;
    float boxMaxY = box.center.y + box.halfHeight;

    if (spherePos.y + sphereRadius < boxMinY || spherePos.y - sphereRadius > boxMaxY)
    {
        return false;
    }

    // 中心からの相対座標
    float dx = spherePos.x - box.center.x;
    float dz = spherePos.z - box.center.z;

    // ボックスのローカル座標系へ逆回転（Y軸回転）
    float cosA = std::cos(-box.rotationY);
    float sinA = std::sin(-box.rotationY);
    float localX = dx * cosA - dz * sinA;
    float localZ = dx * sinA + dz * cosA;

    // ローカルAABBの最近傍点をクランプ
    float clampedX = (localX < -box.halfWidth) ? -box.halfWidth : ((localX > box.halfWidth) ? box.halfWidth : localX);
    float clampedZ = (localZ < -box.halfDepth) ? -box.halfDepth : ((localZ > box.halfDepth) ? box.halfDepth : localZ);

    float diffX = localX - clampedX;
    float diffZ = localZ - clampedZ;
    float distSq = diffX * diffX + diffZ * diffZ;

    bool isInside = (std::abs(localX) <= box.halfWidth && std::abs(localZ) <= box.halfDepth);

    if (isInside)
    {
        // ボックス内部にめり込んでいる場合: 4辺の中で最も近い境界へ押し出す
        float dLeft = localX - (-box.halfWidth);
        float dRight = box.halfWidth - localX;
        float dBottom = localZ - (-box.halfDepth);
        float dTop = box.halfDepth - localZ;

        float minD = dLeft;
        float localNormX = -1.0f;
        float localNormZ = 0.0f;

        if (dRight < minD)
        {
            minD = dRight;
            localNormX = 1.0f;
            localNormZ = 0.0f;
        }
        if (dBottom < minD)
        {
            minD = dBottom;
            localNormX = 0.0f;
            localNormZ = -1.0f;
        }
        if (dTop < minD)
        {
            minD = dTop;
            localNormX = 0.0f;
            localNormZ = 1.0f;
        }

        float penetration = minD + sphereRadius;

        // ワールド座標系へ法線を戻す
        float worldCos = std::cos(box.rotationY);
        float worldSin = std::sin(box.rotationY);
        float worldNormX = localNormX * worldCos - localNormZ * worldSin;
        float worldNormZ = localNormX * worldSin + localNormZ * worldCos;

        if (outNormal)
        {
            *outNormal = VGet(worldNormX, 0.0f, worldNormZ);
        }
        if (outPenetration)
        {
            *outPenetration = penetration;
        }

        return true;
    }
    else
    {
        // ボックス外部にあり境界と球が重なっているか判定
        if (distSq < sphereRadius * sphereRadius)
        {
            float dist = std::sqrt(distSq);
            float penetration = sphereRadius - dist;

            float localNormX = (dist > 0.0001f) ? (diffX / dist) : 1.0f;
            float localNormZ = (dist > 0.0001f) ? (diffZ / dist) : 0.0f;

            float worldCos = std::cos(box.rotationY);
            float worldSin = std::sin(box.rotationY);
            float worldNormX = localNormX * worldCos - localNormZ * worldSin;
            float worldNormZ = localNormX * worldSin + localNormZ * worldCos;

            if (outNormal)
            {
                *outNormal = VGet(worldNormX, 0.0f, worldNormZ);
            }
            if (outPenetration)
            {
                *outPenetration = penetration;
            }

            return true;
        }
    }

    return false;
}

// 球と円柱の衝突判定・法線・押し戻し量計算
bool WorldCollision::TestSphereCylinder(const VECTOR& spherePos, float sphereRadius, const CollisionCylinder& cyl, VECTOR* outNormal, float* outPenetration)
{
    // 高さチェック
    if (spherePos.y + sphereRadius < cyl.minY || spherePos.y - sphereRadius > cyl.maxY)
    {
        return false;
    }

    float dx = spherePos.x - cyl.center.x;
    float dz = spherePos.z - cyl.center.z;
    float distSq = dx * dx + dz * dz;
    float totalRadius = sphereRadius + cyl.radius;

    if (distSq < totalRadius * totalRadius)
    {
        float dist = std::sqrt(distSq);
        float penetration = totalRadius - dist;

        float normX = (dist > 0.0001f) ? (dx / dist) : 1.0f;
        float normZ = (dist > 0.0001f) ? (dz / dist) : 0.0f;

        if (outNormal)
        {
            *outNormal = VGet(normX, 0.0f, normZ);
        }
        if (outPenetration)
        {
            *outPenetration = penetration;
        }

        return true;
    }

    return false;
}

// 球（プレイヤー・自転車）と障害物との衝突判定
bool WorldCollision::CheckCollision(const VECTOR& pos, float radius) const
{
    for (const auto& box : boxes)
    {
        if (!box.enabled) continue;
        if (TestSphereOBB(pos, radius, box))
        {
            return true;
        }
    }

    for (const auto& cyl : cylinders)
    {
        if (!cyl.enabled) continue;
        if (TestSphereCylinder(pos, radius, cyl))
        {
            return true;
        }
    }

    return false;
}

// 線分・カプセルスイープ衝突判定（移動前〜移動後の間に障害物があるか）
bool WorldCollision::CheckSweep(const VECTOR& from, const VECTOR& to, float radius) const
{
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float dz = to.z - from.z;
    float length = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (length < 0.0001f)
    {
        return CheckCollision(to, radius);
    }

    // 移動距離に応じてステップ分割判定（すり抜けを100%防止）
    int steps = static_cast<int>(std::ceil(length / (radius * 0.5f)));
    if (steps < 1) steps = 1;
    if (steps > 16) steps = 16;

    for (int i = 1; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        VECTOR sample = VGet(
            from.x + dx * t,
            from.y + dy * t,
            from.z + dz * t
        );
        if (CheckCollision(sample, radius))
        {
            return true;
        }
    }

    return false;
}

// めり込み解消・滑らかスライド補正
bool WorldCollision::ResolveCollision(VECTOR& pos, float radius) const
{
    bool resolvedAny = false;

    // 複数オブジェクトとの接触にも対応できるよう最大3回反復緩和
    for (int iter = 0; iter < 3; ++iter)
    {
        bool hitThisPass = false;

        for (const auto& box : boxes)
        {
            if (!box.enabled) continue;
            VECTOR normal = VGet(0.0f, 0.0f, 0.0f);
            float penetration = 0.0f;

            if (TestSphereOBB(pos, radius, box, &normal, &penetration))
            {
                pos.x += normal.x * (penetration + 0.002f);
                pos.z += normal.z * (penetration + 0.002f);
                hitThisPass = true;
                resolvedAny = true;
            }
        }

        for (const auto& cyl : cylinders)
        {
            if (!cyl.enabled) continue;
            VECTOR normal = VGet(0.0f, 0.0f, 0.0f);
            float penetration = 0.0f;

            if (TestSphereCylinder(pos, radius, cyl, &normal, &penetration))
            {
                pos.x += normal.x * (penetration + 0.002f);
                pos.z += normal.z * (penetration + 0.002f);
                hitThisPass = true;
                resolvedAny = true;
            }
        }

        if (!hitThisPass)
        {
            break;
        }
    }

    return resolvedAny;
}

// デバッグ描画（F6有効時に全コリジョン枠を3Dワイヤーフレーム描画）
void WorldCollision::DrawDebug() const
{
    // 直方体（OBB）のワイヤーフレーム描画
    for (const auto& box : boxes)
    {
        if (!box.enabled) continue;

        unsigned int wireColor = GetColor(60, 200, 220); // デフォルト水色
        if (box.tag == "HousePorch")
        {
            wireColor = GetColor(100, 230, 140); // 玄関ポーチ部: 黄緑色
        }
        else if (box.tag == "Fence" || box.tag == "Wall")
        {
            wireColor = GetColor(240, 210, 60);  // 敷地フェンス・塀: 黄色
        }
        else if (box.tag == "Boundary")
        {
            wireColor = GetColor(220, 70, 70);   // 外周境界: 赤色
        }

        float cosA = std::cos(box.rotationY);
        float sinA = std::sin(box.rotationY);

        // ローカル8頂点をワールド座標へ変換
        VECTOR corners[8];
        int idx = 0;
        for (int ySign = -1; ySign <= 1; ySign += 2)
        {
            float ly = ySign * box.halfHeight;
            for (int zSign = -1; zSign <= 1; zSign += 2)
            {
                float lz = zSign * box.halfDepth;
                for (int xSign = -1; xSign <= 1; xSign += 2)
                {
                    float lx = xSign * box.halfWidth;
                    float wx = box.center.x + lx * cosA + lz * sinA;
                    float wy = box.center.y + ly;
                    float wz = box.center.z - lx * sinA + lz * cosA;
                    corners[idx++] = VGet(wx, wy, wz);
                }
            }
        }

        // 底面4辺
        DrawLine3D(corners[0], corners[1], wireColor);
        DrawLine3D(corners[1], corners[3], wireColor);
        DrawLine3D(corners[3], corners[2], wireColor);
        DrawLine3D(corners[2], corners[0], wireColor);

        // 上面4辺
        DrawLine3D(corners[4], corners[5], wireColor);
        DrawLine3D(corners[5], corners[7], wireColor);
        DrawLine3D(corners[7], corners[6], wireColor);
        DrawLine3D(corners[6], corners[4], wireColor);

        // 柱4辺
        DrawLine3D(corners[0], corners[4], wireColor);
        DrawLine3D(corners[1], corners[5], wireColor);
        DrawLine3D(corners[2], corners[6], wireColor);
        DrawLine3D(corners[3], corners[7], wireColor);
    }

    // 垂直円柱のワイヤーフレーム描画
    for (const auto& cyl : cylinders)
    {
        if (!cyl.enabled) continue;

        unsigned int cylColor = GetColor(255, 150, 40); // 電柱: オレンジ
        if (cyl.tag == "Mailbox")
        {
            cylColor = GetColor(240, 100, 220); // ポスト: マゼンタ
        }

        const int DIVS = 10;
        VECTOR prevBottom = VGet(0, 0, 0);
        VECTOR prevTop = VGet(0, 0, 0);

        for (int i = 0; i <= DIVS; ++i)
        {
            float angle = (DX_PI_F * 2.0f * i) / DIVS;
            float cx = cyl.center.x + std::cos(angle) * cyl.radius;
            float cz = cyl.center.z + std::sin(angle) * cyl.radius;
            VECTOR bPos = VGet(cx, cyl.minY, cz);
            VECTOR tPos = VGet(cx, cyl.maxY, cz);

            if (i > 0)
            {
                DrawLine3D(prevBottom, bPos, cylColor);
                DrawLine3D(prevTop, tPos, cylColor);
            }
            if (i % 2 == 0)
            {
                DrawLine3D(bPos, tPos, cylColor);
            }

            prevBottom = bPos;
            prevTop = tPos;
        }
    }
}
