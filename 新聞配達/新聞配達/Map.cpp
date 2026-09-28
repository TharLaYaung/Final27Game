#include "Map.h"
#include "DxLib.h"
#include <cmath>
#include <vector>

// 基本住宅モデルハンドル（4種類）
static int g_houseBaseModels[4] = { -1, -1, -1, -1 };

// 配置された8棟の住宅インスタンスハンドル
static int g_houseHandles[NEIGHBORHOOD_HOUSE_COUNT] = { -1, -1, -1, -1, -1, -1, -1, -1 };

// 8棟の住宅定義データ
static std::vector<HouseData> g_houses;

// マップ初期化
void MapInit()
{
    // 基本モデル4種をディスクから1度だけ読み込み
    g_houseBaseModels[0] = MV1LoadModel("Data/Model/house/house1.mv1");
    g_houseBaseModels[1] = MV1LoadModel("Data/Model/house/house2.mv1");
    g_houseBaseModels[2] = MV1LoadModel("Data/Model/house/house3.mv1");
    g_houseBaseModels[3] = MV1LoadModel("Data/Model/house/house4.mv1");

    // 8棟の住宅データを初期化
    g_houses.clear();

    // 住宅0: 西通り（X = -25）の西側、東向き
    g_houses.push_back({
        0, 0,
        VGet(-33.0f, 3.80f, 0.0f),
        -DX_PI_F * 0.5f,
        VGet(-28.0f, 0.0f, 1.5f),
        -DX_PI_F * 0.5f,
        true, 7.5f
    });

    // 住宅1: 北横通り（Z = +15）の北側、南向き
    g_houses.push_back({
        1, 1,
        VGet(-12.0f, 3.80f, 23.0f),
        DX_PI_F,
        VGet(-10.5f, 0.0f, 18.0f),
        DX_PI_F,
        true, 7.5f
    });

    // 住宅2: 北路地行き止まり（X = 0, Z = +35〜）の最奥、南向き
    g_houses.push_back({
        2, 2,
        VGet(0.0f, 3.80f, 42.0f),
        DX_PI_F,
        VGet(1.8f, 0.0f, 37.0f),
        DX_PI_F,
        false, 7.5f
    });

    // 住宅3: 北横通り（Z = +15）の北東側、南向き
    g_houses.push_back({
        3, 0,
        VGet(15.0f, 3.80f, 23.0f),
        DX_PI_F,
        VGet(13.5f, 0.0f, 18.0f),
        DX_PI_F,
        true, 7.5f
    });

    // 住宅4: 中央横通り（Z = -10）の北側、南向き
    g_houses.push_back({
        4, 1,
        VGet(0.0f, 3.80f, -2.0f),
        DX_PI_F,
        VGet(1.8f, 0.0f, -7.0f),
        DX_PI_F,
        true, 7.5f
    });

    // 住宅5: 東通り（X = +25）の東側、西向き
    g_houses.push_back({
        5, 2,
        VGet(33.0f, 3.80f, 0.0f),
        DX_PI_F * 0.5f,
        VGet(28.0f, 0.0f, 1.5f),
        DX_PI_F * 0.5f,
        false, 7.5f
    });

    // 住宅6: 東通り南端（X = +25, Z = -25）の東側、西向き
    g_houses.push_back({
        6, 0,
        VGet(33.0f, 3.80f, -25.0f),
        DX_PI_F * 0.5f,
        VGet(28.0f, 0.0f, -23.5f),
        DX_PI_F * 0.5f,
        true, 7.5f
    });

    // 住宅7: 西通り南端（X = -25, Z = -25）の西側、東向き
    g_houses.push_back({
        7, 1,
        VGet(-33.0f, 3.80f, -25.0f),
        -DX_PI_F * 0.5f,
        VGet(-28.0f, 0.0f, -23.5f),
        -DX_PI_F * 0.5f,
        true, 7.5f
    });

    // 各住宅モデルを複製し、配置・スケール・コリジョンを設定
    for (size_t i = 0; i < g_houses.size() && i < NEIGHBORHOOD_HOUSE_COUNT; i++)
    {
        const HouseData& h = g_houses[i];
        int baseHandle = g_houseBaseModels[h.modelType];

        if (baseHandle != -1)
        {
            // モデル複製（省メモリかつ高速）
            g_houseHandles[i] = MV1DuplicateModel(baseHandle);

            // モデル種別に応じたスケール（house1〜3は0.5倍、センチメートル単位のhouse4は0.005倍）
            float s = (h.modelType == 3) ? 0.005f : 0.50f;
            MV1SetScale(g_houseHandles[i], VGet(s, s, s));

            // ワールド座標・向き設定
            MV1SetPosition(g_houseHandles[i], h.position);
            MV1SetRotationXYZ(g_houseHandles[i], VGet(0.0f, h.rotationY, 0.0f));

            // コリジョン情報の構築（カプセル判定用）
            MV1SetupCollInfo(g_houseHandles[i], -1, 8, 8, 8);
        }
        else
        {
            g_houseHandles[i] = -1;
        }
    }
}

// マップ描画
void MapDraw()
{
    // 全8棟の住宅モデルを描画
    for (int i = 0; i < NEIGHBORHOOD_HOUSE_COUNT; i++)
    {
        if (g_houseHandles[i] != -1)
        {
            MV1DrawModel(g_houseHandles[i]);
        }
    }

    // 玄関灯（点灯設定されている住宅のポーチランプ球体描画）
    for (size_t i = 0; i < g_houses.size(); i++)
    {
        if (g_houses[i].porchLight)
        {
            // 住宅の向きから玄関付近のオフセットを計算
            float angle = g_houses[i].rotationY;
            VECTOR fwd = VGet(sinf(angle), 0.0f, cosf(angle));
            VECTOR right = VGet(cosf(angle), 0.0f, -sinf(angle));

            VECTOR lampPos = VAdd(g_houses[i].position, VScale(fwd, -2.5f));
            lampPos = VAdd(lampPos, VScale(right, 1.2f));
            lampPos.y = 2.2f;

            unsigned int lampCol = GetColor(200, 160, 90);
            DrawSphere3D(lampPos, 0.12f, 6, lampCol, lampCol, TRUE);
        }
    }
}

// マップデバッグ描画（F6キー有効時）
void MapDrawDebug(int currentTargetId)
{
    for (size_t i = 0; i < g_houses.size(); i++)
    {
        const HouseData& h = g_houses[i];
        bool isTarget = (h.id == currentTargetId);

        // 住宅のバウンディング円筒ワイヤー描画
        unsigned int houseCol = isTarget ? GetColor(255, 120, 50) : GetColor(70, 90, 80);
        VECTOR bBottom = VGet(h.position.x, 0.1f, h.position.z);
        VECTOR bTop = VGet(h.position.x, 5.0f, h.position.z);
        DrawCapsule3D(bBottom, bTop, h.boundRadius, 8, houseCol, houseCol, FALSE);

        // 3Dスクリーン投影テキスト
        VECTOR screenPos;
        VECTOR labelWorldPos = VGet(h.position.x, 4.5f, h.position.z);
        screenPos = ConvWorldPosToScreenPos(labelWorldPos);

        if (screenPos.z >= 0.0f && screenPos.z <= 1.0f)
        {
            char str[64];
            snprintf(str, sizeof(str), "HOUSE %d%s", h.id, isTarget ? " [TARGET]" : "");
            DrawString((int)screenPos.x - 30, (int)screenPos.y, str, houseCol);
        }

        // ポストの位置マーカー
        unsigned int mbCol = isTarget ? GetColor(255, 220, 80) : GetColor(120, 140, 130);
        DrawSphere3D(VGet(h.mailboxPosition.x, 1.2f, h.mailboxPosition.z), 0.35f, 6, mbCol, mbCol, FALSE);

        VECTOR mbScreen = ConvWorldPosToScreenPos(VGet(h.mailboxPosition.x, 1.8f, h.mailboxPosition.z));
        if (mbScreen.z >= 0.0f && mbScreen.z <= 1.0f)
        {
            char str[32];
            snprintf(str, sizeof(str), "POST %d", h.id);
            DrawString((int)mbScreen.x - 20, (int)mbScreen.y, str, mbCol);
        }
    }
}

// 壁・住宅との当たり判定
bool MapCheckWallCollision(VECTOR pos, float radius)
{
    // 街の外周フェンス境界制限（街の外へ脱落しないように制限）
    if (pos.x < -43.0f || pos.x > 43.0f || pos.z < -38.0f || pos.z > 52.0f)
    {
        return true;
    }

    VECTOR top = VGet(pos.x, pos.y, pos.z);
    VECTOR bottom = VGet(pos.x, pos.y - 1.5f, pos.z);

    // 各住宅との詳細ポリゴン当たり判定
    for (int i = 0; i < NEIGHBORHOOD_HOUSE_COUNT; i++)
    {
        if (g_houseHandles[i] == -1) continue;

        // 広域判定（ブロードフェーズ）: 住宅中心から離れている場合は詳細判定をスキップ
        float dx = pos.x - g_houses[i].position.x;
        float dz = pos.z - g_houses[i].position.z;
        float distSq = dx * dx + dz * dz;
        float maxCheck = g_houses[i].boundRadius + radius;

        if (distSq > maxCheck * maxCheck)
        {
            continue;
        }

        // ナローフェーズ: DxLibポリゴンカプセル判定
        MV1_COLL_RESULT_POLY_DIM result =
            MV1CollCheck_Capsule(
                g_houseHandles[i],
                -1,
                top,
                bottom,
                radius
            );

        bool hit = (result.HitNum > 0);
        MV1CollResultPolyDimTerminate(result);

        if (hit)
        {
            return true;
        }
    }

    return false;
}

// 床の高さ取得
float MapGetFloorY(VECTOR pos)
{
    VECTOR start = VGet(pos.x, pos.y + 0.5f, pos.z);
    VECTOR end = VGet(pos.x, pos.y - 3.0f, pos.z);
    float bestY = 0.0f;

    for (int i = 0; i < NEIGHBORHOOD_HOUSE_COUNT; i++)
    {
        if (g_houseHandles[i] == -1) continue;

        MV1_COLL_RESULT_POLY result =
            MV1CollCheck_Line(
                g_houseHandles[i],
                -1,
                start,
                end
            );

        if (result.HitFlag)
        {
            if (result.HitPosition.y > bestY)
            {
                bestY = result.HitPosition.y;
            }
        }
    }

    return bestY;
}

// 全住宅データの参照取得
const std::vector<HouseData>& MapGetHouses()
{
    return g_houses;
}

// 特定住宅データの取得
const HouseData* MapGetHouseById(int id)
{
    for (size_t i = 0; i < g_houses.size(); i++)
    {
        if (g_houses[i].id == id)
        {
            return &g_houses[i];
        }
    }
    return nullptr;
}

// マップ終了処理
void MapFinalize()
{
    for (int i = 0; i < NEIGHBORHOOD_HOUSE_COUNT; i++)
    {
        if (g_houseHandles[i] != -1)
        {
            MV1DeleteModel(g_houseHandles[i]);
            g_houseHandles[i] = -1;
        }
    }

    for (int t = 0; t < 4; t++)
    {
        if (g_houseBaseModels[t] != -1)
        {
            MV1DeleteModel(g_houseBaseModels[t]);
            g_houseBaseModels[t] = -1;
        }
    }

    g_houses.clear();
}