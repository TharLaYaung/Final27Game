#include "DxLib.h"
#include "Ground.h"
#include "Player.h"
#include "Interaction.h"
#include "Bicycle.h"
#include "Newspaper.h"
#include "Map.h"

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

    MapInit();

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

    // 新聞を作成する
    Newspaper newspaper;

    // 自転車モデルを読み込む
    if (bicycle.Initialize() == false)
    {
        MessageBox(
            NULL,
            "自転車モデルの読み込みに失敗しました。",
            "エラー",
            MB_OK
        );
    }

    // 新聞モデルを読み込む
    if (newspaper.Initialize() == false)
    {
        MessageBox(
            NULL,
            "新聞モデルの読み込みに失敗しました。",
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

        // 自転車の位置をプレイヤーに渡す
        player.SetBicyclePosition(
            bicycle.GetPosition()
        );

        // 自転車に乗っていない時だけ当たり判定を使う
        player.SetBicycleCollisionEnabled(
            bicycle.IsRiding() == false
        );

        // 自転車に乗っていない場合
        if (bicycle.IsRiding() == false)
        {
            // 徒歩移動とマウス視点を更新する
            player.Update();
        }
        else
        {
            // 自転車に乗っている時はマウス視点だけ更新する
            player.UpdateLook();
        }

        // 自転車を更新する
        bicycle.Update(player);

        // 新聞を自転車の位置に合わせる
        newspaper.Update(
            bicycle.GetPosition()
        );

        // カメラを更新する
        player.UpdateCamera();

        // インタラクト処理を更新する
        interaction.Update(player);

        // 地面を描画する
        DrawGround();

        // 家の描画
        MapDraw();

        // 自転車を描画する
        bicycle.Draw();

        // 新聞を描画する
        newspaper.Draw();

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

        // 自転車のUIを描画する
        bicycle.DrawUI();

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