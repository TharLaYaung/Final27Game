#pragma once
#include "DxLib.h"

// マップ初期化
void MapInit();

// マップ描画
void MapDraw();

// 壁との当たり判定
bool MapCheckWallCollision(VECTOR pos, float radius);

// 床の高さ取得
float MapGetFloorY(VECTOR pos);