#pragma once
#include "DxLib.h"

void MapInit();
void MapDraw();

// 壁の当たり判定チェック（posを中心とするカプセルが家と当たるか）
bool MapCheckWallCollision(VECTOR pos, float radius);

// 床・階段のY座標を取得（何も当たらなければ -9999.0f を返す）
float MapGetFloorY(VECTOR pos);
