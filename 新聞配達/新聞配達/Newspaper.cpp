#include "Newspaper.h"
#include "Player.h"
#include <cmath>

// コンストラクタ
Newspaper::Newspaper()
{
    // モデル未読み込み
    modelHandle = -1;

    // 自転車の初期位置
    bicyclePosition = VGet(
        0.0f,
        0.0f,
        0.0f
    );

    // 自転車の初期角度
    bicycleAngle = 0.0f;

    // カゴ内での新聞の固定位置
    localPosition = VGet(
        1.03f,
        0.55f,
        -0.24f
    );

    // カゴ内での新聞の固定角度
    localRotation = VGet(
        0.0f,
        1.1f * DX_PI_F / 180.0f,
        -39.0f * DX_PI_F / 180.0f
    );

    // 新聞の大きさ
    modelScale = 0.0100f;

    // 新聞同士の間隔
    stackInterval = 0.020f;

    // 最初は8枚
    newspaperCount = 8;

    // 最初は新聞を持っていない
    isHolding = false;

    // 最初は取れない
    canTake = false;

    // 左クリック状態
    oldLeftClick = false;

    // プレイヤー位置
    playerPosition = VGet(
        0.0f,
        0.0f,
        0.0f
    );

    // プレイヤー方向
    playerForward = VGet(
        0.0f,
        0.0f,
        1.0f
    );
}


// デストラクタ
Newspaper::~Newspaper()
{
    Finalize();
}


// 初期化
bool Newspaper::Initialize()
{
    // 新聞モデルを読み込む
    modelHandle = MV1LoadModel(
        "Data/Model/Newspaper.mv1"
    );

    // 読み込み失敗
    if (modelHandle == -1)
    {
        return false;
    }

    return true;
}


// カゴの新聞の現在位置を取得
VECTOR Newspaper::GetBasketNewspaperPosition() const
{
    // 自転車の角度
    float cosAngle =
        cosf(bicycleAngle);

    float sinAngle =
        sinf(bicycleAngle);

    // カゴ内のローカル位置を
    // 自転車の向きに合わせる
    float worldOffsetX =
        localPosition.x * cosAngle +
        localPosition.z * sinAngle;

    float worldOffsetZ =
        -localPosition.x * sinAngle +
        localPosition.z * cosAngle;

    // 新聞の一番上あたりを狙いやすくする
    float topHeight = 0.0f;

    if (newspaperCount > 0)
    {
        topHeight =
            (newspaperCount - 1) *
            stackInterval;
    }

    return VGet(
        bicyclePosition.x +
        worldOffsetX,

        bicyclePosition.y +
        localPosition.y +
        topHeight,

        bicyclePosition.z +
        worldOffsetZ
    );
}


// 更新
void Newspaper::Update(
    Player& player,
    VECTOR newBicyclePosition,
    float newBicycleAngle,
    bool bicycleRiding
)
{
    // 自転車の現在位置
    bicyclePosition =
        newBicyclePosition;

    // 自転車の現在角度
    bicycleAngle =
        newBicycleAngle;

    // プレイヤーの位置を保存
    playerPosition =
        player.GetPosition();

    // プレイヤーの向きを保存
    playerForward =
        player.GetForward();

    // 毎フレーム初期化
    canTake = false;

    // 左クリックの現在状態
    bool leftClick =
        (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;

    // 自転車に乗っていない
    // 新聞が残っている
    // 新聞をまだ持っていない
    if (
        bicycleRiding == false &&
        newspaperCount > 0 &&
        isHolding == false
        )
    {
        // カゴの新聞位置
        VECTOR newspaperPosition =
            GetBasketNewspaperPosition();

        // プレイヤーから新聞への方向
        VECTOR toNewspaper =
            VSub(
                newspaperPosition,
                playerPosition
            );

        // 新聞までの距離
        float distance =
            VSize(
                toNewspaper
            );

        // 3メートル以内
        if (distance <= 3.0f)
        {
            // 方向を正規化
            VECTOR direction =
                VNorm(
                    toNewspaper
                );

            // プレイヤーがどれくらい
            // 新聞の方向を見ているか
            float dot =
                VDot(
                    playerForward,
                    direction
                );

            // 新聞を見ている場合
            if (dot >= 0.94f)
            {
                canTake = true;
            }
        }
    }

    // 新聞を狙っている
    // 左クリックを押した瞬間
    if (
        canTake == true &&
        leftClick == true &&
        oldLeftClick == false
        )
    {
        // カゴから新聞を1枚減らす
        newspaperCount--;

        // プレイヤーが新聞を持つ
        isHolding = true;

        // 取得したので表示を消す
        canTake = false;
    }

    // 左クリック状態を保存
    oldLeftClick =
        leftClick;
}


// カゴの新聞を描画
void Newspaper::DrawBasketNewspapers()
{
    // 新聞が0枚なら描画しない
    if (newspaperCount <= 0)
    {
        return;
    }

    // 自転車の回転行列
    MATRIX bicycleRotationMatrix =
        MGetRotY(
            bicycleAngle
        );

    // 自転車の移動行列
    MATRIX bicycleTranslationMatrix =
        MGetTranslate(
            bicyclePosition
        );

    // 自転車全体の行列
    MATRIX bicycleMatrix =
        MMult(
            bicycleRotationMatrix,
            bicycleTranslationMatrix
        );

    // 新聞サイズ
    MATRIX scaleMatrix =
        MGetScale(
            VGet(
                modelScale,
                modelScale,
                modelScale
            )
        );

    // 新聞の固定回転
    MATRIX rotationXMatrix =
        MGetRotX(
            localRotation.x
        );

    MATRIX rotationYMatrix =
        MGetRotY(
            localRotation.y
        );

    MATRIX rotationZMatrix =
        MGetRotZ(
            localRotation.z
        );

    MATRIX newspaperRotationMatrix =
        MMult(
            rotationXMatrix,
            rotationYMatrix
        );

    newspaperRotationMatrix =
        MMult(
            newspaperRotationMatrix,
            rotationZMatrix
        );

    // 残っている新聞を描画
    for (int i = 0; i < newspaperCount; i++)
    {
        // カゴ内での固定位置
        VECTOR stackPosition =
            VGet(
                localPosition.x,

                localPosition.y +
                i * stackInterval,

                localPosition.z
            );

        // カゴ内での移動行列
        MATRIX newspaperPositionMatrix =
            MGetTranslate(
                stackPosition
            );

        // 新聞の最終行列
        MATRIX newspaperMatrix =
            scaleMatrix;

        newspaperMatrix =
            MMult(
                newspaperMatrix,
                newspaperRotationMatrix
            );

        newspaperMatrix =
            MMult(
                newspaperMatrix,
                newspaperPositionMatrix
            );

        newspaperMatrix =
            MMult(
                newspaperMatrix,
                bicycleMatrix
            );

        // 新聞に行列を設定
        MV1SetMatrix(
            modelHandle,
            newspaperMatrix
        );

        // 描画
        MV1DrawModel(
            modelHandle
        );
    }
}


// 手に持っている新聞を描画
void Newspaper::DrawHeldNewspaper()
{
    // 持っていない場合
    if (isHolding == false)
    {
        return;
    }

    // プレイヤーの前方向
    VECTOR forward =
        VNorm(
            playerForward
        );

    // プレイヤーの右方向
    VECTOR right =
        VGet(
            forward.z,
            0.0f,
            -forward.x
        );

    // 右方向を正規化
    if (VSize(right) > 0.001f)
    {
        right =
            VNorm(
                right
            );
    }

    // 手に持つ新聞の位置
    VECTOR heldPosition =
        VAdd(
            playerPosition,
            VScale(
                forward,
                0.75f
            )
        );

    // 少し右にする
    heldPosition =
        VAdd(
            heldPosition,
            VScale(
                right,
                0.28f
            )
        );

    // 少し下にする
    heldPosition.y -=
        0.30f;

    // プレイヤーの向きから
    // Y軸角度を計算
    float playerYaw =
        atan2f(
            forward.x,
            forward.z
        );

    // 上下を見る角度
    float playerPitch =
        -asinf(
            forward.y
        );

    // 手持ち新聞は少し小さくする
    float heldScale =
        0.0060f;

    // サイズ
    MATRIX scaleMatrix =
        MGetScale(
            VGet(
                heldScale,
                heldScale,
                heldScale
            )
        );

    // 手持ち新聞の回転
    MATRIX rotationXMatrix =
        MGetRotX(
            playerPitch
        );

    MATRIX rotationYMatrix =
        MGetRotY(
            playerYaw
        );

    // 少し斜めに持つ
    MATRIX rotationZMatrix =
        MGetRotZ(
            -25.0f *
            DX_PI_F /
            180.0f
        );

    MATRIX rotationMatrix =
        MMult(
            rotationXMatrix,
            rotationYMatrix
        );

    rotationMatrix =
        MMult(
            rotationMatrix,
            rotationZMatrix
        );

    // 手の位置
    MATRIX positionMatrix =
        MGetTranslate(
            heldPosition
        );

    // 最終行列
    MATRIX heldMatrix =
        scaleMatrix;

    heldMatrix =
        MMult(
            heldMatrix,
            rotationMatrix
        );

    heldMatrix =
        MMult(
            heldMatrix,
            positionMatrix
        );

    // モデルに設定
    MV1SetMatrix(
        modelHandle,
        heldMatrix
    );

    // 描画
    MV1DrawModel(
        modelHandle
    );
}


// 描画
void Newspaper::Draw()
{
    // モデルがない場合
    if (modelHandle == -1)
    {
        return;
    }

    // カゴの新聞
    DrawBasketNewspapers();

    // 手持ち新聞
    DrawHeldNewspaper();
}


// UI描画
void Newspaper::DrawUI()
{
    // 新聞を取れる場合
    if (canTake == true)
    {
        DrawString(
            540,
            410,
            "Left Click : Take Newspaper",
            GetColor(
                255,
                255,
                0
            )
        );
    }

    // 残り新聞枚数
    DrawFormatString(
        20,
        100,
        GetColor(
            255,
            255,
            255
        ),
        "Newspaper : %d",
        newspaperCount
    );

    // 新聞を持っている場合
    if (isHolding == true)
    {
        DrawString(
            20,
            120,
            "Holding Newspaper",
            GetColor(
                255,
                255,
                0
            )
        );
    }
}


// 新聞を持っているか
bool Newspaper::IsHolding() const
{
    return isHolding;
}


// 残り新聞枚数
int Newspaper::GetNewspaperCount() const
{
    return newspaperCount;
}


// 終了処理
void Newspaper::Finalize()
{
    // モデルがある場合
    if (modelHandle != -1)
    {
        // モデル削除
        MV1DeleteModel(
            modelHandle
        );

        // リセット
        modelHandle = -1;
    }
}