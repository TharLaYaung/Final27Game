#include "Map.h"
#include "DxLib.h"

const int HOUSE_NUM = 1;
const int HOUSE_TYPES = 4;

int HouseModel[HOUSE_TYPES];
int House[HOUSE_NUM];

// テクスチャを外してマテリアルに直接色を設定
void SetHouseColor(int handle, float r, float g, float b)
{
    int matNum = MV1GetMaterialNum(handle);
    for (int m = 0; m < matNum; m++)
    {
        MV1SetMaterialDifMapTexture(handle, m, -1);
        MV1SetMaterialDifColor(handle, m, GetColorF(r, g, b, 1.0f));
        MV1SetMaterialAmbColor(handle, m, GetColorF(r * 0.3f, g * 0.3f, b * 0.3f, 1.0f));
    }
}

void MapInit()
{
    HouseModel[0] = MV1LoadModel("Data/house/house1.mv1");
    HouseModel[1] = MV1LoadModel("Data/house/house2.mv1");
    HouseModel[2] = MV1LoadModel("Data/house/house3.mv1");
    HouseModel[3] = MV1LoadModel("Data/house/house4.mv1");

    float scale = 0.015f;

    // ホラーゲーム向け：暗くくすんだ色合い10種類
    float houseColors[][3] = {
        {0.28f, 0.22f, 0.20f},  // 焦げ茶
        {0.20f, 0.24f, 0.30f},  // 暗い青灰
        {0.35f, 0.22f, 0.18f},  // 錆び赤
        {0.18f, 0.22f, 0.18f},  // 暗いオリーブ
        {0.30f, 0.20f, 0.20f},  // くすんだ赤
        {0.20f, 0.20f, 0.28f},  // 暗い青紫
        {0.35f, 0.30f, 0.18f},  // 古びた黄土
        {0.15f, 0.20f, 0.15f},  // 暗い緑
        {0.28f, 0.20f, 0.28f},  // 暗い紫
        {0.22f, 0.18f, 0.14f},  // 汚れた茶
    };
    int colorCount = 10;

    int index = 0;

    for (int z = 0; z < 10; z++)
    {
        // 左側の家
        if (index < HOUSE_NUM) {
            int type = GetRand(HOUSE_TYPES - 1);
            if (HouseModel[type] != -1) {
                House[index] = MV1DuplicateModel(HouseModel[type]);
                MV1SetScale(House[index], VGet(scale, scale, scale));
                MV1SetPosition(House[index], VGet(-35.0f, 0.0f, (float)z * 50.0f));
                MV1SetRotationXYZ(House[index], VGet(0.0f, -DX_PI_F / 2.0f, 0.0f));

                // ★位置・回転・スケール設定後に当たり判定を初期化
                MV1SetupCollInfo(House[index], -1, 8, 8, 8);

                int col = index % colorCount;
                SetHouseColor(House[index], houseColors[col][0], houseColors[col][1], houseColors[col][2]);
                index++;
            }
        }

        // 右側の家
        if (index < HOUSE_NUM) {
            int type = GetRand(HOUSE_TYPES - 1);
            if (HouseModel[type] != -1) {
                House[index] = MV1DuplicateModel(HouseModel[type]);
                MV1SetScale(House[index], VGet(scale, scale, scale));
                MV1SetPosition(House[index], VGet(35.0f, 0.0f, (float)z * 50.0f));
                MV1SetRotationXYZ(House[index], VGet(0.0f, DX_PI_F / 2.0f, 0.0f));

                MV1SetupCollInfo(House[index], -1, 8, 8, 8);

                int col = (index + 5) % colorCount;
                SetHouseColor(House[index], houseColors[col][0], houseColors[col][1], houseColors[col][2]);
                index++;
            }
        }
    }

    for (int i = index; i < HOUSE_NUM; i++) {
        House[i] = -1;
    }
}

void MapDraw()
{
    for (int i = 0; i < HOUSE_NUM; i++)
    {
        if (House[i] != -1) {
            MV1DrawModel(House[i]);
        }
    }
}

// ============================================================
// 壁の当たり判定
//   pos     : プレイヤーの現在位置（目の高さ）
//   radius  : 当たり判定の半径
//   戻り値   : 当たっていれば true
// ============================================================
bool MapCheckWallCollision(VECTOR pos, float radius)
{
    // カプセルの上端（目）と下端（足）
    VECTOR top    = VGet(pos.x, pos.y,        pos.z);
    VECTOR bottom = VGet(pos.x, pos.y - 1.5f, pos.z);

    for (int i = 0; i < HOUSE_NUM; i++)
    {
        if (House[i] == -1) continue;

        MV1_COLL_RESULT_POLY_DIM result =
            MV1CollCheck_Capsule(House[i], -1, top, bottom, radius);

        bool hit = (result.HitNum > 0);
        MV1CollResultPolyDimTerminate(result);  // メモリ解放

        if (hit) return true;
    }
    return false;
}

// ============================================================
// 床・階段のY座標取得
//   pos    : プレイヤーの現在位置（目の高さ）
//   戻り値  : 当たった面のY座標。何も当たらなければ -9999.0f
// ============================================================
float MapGetFloorY(VECTOR pos)
{
    // 少し上からプレイヤーの足元より下に向けてレイを飛ばす
    VECTOR start = VGet(pos.x, pos.y + 0.5f, pos.z);
    VECTOR end   = VGet(pos.x, pos.y - 3.0f, pos.z);

    float bestY = -9999.0f;

    for (int i = 0; i < HOUSE_NUM; i++)
    {
        if (House[i] == -1) continue;

        MV1_COLL_RESULT_POLY result =
            MV1CollCheck_Line(House[i], -1, start, end);

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
