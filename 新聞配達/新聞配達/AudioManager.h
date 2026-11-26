#pragma once

#include "DxLib.h"

// 空間音響付き環境・心霊ホラーイベント効果音種別
enum class HorrorEventSfx
{
    WomanCrying = 0,
    DistantScream,
    PhantomFootsteps,
    BabyCrying,
    CreepyLaugh,
    CloseWhisper,
    HeavyBreathing,
    DoorKnock,
    StaticVoice,
    MetalScrape,
    Count
};

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

    // 背景音楽の制御
    void PlayTitleBgm();
    void PlayLoadingBgm();
    void PlayClearWindAmbience();
    void PlayGameAmbienceBgm();
    void StopBgm();
    void SetPausedDucking(bool paused);
    void SetHorrorTension(float chaseTension);
    void SetRainAmbience(float rainLevel);
    void SetTitleWindLevel(float windLevel);

    // 効果音の再生機能
    void PlayFootstep();
    void PlayBicycleRoll(bool isMoving);
    void StopBicycleRoll();
    void PlayHeadlightSwitch(bool hasBattery);
    void PlayNewspaperPickup();
    void PlayDeliverySuccess();
    void PlayBatteryPickup();
    void PlayBatteryReload();
    void PlayHeartbeat(float fearPercent);
    void PlayMonsterRoar();
    void PlayZombieScream();
    void PlayJumpscare();
    void PlayGameClearSting();
    void PlayGameOverSting();

    // 3D位置に応じた左右パン＆距離減衰付きホラーイベント音声再生
    void PlaySpatialHorrorEvent(
        HorrorEventSfx kind,
        const VECTOR& listenerPos,
        const VECTOR& listenerFwd,
        const VECTOR& sourcePos,
        float maxRange = 28.0f,
        float gainScale = 1.0f
    );
    void StopAllHorrorEvents();
    bool IsHorrorEventSfxLoaded(HorrorEventSfx kind) const;

    // 画面表示＆タイトル・ローディング用の効果音
    void PlayMenuClick();
    void PlayMenuHover();
    void PlayMenuAdjust();
    void PlayTestAudioPulse();
    void PlayTitleConfirm();
    void PlayLampFlickerBuzz(int pan = -55);
    void PlayTapeInsert();
    void PlayLoadingStaticBurst();
    void PlayLoadingComplete();
    bool AreSceneAudioAssetsLoaded() const;

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
    int sndLoadingBgm;
    int sndWindAmbience;
    int sndGameAmbience;
    int sndTensionLoop;
    int sndRainAmbience;
    int sndFootstep;
    int sndBicycleRoll;
    int sndHeadlightSwitch;
    int sndNewspaperPickup;
    int sndDeliverySuccess;
    int sndBatteryPickup;
    int sndBatteryReload;
    int sndFearHeartbeat;
    int sndMonsterRoar;
    int sndZombieScream;
    int sndMonsterJumpscare;
    int sndGameClearSting;
    int sndGameOverSting;
    int sndHorrorEvents[static_cast<int>(HorrorEventSfx::Count)];

    // 画面表示＆シーン演出用効果音の識別子
    int sndMenuClick;
    int sndMenuHover;
    int sndMenuAdjust;
    int sndTestPulse;
    int sndTitleConfirm;
    int sndLampBuzz;
    int sndTapeInsert;
    int sndTapeGlitch;
    int sndTapeLock;

    // 再生状態管理
    int currentBgmType; // 0: None, 1: Title/Menu, 2: Game, 3: Loading, 4: ClearWind
    bool isBicycleRolling;
    bool isPausedDuck;
    bool footstepLeftSide;
    float footstepCooldown;
    float heartbeatTimer;
    float targetTensionMix;
    float currentTensionMix;
    float targetRainMix;
    float currentRainMix;
    float targetWindMix;
    float currentWindMix;
    float roarCooldown;

    // 実効音量計算ヘルパー
    int CalcBgmVolume(float relativeScale = 1.0f) const;
    int CalcSfxVolume(float relativeScale = 1.0f) const;

    // ホラー調ステレオアナログUIサウンド生成 (0: Click, 1: Adjust, 2: Pulse, 3: Hover)
    int CreateHorrorUiSfx(int kind);
};
