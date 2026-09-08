#include "DxLib.h"

// ゲームの開始
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    // ウィンドウモード
    ChangeWindowMode(TRUE);

    // 画面サイズ
    SetGraphMode(1280, 720, 32);

    // DxLib初期化
    if (DxLib_Init() == -1)
    {
        return -1;
    }

    // 裏画面を使用
    SetDrawScreen(DX_SCREEN_BACK);

    // ゲームループ
    while (ProcessMessage() == 0)
    {
        // ESCで終了
        if (CheckHitKey(KEY_INPUT_ESCAPE))
        {
            break;
        }

        // 画面を消す
        ClearDrawScreen();

        // 仮の文字
        DrawString(
            50,
            50,
            "Final27 - Newspaper Delivery",
            GetColor(255, 255, 255)
        );

        // 裏画面を表示
        ScreenFlip();
    }

    // DxLib終了
    DxLib_End();

    return 0;
}