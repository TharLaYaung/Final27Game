#include "DxLib.h"
#include "Ground.h"
#include "Player.h"
#include "Interaction.h"
#include "Bicycle.h"

// Windowsアプリケーションの開始地点
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    // ウィンドウモードで起動する
    ChangeWindowMode(TRUE);

    // ゲーム画面を1280×720に設定する
    SetGraphMode(
        1280,
        720,
        32
    );

    // DxLibを初期化する
    if (DxLib_Init() == -1)
    {
        return -1;
    }

    // 裏画面に描画する
    SetDrawScreen(
        DX_SCREEN_BACK
    );

    // 3Dカメラの表示距離を設定する
    SetCameraNearFar(
        0.1f,
        1000.0f
    );

    // プレイヤーを作成する
    Player player;

    // インタラクトシステムを作成する
    Interaction interaction;

    // 自転車を作成する
    Bicycle bicycle;

    // 自転車モデルを読み込む
    if (bicycle.Initialize() == false)
    {
        // 読み込みに失敗した場合はエラーを表示する
        MessageBox(
            NULL,
            "自転車モデルの読み込みに失敗しました。",
            "エラー",
            MB_OK
        );
    }

    // ゲームループ
    while (ProcessMessage() == 0)
    {
        // ESCキーでゲームを終了する
        if (CheckHitKey(KEY_INPUT_ESCAPE))
        {
            break;
        }

        // 前の画面を消す
        ClearDrawScreen();

        // プレイヤーを更新する
        player.Update();

        // 自転車を更新する
        bicycle.Update(player);

        // カメラを更新する
        player.UpdateCamera();

        // インタラクト処理を更新する
        interaction.Update(player);

        // 地面を描画する
        DrawGround();

        // 自転車モデルを描画する
        bicycle.Draw();

        // テスト用の赤い箱を描画する
        DrawCube3D(
            VGet(
                -1.0f,
                0.0f,
                5.0f
            ),

            VGet(
                1.0f,
                2.0f,
                7.0f
            ),

            GetColor(
                200,
                50,
                50
            ),

            GetColor(
                255,
                255,
                255
            ),

            TRUE
        );

        // インタラクトUIを描画する
        interaction.Draw();

        // 移動方法を表示する
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

        // マウス操作を表示する
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

        // インタラクト操作を表示する
        DrawString(
            20,
            60,
            "E : Interact",
            GetColor(
                255,
                255,
                255
            )
        );

        // 終了方法を表示する
        DrawString(
            20,
            80,
            "ESC : Exit",
            GetColor(
                255,
                255,
                255
            )
        );

        // 裏画面を表画面に表示する
        ScreenFlip();
    }

    // DxLibを終了する
    DxLib_End();

    // 正常終了
    return 0;
}