// Ground.hを読み込む
#include "Ground.h"

// 道路矩形を描画するヘルパー関数
static void DrawRoadQuad(float x1, float z1, float x2, float z2, unsigned int color, float y = 0.01f)
{
    VECTOR p1 = VGet(x1, y, z1);
    VECTOR p2 = VGet(x2, y, z1);
    VECTOR p3 = VGet(x1, y, z2);
    VECTOR p4 = VGet(x2, y, z2);

    DrawTriangle3D(p1, p2, p3, color, TRUE);
    DrawTriangle3D(p2, p4, p3, color, TRUE);
}

// 地面および住宅街道路を描画する関数
// 深夜3時の冷たいアスファルト路面と狭小住宅街の道路網を描画
void DrawGround()
{
    // 敷地・土台ベース地面色（深夜の極めて暗い土・草地色 RGB = 15, 18, 16）
    unsigned int groundBaseColor = GetColor(15, 18, 16);

    // フォグ最遠部（22m〜45m）を覆うベース地面
    const float GROUND_SIZE = 120.0f;
    VECTOR gp1 = VGet(-GROUND_SIZE, 0.0f, -GROUND_SIZE);
    VECTOR gp2 = VGet( GROUND_SIZE, 0.0f, -GROUND_SIZE);
    VECTOR gp3 = VGet(-GROUND_SIZE, 0.0f,  GROUND_SIZE);
    VECTOR gp4 = VGet( GROUND_SIZE, 0.0f,  GROUND_SIZE);

    DrawTriangle3D(gp1, gp2, gp3, groundBaseColor, TRUE);
    DrawTriangle3D(gp2, gp4, gp3, groundBaseColor, TRUE);

    // ==========================================
    // 深夜アスファルト道路面描画（幅約6.0m・暗灰色）
    // ==========================================
    unsigned int asphaltColor = GetColor(28, 32, 34);

    // 1. 南大通り・スタートエリア（Z = -30, X = -28 〜 +28）
    DrawRoadQuad(-28.0f, -33.0f, 28.0f, -27.0f, asphaltColor);

    // 2. 西通り・Road A（X = -25, Z = -30 〜 +18）
    DrawRoadQuad(-28.0f, -30.0f, -22.0f, 18.0f, asphaltColor);

    // 3. 東通り・Road D（X = +25, Z = -30 〜 +18）
    DrawRoadQuad(22.0f, -30.0f, 28.0f, 18.0f, asphaltColor);

    // 4. 中央横通り・Road E（Z = -10, X = -25 〜 +25）
    DrawRoadQuad(-25.0f, -13.0f, 25.0f, -7.0f, asphaltColor);

    // 5. 北横通り・Road B（Z = +15, X = -28 〜 +28）
    DrawRoadQuad(-28.0f, 12.0f, 28.0f, 18.0f, asphaltColor);

    // 6. 北路地行き止まり・Road C（X = 0, Z = +15 〜 +40）
    DrawRoadQuad(-3.0f, 15.0f, 3.0f, 40.0f, asphaltColor);

    // ==========================================
    // 道路白線・路肩境界線の描画（褪せた暗白緑色）
    // ==========================================
    unsigned int curbLineColor = GetColor(50, 60, 56);
    float lineY = 0.015f;

    // 西通りの路肩白線（幅0.15m）
    DrawRoadQuad(-27.8f, -29.8f, -27.65f, 17.8f, curbLineColor, lineY);
    DrawRoadQuad(-22.35f, -29.8f, -22.2f, 17.8f, curbLineColor, lineY);

    // 東通りの路肩白線
    DrawRoadQuad(22.2f, -29.8f, 22.35f, 17.8f, curbLineColor, lineY);
    DrawRoadQuad(27.65f, -29.8f, 27.8f, 17.8f, curbLineColor, lineY);

    // 南通りの路肩白線
    DrawRoadQuad(-27.8f, -32.8f, 27.8f, -32.65f, curbLineColor, lineY);
    DrawRoadQuad(-27.8f, -27.35f, 27.8f, -27.2f, curbLineColor, lineY);

    // 北通りの路肩白線
    DrawRoadQuad(-27.8f, 12.2f, 27.8f, 12.35f, curbLineColor, lineY);
    DrawRoadQuad(-27.8f, 17.65f, 27.8f, 17.8f, curbLineColor, lineY);

    // 北路地行き止まりの境界線
    DrawRoadQuad(-2.85f, 18.0f, -2.7f, 39.8f, curbLineColor, lineY);
    DrawRoadQuad( 2.7f,  18.0f,  2.85f, 39.8f, curbLineColor, lineY);
    DrawRoadQuad(-2.85f, 39.65f, 2.85f, 39.8f, curbLineColor, lineY);
}