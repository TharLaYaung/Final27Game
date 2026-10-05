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
#include "SceneManager.h"

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

    // 街並みマップモデル初期化
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
    SceneManager sceneManager;

    // ホラーUI総合管理システムの初期化（フォント生成など）
    HorrorUI::Instance().Initialize();

    // 深夜環境・照明・フォグ管理システムの初期化
    nightEnv.Initialize();

    // 動的ランダム天候管理システムの初期化
    weatherManager.Initialize();

    // 恐怖・怪異・ゲームオーバー管理システムの初期化
    horrorManager.Initialize();

    // 自転車モデル初期化
    if (bicycle.Initialize() == false)
    {
        MessageBox(
            NULL,
            "Failed to load bicycle model.",
            "Error",
            MB_OK
        );
    }

    // 新聞モデル初期化
    if (newspaper.Initialize() == false)
    {
        MessageBox(
            NULL,
            "Failed to load newspaper model.",
            "Error",
            MB_OK
        );
    }

    // 郵便ポストモデル初期化
    if (mailbox.Initialize() == false)
    {
        MessageBox(
            NULL,
            "Failed to load mailbox model.",
            "Error",
            MB_OK
        );
    }

    // 配達管理システム初期化
    deliveryManager.Initialize();

    // ミニマップ初期化
    minimap.Initialize();

    // シーン統合管理システム初期化
    sceneManager.Initialize();

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
    bool oldKeyJ = false;
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

        // 終了要求フラグ
        bool quitRequested = false;

        // シーン管理更新（トランジション進行・各シーン制御・シネマティックカメラ）
        sceneManager.Update(
            0.01666f,
            player,
            bicycle,
            newspaper,
            mailbox,
            deliveryManager,
            nightEnv,
            weatherManager,
            horrorManager,
            quitRequested
        );

        // シーンから終了要求が出た場合はループを脱出
        if (quitRequested)
        {
            break;
        }

        // ゲーム本編シーン進行中の個別システム更新（ポーズ中以外）
        if (sceneManager.GetCurrentScene() == SceneType::Game && !sceneManager.IsPaused())
        {
            // デバッグ操作キー入力判定（F1〜F11および数字キー、Jキー）

            // F1キー: フォグON/OFF切り替え
            bool curF1 = (CheckHitKey(KEY_INPUT_F1) != 0);
            if (curF1 && !oldF1Key)
            {
                nightEnv.ToggleFog();
            }
            oldF1Key = curF1;

            // F2キー: 環境プリセット順送り切り替え
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

            // F5キー: テスト街灯明滅トリガー
            bool curF5 = (CheckHitKey(KEY_INPUT_F5) != 0);
            if (curF5 && !oldF5Key)
            {
                nightEnv.TriggerStreetLightFlicker(3);
            }
            oldF5Key = curF5;

            // F6キー: マップデバッグ表示ON/OFF切り替え
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

            // 数字キー5: 恐怖度100%ジャンプスケア即時トリガー
            bool curK5 = (CheckHitKey(KEY_INPUT_5) != 0);
            if (curK5 && !oldKey5)
            {
                horrorManager.DebugTriggerMaxFearGameOver(player);
            }
            oldKey5 = curK5;

            // Jキー: カメラジャンプスケア即時テストトリガー
            bool curKeyJ = (CheckHitKey(KEY_INPUT_J) != 0);
            if (curKeyJ && !oldKeyJ)
            {
                horrorManager.DebugTriggerMaxFearGameOver(player);
            }
            oldKeyJ = curKeyJ;

            // 数字キー6: 怪異モデル強制出現テスト
            bool curK6 = (CheckHitKey(KEY_INPUT_6) != 0);
            if (curK6 && !oldKey6)
            {
                horrorManager.DebugTriggerSpawn(player);
            }
            oldKey6 = curK6;

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
        }

        // 画面クリア
        ClearDrawScreen();

        // シーン管理統合描画（3Dワールド・各シーン2D HUD・CRT/VHSノイズ・トランジション）
        sceneManager.Draw(
            player,
            bicycle,
            newspaper,
            mailbox,
            deliveryManager,
            minimap,
            nightEnv,
            weatherManager,
            horrorManager,
            mapDebugEnabled
        );

        // 裏画面の内容を表画面へ反映
        ScreenFlip();
    }

    // シーン管理システムの終了処理
    sceneManager.Finalize();

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