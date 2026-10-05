#include "Bicycle.h"
#include "Player.h"
#include "HorrorUI.h"
#include "Map.h"
#include <cmath>
#include <cstdlib>

// コンストラクタ
Bicycle::Bicycle()
{
    // モデル未読み込み
    modelHandle = -1;

    // 自転車の初期位置（南通りスタート地点、プレイヤーの隣）
    position = VGet(
        1.8f,
        0.18f,
        -30.0f
    );

    // 自転車の向き（西向き、配達先0方向）
    angle = DX_PI_F;

    // 最初は乗っていない
    isRiding = false;

    // 最初は選択していない
    canRide = false;

    // 最初はEキーを押していない
    oldEKey = false;

    // 自転車の移動速度
    moveSpeed = 0.14f;

    // ヘッドライト・バッテリー初期設定
    // 深夜3時の暗闇で最初から灯りが点いている安心感を提供
    headlightOn = true;
    oldFKey = false;
    oldF8Key = false;

    // 自転車ローカル基準のライト配置オフセット（前輪フォーク・ライト固定位置）
    headlightOffsetForward = 0.85f;
    headlightOffsetHeight = 0.88f;
    headlightOffsetSide = 0.0f;

    // 基本照射距離（霧を貫通しすぎないホラー適正値: 14m）
    headlightBaseRange = 14.0f;
    headlightIntensity = 1.0f;
    flickerMultiplier = 1.0f;
    flickerTimer = 180;
    flickerPhase = 0;
    externalBlackoutTimer = 0.0f;

    // バッテリー初期値: 100%
    batteryMax = 100.0f;
    batteryCurrent = 100.0f;
    batteryDrainMultiplier = 1.0f;
    debugFastDrain = false;

    lastUpdateTime = 0;
    emptyWarningTimer = 0;
}

// デストラクタ
Bicycle::~Bicycle()
{
    // モデルを削除する
    Finalize();
}

// 自転車モデルを初期化する
bool Bicycle::Initialize()
{
    // 自転車モデルを読み込む
    modelHandle = MV1LoadModel(
        "Data/Model/Bicycle.mv1"
    );

    // 読み込み失敗
    if (modelHandle == -1)
    {
        return false;
    }

    // 自転車の位置を設定する
    MV1SetPosition(
        modelHandle,
        position
    );

    // 自転車の大きさを設定する
    MV1SetScale(
        modelHandle,
        VGet(
            0.01f,
            0.01f,
            0.01f
        )
    );

    // 自転車の向きを設定する
    MV1SetRotationXYZ(
        modelHandle,
        VGet(
            0.0f,
            angle,
            0.0f
        )
    );

    lastUpdateTime = GetNowHiPerformanceCount();

    return true;
}

// 自転車を更新する
void Bicycle::Update(Player& player)
{
    // モデルがない場合
    if (modelHandle == -1)
    {
        return;
    }

    // デルタタイム計算（フレームレート非依存処理用）
    LONGLONG now = GetNowHiPerformanceCount();
    float dt = 0.01666f;
    if (lastUpdateTime != 0)
    {
        dt = (float)(now - lastUpdateTime) / 1000000.0f;
        if (dt < 0.0001f) dt = 0.0001f;
        if (dt > 0.1f) dt = 0.1f; // 極端なスパイクをクランプ
    }
    lastUpdateTime = now;

    // ヘッドライト・バッテリー操作入力判定
    // Fキー: ヘッドライトON/OFFトグル（エッジ検出: 1回押しで1回だけ切り替え）
    bool currentFKey = (CheckHitKey(KEY_INPUT_F) != 0);
    if (currentFKey && !oldFKey)
    {
        ToggleHeadlight();
    }
    oldFKey = currentFKey;

    // F8キー: 高速消費デバッグモードの切り替え（10倍速）
    bool currentF8Key = (CheckHitKey(KEY_INPUT_F8) != 0);
    if (currentF8Key && !oldF8Key)
    {
        ToggleDebugFastDrain();
    }
    oldF8Key = currentF8Key;

    // バッテリー消費とライト挙動（減光・フリッカー・消灯）の更新
    UpdateBatteryAndLighting(dt);

    // 自転車の乗降および移動処理
    // Eキーの状態を取得する
    bool currentEKey =
        CheckHitKey(KEY_INPUT_E) != 0;

    // 自転車に乗っていない場合
    if (isRiding == false)
    {
        // プレイヤーの位置を取得する
        VECTOR playerPosition =
            player.GetPosition();

        // プレイヤーが見ている方向を取得する
        VECTOR playerForward =
            player.GetForward();

        // プレイヤーから自転車への方向を計算する
        VECTOR toBicycle =
            VSub(
                position,
                playerPosition
            );

        // プレイヤーと自転車の距離を計算する
        float distance =
            sqrtf(
                toBicycle.x * toBicycle.x +
                toBicycle.y * toBicycle.y +
                toBicycle.z * toBicycle.z
            );

        // 自転車を選択していない状態にする
        canRide = false;

        // 3メートル以内の場合
        if (distance <= 3.0f)
        {
            // 距離が0ではない場合
            if (distance > 0.001f)
            {
                // 自転車方向を正規化する
                VECTOR direction;

                direction.x =
                    toBicycle.x / distance;

                direction.y =
                    toBicycle.y / distance;

                direction.z =
                    toBicycle.z / distance;

                // 視線と自転車方向の一致度を計算する
                float dot =
                    playerForward.x * direction.x +
                    playerForward.y * direction.y +
                    playerForward.z * direction.z;

                // カーソルが自転車を向いている場合
                if (dot >= 0.95f)
                {
                    // 自転車を選択状態にする
                    canRide = true;
                }
            }
        }

        // 自転車を選択中にEキーを押した場合
        if (
            canRide == true &&
            currentEKey == true &&
            oldEKey == false
            )
        {
            // 自転車に乗る
            isRiding = true;

            // 選択状態を解除する
            canRide = false;

            // プレイヤーを自転車の位置へ移動する
            player.SetPosition(
                VGet(
                    position.x,
                    position.y + 1.2f,
                    position.z
                )
            );
        }
    }
    else
    {
        // 自転車の前方向を計算する
        VECTOR forward =
            VGet(
                cosf(angle),
                0.0f,
                -sinf(angle)
            );

        // 次の位置
        VECTOR nextPosition =
            position;

        // Wキーで前進する
        if (CheckHitKey(KEY_INPUT_W))
        {
            nextPosition.x +=
                forward.x * moveSpeed;

            nextPosition.z +=
                forward.z * moveSpeed;
        }

        // Sキーで後退する
        if (CheckHitKey(KEY_INPUT_S))
        {
            nextPosition.x -=
                forward.x * moveSpeed;

            nextPosition.z -=
                forward.z * moveSpeed;
        }

        // 移動できる場合
        if (CanMove(nextPosition))
        {
            // 自転車の位置を更新する
            position =
                nextPosition;
        }

        // Aキーで左に曲がる
        if (CheckHitKey(KEY_INPUT_A))
        {
            angle -= 0.03f;
        }

        // Dキーで右に曲がる
        if (CheckHitKey(KEY_INPUT_D))
        {
            angle += 0.03f;
        }

        // プレイヤーを自転車に合わせる
        player.SetPosition(
            VGet(
                position.x,
                position.y + 1.2f,
                position.z
            )
        );

        // Eキーを押した場合
        if (
            currentEKey == true &&
            oldEKey == false
            )
        {
            // 自転車から降りる（※ライトが点灯していれば自転車に付いたまま路面を照らし続ける）
            isRiding = false;

            // プレイヤーを自転車の横へ移動する
            player.SetPosition(
                VGet(
                    position.x + 1.5f,
                    position.y + 1.7f,
                    position.z
                )
            );
        }
    }

    // Eキーの状態を保存する
    oldEKey =
        currentEKey;

    // 自転車の位置を更新する
    MV1SetPosition(
        modelHandle,
        position
    );

    // 自転車の向きを更新する
    MV1SetRotationXYZ(
        modelHandle,
        VGet(
            0.0f,
            angle,
            0.0f
        )
    );
}

// バッテリー消費とライト挙動の更新
void Bicycle::UpdateBatteryAndLighting(float dt)
{
    // バッテリー消費処理（ヘッドライト点灯中のみ減少、120秒で100%消費する高速消費速度）
    const float BASE_DRAIN_PER_SEC = 100.0f / 120.0f; // 約0.833% / 秒

    if (headlightOn && batteryCurrent > 0.0f)
    {
        float actualMultiplier = batteryDrainMultiplier * (debugFastDrain ? 10.0f : 1.0f);
        batteryCurrent -= BASE_DRAIN_PER_SEC * actualMultiplier * dt;

        // バッテリー完全枯渇
        if (batteryCurrent <= 0.0f)
        {
            batteryCurrent = 0.0f;
            headlightOn = false;
            emptyWarningTimer = 180; // 約3秒間警告表示
            PlayElectricalSoundHook();
        }
    }

    // バッテリー警告タイマーのカウントダウン
    if (emptyWarningTimer > 0)
    {
        emptyWarningTimer--;
    }

    // 外部からの強制ブラックアウトタイマー更新
    if (externalBlackoutTimer > 0.0f)
    {
        externalBlackoutTimer -= dt;
        if (externalBlackoutTimer < 0.0f) externalBlackoutTimer = 0.0f;
    }

    // 2. 残量に応じたライト輝度・フリッカー更新
    if (!headlightOn || batteryCurrent <= 0.0f || externalBlackoutTimer > 0.0f)
    {
        headlightIntensity = 0.0f;
        flickerMultiplier = 0.0f;
        return;
    }

    float pct = GetBatteryPercent();

    // NORMAL (51%〜100%): 安定した明るさ
    if (pct > 50.0f)
    {
        flickerMultiplier = 1.0f;
        headlightIntensity = 1.0f;
    }
    // LOW (21%〜50%): わずかな減光（約88%輝度）と時折の微小な揺らぎ
    else if (pct > 20.0f)
    {
        UpdateFlickerLogic(dt);
        headlightIntensity = 0.88f * flickerMultiplier;
    }
    // CRITICAL (5%〜20%): 不規則なフリッカー、電圧低下、一瞬の停電
    else if (pct > 5.0f)
    {
        UpdateFlickerLogic(dt);
        headlightIntensity = 0.80f * flickerMultiplier;
    }
    // VERY LOW (0%〜5%): 著しく不安定な明滅
    else
    {
        UpdateFlickerLogic(dt);
        headlightIntensity = 0.70f * flickerMultiplier;
    }
}

// 不規則なライト明滅ロジック
void Bicycle::UpdateFlickerLogic(float dt)
{
    flickerTimer--;
    if (flickerTimer > 0) return;

    float pct = GetBatteryPercent();

    // 21%〜50%（LOW）: 非常に稀で控えめな電圧の揺らぎ
    if (pct > 20.0f)
    {
        switch (flickerPhase)
        {
        case 0:
            // 安定点灯（4〜8秒間隔）
            flickerMultiplier = 1.0f;
            flickerPhase = 1;
            flickerTimer = 240 + (rand() % 240);
            break;

        case 1:
            // わずかな減光（2フレーム）
            flickerMultiplier = 0.78f;
            flickerPhase = 0;
            flickerTimer = 3;
            break;
        }
        return;
    }

    // 1%〜20%（CRITICAL）: 予測不能な不気味な明滅シーケンス
    int baseWait = (pct <= 5.0f) ? 60 : 180;
    int randWait = (pct <= 5.0f) ? 90 : 200;

    switch (flickerPhase)
    {
    case 0:
        // 安定点灯フェーズ
        flickerMultiplier = 1.0f;
        flickerPhase = 1;
        flickerTimer = baseWait + (rand() % randWait);
        break;

    case 1:
        // 一瞬の急減光（2〜3フレーム）
        flickerMultiplier = 0.35f;
        flickerPhase = 2;
        flickerTimer = 3;
        break;

    case 2:
        // 短い復帰（4フレーム）
        flickerMultiplier = 0.85f;
        flickerPhase = 3;
        flickerTimer = 4;
        break;

    case 3:
        // 一瞬の完全ブラックアウト（恐怖の暗闇: 5〜8フレーム）
        flickerMultiplier = 0.0f;
        flickerPhase = 4;
        flickerTimer = 6;
        break;

    case 4:
        // パッと点灯復帰（3フレーム）
        flickerMultiplier = 1.0f;
        flickerPhase = 5;
        flickerTimer = 3;
        break;

    case 5:
        // 残響フリッカー（3フレーム）
        flickerMultiplier = 0.50f;
        flickerPhase = 0; // 再び安定フェーズへ
        flickerTimer = 3;
        break;

    default:
        flickerPhase = 0;
        flickerTimer = 60;
        flickerMultiplier = 1.0f;
        break;
    }
}

// ヘッドライトの点灯/消灯切り替え（Fキー）
void Bicycle::ToggleHeadlight()
{
    // バッテリーが完全に空の場合は点灯できない
    if (batteryCurrent <= 0.0f)
    {
        headlightOn = false;
        emptyWarningTimer = 120; // 警告表示
        PlayElectricalSoundHook();
        return;
    }

    headlightOn = !headlightOn;
    PlayElectricalSoundHook();
}

// 高速消費デバッグモードの切り替え（F8キー）
void Bicycle::ToggleDebugFastDrain()
{
    debugFastDrain = !debugFastDrain;
}

// ヘッドライトが点灯しているか
bool Bicycle::IsHeadlightOn() const
{
    return headlightOn && (batteryCurrent > 0.0f);
}

// 現在のバッテリー残量
float Bicycle::GetBatteryCurrent() const
{
    return batteryCurrent;
}

// バッテリー最大容量
float Bicycle::GetBatteryMax() const
{
    return batteryMax;
}

// バッテリー残量パーセント (0.0f〜100.0f)
float Bicycle::GetBatteryPercent() const
{
    if (batteryMax <= 0.0f) return 0.0f;
    float p = (batteryCurrent / batteryMax) * 100.0f;
    if (p < 0.0f) p = 0.0f;
    if (p > 100.0f) p = 100.0f;
    return p;
}

// バッテリーが空かどうか
bool Bicycle::IsBatteryEmpty() const
{
    return (batteryCurrent <= 0.0f);
}

// 高速消費中か
bool Bicycle::IsDebugFastDrain() const
{
    return debugFastDrain;
}

// バッテリー切れ警告表示中か
bool Bicycle::IsEmptyWarningActive() const
{
    return (emptyWarningTimer > 0);
}

// ヘッドライトのワールド位置（ローカルオフセットを自転車角度で回転変換）
VECTOR Bicycle::GetHeadlightPosition() const
{
    // 自転車の前方向と右方向ベクトル
    VECTOR forward = VGet(
        cosf(angle),
        0.0f,
        -sinf(angle)
    );

    VECTOR right = VGet(
        -sinf(angle),
        0.0f,
        -cosf(angle)
    );

    // 自転車の中心から前・横・上へオフセット
    VECTOR p = position;
    p = VAdd(p, VScale(forward, headlightOffsetForward));
    p = VAdd(p, VScale(right, headlightOffsetSide));
    p.y += headlightOffsetHeight;

    return p;
}

// ヘッドライトの照射方向ベクトル（自転車の前方かつわずかに下向き路面方向）
VECTOR Bicycle::GetHeadlightDirection() const
{
    VECTOR forward = VGet(
        cosf(angle),
        0.0f,
        -sinf(angle)
    );

    return VNorm(VGet(
        forward.x,
        -0.16f,
        forward.z
    ));
}

// 現在の輝度倍率（減光・フリッカー反映）
float Bicycle::GetHeadlightIntensity() const
{
    return headlightIntensity;
}

// 照射距離（バッテリー残量によりわずかに低下）
float Bicycle::GetHeadlightRange() const
{
    float pct = GetBatteryPercent();
    if (pct <= 20.0f)
    {
        // クリティカル時は11m〜14mに微小減衰
        return headlightBaseRange * (0.80f + 0.20f * (pct / 20.0f));
    }
    return headlightBaseRange;
}

// ホラーイベント用API: 強制フリッカートリガー
void Bicycle::TriggerHeadlightFlicker()
{
    if (headlightOn && batteryCurrent > 0.0f)
    {
        flickerPhase = 1;
        flickerTimer = 1;
    }
}

// ホラーイベント用API: 一時的消灯トリガー
void Bicycle::TriggerHeadlightBlackout(float durationSeconds)
{
    externalBlackoutTimer = durationSeconds;
}

// ホラーイベント用API: バッテリー消費倍率変更
void Bicycle::SetBatteryDrainMultiplier(float multiplier)
{
    batteryDrainMultiplier = multiplier;
}

// 効果音用フック
void Bicycle::PlayElectricalSoundHook()
{
    // 将来の電気カチッ音・スイッチ音実装用フック
}

// 自転車を描画する
void Bicycle::Draw()
{
    // モデルがない場合
    if (modelHandle == -1)
    {
        return;
    }

    // 自転車モデルを描画する（照明はNightEnvironmentのヘッドライトと環境光が適用される）
    MV1DrawModel(
        modelHandle
    );
}

// UIを描画する（ホラーUIへのプロンプト伝達）
void Bicycle::DrawUI()
{
    // 自転車を選択している場合
    if (canRide == true)
    {
        HorrorUI::Instance().SetPrompt(PromptType::RideBicycle);
    }
    // 自転車に乗っている場合
    else if (isRiding == true)
    {
        HorrorUI::Instance().SetPrompt(PromptType::DismountBicycle);
    }
}

// 自転車を選択可能か取得する
bool Bicycle::CanRide() const
{
    return canRide;
}

// 自転車が移動できるか確認する
bool Bicycle::CanMove(VECTOR nextPosition)
{
    // 自転車の当たり判定サイズ
    float bicycleRadius =
        0.8f;

    // 家との当たり判定
    if (MapCheckWallCollision(nextPosition, bicycleRadius))
    {
        return false;
    }

    // 移動できる
    return true;
}

// 自転車モデルを削除する
void Bicycle::Finalize()
{
    // モデルがある場合
    if (modelHandle != -1)
    {
        // モデルを削除する
        MV1DeleteModel(
            modelHandle
        );

        // ハンドルをリセットする
        modelHandle = -1;
    }
}

// 自転車の状態を初期状態にリセットする
void Bicycle::Reset()
{
    position = VGet(
        1.8f,
        0.18f,
        -30.0f
    );
    angle = DX_PI_F;
    isRiding = false;
    canRide = false;
    oldEKey = false;
    headlightOn = true;
    batteryCurrent = batteryMax;
    batteryDrainMultiplier = 1.0f;
    flickerMultiplier = 1.0f;
    flickerTimer = 180;
    flickerPhase = 0;
    externalBlackoutTimer = 0.0f;
    emptyWarningTimer = 0;
    lastUpdateTime = GetNowHiPerformanceCount();

    if (modelHandle != -1)
    {
        MV1SetPosition(modelHandle, position);
        MV1SetRotationXYZ(modelHandle, VGet(0.0f, angle, 0.0f));
    }
}

// 自転車に乗っているか取得する
bool Bicycle::IsRiding() const
{
    return isRiding;
}

// 自転車の位置を取得する
VECTOR Bicycle::GetPosition() const
{
    return position;
}

// 自転車の向きを取得する
float Bicycle::GetAngle() const
{
    return angle;
}