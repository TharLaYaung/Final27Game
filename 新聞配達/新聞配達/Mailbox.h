#pragma once

#include "DxLib.h"
#include <vector>

// 前方宣言
class Player;
class Newspaper;
class DeliveryManager;

// メールボックス（郵便ポスト群）管理クラス
class Mailbox
{
public:

    // コンストラクタ
    Mailbox();

    // デストラクタ
    ~Mailbox();

    // 初期化（ポスト3Dモデルの読み込み）
    bool Initialize();

    // 更新処理（プレイヤーの接近・照準・配達判定）
    void Update(Player& player, Newspaper& newspaper, DeliveryManager& deliveryManager);

    // 描画処理（全8箇所のポスト3Dモデル描画）
    void Draw(const DeliveryManager& deliveryManager);

    // UI描画処理（「Left Click : Deliver Newspaper」の案内表示）
    void DrawUI(const DeliveryManager& deliveryManager);

    // 終了処理
    void Finalize();

    // 現在プレイヤーが注視しているポストID（0なら注視なし）
    int GetAimedMailboxId() const;

    // 配達可能状態か取得する
    bool CanDeliver() const;

private:

    // メールボックス3Dモデルハンドル
    int modelHandle;

    // 表示スケール
    float scale;

    // 配達可能な状態か（接近してポストを見ている）
    bool canDeliver;

    // 現在注視中のポストID (1〜8, 0は注視なし)
    int aimedMailboxId;

    // 前フレームのマウスクリック状態（エッジトリガー検出用）
    bool oldLeftClick;
};