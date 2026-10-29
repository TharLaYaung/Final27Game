#include "AudioManager.h"
#include <cmath>
#include <vector>
#include <algorithm>

AudioManager& AudioManager::Instance()
{
    static AudioManager instance;
    return instance;
}

AudioManager::AudioManager()
    : masterVolume(80)
    , bgmVolume(70)
    , sfxVolume(85)
    , sndTitleAmbience(-1)
    , sndGameAmbience(-1)
    , sndFootstep(-1)
    , sndBicycleRoll(-1)
    , sndDeliverySuccess(-1)
    , sndBatteryPickup(-1)
    , sndBatteryReload(-1)
    , sndFearHeartbeat(-1)
    , sndMonsterRoar(-1)
    , sndMonsterJumpscare(-1)
    , sndMenuClick(-1)
    , sndMenuAdjust(-1)
    , sndTestPulse(-1)
    , currentBgmType(0)
    , isBicycleRolling(false)
    , footstepCooldown(0.0f)
    , heartbeatTimer(0.0f)
{
}

AudioManager::~AudioManager()
{
    Finalize();
}

int AudioManager::CreateSineWav(float freq, float duration, float decay)
{
    int sampleRate = 44100;
    int numSamples = (int)(sampleRate * duration);
    if (numSamples <= 0) return -1;

    std::vector<char> wav;
    wav.resize(44 + numSamples * 2);

    int dataSize = numSamples * 2;
    int fileSize = 36 + dataSize;

    // WAV RIFFヘッダー
    memcpy(&wav[0], "RIFF", 4);
    memcpy(&wav[4], &fileSize, 4);
    memcpy(&wav[8], "WAVE", 4);
    memcpy(&wav[12], "fmt ", 4);

    int subchunk1Size = 16;
    short audioFormat = 1;
    short numChannels = 1;
    int byteRate = sampleRate * 2;
    short blockAlign = 2;
    short bitsPerSample = 16;

    memcpy(&wav[16], &subchunk1Size, 4);
    memcpy(&wav[20], &audioFormat, 2);
    memcpy(&wav[22], &numChannels, 2);
    memcpy(&wav[24], &sampleRate, 4);
    memcpy(&wav[28], &byteRate, 4);
    memcpy(&wav[32], &blockAlign, 2);
    memcpy(&wav[34], &bitsPerSample, 2);

    memcpy(&wav[36], "data", 4);
    memcpy(&wav[40], &dataSize, 4);

    // PCM波形合成
    short* samples = (short*)&wav[44];
    for (int i = 0; i < numSamples; ++i)
    {
        float t = (float)i / (float)sampleRate;
        float env = std::exp(-t * decay);
        float s = std::sin(2.0f * 3.14159265f * freq * t) * env;
        int val = (int)(s * 32767.0f);
        if (val > 32767) val = 32767;
        if (val < -32767) val = -32767;
        samples[i] = (short)val;
    }

    return LoadSoundMemByMemImage(&wav[0], (int)wav.size());
}

void AudioManager::Initialize()
{
    // 既存ハンドルの安全な破棄
    Finalize();

    // ホラーサウンドファイルの読み込み
    sndTitleAmbience    = LoadSoundMem("Data/Audio/title_ambience.wav");
    sndGameAmbience     = LoadSoundMem("Data/Audio/ambience_horror.wav");
    sndFootstep         = LoadSoundMem("Data/Audio/footstep.wav");
    sndBicycleRoll      = LoadSoundMem("Data/Audio/bicycle_roll.wav");
    sndDeliverySuccess  = LoadSoundMem("Data/Audio/delivery_success.wav");
    sndBatteryPickup    = LoadSoundMem("Data/Audio/battery_pickup.wav");
    sndBatteryReload    = LoadSoundMem("Data/Audio/battery_reload.wav");
    sndFearHeartbeat    = LoadSoundMem("Data/Audio/fear_heartbeat.wav");
    sndMonsterRoar      = LoadSoundMem("Data/Audio/monster_roar_chase.wav");
    sndMonsterJumpscare = LoadSoundMem("Data/Audio/monster_jumpscare.wav");

    // プロシージャルUIサウンドの生成
    sndMenuClick  = CreateSineWav(580.0f, 0.05f, 40.0f);
    sndMenuAdjust = CreateSineWav(440.0f, 0.04f, 50.0f);
    sndTestPulse  = CreateSineWav(1000.0f, 0.12f, 18.0f);

    currentBgmType = 0;
    isBicycleRolling = false;
    footstepCooldown = 0.0f;
    heartbeatTimer = 0.0f;
}

void AudioManager::Finalize()
{
    StopBgm();
    StopBicycleRoll();

    auto SafeDelete = [](int& handle) {
        if (handle != -1)
        {
            DeleteSoundMem(handle);
            handle = -1;
        }
    };

    SafeDelete(sndTitleAmbience);
    SafeDelete(sndGameAmbience);
    SafeDelete(sndFootstep);
    SafeDelete(sndBicycleRoll);
    SafeDelete(sndDeliverySuccess);
    SafeDelete(sndBatteryPickup);
    SafeDelete(sndBatteryReload);
    SafeDelete(sndFearHeartbeat);
    SafeDelete(sndMonsterRoar);
    SafeDelete(sndMonsterJumpscare);
    SafeDelete(sndMenuClick);
    SafeDelete(sndMenuAdjust);
    SafeDelete(sndTestPulse);

    currentBgmType = 0;
}

int AudioManager::CalcBgmVolume() const
{
    int vol = (masterVolume * bgmVolume * 255) / 10000;
    return std::clamp(vol, 0, 255);
}

int AudioManager::CalcSfxVolume(float relativeScale) const
{
    int vol = (int)((masterVolume * sfxVolume * 255 * relativeScale) / 10000.0f);
    return std::clamp(vol, 0, 255);
}

void AudioManager::Update(float dt)
{
    if (footstepCooldown > 0.0f)
    {
        footstepCooldown -= dt;
    }
    if (heartbeatTimer > 0.0f)
    {
        heartbeatTimer -= dt;
    }

    // 現在再生中のBGM音量を動的同期
    int bgmVol = CalcBgmVolume();
    if (currentBgmType == 1 && sndTitleAmbience != -1 && CheckSoundMem(sndTitleAmbience))
    {
        ChangeVolumeSoundMem(bgmVol, sndTitleAmbience);
    }
    else if (currentBgmType == 2 && sndGameAmbience != -1 && CheckSoundMem(sndGameAmbience))
    {
        ChangeVolumeSoundMem(bgmVol, sndGameAmbience);
    }

    // 自転車走行音の音量同期
    if (isBicycleRolling && sndBicycleRoll != -1 && CheckSoundMem(sndBicycleRoll))
    {
        ChangeVolumeSoundMem(CalcSfxVolume(0.65f), sndBicycleRoll);
    }
}

void AudioManager::SetMasterVolume(int vol)
{
    masterVolume = std::clamp(vol, 0, 100);
}

void AudioManager::SetBgmVolume(int vol)
{
    bgmVolume = std::clamp(vol, 0, 100);
}

void AudioManager::SetSfxVolume(int vol)
{
    sfxVolume = std::clamp(vol, 0, 100);
}

void AudioManager::PlayTitleBgm()
{
    if (currentBgmType == 1 && sndTitleAmbience != -1 && CheckSoundMem(sndTitleAmbience))
    {
        return; // 既に再生中
    }

    if (currentBgmType == 2 && sndGameAmbience != -1)
    {
        StopSoundMem(sndGameAmbience);
    }

    if (sndTitleAmbience != -1)
    {
        ChangeVolumeSoundMem(CalcBgmVolume(), sndTitleAmbience);
        PlaySoundMem(sndTitleAmbience, DX_PLAYTYPE_LOOP);
        currentBgmType = 1;
    }
}

void AudioManager::PlayGameAmbienceBgm()
{
    if (currentBgmType == 2 && sndGameAmbience != -1 && CheckSoundMem(sndGameAmbience))
    {
        return; // 既に再生中
    }

    if (currentBgmType == 1 && sndTitleAmbience != -1)
    {
        StopSoundMem(sndTitleAmbience);
    }

    if (sndGameAmbience != -1)
    {
        ChangeVolumeSoundMem(CalcBgmVolume(), sndGameAmbience);
        PlaySoundMem(sndGameAmbience, DX_PLAYTYPE_LOOP);
        currentBgmType = 2;
    }
}

void AudioManager::StopBgm()
{
    if (sndTitleAmbience != -1 && CheckSoundMem(sndTitleAmbience))
    {
        StopSoundMem(sndTitleAmbience);
    }
    if (sndGameAmbience != -1 && CheckSoundMem(sndGameAmbience))
    {
        StopSoundMem(sndGameAmbience);
    }
    currentBgmType = 0;
}

void AudioManager::PlayFootstep()
{
    if (footstepCooldown > 0.0f || sndFootstep == -1)
    {
        return;
    }

    ChangeVolumeSoundMem(CalcSfxVolume(0.85f), sndFootstep);
    PlaySoundMem(sndFootstep, DX_PLAYTYPE_BACK);
    footstepCooldown = 0.34f;
}

void AudioManager::PlayBicycleRoll(bool isMoving)
{
    if (sndBicycleRoll == -1) return;

    if (isMoving)
    {
        if (!isBicycleRolling || !CheckSoundMem(sndBicycleRoll))
        {
            ChangeVolumeSoundMem(CalcSfxVolume(0.65f), sndBicycleRoll);
            PlaySoundMem(sndBicycleRoll, DX_PLAYTYPE_LOOP);
            isBicycleRolling = true;
        }
    }
    else
    {
        if (isBicycleRolling)
        {
            StopSoundMem(sndBicycleRoll);
            isBicycleRolling = false;
        }
    }
}

void AudioManager::StopBicycleRoll()
{
    if (sndBicycleRoll != -1 && CheckSoundMem(sndBicycleRoll))
    {
        StopSoundMem(sndBicycleRoll);
    }
    isBicycleRolling = false;
}

void AudioManager::PlayDeliverySuccess()
{
    if (sndDeliverySuccess != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(1.0f), sndDeliverySuccess);
        PlaySoundMem(sndDeliverySuccess, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayBatteryPickup()
{
    if (sndBatteryPickup != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(0.95f), sndBatteryPickup);
        PlaySoundMem(sndBatteryPickup, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayBatteryReload()
{
    if (sndBatteryReload != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(1.0f), sndBatteryReload);
        PlaySoundMem(sndBatteryReload, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayHeartbeat(float fearPercent)
{
    if (fearPercent <= 0.30f || sndFearHeartbeat == -1)
    {
        return;
    }

    // 恐怖度が高いほど鼓動の間隔が短縮（1.15秒〜0.45秒）
    float interval = 1.15f - (fearPercent * 0.70f);
    if (heartbeatTimer <= 0.0f)
    {
        heartbeatTimer = interval;
        float relativeVol = 0.4f + (fearPercent * 0.6f);
        ChangeVolumeSoundMem(CalcSfxVolume(relativeVol), sndFearHeartbeat);
        PlaySoundMem(sndFearHeartbeat, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayMonsterRoar()
{
    if (sndMonsterRoar != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(1.0f), sndMonsterRoar);
        PlaySoundMem(sndMonsterRoar, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayJumpscare()
{
    if (sndMonsterJumpscare != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(1.0f), sndMonsterJumpscare);
        PlaySoundMem(sndMonsterJumpscare, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayMenuClick()
{
    if (sndMenuClick != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(0.9f), sndMenuClick);
        PlaySoundMem(sndMenuClick, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayMenuAdjust()
{
    if (sndMenuAdjust != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(0.9f), sndMenuAdjust);
        PlaySoundMem(sndMenuAdjust, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayTestAudioPulse()
{
    if (sndTestPulse != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(1.0f), sndTestPulse);
        PlaySoundMem(sndTestPulse, DX_PLAYTYPE_BACK);
    }
}
