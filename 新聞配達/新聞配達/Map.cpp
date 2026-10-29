#include "Map.h"
#include "WorldCollision.h"
#include "DxLib.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdio>

// 基本住宅モデルハンドル（4種類）
static int g_houseBaseModels[4] = { -1, -1, -1, -1 };

// 配置された8棟の住宅インスタンスハンドル
static int g_houseHandles[NEIGHBORHOOD_HOUSE_COUNT] = { -1, -1, -1, -1, -1, -1, -1, -1 };

// 各住宅の玄関灯ポイントライトハンドル
static int g_houseLightHandles[NEIGHBORHOOD_HOUSE_COUNT] = { -1, -1, -1, -1, -1, -1, -1, -1 };

// 住宅テクスチャハンドル（高解像度アトラス）
static int g_houseTexture = -1;

// 8棟の住宅定義データ
static std::vector<HouseData> g_houses;

// 3D直方体ブロック（塀・壁）を描画するヘルパー関数
static void DrawBlockWall(float cx, float cy, float cz, float hw, float hh, float hd, unsigned int faceCol, unsigned int capCol)
{
    float x1 = cx - hw, x2 = cx + hw;
    float y1 = cy - hh, y2 = cy + hh;
    float z1 = cz - hd, z2 = cz + hd;

    // 前面 (Z-)
    DrawTriangle3D(VGet(x1, y1, z1), VGet(x2, y1, z1), VGet(x1, y2, z1), faceCol, TRUE);
    DrawTriangle3D(VGet(x2, y1, z1), VGet(x2, y2, z1), VGet(x1, y2, z1), faceCol, TRUE);
    // 背面 (Z+)
    DrawTriangle3D(VGet(x1, y1, z2), VGet(x1, y2, z2), VGet(x2, y1, z2), faceCol, TRUE);
    DrawTriangle3D(VGet(x2, y1, z2), VGet(x1, y2, z2), VGet(x2, y2, z2), faceCol, TRUE);
    // 左面 (X-)
    DrawTriangle3D(VGet(x1, y1, z1), VGet(x1, y2, z1), VGet(x1, y1, z2), faceCol, TRUE);
    DrawTriangle3D(VGet(x1, y2, z1), VGet(x1, y2, z2), VGet(x1, y1, z2), faceCol, TRUE);
    // 右面 (X+)
    DrawTriangle3D(VGet(x2, y1, z1), VGet(x2, y1, z2), VGet(x2, y2, z1), faceCol, TRUE);
    DrawTriangle3D(VGet(x2, y2, z1), VGet(x2, y1, z2), VGet(x2, y2, z2), faceCol, TRUE);
    // 天面 (Y+)
    DrawTriangle3D(VGet(x1, y2, z1), VGet(x2, y2, z1), VGet(x1, y2, z2), capCol, TRUE);
    DrawTriangle3D(VGet(x2, y2, z1), VGet(x2, y2, z2), VGet(x1, y2, z2), capCol, TRUE);
}

// 静的コリジョン構築関数（住宅・塀・外周境界・ポスト）
static void BuildMapCollision()
{
    WorldCollision& col = WorldCollision::Instance();
    col.Clear();

    // 1. 各住宅の複合コリジョン登録（主屋体＋別棟/袖部、玄関経路は開放）
    for (size_t i = 0; i < g_houses.size(); ++i)
    {
        const HouseData& h = g_houses[i];

        // 主屋体ボックス（幅約8.8m x 奥行約10.4m x 高約7.0m）
        col.AddBox(
            static_cast<int>(i) * 10 + 1,
            "HouseBody",
            VGet(h.position.x, 3.5f, h.position.z),
            4.4f, 3.5f, 5.2f,
            h.rotationY
        );

        // 別棟/裏手付属部（主屋の裏手方向へオフセット、玄関前は遮らない）
        float sinA = std::sin(h.rotationY);
        float cosA = std::cos(h.rotationY);
        float rearOffset = -2.2f;
        float annexX = h.position.x - rearOffset * sinA;
        float annexZ = h.position.z - rearOffset * cosA;

        col.AddBox(
            static_cast<int>(i) * 10 + 2,
            "HousePorch",
            VGet(annexX, 3.0f, annexZ),
            2.8f, 3.0f, 2.8f,
            h.rotationY
        );

        // 郵便ポストの垂直円柱コリジョン（半径0.35m、高さ1.5m）
        col.AddCylinder(
            static_cast<int>(i),
            "Mailbox",
            h.mailboxPosition,
            0.35f, 0.0f, 1.5f
        );
    }

    // 2. 街の外周境界フェンス・壁（プレイヤーの脱落・虚空進入防止）
    // 南側外周壁 (Z = -36.5)
    col.AddBox(101, "Boundary", VGet(0.0f, 2.0f, -36.5f), 44.0f, 3.0f, 1.0f, 0.0f);
    // 北側外周壁 (Z = +52.0)
    col.AddBox(102, "Boundary", VGet(0.0f, 2.0f, 52.0f), 44.0f, 3.0f, 1.0f, 0.0f);
    // 西側外周壁 (X = -43.0)
    col.AddBox(103, "Boundary", VGet(-43.0f, 2.0f, 8.0f), 1.0f, 3.0f, 44.0f, 0.0f);
    // 東側外周壁 (X = +43.0)
    col.AddBox(104, "Boundary", VGet(43.0f, 2.0f, 8.0f), 1.0f, 3.0f, 44.0f, 0.0f);

    // 3. 敷地境界ブロック塀・フェンス（日本の深夜住宅街らしい狭小路地・区画構成）
    // 西側敷地境界塀（住宅7と住宅0の間）
    col.AddBox(105, "Fence", VGet(-35.0f, 1.0f, -12.5f), 7.0f, 1.2f, 0.3f, 0.0f);
    // 東側敷地境界塀（住宅6と住宅5の間）
    col.AddBox(106, "Fence", VGet(35.0f, 1.0f, -12.5f), 7.0f, 1.2f, 0.3f, 0.0f);
    // 北路地西側塀（路地Cと住宅1の裏庭間）
    col.AddBox(107, "Wall", VGet(-4.2f, 1.0f, 28.0f), 0.3f, 1.2f, 11.5f, 0.0f);
    // 北路地東側塀（路地Cと住宅3の裏庭間）
    col.AddBox(108, "Wall", VGet(4.2f, 1.0f, 28.0f), 0.3f, 1.2f, 11.5f, 0.0f);
    // 住宅4の裏手境界壁（住宅4と北横通りBの間）
    col.AddBox(109, "Fence", VGet(0.0f, 1.0f, 6.0f), 8.5f, 1.2f, 0.3f, 0.0f);
    // 住宅1の北側敷地奥壁
    col.AddBox(110, "Fence", VGet(-13.0f, 1.0f, 34.0f), 8.0f, 1.2f, 0.3f, 0.0f);
    // 住宅3の北側敷地奥壁
    col.AddBox(111, "Fence", VGet(15.0f, 1.0f, 34.0f), 8.0f, 1.2f, 0.3f, 0.0f);
}

// 街灯電柱コリジョンの登録
void MapRegisterLampPoles(const std::vector<VECTOR>& polePositions)
{
    WorldCollision& col = WorldCollision::Instance();
    for (size_t i = 0; i < polePositions.size(); ++i)
    {
        col.AddCylinder(
            200 + static_cast<int>(i),
            "LampPole",
            polePositions[i],
            0.25f, 0.0f, 4.5f
        );
    }
}

// マップ初期化
void MapInit()
{
    // 基本モデル4種をディスクから読み込み
    g_houseBaseModels[0] = MV1LoadModel("Data/Model/house/house1.mv1");
    g_houseBaseModels[1] = MV1LoadModel("Data/Model/house/house2.mv1");
    g_houseBaseModels[2] = MV1LoadModel("Data/Model/house/house3.mv1");
    g_houseBaseModels[3] = MV1LoadModel("Data/Model/house/house4.mv1");

    // 高解像度テクスチャの読み込み
    g_houseTexture = LoadGraph("Data/Model/house/color_1024x1024.jpg");

    // 8棟の住宅データを初期化（リアルな住宅街配置・向き・アクセス可能なポスト）
    g_houses.clear();

    // 住宅0: 西通り（X = -25）の西側、東向き（配達先0・縁石際設置）
    g_houses.push_back({
        0, 0,
        VGet(-34.0f, 0.0f, 0.0f),
        DX_PI_F * 0.5f,
        VGet(-27.6f, 0.0f, 1.5f),
        DX_PI_F * 0.5f,
        true, 8.0f
    });

    // 住宅1: 北横通り（Z = +15）の北西側、南向き（配達先1・縁石際設置）
    g_houses.push_back({
        1, 1,
        VGet(-13.0f, 0.0f, 25.0f),
        DX_PI_F,
        VGet(-11.0f, 0.0f, 18.4f),
        DX_PI_F,
        true, 8.0f
    });

    // 住宅2: 北路地行き止まり（X = 0, Z = +38）の最奥、南向き（配達先2・奥地縁石際設置）
    g_houses.push_back({
        2, 2,
        VGet(0.0f, 0.0f, 45.0f),
        DX_PI_F,
        VGet(2.4f, 0.0f, 39.3f),
        DX_PI_F,
        true, 8.0f
    });

    // 住宅3: 北横通り（Z = +15）の北東側、南向き（配達先3・縁石際設置）
    g_houses.push_back({
        3, 3,
        VGet(15.0f, 0.0f, 25.0f),
        DX_PI_F,
        VGet(13.5f, 0.0f, 18.4f),
        DX_PI_F,
        true, 8.0f
    });

    // 住宅4: 中央横通り（Z = -10）の北側街区、南向き（配達先4・縁石際設置）
    g_houses.push_back({
        4, 1,
        VGet(0.0f, 0.0f, -2.0f),
        DX_PI_F,
        VGet(1.8f, 0.0f, -6.8f),
        DX_PI_F,
        true, 8.0f
    });

    // 住宅5: 東通り（X = +25）の東側、西向き（配達先5・縁石際設置）
    g_houses.push_back({
        5, 0,
        VGet(34.0f, 0.0f, 0.0f),
        -DX_PI_F * 0.5f,
        VGet(27.6f, 0.0f, 1.5f),
        -DX_PI_F * 0.5f,
        true, 8.0f
    });

    // 住宅6: 東通り南東（X = +25, Z = -25）の東側、西向き（配達先6・縁石際設置）
    g_houses.push_back({
        6, 3,
        VGet(34.0f, 0.0f, -25.0f),
        -DX_PI_F * 0.5f,
        VGet(27.6f, 0.0f, -23.5f),
        -DX_PI_F * 0.5f,
        true, 8.0f
    });

    // 住宅7: 西通り南西（X = -25, Z = -25）の西側、東向き（配達先7・縁石際設置）
    g_houses.push_back({
        7, 2,
        VGet(-34.0f, 0.0f, -25.0f),
        DX_PI_F * 0.5f,
        VGet(-27.6f, 0.0f, -23.5f),
        DX_PI_F * 0.5f,
        true, 8.0f
    });

    // 各住宅モデルインスタンスの生成・配置
    for (size_t i = 0; i < g_houses.size() && i < NEIGHBORHOOD_HOUSE_COUNT; i++)
    {
        const HouseData& h = g_houses[i];
        int baseHandle = g_houseBaseModels[h.modelType];

        if (baseHandle != -1)
        {
            g_houseHandles[i] = MV1DuplicateModel(baseHandle);

            float s = 0.008f;
            MV1SetScale(g_houseHandles[i], VGet(s, s, s));
            MV1SetPosition(g_houseHandles[i], h.position);
            MV1SetRotationXYZ(g_houseHandles[i], VGet(0.0f, h.rotationY, 0.0f));

            int matNum = MV1GetMaterialNum(g_houseHandles[i]);
            for (int m = 0; m < matNum; ++m)
            {
                MV1SetMaterialDifColor(g_houseHandles[i], m, GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
                MV1SetMaterialAmbColor(g_houseHandles[i], m, GetColorF(0.85f, 0.85f, 0.85f, 1.0f));
                MV1SetMaterialEmiColor(g_houseHandles[i], m, GetColorF(0.18f, 0.20f, 0.22f, 1.0f));
            }

            int texNum = MV1GetTextureNum(g_houseHandles[i]);
            for (int ti = 0; ti < texNum; ++ti)
            {
                if (g_houseTexture != -1)
                {
                    MV1SetTextureGraphHandle(g_houseHandles[i], ti, g_houseTexture, FALSE);
                }
            }

            int meshNum = MV1GetMeshNum(g_houseHandles[i]);
            for (int mi = 0; mi < meshNum; ++mi)
            {
                MV1SetMeshBackCulling(g_houseHandles[i], mi, DX_CULLING_NONE);
            }

            // 玄関灯ポイントライトの初期生成
            if (h.porchLight)
            {
                float angle = h.rotationY;
                VECTOR fwd = VGet(sinf(angle), 0.0f, cosf(angle));
                VECTOR right = VGet(cosf(angle), 0.0f, -sinf(angle));
                VECTOR lampPos = VAdd(h.position, VScale(fwd, 4.2f));
                lampPos = VAdd(lampPos, VScale(right, 1.2f));
                lampPos.y = 2.4f;

                int lightH = CreatePointLightHandle(lampPos, 14.0f, 1.0f, 0.12f, 0.035f);
                if (lightH != -1)
                {
                    SetLightEnableHandle(lightH, TRUE);
                    SetLightDifColorHandle(lightH, GetColorF(1.0f, 0.88f, 0.62f, 1.0f));
                    SetLightSpcColorHandle(lightH, GetColorF(0.25f, 0.20f, 0.12f, 1.0f));
                    g_houseLightHandles[i] = lightH;
                }
            }
        }
        else
        {
            g_houseHandles[i] = -1;
            g_houseLightHandles[i] = -1;
        }
    }

    // 静的ワールドコリジョンの構築
    BuildMapCollision();
}

// 日別シードに基づく玄関灯の点灯・消灯制御
void MapSetDayPorchLights(int day, unsigned int daySeed)
{
    if (day < 1) day = 1;
    if (day > 7) day = 7;

    // 疑似乱数ジェネレータで日毎に点灯住宅を決定
    unsigned int rng = daySeed;
    auto NextRand = [&]() -> unsigned int {
        rng = rng * 1664525u + 1013904223u;
        return rng;
    };

    // 日別点灯目標数（Day 1: 6棟点灯 〜 Day 7: 1棟のみ）
    int targetLitCount = 6;
    switch (day)
    {
    case 1: targetLitCount = 6; break;
    case 2: targetLitCount = 5; break;
    case 3: targetLitCount = 4; break;
    case 4: targetLitCount = 3; break;
    case 5: targetLitCount = 2; break;
    case 6: targetLitCount = 2; break;
    case 7:
    default: targetLitCount = 1; break;
    }

    std::vector<int> houseIndices = { 0, 1, 2, 3, 4, 5, 6, 7 };
    for (size_t i = houseIndices.size() - 1; i > 0; --i)
    {
        size_t j = NextRand() % (i + 1);
        std::swap(houseIndices[i], houseIndices[j]);
    }

    for (size_t i = 0; i < g_houses.size() && i < NEIGHBORHOOD_HOUSE_COUNT; ++i)
    {
        bool shouldLight = false;
        for (int k = 0; k < targetLitCount; ++k)
        {
            if (houseIndices[k] == static_cast<int>(i))
            {
                shouldLight = true;
                break;
            }
        }
        g_houses[i].porchLight = shouldLight;

        // DxLibライトハンドルの更新
        if (g_houseLightHandles[i] != -1)
        {
            SetLightEnableHandle(g_houseLightHandles[i], shouldLight ? TRUE : FALSE);
        }
    }
}

// 安全性検証（スポーン地点および全8ポストの侵入可能チェック）
bool MapValidateSafety()
{
    const WorldCollision& col = WorldCollision::Instance();

    // プレイヤー初期位置 (0, 1.7, -30) のコリジョンチェック
    if (col.CheckCollision(VGet(0.0f, 1.7f, -30.0f), 0.45f))
    {
        return false;
    }

    // 自転車初期位置 (1.8, 0.18, -30) のコリジョンチェック
    if (col.CheckCollision(VGet(1.8f, 0.18f, -30.0f), 0.85f))
    {
        return false;
    }

    // 全8棟のポスト前アプローチ位置の検証
    for (size_t i = 0; i < g_houses.size(); ++i)
    {
        const HouseData& h = g_houses[i];
        float angle = h.mailboxRotationY;
        VECTOR fwd = VGet(sinf(angle), 0.0f, cosf(angle));
        // ポスト正面1.0mのアプローチ地点
        VECTOR approachPos = VAdd(h.mailboxPosition, VScale(fwd, 1.0f));
        approachPos.y = 1.0f;

        // ポスト周囲からアプローチ可能であること（半径0.4mで非衝突）
        if (col.CheckCollision(approachPos, 0.40f))
        {
            return false;
        }
    }

    return true;
}

// マップ描画（住宅モデル・敷地ブロック塀・玄関灯描画）
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

    // 住宅ごとの敷石エントランス舗装（玄関ポーチからポスト・道路縁石までを自然に接続）
    unsigned int paveFaceCol = GetColor(36, 40, 42);
    unsigned int paveTopCol = GetColor(48, 52, 54);

    // 住宅0エントランス敷石（西通り）
    DrawBlockWall(-30.8f, 0.02f, 1.5f, 3.2f, 0.02f, 1.4f, paveFaceCol, paveTopCol);
    // 住宅1エントランス敷石（北横通り西）
    DrawBlockWall(-11.0f, 0.02f, 21.6f, 1.4f, 0.02f, 3.2f, paveFaceCol, paveTopCol);
    // 住宅2エントランス敷石（北路地行き止まり）
    DrawBlockWall(2.4f, 0.02f, 42.1f, 1.4f, 0.02f, 2.8f, paveFaceCol, paveTopCol);
    // 住宅3エントランス敷石（北横通り東）
    DrawBlockWall(13.5f, 0.02f, 21.6f, 1.4f, 0.02f, 3.2f, paveFaceCol, paveTopCol);
    // 住宅4エントランス敷石（中央横通り）
    DrawBlockWall(1.8f, 0.02f, -4.5f, 1.4f, 0.02f, 2.4f, paveFaceCol, paveTopCol);
    // 住宅5エントランス敷石（東通り）
    DrawBlockWall(30.8f, 0.02f, 1.5f, 3.2f, 0.02f, 1.4f, paveFaceCol, paveTopCol);
    // 住宅6エントランス敷石（東通り南）
    DrawBlockWall(30.8f, 0.02f, -23.5f, 3.2f, 0.02f, 1.4f, paveFaceCol, paveTopCol);
    // 住宅7エントランス敷石（西通り南）
    DrawBlockWall(-30.8f, 0.02f, -23.5f, 3.2f, 0.02f, 1.4f, paveFaceCol, paveTopCol);

    // 敷地ブロック塀（ブロック塀・フェンス）の3D描画
    unsigned int wallFaceCol = GetColor(26, 30, 32);
    unsigned int wallCapCol = GetColor(42, 48, 50);

    // 西側敷地境界塀（住宅7と住宅0の間）
    DrawBlockWall(-35.0f, 0.6f, -12.5f, 7.0f, 0.6f, 0.25f, wallFaceCol, wallCapCol);
    // 東側敷地境界塀（住宅6と住宅5の間）
    DrawBlockWall(35.0f, 0.6f, -12.5f, 7.0f, 0.6f, 0.25f, wallFaceCol, wallCapCol);
    // 北路地両脇のブロック塀
    DrawBlockWall(-4.2f, 0.6f, 28.0f, 0.25f, 0.6f, 11.5f, wallFaceCol, wallCapCol);
    DrawBlockWall(4.2f, 0.6f, 28.0f, 0.25f, 0.6f, 11.5f, wallFaceCol, wallCapCol);
    // 住宅4裏手境界塀
    DrawBlockWall(0.0f, 0.6f, 6.0f, 8.5f, 0.6f, 0.25f, wallFaceCol, wallCapCol);
    // 北側奥壁
    DrawBlockWall(-13.0f, 0.6f, 34.0f, 8.0f, 0.6f, 0.25f, wallFaceCol, wallCapCol);
    DrawBlockWall(15.0f, 0.6f, 34.0f, 8.0f, 0.6f, 0.25f, wallFaceCol, wallCapCol);

    // 外周境界壁
    DrawBlockWall(0.0f, 1.2f, -36.0f, 43.0f, 1.2f, 0.5f, wallFaceCol, wallCapCol);
    DrawBlockWall(0.0f, 1.2f, 51.0f, 43.0f, 1.2f, 0.5f, wallFaceCol, wallCapCol);
    DrawBlockWall(-42.5f, 1.2f, 8.0f, 0.5f, 1.2f, 43.0f, wallFaceCol, wallCapCol);
    DrawBlockWall(42.5f, 1.2f, 8.0f, 0.5f, 1.2f, 43.0f, wallFaceCol, wallCapCol);

    // 玄関灯（点灯設定されている住宅のポーチランプ球体と光彩描画）
    for (size_t i = 0; i < g_houses.size(); i++)
    {
        if (g_houses[i].porchLight)
        {
            float angle = g_houses[i].rotationY;
            VECTOR fwd = VGet(sinf(angle), 0.0f, cosf(angle));
            VECTOR right = VGet(cosf(angle), 0.0f, -sinf(angle));

            VECTOR lampPos = VAdd(g_houses[i].position, VScale(fwd, 4.2f));
            lampPos = VAdd(lampPos, VScale(right, 1.2f));
            lampPos.y = 2.4f;

            SetUseLighting(FALSE);

            // ランプ本体球体
            unsigned int bulbColor = GetColor(255, 230, 160);
            DrawSphere3D(lampPos, 0.16f, 8, bulbColor, bulbColor, TRUE);

            // やわらかな光彩グロー
            SetDrawBlendMode(DX_BLENDMODE_ADD, 90);
            unsigned int glowColor = GetColor(180, 140, 50);
            DrawSphere3D(lampPos, 0.38f, 8, glowColor, glowColor, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            SetUseLighting(TRUE);
        }
    }
}

// マップデバッグ描画（F6キー有効時）
void MapDrawDebug(int currentTargetId)
{
    // 静的ワールドコリジョン全枠のワイヤーフレーム描画
    WorldCollision::Instance().DrawDebug();

    // 各住宅・ポストの3Dスクリーンラベル描画
    for (size_t i = 0; i < g_houses.size(); i++)
    {
        const HouseData& h = g_houses[i];
        bool isTarget = (h.id == currentTargetId);

        unsigned int houseCol = isTarget ? GetColor(255, 120, 50) : GetColor(70, 90, 80);

        // 3Dスクリーン投影テキスト（住宅番号）
        VECTOR labelWorldPos = VGet(h.position.x, 4.8f, h.position.z);
        VECTOR screenPos = ConvWorldPosToScreenPos(labelWorldPos);

        if (screenPos.z >= 0.0f && screenPos.z <= 1.0f)
        {
            char str[64];
            snprintf(str, sizeof(str), "HOUSE %d%s", h.id, isTarget ? " [TARGET]" : "");
            DrawString((int)screenPos.x - 30, (int)screenPos.y, str, houseCol);
        }

        // ポスト位置マーカー
        unsigned int mbCol = isTarget ? GetColor(255, 220, 80) : GetColor(140, 160, 150);
        VECTOR mbScreen = ConvWorldPosToScreenPos(VGet(h.mailboxPosition.x, 1.8f, h.mailboxPosition.z));
        if (mbScreen.z >= 0.0f && mbScreen.z <= 1.0f)
        {
            char str[32];
            snprintf(str, sizeof(str), "POST %d", h.id);
            DrawString((int)mbScreen.x - 20, (int)mbScreen.y, str, mbCol);
        }
    }
}

// 壁・住宅・障害物との当たり判定（静的ワールドコリジョン照会）
bool MapCheckWallCollision(VECTOR pos, float radius)
{
    return WorldCollision::Instance().CheckCollision(pos, radius);
}

// 床の高さ取得
float MapGetFloorY(VECTOR pos)
{
    (void)pos;
    return 0.0f; // フラットアスファルト路面
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
    // 配置された各住宅モデルの解放
    for (int i = 0; i < NEIGHBORHOOD_HOUSE_COUNT; i++)
    {
        if (g_houseHandles[i] != -1)
        {
            MV1DeleteModel(g_houseHandles[i]);
            g_houseHandles[i] = -1;
        }

        if (g_houseLightHandles[i] != -1)
        {
            DeleteLightHandle(g_houseLightHandles[i]);
            g_houseLightHandles[i] = -1;
        }
    }

    // 基本モデル4種の解放
    for (int t = 0; t < 4; t++)
    {
        if (g_houseBaseModels[t] != -1)
        {
            MV1DeleteModel(g_houseBaseModels[t]);
            g_houseBaseModels[t] = -1;
        }
    }

    // 住宅テクスチャの解放
    if (g_houseTexture != -1)
    {
        DeleteGraph(g_houseTexture);
        g_houseTexture = -1;
    }

    g_houses.clear();
    WorldCollision::Instance().Clear();
}