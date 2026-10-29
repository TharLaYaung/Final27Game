#pragma once

#include "DxLib.h"

// 統合ホラー音響システム
class AudioManager
{
public:
    static AudioManager& Instance();

    // 初期化（サウンド読み込み・音量初期設定）
    void Initialize();

    // 終了処理（サウンドハンドルの解放）
    void Finalize();

    // 毎フレーム更新（BGMループ制御・音量反映・動的環境演出）
    void Update(float dt);

    // 音量設定（0〜100%）
    void SetMasterVolume(int vol);
    void SetBgmVolume(int vol);
    void SetSfxVolume(int vol);

    int GetMasterVolume() const { return masterVolume; }
    int GetBgmVolume() const { return bgmVolume; }
    int GetSfxVolume() const { return sfxVolume; }

    float GetMasterVolumeRate() const { return masterVolume / 100.0f; }
    float GetBgmVolumeRate() const { return (masterVolume / 100.0f) * (bgmVolume / 100.0f); }
    float GetSfxVolumeRate() const { return (masterVolume / 100.0f) * (sfxVolume / 100.0f); }

    // BGM制御
    void PlayTitleBgm();
    void PlayGameAmbienceBgm();
    void StopBgm();

    // SE再生メソッド群
    void PlayFootstep();
    void PlayBicycleRoll(bool isMoving);
    void StopBicycleRoll();
    void PlayDeliverySuccess();
    void PlayBatteryPickup();
    void PlayBatteryReload();
    void PlayHeartbeat(float fearPercent);
    void PlayMonsterRoar();
    void PlayJumpscare();

    // UI効果音
    void PlayMenuClick();
    void PlayMenuAdjust();
    void PlayTestAudioPulse();

private:
    AudioManager();
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    int masterVolume;
    int bgmVolume;
    int sfxVolume;

    // サウンドハンドル
    int sndTitleAmbience;
    int sndGameAmbience;
    int sndFootstep;
    int sndBicycleRoll;
    int sndDeliverySuccess;
    int sndBatteryPickup;
    int sndBatteryReload;
    int sndFearHeartbeat;
    int sndMonsterRoar;
    int sndMonsterJumpscare;

    // UI効果音ハンドル
    int sndMenuClick;
    int sndMenuAdjust;
    int sndTestPulse;

    // 再生状態管理
    int currentBgmType; // 0: None, 1: Title, 2: Game
    bool isBicycleRolling;
    float footstepCooldown;
    float heartbeatTimer;

    // 実効音量計算ヘルパー
    int CalcBgmVolume() const;
    int CalcSfxVolume(float relativeScale = 1.0f) const;

    // プロシージャルUIサウンド生成
    int CreateSineWav(float freq, float duration, float decay);
};
