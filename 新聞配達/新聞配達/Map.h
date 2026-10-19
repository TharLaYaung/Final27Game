#pragma once

#include "DxLib.h"
#include <vector>

// 住宅データ構造体
struct HouseData
{
    int id;                     // 住宅・配達先固有ID (0〜7)
    int modelType;              // モデル種別 (0: house1, 1: house2, 2: house3, 3: house4)
    VECTOR position;            // 住宅ワールド座標
    float rotationY;            // Y軸回転角度（ラジアン）
    VECTOR mailboxPosition;     // 対応するポストの座標
    float mailboxRotationY;     // ポストの向き
    bool porchLight;            // 玄関灯点灯フラグ
    float boundRadius;          // 簡易当たり判定半径
};

// 住宅数定数
const int NEIGHBORHOOD_HOUSE_COUNT = 8;

// マップ初期化（住宅モデル複製・詳細コリジョン構築・道路設定）
void MapInit();

// マップ描画（全8棟の住宅モデル・ブロック塀・玄関灯描画）
void MapDraw();

// マップデバッグ描画（F6キー有効時、全コリジョン枠・各住宅・ポストIDを描画）
void MapDrawDebug(int currentTargetId);

// マップ終了処理（モデル解放）
void MapFinalize();

// 壁・住宅・障害物との当たり判定（静的ワールドコリジョン照会）
bool MapCheckWallCollision(VECTOR pos, float radius);

// 床の高さ取得
float MapGetFloorY(VECTOR pos);

// 全住宅データの参照取得
const std::vector<HouseData>& MapGetHouses();

// 特定住宅データの取得（見つからない場合はnullptr）
const HouseData* MapGetHouseById(int id);

// 日別シードに基づく玄関灯の点灯・消灯制御（Day 1: ほぼ点灯 〜 Day 7: ほぼ全消灯）
void MapSetDayPorchLights(int day, unsigned int daySeed);

// スポーン地点・ポスト接近性の安全検証
bool MapValidateSafety();

// 街灯電柱コリジョンの登録
void MapRegisterLampPoles(const std::vector<VECTOR>& polePositions);