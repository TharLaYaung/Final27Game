#include "DxLib.h"
#include "Ground.h"
#include "Player.h"
#include "Interaction.h"
#include "Bicycle.h"
#include "Newspaper.h"
#include "Mailbox.h"

// Windowsアプリケーションの開始地点
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    // ウィンドウモード
    ChangeWindowMode(TRUE);

    // 画面サイズ
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

    // 裏画面描画
    SetDrawScreen(
        DX_SCREEN_BACK
    );

    // カメラ描画距離
    SetCameraNearFar(
        0.1f,
        1000.0f
    );

    // プレイヤー
    Player player;

    // インタラクト
    Interaction interaction;

    // 自転車
    Bicycle bicycle;

    // 新聞
    Newspaper newspaper;

    // メールボックス（郵便ポスト）
    Mailbox mailbox;

    // 自転車初期化
    if (bicycle.Initialize() == false)
    {
        MessageBox(
            NULL,
            "自転車モデルの読み込みに失敗しました。",
            "エラー",
            MB_OK
        );
    }

    // 新聞初期化
    if (newspaper.Initialize() == false)
    {
        MessageBox(
            NULL,
            "新聞モデルの読み込みに失敗しました。",
            "エラー",
            MB_OK
        );
    }

    // メールボックス初期化
    if (mailbox.Initialize() == false)
    {
        MessageBox(
            NULL,
            "メールボックスモデルの読み込みに失敗しました。",
            "エラー",
            MB_OK
        );
    }

    // 60FPS制御用の時間記録
    LONGLONG prevTime = GetNowHiPerformanceCount();

    // ゲームループ
    while (ProcessMessage() == 0)
    {
        // 60FPS（約16.6ミリ秒）に速度を一定化
        while (GetNowHiPerformanceCount() - prevTime < 16666)
        {
            Sleep(0);
        }
        prevTime = GetNowHiPerformanceCount();
        // ESCで終了
        if (CheckHitKey(KEY_INPUT_ESCAPE))
        {
            break;
        }

        // 画面クリア
        ClearDrawScreen();

        // 自転車位置をプレイヤーに渡す
        player.SetBicyclePosition(
            bicycle.GetPosition()
        );

        // 自転車に乗っていない時だけ
        // 自転車との衝突を有効にする
        player.SetBicycleCollisionEnabled(
            bicycle.IsRiding() == false
        );

        // 徒歩状態
        if (bicycle.IsRiding() == false)
        {
            // 移動＋マウス視点
            player.Update();
        }
        else
        {
            // 乗車中はマウス視点だけ
            player.UpdateLook();
        }

        // 自転車更新
        bicycle.Update(
            player
        );

        // 新聞更新
        newspaper.Update(
            player,
            bicycle.GetPosition(),
            bicycle.GetAngle(),
            bicycle.IsRiding()
        );

        // メールボックス更新
        mailbox.Update(
            player,
            newspaper
        );

        // カメラ更新
        player.UpdateCamera();

        // 通常インタラクト更新
        interaction.Update(
            player
        );

        // 地面描画
        DrawGround();

        // 自転車描画
        bicycle.Draw();

        // 新聞描画
        newspaper.Draw();

        // メールボックス描画
        mailbox.Draw();



        // 自転車UI
        bicycle.DrawUI();

        // 新聞UI
        newspaper.DrawUI();

        // メールボックスUI
        mailbox.DrawUI();

        // 通常インタラクトUI
        interaction.Draw();

        // 操作説明
        DrawString(
            20,
            20,
            "WASD : Move",
            GetColor(
                255,
                255,
                255
            )
        );

        DrawString(
            20,
            40,
            "Mouse : Look",
            GetColor(
                255,
                255,
                255
            )
        );

        DrawString(
            20,
            60,
            "E : Ride / Interact",
            GetColor(
                255,
                255,
                255
            )
        );

        DrawString(
            20,
            80,
            "Left Click : Take / Deliver Newspaper",
            GetColor(
                255,
                255,
                255
            )
        );

        // 画面中央の照準
        DrawCircle(
            640,
            360,
            3,
            GetColor(
                255,
                255,
                255
            ),
            TRUE
        );

        // 画面表示
        ScreenFlip();
    }

    // DxLib終了
    DxLib_End();

    return 0;
}