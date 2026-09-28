#include "Map.h"
#include "DxLib.h"

const int HOUSE_NUM = 2;
const int HOUSE_TYPES = 4;

int HouseModel[HOUSE_TYPES];
int House[HOUSE_NUM];

void MapInit()
{
    HouseModel[0] = MV1LoadModel("Data/Model/house/house1.mv1");
    HouseModel[1] = MV1LoadModel("Data/Model/house/house2.mv1");
    HouseModel[2] = MV1LoadModel("Data/Model/house/house3.mv1");
    HouseModel[3] = MV1LoadModel("Data/Model/house/house4.mv1");

    float scale = 0.015f;

    int index = 0;

    for (int z = 0; z < 10; z++)
    {
        // ¶‘¤
        if (index < HOUSE_NUM)
        {
            int type = GetRand(HOUSE_TYPES - 1);

            if (HouseModel[type] != -1)
            {
                House[index] = MV1DuplicateModel(HouseModel[type]);

                MV1SetScale(House[index], VGet(scale, scale, scale));
                MV1SetPosition(House[index], VGet(-35.0f, 0.0f, (float)z * 50.0f));
                MV1SetRotationXYZ(House[index],
                    VGet(0.0f, -DX_PI_F / 2.0f, 0.0f));

                MV1SetupCollInfo(House[index], -1, 8, 8, 8);

                index++;
            }
        }

        // ‰E‘¤
        if (index < HOUSE_NUM)
        {
            int type = GetRand(HOUSE_TYPES - 1);

            if (HouseModel[type] != -1)
            {
                House[index] = MV1DuplicateModel(HouseModel[type]);

                MV1SetScale(House[index], VGet(scale, scale, scale));
                MV1SetPosition(House[index], VGet(35.0f, 0.0f, (float)z * 50.0f));
                MV1SetRotationXYZ(House[index],
                    VGet(0.0f, DX_PI_F / 2.0f, 0.0f));

                MV1SetupCollInfo(House[index], -1, 8, 8, 8);

                index++;
            }
        }
    }

    for (int i = index; i < HOUSE_NUM; i++)
    {
        House[i] = -1;
    }
}

void MapDraw()
{
    for (int i = 0; i < HOUSE_NUM; i++)
    {
        if (House[i] != -1)
        {
            MV1DrawModel(House[i]);
        }
    }
}

bool MapCheckWallCollision(VECTOR pos, float radius)
{
    VECTOR top = VGet(pos.x, pos.y, pos.z);
    VECTOR bottom = VGet(pos.x, pos.y - 1.5f, pos.z);

    for (int i = 0; i < HOUSE_NUM; i++)
    {
        if (House[i] == -1) continue;

        MV1_COLL_RESULT_POLY_DIM result =
            MV1CollCheck_Capsule(
                House[i],
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

float MapGetFloorY(VECTOR pos)
{
    VECTOR start =
        VGet(pos.x, pos.y + 0.5f, pos.z);

    VECTOR end =
        VGet(pos.x, pos.y - 3.0f, pos.z);

    float bestY = -9999.0f;

    for (int i = 0; i < HOUSE_NUM; i++)
    {
        if (House[i] == -1) continue;

        MV1_COLL_RESULT_POLY result =
            MV1CollCheck_Line(
                House[i],
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