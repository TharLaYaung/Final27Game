#include "DxLib.h"
#include "Ground.h"
#include "Player.h"
#include "Interaction.h"
#include "Bicycle.h"
#include "Newspaper.h"
#include "Mailbox.h"
#include "DeliveryManager.h"
#include "Minimap.h"
#include "HorrorUI.h"

// Windowsアプリケーションのエントリポイント
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    // ウィンドウモードで起動
    ChangeWindowMode(TRUE);

    // 画面解像度 1280x720, 32bitカラー
    SetGraphMode(
        1280,
        720,
        32
    );

    // DxLib初期化
    if (DxLib_Init() == -1)
    {
        return -1;
    }

    // 描画先を裏画面に設定（ダブルバッファリング）
    SetDrawScreen(
        DX_SCREEN_BACK
    );

    // カメラのクリップ距離設定
    SetCameraNearFar(
        0.1f,
        1000.0f
    );

    // 各ゲームシステムのインスタンス生成
    Player player;
    Interaction interaction;
    Bicycle bicycle;
    Newspaper newspaper;
    Mailbox mailbox;
    DeliveryManager deliveryManager;
    Minimap minimap;

    // ホラーUI総合管理システムの初期化（フォント生成など）
    HorrorUI::Instance().Initialize();

    // 自転車モデル初期化
    if (bicycle.Initialize() == false)
    {
        MessageBox(
            NULL,
            "自転車モデルの読み込みに失敗しました。",
            "エラー",
            MB_OK
        );
    }

    // 新聞モデル初期化
    if (newspaper.Initialize() == false)
    {
        MessageBox(
            NULL,
            "新聞モデルの読み込みに失敗しました。",
            "エラー",
            MB_OK
        );
    }

    // 郵便ポストモデル初期化
    if (mailbox.Initialize() == false)
    {
        MessageBox(
            NULL,
            "メールボックスモデルの読み込みに失敗しました。",
            "エラー",
            MB_OK
        );
    }

    // 配達管理システム初期化
    deliveryManager.Initialize();

    // ミニマップ初期化
    minimap.Initialize();

    // 60FPS制御用
    LONGLONG prevTime = GetNowHiPerformanceCount();

    // デバッグ表示トグルフラグ
    bool showDebugHUD = false;
    bool oldF3Key = false;
    bool oldF4Key = false;
    bool oldF5Key = false;

    // メインゲームループ
    while (ProcessMessage() == 0)
    {
        // 60FPS（約16.6ms）ウェイト制御
        while (GetNowHiPerformanceCount() - prevTime < 16666)
        {
            Sleep(0);
        }
        prevTime = GetNowHiPerformanceCount();

        // ESCキーで終了
        if (CheckHitKey(KEY_INPUT_ESCAPE))
        {
            break;
        }

        // ==========================================
        // デバッグ操作キー入力判定
        // ==========================================
        // F3キー: 開発用デバッグ数値HUDの表示/非表示切り替え
        bool curF3 = (CheckHitKey(KEY_INPUT_F3) != 0);
        if (curF3 && !oldF3Key)
        {
            showDebugHUD = !showDebugHUD;
        }
        oldF3Key = curF3;

        // F4キー: ホラーUI効果（ノイズ・歪み・ビネット）のON/OFF比較
        bool curF4 = (CheckHitKey(KEY_INPUT_F4) != 0);
        if (curF4 && !oldF4Key)
        {
            HorrorUI::Instance().ToggleHorrorEffects();
        }
        oldF4Key = curF4;

        // F5キー: テスト用UIグリッチ手動発生
        bool curF5 = (CheckHitKey(KEY_INPUT_F5) != 0);
        if (curF5 && !oldF5Key)
        {
            HorrorUI::Instance().TriggerUIGlitch(20);
            minimap.TriggerMinimapGlitch(20);
        }
        oldF5Key = curF5;

        // 画面クリア
        ClearDrawScreen();

        // ==========================================
        // 各種状態更新処理
        // ==========================================

        // ホラーUI状態更新（時計・プロンプトフェードなど）
        HorrorUI::Instance().Update();

        // 自転車位置をプレイヤーに伝達
        player.SetBicyclePosition(
            bicycle.GetPosition()
        );

        // 乗車していない時のみ自転車との当たり判定を有効化
        player.SetBicycleCollisionEnabled(
            bicycle.IsRiding() == false
        );

        // 徒歩時と乗車時の操作分岐
        if (bicycle.IsRiding() == false)
        {
            // 徒歩：移動＋マウス視線更新
            player.Update();
        }
        else
        {
            // 乗車中：視線操作のみ
            player.UpdateLook();
        }

        // 自転車更新
        bicycle.Update(
            player
        );

        // 新聞更新（カゴ座標追従および手持ち描画管理）
        newspaper.Update(
            player,
            bicycle.GetPosition(),
            bicycle.GetAngle(),
            bicycle.IsRiding()
        );

        // 配達進行管理の更新
        deliveryManager.Update();

        // ポスト注視・配達判定更新
        mailbox.Update(
            player,
            newspaper,
            deliveryManager
        );

        // カメラ更新
        player.UpdateCamera();

        // 通常インタラクト更新
        interaction.Update(
            player
        );

        // ミニマップ更新
        minimap.Update();

        // ホラー進行判定用フック（全配達完了時）
        if (deliveryManager.IsAllDeliveriesComplete())
        {
            // Later:
            // StartFinalDeliveryEvent();
        }

        // ==========================================
        // 3Dモデル・ワールド描画処理
        // ==========================================

        // 地面描画
        DrawGround();

        // 自転車3Dモデル描画
        bicycle.Draw();

        // 新聞3Dモデル描画（カゴ内＋手持ち）
        newspaper.Draw();

        // 全8箇所の郵便ポスト3Dモデル描画
        mailbox.Draw(
            deliveryManager
        );

        // ==========================================
        // 2D UI・HUD描画処理（Chilla's Art風ホラーUI）
        // ==========================================

        // 自転車・新聞・ポストのプロンプト要求伝達
        bicycle.DrawUI();
        newspaper.DrawUI();
        mailbox.DrawUI(deliveryManager);
        interaction.Draw();

        // 画面周辺の減光ビネット効果
        HorrorUI::Instance().DrawScreenVignette();

        // 画面左上：深夜デジタル時計（例 03:17 AM）
        HorrorUI::Instance().DrawClock();

        // 画面右上：レトロナビゲーションGPS端末風ミニマップおよび配達状況
        minimap.Draw(
            player,
            bicycle,
            deliveryManager,
            newspaper
        );

        // 画面下部：新聞所持部数表示（PAPERS 08 [HOLDING]）
        HorrorUI::Instance().DrawPaperCargo(
            newspaper.GetNewspaperCount(),
            newspaper.IsHolding()
        );

        // 画面中央下部：インタラクト案内（スムーズなフェード演出付き）
        HorrorUI::Instance().DrawInteractionPrompt();

        // 画面中央：極小の照準レティクル（インタラクト接近時に控えめに強調）
        bool isAiming = bicycle.CanRide() || newspaper.CanTake() || mailbox.CanDeliver();
        HorrorUI::Instance().DrawCrosshair(isAiming);

        // ==========================================
        // 開発用デバッグ情報（F3キーでトグル表示）
        // ==========================================
        if (showDebugHUD)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 160);
            DrawBox(15, 540, 240, 650, HorrorUI::COL_BG, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            DrawFormatString(20, 545, HorrorUI::COL_TEXT_DIM, "[DEBUG HUD - F3]");
            DrawFormatString(20, 565, HorrorUI::COL_TEXT, "Delivery : %d", deliveryManager.GetCurrentDelivery());
            DrawFormatString(20, 585, HorrorUI::COL_TEXT, "Completed: %d / %d", deliveryManager.GetCompletedDeliveries(), deliveryManager.GetTotalDeliveries());
            DrawFormatString(20, 605, HorrorUI::COL_TEXT, "Target ID: %d", deliveryManager.GetCurrentTargetId());
            DrawFormatString(20, 625, HorrorUI::COL_TEXT, "Effect F4: %s", HorrorUI::Instance().IsHorrorEffectsEnabled() ? "ON" : "OFF");
        }

        // 裏画面の内容を表画面へ反映
        ScreenFlip();
    }

    // ホラーUIシステムの終了処理
    HorrorUI::Instance().Finalize();

    // DxLib終了処理
    DxLib_End();

    return 0;
}