#include "Minimap.h"
#include "HorrorUI.h"
#include "Player.h"
#include "Bicycle.h"
#include "DeliveryManager.h"
#include "Newspaper.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

// コンストラクタ
Minimap::Minimap()
{
    panelX1 = 1045;
    panelY1 = 20;
    panelX2 = 1255;
    panelY2 = 215;

    displayCenterX = 1150;
    displayCenterY = 92;
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

    // ==========================================
    // 1. GPS端末パネル外枠・背景
    // ==========================================
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

    // パネル背景（ほぼ黒・暗灰緑）と枠線
    ui.DrawRetroPanel(px1, py1, px2, py2, HorrorUI::COL_BORDER, HorrorUI::COL_BG, 185);

    // 下部情報バーとの区切り線
    int dividerY = py2 - 42;
    DrawLine(px1 + 4, dividerY, px2 - 4, dividerY, HorrorUI::COL_BORDER_DIM);

    // 非アクティブ（停電・故障）時の画面表示
    if (!isEnabled)
    {
        ui.DrawJitterString(px1 + 65, py1 + 60, "[ NO SIGNAL ]", HorrorUI::COL_TEXT_DIM, ui.GetFontSmall());
        return;
    }

    // ==========================================
    // 2. 街並み道路グリッドライン（控えめな暗灰色）
    // ==========================================
    VECTOR playerPos = player.GetPosition();
    VECTOR playerFwd = player.GetForward();

    // プレイヤーの相対位置に応じた簡易道路ガイド線
    int cx = displayCenterX + jitterX;
    int cy = displayCenterY + jitterY;

    // 画面クリッピング用矩形範囲
    int clipMinX = cx - viewHalfWidth;
    int clipMaxX = cx + viewHalfWidth;
    int clipMinY = cy - viewHalfHeight;
    int clipMaxY = cy + viewHalfHeight;

    // 十字路・道路の基準線（プレイヤーを中心とした相対移動）
    float roadOffsetX = -playerPos.x * mapScale;
    float roadOffsetZ = playerPos.z * mapScale;

    // 水平道路（東-西方向）
    int roadY1 = cy + (int)roadOffsetZ;
    if (roadY1 >= clipMinY && roadY1 <= clipMaxY)
    {
        DrawLine(clipMinX, roadY1, clipMaxX, roadY1, HorrorUI::COL_BORDER_DIM);
    }
    // 垂直道路（北-南方向）
    int roadX1 = cx + (int)roadOffsetX;
    if (roadX1 >= clipMinX && roadX1 <= clipMaxX)
    {
        DrawLine(roadX1, clipMinY, roadX1, clipMaxY, HorrorUI::COL_BORDER_DIM);
    }

    // レトロ端末風の微細な走査線（スキャンライン）演出
    if (ui.IsHorrorEffectsEnabled())
    {
        int scanY = clipMinY + (animFrame * 2) % (clipMaxY - clipMinY);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 25);
        DrawLine(clipMinX, scanY, clipMaxX, scanY, HorrorUI::COL_BORDER);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // ==========================================
    // 3. 自転車位置アイコン（徒歩時のみ表示）
    // ==========================================
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
            // 小さな淡い青緑の矩形
            DrawBox(bikeMapX - 2, bikeMapY - 2, bikeMapX + 2, bikeMapY + 2, HorrorUI::COL_BIKE, TRUE);
            ui.DrawJitterString(bikeMapX + 4, bikeMapY - 6, "B", HorrorUI::COL_BIKE, ui.GetFontSmall());
        }
    }

    // ==========================================
    // 4. 配達先目標マーカー [X] および残距離
    // ==========================================
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

        // 目標マーカー：控えめな暗赤色 / 濁ったオレンジ色（Chilla's Artトーン）
        // わずかな明滅（派手なネオンではなく、低照度ランプ風）
        int blinkAlpha = 180 + (int)(sinf(animFrame * 0.12f) * 45.0f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha);

        // [X] 印の描画（小サイズ）
        DrawLine(finalTargetX - 3, finalTargetY - 3, finalTargetX + 3, finalTargetY + 3, HorrorUI::COL_TARGET);
        DrawLine(finalTargetX - 3, finalTargetY + 3, finalTargetX + 3, finalTargetY - 3, HorrorUI::COL_TARGET);

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // クランプされていない場合は目的地番号を小さく添える
        if (!isClamped)
        {
            char idStr[8];
            snprintf(idStr, sizeof(idStr), "#%d", deliveryManager.GetCurrentTargetId());
            ui.DrawJitterString(finalTargetX + 5, finalTargetY - 7, idStr, HorrorUI::COL_TARGET, ui.GetFontSmall());
        }
    }

    // ==========================================
    // 5. プレイヤー現在地アイコン（▲ キャレット）
    // ==========================================
    // プレイヤーの視線ベクトル（正規化）
    float fwdLen = sqrtf(playerFwd.x * playerFwd.x + playerFwd.z * playerFwd.z);
    float normFwdX = 0.0f;
    float normFwdZ = 1.0f;
    if (fwdLen > 0.001f)
    {
        normFwdX = playerFwd.x / fwdLen;
        normFwdZ = playerFwd.z / fwdLen;
    }

    // ▲ の頂点座標計算（中央 cx, cy を起点に視線方向へ向く）
    int tipX   = cx + (int)(normFwdX * 6.0f);
    int tipY   = cy - (int)(normFwdZ * 6.0f);
    int leftX  = cx - (int)(normFwdZ * 4.0f + normFwdX * 4.0f);
    int leftY  = cy - (int)(normFwdX * 4.0f - normFwdZ * 4.0f);
    int rightX = cx + (int)(normFwdZ * 4.0f - normFwdX * 4.0f);
    int rightY = cy + (int)(normFwdX * 4.0f + normFwdZ * 4.0f);

    DrawTriangle(tipX, tipY, leftX, leftY, rightX, rightY, HorrorUI::COL_PLAYER, TRUE);

    // ==========================================
    // 6. 配達情報ステータス（下部バー）
    // ==========================================
    int textY1 = dividerY + 5;
    int textY2 = dividerY + 22;

    // TARGET  xxx m
    char targetStr[32];
    if (deliveryManager.HasActiveTarget())
    {
        snprintf(targetStr, sizeof(targetStr), "TARGET    %3dm", targetDistanceMeters);
    }
    else
    {
        snprintf(targetStr, sizeof(targetStr), "TARGET    ---");
    }
    ui.DrawJitterString(px1 + 10, textY1, targetStr, HorrorUI::COL_TEXT, ui.GetFontSmall());

    // DELIVERY 03/08
    char deliveryStr[32];
    if (!deliveryManager.IsAllDeliveriesComplete())
    {
        snprintf(
            deliveryStr,
            sizeof(deliveryStr),
            "DELIVERY  %02d/%02d",
            deliveryManager.GetCompletedDeliveries(),
            deliveryManager.GetTotalDeliveries()
        );
    }
    else
    {
        snprintf(deliveryStr, sizeof(deliveryStr), "DELIVERY  COMPLETE");
    }
    ui.DrawJitterString(px1 + 10, textY2, deliveryStr, HorrorUI::COL_TEXT_DIM, ui.GetFontSmall());

    // ==========================================
    // 7. 各種センターメッセージ演出（フェード付き）
    // ==========================================

    // 配達成功: DELIVERED
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
        int msgX = 580;
        int msgY = 320;
        int a = (int)(feedbackAlpha * 220.0f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
        DrawBox(msgX - 16, msgY - 6, msgX + 130, msgY + 28, HorrorUI::COL_BG, TRUE);
        DrawBox(msgX - 16, msgY - 6, msgX + 130, msgY + 28, HorrorUI::COL_BORDER, FALSE);
        ui.DrawJitterString(msgX, msgY, "DELIVERED", HorrorUI::COL_TEXT, ui.GetFontMedium());
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 誤配先: WRONG ADDRESS
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
        int msgX = 560;
        int msgY = 320;
        int a = (int)(wrongAlpha * 220.0f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
        DrawBox(msgX - 16, msgY - 6, msgX + 170, msgY + 28, HorrorUI::COL_BG, TRUE);
        DrawBox(msgX - 16, msgY - 6, msgX + 170, msgY + 28, HorrorUI::COL_WARNING, FALSE);
        ui.DrawJitterString(msgX, msgY, "WRONG ADDRESS", HorrorUI::COL_WARNING, ui.GetFontMedium());
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 全配達完了: DELIVERIES COMPLETE 08 / 08
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
        int msgX = 510;
        int msgY = 260;
        int a = (int)(allCompleteAlpha * 230.0f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
        DrawBox(msgX - 25, msgY - 10, msgX + 280, msgY + 60, HorrorUI::COL_BG, TRUE);
        DrawBox(msgX - 25, msgY - 10, msgX + 280, msgY + 60, HorrorUI::COL_BORDER, FALSE);
        ui.DrawJitterString(msgX + 10, msgY + 2, "DELIVERIES COMPLETE", HorrorUI::COL_TEXT, ui.GetFontMedium());
        ui.DrawJitterString(msgX + 85, msgY + 30, "08 / 08", HorrorUI::COL_TEXT_DIM, ui.GetFontSmall());
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
            int msgX = 525;
            int msgY = 380;
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
            DrawBox(msgX - 12, msgY - 4, msgX + 235, msgY + 22, HorrorUI::COL_BG, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            ui.DrawJitterString(msgX, msgY, "NO NEWSPAPERS REMAINING", HorrorUI::COL_WARNING, ui.GetFontSmall());
        }
    }
}
