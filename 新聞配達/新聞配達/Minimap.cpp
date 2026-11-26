#include "Minimap.h"
#include "HorrorUI.h"
#include "Player.h"
#include "Bicycle.h"
#include "DeliveryManager.h"
#include "Newspaper.h"
#include "Map.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

// コンストラクタ
Minimap::Minimap()
{
    panelX1 = 26;
    panelY1 = 74;
    panelX2 = 236;
    panelY2 = 269;

    displayCenterX = 131;
    displayCenterY = 146;
    viewHalfWidth = 98;
    viewHalfHeight = 65;
    mapScale = 4.2f;

    animFrame = 0;
    glitchTimer = 0;
    interference = 0.0f;
    isEnabled = true;

    feedbackAlpha = 0.0f;
    wrongAlpha = 0.0f;
    allCompleteAlpha = 0.0f;
}

// デストラクタ
Minimap::~Minimap()
{
}

// 初期化
void Minimap::Initialize()
{
    animFrame = 0;
    glitchTimer = 0;
    interference = 0.0f;
    isEnabled = true;

    feedbackAlpha = 0.0f;
    wrongAlpha = 0.0f;
    allCompleteAlpha = 0.0f;
}

// 毎フレーム更新
void Minimap::Update()
{
    animFrame++;

    if (glitchTimer > 0)
    {
        glitchTimer--;
    }

    // 配達成功フェード制御
    // （外部のDeliveryManagerの状態に呼応）
    // 状態判定はDraw内で行うか、Updateで滑らかに補間
}

// 電波障害度設定
void Minimap::SetMinimapInterference(float amount)
{
    if (amount < 0.0f) amount = 0.0f;
    if (amount > 1.0f) amount = 1.0f;
    interference = amount;
}

float Minimap::GetMinimapInterference() const
{
    return interference;
}

// グリッチトリガー
void Minimap::TriggerMinimapGlitch(int durationFrames)
{
    glitchTimer = durationFrames;
}

// 有効/無効設定
void Minimap::SetMinimapEnabled(bool enabled)
{
    isEnabled = enabled;
}

bool Minimap::IsMinimapEnabled() const
{
    return isEnabled;
}

// ミニマップおよび配達進行情報の描画
void Minimap::Draw(
    const Player& player,
    const Bicycle& bicycle,
    const DeliveryManager& deliveryManager,
    const Newspaper& newspaper
)
{
    HorrorUI& ui = HorrorUI::Instance();

    // 地図端末の外枠と背景
    int jitterX = 0;
    int jitterY = 0;

    // グリッチ・干渉発生時の微小なブレ
    if (ui.IsHorrorEffectsEnabled())
    {
        if (glitchTimer > 0 || (interference > 0.4f && (animFrame % 30 == 0)))
        {
            jitterX = (rand() % 3) - 1;
            jitterY = (rand() % 3) - 1;
        }
    }

    int px1 = panelX1 + jitterX;
    int py1 = panelY1 + jitterY;
    int px2 = panelX2 + jitterX;
    int py2 = panelY2 + jitterY;

    // パネル背景（ほぼ黒・暗灰緑）とヴィンテージCRT枠線
    ui.DrawRetroPanel(px1, py1, px2, py2, HorrorUI::COL_BORDER, HorrorUI::COL_BG, 195);

    // 地図内の暗室背景を一段暗く落とし、道路・建物を際立たせる
    int clipMinX = displayCenterX + jitterX - viewHalfWidth;
    int clipMaxX = displayCenterX + jitterX + viewHalfWidth;
    int clipMinY = displayCenterY + jitterY - viewHalfHeight;
    int clipMaxY = displayCenterY + jitterY + viewHalfHeight;
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 175);
    DrawBox(clipMinX, clipMinY, clipMaxX, clipMaxY, GetColor(5, 8, 7), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 下部情報バーとの区切り線
    int dividerY = py2 - 46;
    DrawLine(px1 + 5, dividerY, px2 - 5, dividerY, HorrorUI::COL_BORDER);

    // 非アクティブ（停電・故障）時の画面表示
    if (!isEnabled)
    {
        int monoF = (ui.GetFontMono() != -1) ? ui.GetFontMono() : ui.GetFontSmall();
        ui.DrawJitterString(px1 + 52, py1 + 60, "[ NO SIGNAL ]", HorrorUI::COL_WARNING, monoF);
        return;
    }

    // 街並み道路グリッドライン（控えめな暗灰色）
    VECTOR playerPos = player.GetPosition();
    VECTOR playerFwd = player.GetForward();

    // プレイヤーの相対位置に応じた簡易道路ガイド線
    int cx = displayCenterX + jitterX;
    int cy = displayCenterY + jitterY;

    // 画面クリッピング領域設定（地図枠内に描画を限定）
    SetDrawArea(clipMinX, clipMinY, clipMaxX, clipMaxY);

    // 極薄の夜間ソナー・レーダー同心円＆十字軸
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 34);
    DrawCircle(cx, cy, 28, GetColor(75, 115, 98), FALSE);
    DrawCircle(cx, cy, 56, GetColor(75, 115, 98), FALSE);
    DrawLine(clipMinX, cy, clipMaxX, cy, GetColor(52, 78, 68));
    DrawLine(cx, clipMinY, cx, clipMaxY, GetColor(52, 78, 68));
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 住宅街道路ネットワーク（レトロGPSナビ描画）
    auto WorldToMapX = [&](float wx) -> int {
        return cx + (int)((wx - playerPos.x) * mapScale);
    };
    auto WorldToMapY = [&](float wz) -> int {
        return cy - (int)((wz - playerPos.z) * mapScale);
    };

    auto DrawStreetLine = [&](float wx1, float wz1, float wx2, float wz2, unsigned int col) {
        int x1 = WorldToMapX(wx1);
        int y1 = WorldToMapY(wz1);
        int x2 = WorldToMapX(wx2);
        int y2 = WorldToMapY(wz2);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 105);
        DrawLine(x1, y1, x2, y2, GetColor(38, 58, 50), 7);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawLine(x1, y1, x2, y2, col, 2);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    };

    // 道路線（視認しやすい褪せた燐光スレート緑）
    unsigned int roadCol = GetColor(72, 108, 92);

    // 南大通り（Road F: Z = -30, X = -28 〜 +28）
    DrawStreetLine(-28.0f, -30.0f, 28.0f, -30.0f, roadCol);

    // 西通り（Road A: X = -25, Z = -30 〜 +18）
    DrawStreetLine(-25.0f, -30.0f, -25.0f, 18.0f, roadCol);

    // 東通り（Road D: X = +25, Z = -30 〜 +18）
    DrawStreetLine(25.0f, -30.0f, 25.0f, 18.0f, roadCol);

    // 中央横通り（Road E: Z = -10, X = -25 〜 +25）
    DrawStreetLine(-25.0f, -10.0f, 25.0f, -10.0f, roadCol);

    // 北横通り（Road B: Z = +15, X = -28 〜 +28）
    DrawStreetLine(-28.0f, 15.0f, 28.0f, 15.0f, roadCol);

    // 北路地行き止まり（Road C: X = 0, Z = +15 〜 +40）
    DrawStreetLine(0.0f, 15.0f, 0.0f, 40.0f, roadCol);

    // 住宅の建物フットプリント矩形描画（陰影とアウトラインで街並みを鮮明化）
    unsigned int houseFillCol = GetColor(26, 48, 40);
    unsigned int houseLineCol = GetColor(60, 92, 78);
    const std::vector<HouseData>& houses = MapGetHouses();
    for (size_t i = 0; i < houses.size(); ++i)
    {
        const HouseData& h = houses[i];
        int hx1 = WorldToMapX(h.position.x - 3.2f);
        int hx2 = WorldToMapX(h.position.x + 3.2f);
        int hy1 = WorldToMapY(h.position.z + 3.8f);
        int hy2 = WorldToMapY(h.position.z - 3.8f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 195);
        DrawBox(hx1, hy1, hx2, hy2, houseFillCol, TRUE);
        DrawBox(hx1, hy1, hx2, hy2, houseLineCol, FALSE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // レトロ端末風の微細な走査線（スキャンライン）演出
    if (ui.IsHorrorEffectsEnabled())
    {
        int scanY = clipMinY + (animFrame * 2) % (clipMaxY - clipMinY);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 42);
        DrawLine(clipMinX, scanY, clipMaxX, scanY, HorrorUI::COL_BORDER);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // クリッピング領域を全画面へ復元
    SetDrawArea(0, 0, 1280, 720);
    DrawBox(clipMinX, clipMinY, clipMaxX, clipMaxY, HorrorUI::COL_BORDER_DIM, FALSE);

    int monoF = (ui.GetFontMono() != -1) ? ui.GetFontMono() : ui.GetFontSmall();

    // 自転車位置アイコン（徒歩時のみ表示）
    if (bicycle.IsRiding() == false)
    {
        VECTOR bikePos = bicycle.GetPosition();
        float relBikeX = bikePos.x - playerPos.x;
        float relBikeZ = bikePos.z - playerPos.z;

        int bikeMapX = cx + (int)(relBikeX * mapScale);
        int bikeMapY = cy - (int)(relBikeZ * mapScale);

        if (bikeMapX >= clipMinX + 4 && bikeMapX <= clipMaxX - 4 &&
            bikeMapY >= clipMinY + 4 && bikeMapY <= clipMaxY - 4)
        {
            DrawBox(bikeMapX - 3, bikeMapY - 3, bikeMapX + 3, bikeMapY + 3, GetColor(4, 6, 6), TRUE);
            DrawBox(bikeMapX - 2, bikeMapY - 2, bikeMapX + 2, bikeMapY + 2, HorrorUI::COL_BIKE, TRUE);
            ui.DrawJitterString(bikeMapX + 5, bikeMapY - 7, "B", HorrorUI::COL_BIKE, monoF);
        }
    }

    // 配達先目標マーカー [X] および残距離
    int targetDistanceMeters = 0;

    if (deliveryManager.HasActiveTarget())
    {
        VECTOR targetPos = deliveryManager.GetCurrentTargetPosition();
        float relTargetX = targetPos.x - playerPos.x;
        float relTargetZ = targetPos.z - playerPos.z;

        float distance = sqrtf(relTargetX * relTargetX + relTargetZ * relTargetZ);
        targetDistanceMeters = (int)distance;

        float rawTargetX = cx + relTargetX * mapScale;
        float rawTargetY = cy - relTargetZ * mapScale;

        // 地図表示矩形内にクランプ（画面外でも方向が分かるように外周に配置）
        bool isClamped = false;
        int finalTargetX = (int)rawTargetX;
        int finalTargetY = (int)rawTargetY;

        if (finalTargetX < clipMinX + 6) { finalTargetX = clipMinX + 6; isClamped = true; }
        if (finalTargetX > clipMaxX - 6) { finalTargetX = clipMaxX - 6; isClamped = true; }
        if (finalTargetY < clipMinY + 6) { finalTargetY = clipMinY + 6; isClamped = true; }
        if (finalTargetY > clipMaxY - 6) { finalTargetY = clipMaxY - 6; isClamped = true; }

        // 目標マーカー：微弱に脈動するアンバー照射ハロー＋[X]印
        int blinkAlpha = 185 + (int)(sinf(animFrame * 0.12f) * 50.0f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha / 3);
        DrawCircle(finalTargetX, finalTargetY, 6, HorrorUI::COL_TARGET, FALSE);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha);

        // [X] 印の描画
        DrawLine(finalTargetX - 3, finalTargetY - 3, finalTargetX + 3, finalTargetY + 3, HorrorUI::COL_TARGET);
        DrawLine(finalTargetX - 3, finalTargetY + 3, finalTargetX + 3, finalTargetY - 3, HorrorUI::COL_TARGET);

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // クランプされていない場合は目的地番号を小さく添える
        if (!isClamped)
        {
            char idStr[8];
            snprintf(idStr, sizeof(idStr), "#%d", deliveryManager.GetCurrentTargetId());
            ui.DrawJitterString(finalTargetX + 6, finalTargetY - 7, idStr, HorrorUI::COL_TARGET, monoF);
        }
    }

    // プレイヤー現在地アイコン（▲ キャレット）
    float fwdLen = sqrtf(playerFwd.x * playerFwd.x + playerFwd.z * playerFwd.z);
    float normFwdX = 0.0f;
    float normFwdZ = 1.0f;
    if (fwdLen > 0.001f)
    {
        normFwdX = playerFwd.x / fwdLen;
        normFwdZ = playerFwd.z / fwdLen;
    }

    int tipX   = cx + (int)(normFwdX * 6.0f);
    int tipY   = cy - (int)(normFwdZ * 6.0f);
    int leftX  = cx - (int)(normFwdZ * 4.0f + normFwdX * 4.0f);
    int leftY  = cy - (int)(normFwdX * 4.0f - normFwdZ * 4.0f);
    int rightX = cx + (int)(normFwdZ * 4.0f - normFwdX * 4.0f);
    int rightY = cy + (int)(normFwdX * 4.0f + normFwdZ * 4.0f);

    DrawTriangle(tipX + 1, tipY + 1, leftX + 1, leftY + 1, rightX + 1, rightY + 1, GetColor(3, 5, 5), TRUE);
    DrawTriangle(tipX, tipY, leftX, leftY, rightX, rightY, HorrorUI::COL_PLAYER, TRUE);

    // 配達情報ステータス（下部バー：十分な行間と左右分離配置で重なりを完全防止）
    int textY1 = dividerY + 5;
    int textY2 = dividerY + 24;

    // 配達先距離（左側: 目的地、右側: 距離数値で分離）
    ui.DrawJitterString(px1 + 10, textY1, "目的地", HorrorUI::COL_TEXT, ui.GetFontSmall());
    char distValStr[32];
    if (deliveryManager.HasActiveTarget())
    {
        snprintf(distValStr, sizeof(distValStr), "%dm", targetDistanceMeters);
    }
    else
    {
        snprintf(distValStr, sizeof(distValStr), "---");
    }
    int distW = (monoF != -1) ? GetDrawStringWidthToHandle(distValStr, (int)strlen(distValStr), monoF) : 36;
    ui.DrawJitterString(px2 - distW - 10, textY1, distValStr, HorrorUI::COL_TARGET, monoF);

    // 配達進行状況（左側: 配達、右側: 完了数/必要数で分離）
    ui.DrawJitterString(px1 + 10, textY2, "配達", HorrorUI::COL_TEXT_DIM, ui.GetFontSmall());
    char progValStr[32];
    int progFont = monoF;
    if (!deliveryManager.IsAllDeliveriesComplete())
    {
        snprintf(
            progValStr,
            sizeof(progValStr),
            "%02d/%02d (D%d)",
            deliveryManager.GetCompletedDeliveries(),
            deliveryManager.GetTotalDeliveries(),
            deliveryManager.GetCurrentDay()
        );
    }
    else
    {
        snprintf(progValStr, sizeof(progValStr), "完了");
        progFont = ui.GetFontSmall();
    }
    int progW = (progFont != -1) ? GetDrawStringWidthToHandle(progValStr, (int)strlen(progValStr), progFont) : 65;
    ui.DrawJitterString(px2 - progW - 10, textY2, progValStr, HorrorUI::COL_TEXT, progFont);

    // 各種センターメッセージ演出（文字幅に応じた動的センタリング・レトロ端末枠フィッティング）
    int screenCenterX = 640;

    // 配達成功: 配達完了
    if (deliveryManager.IsFeedbackActive())
    {
        feedbackAlpha += 0.15f;
        if (feedbackAlpha > 1.0f) feedbackAlpha = 1.0f;
    }
    else
    {
        feedbackAlpha -= 0.12f;
        if (feedbackAlpha < 0.0f) feedbackAlpha = 0.0f;
    }

    if (feedbackAlpha > 0.01f)
    {
        int msgY = 320;
        int a = (int)(feedbackAlpha * 230.0f);
        const char* msgStr = "配達完了";
        int tw = (ui.GetFontMedium() != -1) ? GetDrawStringWidthToHandle(msgStr, (int)strlen(msgStr), ui.GetFontMedium()) : 120;
        int bw = tw + 52;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
        ui.DrawRetroPanel(screenCenterX - bw / 2, msgY - 7, screenCenterX + bw / 2, msgY + 29, GetColor(105, 165, 130), HorrorUI::COL_BG, 235);
        ui.DrawJitterString(screenCenterX - tw / 2, msgY, msgStr, GetColor(165, 225, 185), ui.GetFontMedium());
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 誤配先: 配達先が違います
    if (deliveryManager.IsWrongDeliveryActive())
    {
        wrongAlpha += 0.15f;
        if (wrongAlpha > 1.0f) wrongAlpha = 1.0f;
    }
    else
    {
        wrongAlpha -= 0.12f;
        if (wrongAlpha < 0.0f) wrongAlpha = 0.0f;
    }

    if (wrongAlpha > 0.01f)
    {
        int msgY = 320;
        int a = (int)(wrongAlpha * 230.0f);
        const char* msgStr = "配達先が違います";
        int tw = (ui.GetFontMedium() != -1) ? GetDrawStringWidthToHandle(msgStr, (int)strlen(msgStr), ui.GetFontMedium()) : 160;
        int bw = tw + 52;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
        ui.DrawRetroPanel(screenCenterX - bw / 2, msgY - 7, screenCenterX + bw / 2, msgY + 29, HorrorUI::COL_WARNING, GetColor(24, 10, 12), 235);
        ui.DrawJitterString(screenCenterX - tw / 2, msgY, msgStr, HorrorUI::COL_WARNING, ui.GetFontMedium());
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 全配達完了: 本日の配達完了
    if (deliveryManager.IsAllDeliveriesComplete() && deliveryManager.GetAllCompleteTimer() > 0)
    {
        allCompleteAlpha += 0.08f;
        if (allCompleteAlpha > 1.0f) allCompleteAlpha = 1.0f;
    }
    else
    {
        allCompleteAlpha -= 0.05f;
        if (allCompleteAlpha < 0.0f) allCompleteAlpha = 0.0f;
    }

    if (allCompleteAlpha > 0.01f)
    {
        int msgY = 260;
        int a = (int)(allCompleteAlpha * 235.0f);
        const char* titleMsg = "本日の配達完了";
        char subMsg[64];
        snprintf(subMsg, sizeof(subMsg), "%02d / %02d 軒",
            deliveryManager.GetCompletedDeliveries(), deliveryManager.GetTotalDeliveries());

        int titleF = (ui.GetFontHorrorSerif() != -1) ? ui.GetFontHorrorSerif() : ui.GetFontMedium();
        int tw1 = (titleF != -1) ? GetDrawStringWidthToHandle(titleMsg, (int)strlen(titleMsg), titleF) : 200;
        int tw2 = (ui.GetFontSmall() != -1) ? GetDrawStringWidthToHandle(subMsg, (int)strlen(subMsg), ui.GetFontSmall()) : 120;
        int bw = (tw1 > tw2 ? tw1 : tw2) + 64;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
        ui.DrawRetroPanel(screenCenterX - bw / 2, msgY - 12, screenCenterX + bw / 2, msgY + 64, GetColor(115, 165, 138), HorrorUI::COL_BG, 238);
        ui.DrawJitterString(screenCenterX - tw1 / 2, msgY, titleMsg, GetColor(220, 232, 222), titleF);
        ui.DrawJitterString(screenCenterX - tw2 / 2, msgY + 34, subMsg, HorrorUI::COL_TARGET, ui.GetFontSmall());
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 新聞切れ安全警告（残数0かつ手持ちなしで未完了の場合）
    if (newspaper.GetNewspaperCount() == 0 &&
        !newspaper.IsHolding() &&
        !deliveryManager.IsAllDeliveriesComplete())
    {
        bool blink = ((animFrame / 30) % 2) == 0;
        if (blink)
        {
            int msgY = 380;
            const char* warnStr = "新聞が残っていません";
            int tw = (ui.GetFontSmall() != -1) ? GetDrawStringWidthToHandle(warnStr, (int)strlen(warnStr), ui.GetFontSmall()) : 200;
            int bw = tw + 44;

            ui.DrawRetroPanel(screenCenterX - bw / 2, msgY - 5, screenCenterX + bw / 2, msgY + 23, HorrorUI::COL_WARNING, GetColor(22, 10, 12), 205);
            ui.DrawJitterString(screenCenterX - tw / 2, msgY, warnStr, HorrorUI::COL_WARNING, ui.GetFontSmall());
        }
    }
}
