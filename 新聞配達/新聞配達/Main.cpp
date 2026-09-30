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
#include "Map.h"
#include "NightEnvironment.h"
#include "WeatherManager.h"
#include "HorrorManager.h"

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

    MapInit();

    // 各ゲームシステムのインスタンス生成
    Player player;
    Interaction interaction;
    Bicycle bicycle;
    Newspaper newspaper;
    Mailbox mailbox;
    DeliveryManager deliveryManager;
    Minimap minimap;
    NightEnvironment nightEnv;
    WeatherManager weatherManager;
    HorrorManager horrorManager;

    // ホラーUI総合管理システムの初期化（フォント生成など）
    HorrorUI::Instance().Initialize();

    // 深夜環境・照明・フォグ管理システムの初期化
    nightEnv.Initialize();

    // 動的ランダム天候管理システムの初期化
    weatherManager.Initialize();

    // 恐怖・怪異・ゲームオーバー管理システムの初期化
    horrorManager.Initialize();

    {
        int h = MV1LoadModel("Data/Model/RunningCrawl.mv1");
        if (h != -1)
        {
            FILE* fp = nullptr;
            fopen_s(&fp, "face_check.txt", "w");
            if (fp)
            {
                int fHead = MV1SearchFrame(h, "Jill_HiRes_Head_Geo");
                int fEyes = MV1SearchFrame(h, "Jill_HiRes_Eyes_Geo");
                int fHips = MV1SearchFrame(h, "Hips");

                VECTOR posH = MV1GetFramePosition(h, fHead);
                VECTOR posE = MV1GetFramePosition(h, fEyes);
                VECTOR posHips = MV1GetFramePosition(h, fHips);

                fprintf(fp, "Base Pose:\n");
                fprintf(fp, "Hips: (%.2f, %.2f, %.2f)\n", posHips.x, posHips.y, posHips.z);
                fprintf(fp, "Head: (%.2f, %.2f, %.2f)\n", posH.x, posH.y, posH.z);
                fprintf(fp, "Eyes: (%.2f, %.2f, %.2f)\n", posE.x, posE.y, posE.z);

                int a = MV1AttachAnim(h, 1, -1, FALSE);
                MV1SetAttachAnimTime(h, a, 0.0f);
                posH = MV1GetFramePosition(h, fHead);
                posE = MV1GetFramePosition(h, fEyes);
                posHips = MV1GetFramePosition(h, fHips);

                fprintf(fp, "Anim at 0.0s:\n");
                fprintf(fp, "Hips: (%.2f, %.2f, %.2f)\n", posHips.x, posHips.y, posHips.z);
                fprintf(fp, "Head: (%.2f, %.2f, %.2f)\n", posH.x, posH.y, posH.z);
                fprintf(fp, "Eyes: (%.2f, %.2f, %.2f)\n", posE.x, posE.y, posE.z);

                MV1SetAttachAnimTime(h, a, 5.0f);
                posH = MV1GetFramePosition(h, fHead);
                posHips = MV1GetFramePosition(h, fHips);
                fprintf(fp, "Anim at 5.0s:\n");
                fprintf(fp, "Hips: (%.2f, %.2f, %.2f)\n", posHips.x, posHips.y, posHips.z);
                fprintf(fp, "Head: (%.2f, %.2f, %.2f)\n", posH.x, posH.y, posH.z);

                fclose(fp);
            }
            MV1DeleteModel(h);
        }
    }

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

    // 60FPS制御用の時間記録
    LONGLONG prevTime = GetNowHiPerformanceCount();

    // デバッグキー前フレーム状態フラグ
    bool oldF1Key = false;
    bool oldF2Key = false;
    bool oldF3Key = false;
    bool oldF4Key = false;
    bool oldF5Key = false;
    bool oldF6Key = false;
    bool oldF7Key = false;
    bool oldF9Key = false;
    bool oldF10Key = false;
    bool oldF11Key = false;
    bool oldKey1 = false;
    bool oldKey2 = false;
    bool oldKey3 = false;
    bool oldKey4 = false;
    bool oldKey5 = false;
    bool oldKey6 = false;
    bool mapDebugEnabled = false;

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

        // デバッグ操作キー入力判定（F1〜F11および数字キー）

        // F1キー: フォグON/OFF切り替え
        bool curF1 = (CheckHitKey(KEY_INPUT_F1) != 0);
        if (curF1 && !oldF1Key)
        {
            nightEnv.ToggleFog();
        }
        oldF1Key = curF1;

        // F2キー: 環境プリセット順送り切り替え（NORMAL -> NIGHT_DELIVERY_STYLE -> EXTREME_DARK）
        bool curF2 = (CheckHitKey(KEY_INPUT_F2) != 0);
        if (curF2 && !oldF2Key)
        {
            nightEnv.CyclePreset();
        }
        oldF2Key = curF2;

        // F3キー: 街灯ON/OFF切り替え
        bool curF3 = (CheckHitKey(KEY_INPUT_F3) != 0);
        if (curF3 && !oldF3Key)
        {
            nightEnv.ToggleStreetLights();
        }
        oldF3Key = curF3;

        // F4キー: ホラー画面効果ON/OFF切り替え
        bool curF4 = (CheckHitKey(KEY_INPUT_F4) != 0);
        if (curF4 && !oldF4Key)
        {
            HorrorUI::Instance().ToggleHorrorEffects();
        }
        oldF4Key = curF4;

        // F5キー: テスト街灯明滅トリガー（ID: 3の街灯）
        bool curF5 = (CheckHitKey(KEY_INPUT_F5) != 0);
        if (curF5 && !oldF5Key)
        {
            nightEnv.TriggerStreetLightFlicker(3);
        }
        oldF5Key = curF5;

        // F6キー: マップデバッグ表示ON/OFF切り替え（住宅・ポストID・当たり判定）
        bool curF6 = (CheckHitKey(KEY_INPUT_F6) != 0);
        if (curF6 && !oldF6Key)
        {
            mapDebugEnabled = !mapDebugEnabled;
        }
        oldF6Key = curF6;

        // F7キー: 天候順送り切り替え
        bool curF7 = (CheckHitKey(KEY_INPUT_F7) != 0);
        if (curF7 && !oldF7Key)
        {
            weatherManager.CycleWeather();
        }
        oldF7Key = curF7;

        // F9キー: 自動天候変化ON/OFF切り替え
        bool curF9 = (CheckHitKey(KEY_INPUT_F9) != 0);
        if (curF9 && !oldF9Key)
        {
            weatherManager.ToggleAutoWeather();
        }
        oldF9Key = curF9;

        // F10キー: 天候デバッグHUD表示ON/OFF切り替え
        bool curF10 = (CheckHitKey(KEY_INPUT_F10) != 0);
        if (curF10 && !oldF10Key)
        {
            weatherManager.ToggleDebugHUD();
        }
        oldF10Key = curF10;

        // F11キー: 恐怖・怪異デバッグHUD表示ON/OFF切り替え
        bool curF11 = (CheckHitKey(KEY_INPUT_F11) != 0);
        if (curF11 && !oldF11Key)
        {
            horrorManager.ToggleDebugHUD();
        }
        oldF11Key = curF11;

        // 数字キー1: 恐怖度25%設定
        bool curK1 = (CheckHitKey(KEY_INPUT_1) != 0);
        if (curK1 && !oldKey1)
        {
            horrorManager.DebugSetFear(0.25f);
        }
        oldKey1 = curK1;

        // 数字キー2: 恐怖度50%設定
        bool curK2 = (CheckHitKey(KEY_INPUT_2) != 0);
        if (curK2 && !oldKey2)
        {
            horrorManager.DebugSetFear(0.50f);
        }
        oldKey2 = curK2;

        // 数字キー3: 恐怖度75%設定
        bool curK3 = (CheckHitKey(KEY_INPUT_3) != 0);
        if (curK3 && !oldKey3)
        {
            horrorManager.DebugSetFear(0.75f);
        }
        oldKey3 = curK3;

        // 数字キー4: 恐怖度95%設定
        bool curK4 = (CheckHitKey(KEY_INPUT_4) != 0);
        if (curK4 && !oldKey4)
        {
            horrorManager.DebugSetFear(0.95f);
        }
        oldKey4 = curK4;

        // 数字キー5: 恐怖度100%ゲームオーバー即時トリガー
        bool curK5 = (CheckHitKey(KEY_INPUT_5) != 0);
        if (curK5 && !oldKey5)
        {
            horrorManager.DebugTriggerMaxFearGameOver(player);
        }
        oldKey5 = curK5;

        // 数字キー6: 怪異モデル強制出現テスト（前方16m）
        bool curK6 = (CheckHitKey(KEY_INPUT_6) != 0);
        if (curK6 && !oldKey6)
        {
            horrorManager.DebugTriggerSpawn(player);
        }
        oldKey6 = curK6;

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

        // 徒歩時と乗車時の操作分岐（最大恐怖演出中およびゲームオーバー中は移動制限）
        if (!horrorManager.IsControlRestricted())
        {
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
        }
        else
        {
            // 演出中：視線操作のみ許容
            player.UpdateLook();
        }

        // 新聞更新（カゴ座標追従および手持ち描画管理）
        newspaper.Update(
            player,
            bicycle.GetPosition(),
            bicycle.GetAngle(),
            bicycle.IsRiding()
        );

        // 深夜環境の更新（自転車ヘッドライト追従・街灯明滅タイマー）
        nightEnv.Update(bicycle);

        // 動的ランダム天候管理システムの更新
        weatherManager.Update(0.01666f, player, bicycle, nightEnv);

        // 恐怖システム・怪異出現・ゲームオーバー管理システムの更新
        horrorManager.Update(0.01666f, player, bicycle, nightEnv);

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

        // 深夜環境（フォグ・背景色・環境光・ライト設定）の適用
        nightEnv.ApplyLightingAndFog();

        // 地面描画
        DrawGround();

        MapDraw();

        // 街灯電柱・ランプ器具の描画
        nightEnv.DrawStreetLights();

        // 自転車3Dモデル描画
        bicycle.Draw();

        // 新聞3Dモデル描画（カゴ内＋手持ち）
        newspaper.Draw();

        // 全8箇所の郵便ポスト3Dモデル描画
        mailbox.Draw(
            deliveryManager
        );

        // 天候パーティクル描画（雨粒・粉雪）
        weatherManager.DrawParticles(bicycle, nightEnv);

        // 3D怪異モデル描画（暗闇・フォグ・街灯演出と同期）
        horrorManager.Draw3D();

        // F6マップデバッグ3D描画（当たり判定ワイヤー・住宅/ポストIDラベル）
        if (mapDebugEnabled)
        {
            MapDrawDebug(deliveryManager.GetCurrentTargetId());
        }

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

        // 画面右上ミニマップ直下：自転車ヘッドライト・バッテリーHUD
        HorrorUI::Instance().DrawBicycleBattery(
            bicycle.GetBatteryPercent(),
            bicycle.IsHeadlightOn(),
            bicycle.IsRiding(),
            bicycle.IsDebugFastDrain()
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

        // デバッグ操作通知HUD（F1〜F5操作時に短時間表示）
        nightEnv.DrawDebugHUD();

        // F6マップデバッグHUD（現在ターゲット・配達状況・各ID表示）
        if (mapDebugEnabled)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210);
            DrawBox(18, 170, 310, 248, GetColor(10, 16, 20), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(18, 170, 310, 248, GetColor(80, 115, 100), FALSE);

            DrawString(26, 176, "[ F6: MAP DEBUG ON ]", GetColor(255, 220, 100));

            char dbgTarget[64];
            int tid = deliveryManager.GetCurrentTargetId();
            if (tid >= 0)
            {
                snprintf(dbgTarget, sizeof(dbgTarget), "CURRENT TARGET: HOUSE %d", tid);
            }
            else
            {
                snprintf(dbgTarget, sizeof(dbgTarget), "CURRENT TARGET: ALL COMPLETED");
            }
            DrawString(26, 198, dbgTarget, GetColor(200, 225, 255));

            char dbgProg[64];
            snprintf(dbgProg, sizeof(dbgProg), "DELIVERY: %02d / %02d HOUSES",
                deliveryManager.GetCompletedDeliveries(), deliveryManager.GetTotalDeliveries());
            DrawString(26, 220, dbgProg, GetColor(170, 195, 185));
        }

        // 天候デバッグHUD描画（F10で切り替え）
        weatherManager.DrawDebugHUD();

        // 恐怖度メーター・ゲームオーバー・演出描画
        horrorManager.DrawUI();

        // 裏画面の内容を表画面へ反映
        ScreenFlip();
    }

    // マップシステムの終了処理
    MapFinalize();

    // 深夜環境システムの終了処理
    nightEnv.Finalize();

    // 動的ランダム天候管理システムの終了処理
    weatherManager.Finalize();

    // 恐怖システム・怪異管理システムの終了処理
    horrorManager.Finalize();

    // ホラーUIシステムの終了処理
    HorrorUI::Instance().Finalize();

    // DxLib終了処理
    DxLib_End();

    return 0;
}