#pragma once

#include "DxLib.h"

// 前方宣言
class Player;
class Newspaper;

// メールボックス（郵便ポスト）を管理するクラス
class Mailbox
{
public:

    // コンストラクタ
    Mailbox();

    // デストラクタ
    ~Mailbox();

    // 初期化処理（モデル読み込みや初期設定）
    bool Initialize();

    // 更新処理（プレイヤーや新聞の状態をもとに配達判定を行う）
    void Update(Player& player, Newspaper& newspaper);

    // 描画処理（ポストの3Dモデルを描画する）
    void Draw();

    // UI描画処理（配達案内や配達完了メッセージを表示する）
    void DrawUI();

    // 終了処理（モデルのメモリ解放）
    void Finalize();

    // ポストの位置を取得する
    VECTOR GetPosition() const;

    // 配達済みかどうかを取得する
    bool IsDelivered() const;

private:

    // メールボックスの3Dモデルハンドル
    int modelHandle;

    // メールボックスの配置位置
    VECTOR position;

    // メールボックスの向き（Y軸回転角度）
    float angle;

    // メールボックスの表示スケール
    float scale;

    // 配達可能な状態かどうか（範囲内かつ注視中）
    bool canDeliver;

    // すでに配達が完了したかどうか
    bool delivered;

    // 前フレームのマウス左クリック状態
    bool oldLeftClick;

    // 配達完了メッセージ（Delivered!）を表示する残り時間
    int messageTimer;
};