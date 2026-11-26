#include "AudioManager.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace
{
    constexpr float AUDIO_PI = 3.14159265358979323846f;

    int LoadSoundWithFallback(const char* relativePath)
    {
        int handle = LoadSoundMem(relativePath);
        if (handle != -1) return handle;

        char buf[512];
        snprintf(buf, sizeof(buf), "新聞配達/%s", relativePath);
        handle = LoadSoundMem(buf);
        if (handle != -1) return handle;

        snprintf(buf, sizeof(buf), "../新聞配達/%s", relativePath);
        handle = LoadSoundMem(buf);
        if (handle != -1) return handle;

        snprintf(buf, sizeof(buf), "../%s", relativePath);
        handle = LoadSoundMem(buf);
        return handle;
    }

    int BuildStereoWavHandle(const std::vector<float>& left, const std::vector<float>& right)
    {
        const int sampleRate = 44100;
        const int numFrames = static_cast<int>(left.size());
        if (numFrames <= 0 || right.size() != left.size()) return -1;

        const int dataSize = numFrames * 4;
        const int fileSize = 36 + dataSize;
        std::vector<char> wav(44 + dataSize, 0);

        memcpy(&wav[0], "RIFF", 4);
        memcpy(&wav[4], &fileSize, 4);
        memcpy(&wav[8], "WAVE", 4);
        memcpy(&wav[12], "fmt ", 4);

        const int subchunk1Size = 16;
        const short audioFormat = 1;
        const short numChannels = 2;
        const int byteRate = sampleRate * 4;
        const short blockAlign = 4;
        const short bitsPerSample = 16;

        memcpy(&wav[16], &subchunk1Size, 4);
        memcpy(&wav[20], &audioFormat, 2);
        memcpy(&wav[22], &numChannels, 2);
        memcpy(&wav[24], &sampleRate, 4);
        memcpy(&wav[28], &byteRate, 4);
        memcpy(&wav[32], &blockAlign, 2);
        memcpy(&wav[34], &bitsPerSample, 2);
        memcpy(&wav[36], "data", 4);
        memcpy(&wav[40], &dataSize, 4);

        short* pcm = reinterpret_cast<short*>(&wav[44]);
        for (int i = 0; i < numFrames; ++i)
        {
            float sl = std::clamp(left[i], -1.0f, 1.0f);
            float sr = std::clamp(right[i], -1.0f, 1.0f);
            pcm[i * 2] = static_cast<short>(sl * 32767.0f);
            pcm[i * 2 + 1] = static_cast<short>(sr * 32767.0f);
        }

        return LoadSoundMemByMemImage(wav.data(), static_cast<int>(wav.size()));
    }
}

AudioManager& AudioManager::Instance()
{
    static AudioManager instance;
    return instance;
}

AudioManager::AudioManager()
    : masterVolume(95)
    , bgmVolume(90)
    , sfxVolume(90)
    , sndTitleAmbience(-1)
    , sndLoadingBgm(-1)
    , sndWindAmbience(-1)
    , sndGameAmbience(-1)
    , sndTensionLoop(-1)
    , sndRainAmbience(-1)
    , sndFootstep(-1)
    , sndBicycleRoll(-1)
    , sndHeadlightSwitch(-1)
    , sndNewspaperPickup(-1)
    , sndDeliverySuccess(-1)
    , sndBatteryPickup(-1)
    , sndBatteryReload(-1)
    , sndFearHeartbeat(-1)
    , sndMonsterRoar(-1)
    , sndZombieScream(-1)
    , sndMonsterJumpscare(-1)
    , sndGameClearSting(-1)
    , sndGameOverSting(-1)
    , sndMenuClick(-1)
    , sndMenuHover(-1)
    , sndMenuAdjust(-1)
    , sndTestPulse(-1)
    , sndTitleConfirm(-1)
    , sndLampBuzz(-1)
    , sndTapeInsert(-1)
    , sndTapeGlitch(-1)
    , sndTapeLock(-1)
    , currentBgmType(0)
    , isBicycleRolling(false)
    , isPausedDuck(false)
    , footstepLeftSide(false)
    , footstepCooldown(0.0f)
    , heartbeatTimer(0.0f)
    , targetTensionMix(0.0f)
    , currentTensionMix(0.0f)
    , targetRainMix(0.0f)
    , currentRainMix(0.0f)
    , targetWindMix(0.0f)
    , currentWindMix(0.0f)
    , roarCooldown(0.0f)
{
    for (int i = 0; i < static_cast<int>(HorrorEventSfx::Count); ++i)
    {
        sndHorrorEvents[i] = -1;
    }
}

AudioManager::~AudioManager()
{
    Finalize();
}

int AudioManager::CreateHorrorUiSfx(int kind)
{
    const int sr = 44100;
    const float duration = (kind == 2) ? 0.36f : (kind == 0 ? 0.11f : (kind == 3 ? 0.036f : 0.08f));
    const int n = static_cast<int>(sr * duration);
    if (n <= 0) return -1;

    std::vector<float> l(n, 0.0f);
    std::vector<float> r(n, 0.0f);
    uint32_t rng = 0x9E3779B9u + static_cast<uint32_t>(kind * 131);
    auto nextSigned = [&]() {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return (static_cast<float>(rng & 0xFFFFu) / 32768.0f) - 1.0f;
    };

    for (int i = 0; i < n; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(sr);
        float sL = 0.0f;
        float sR = 0.0f;

        if (kind == 0)
        {
            // 重厚なアナログスイッチ音＋微細テープ過渡音
            float click = (std::sin(2.0f * AUDIO_PI * 1950.0f * t) * 0.40f + nextSigned() * 0.35f) * std::exp(-t * 210.0f);
            float body = std::sin(2.0f * AUDIO_PI * (125.0f - 45.0f * (t / duration)) * t) * std::exp(-t * 42.0f) * 0.55f;
            float tap2 = (t > 0.024f)
                ? std::sin(2.0f * AUDIO_PI * 640.0f * (t - 0.024f)) * std::exp(-(t - 0.024f) * 140.0f) * 0.28f
                : 0.0f;
            sL = click * 0.92f + body + tap2 * 1.05f;
            sR = click * 1.05f + body + tap2 * 0.92f;
        }
        else if (kind == 1)
        {
            // VCR/ラジオダイヤル刻み音
            float tick = (std::sin(2.0f * AUDIO_PI * 520.0f * t) * 0.55f + std::sin(2.0f * AUDIO_PI * 1320.0f * t) * 0.30f + nextSigned() * 0.20f)
                       * std::exp(-t * 95.0f);
            sL = tick * 0.96f;
            sR = tick * 1.04f;
        }
        else if (kind == 3)
        {
            // ホバー時の控えめなリレー接点微振動音
            float hoverTick = (std::sin(2.0f * AUDIO_PI * 1480.0f * t) * 0.30f + nextSigned() * 0.24f)
                            * std::exp(-t * 165.0f);
            sL = hoverTick * 0.55f;
            sR = hoverTick * 0.50f;
        }
        else
        {
            // 音量テスト用シネマティックVHS低音パルス＋冷たい倍音
            float att = (std::min)(1.0f, t / 0.012f);
            float env = att * std::exp(-t * 7.5f);
            float sub = std::sin(2.0f * AUDIO_PI * 82.41f * t) * 0.52f + std::sin(2.0f * AUDIO_PI * 123.47f * t) * 0.28f;
            float chime = std::sin(2.0f * AUDIO_PI * 329.63f * t) * std::exp(-t * 11.0f) * 0.22f;
            sL = (sub + chime * 1.12f) * env;
            sR = (sub + chime * 0.88f) * env;
        }

        float fadeIn = (std::min)(1.0f, t / 0.002f);
        float fadeOut = (std::min)(1.0f, (duration - t) / 0.018f);
        float w = fadeIn * (std::max)(0.0f, fadeOut);
        l[i] = std::tanh(sL * w * 1.1f) * 0.78f;
        r[i] = std::tanh(sR * w * 1.1f) * 0.78f;
    }

    return BuildStereoWavHandle(l, r);
}

void AudioManager::Initialize()
{
    Finalize();

    // ホラーサウンドファイルの読み込み
    sndTitleAmbience    = LoadSoundWithFallback("Data/Audio/title_horror_bgm.wav");
    if (sndTitleAmbience == -1) sndTitleAmbience = LoadSoundWithFallback("Data/Audio/title_ambience.wav");
    sndLoadingBgm       = LoadSoundWithFallback("Data/Audio/loading_tape_bgm.wav");
    sndWindAmbience     = LoadSoundWithFallback("Data/Audio/AMB_Wind_Moderate_Loop_01.wav");
    sndGameAmbience     = LoadSoundWithFallback("Data/Audio/ambience_horror.wav");
    sndTensionLoop      = LoadSoundWithFallback("Data/Audio/tension_chase_loop.wav");
    sndRainAmbience     = LoadSoundWithFallback("Data/Audio/rain_ambience.wav");
    sndFootstep         = LoadSoundWithFallback("Data/Audio/footstep.wav");
    sndBicycleRoll      = LoadSoundWithFallback("Data/Audio/bicycle_roll.wav");
    sndHeadlightSwitch  = LoadSoundWithFallback("Data/Audio/headlight_switch.wav");
    sndNewspaperPickup  = LoadSoundWithFallback("Data/Audio/newspaper_pickup.wav");
    sndDeliverySuccess  = LoadSoundWithFallback("Data/Audio/delivery_success.wav");
    sndBatteryPickup    = LoadSoundWithFallback("Data/Audio/battery_pickup.wav");
    sndBatteryReload    = LoadSoundWithFallback("Data/Audio/battery_reload.wav");
    sndFearHeartbeat    = LoadSoundWithFallback("Data/Audio/fear_heartbeat.wav");
    sndMonsterRoar      = LoadSoundWithFallback("Data/Audio/monster_roar_chase.wav");
    sndZombieScream     = LoadSoundWithFallback("Data/Audio/zombie_scream.wav");
    sndMonsterJumpscare = LoadSoundWithFallback("Data/Audio/monster_jumpscare.wav");
    sndGameClearSting   = LoadSoundWithFallback("Data/Audio/game_clear_sting.wav");
    sndGameOverSting    = LoadSoundWithFallback("Data/Audio/game_over_sting.wav");

    // 空間音響ホラーイベント効果音の読み込み
    static const char* kHorrorEventPaths[static_cast<int>(HorrorEventSfx::Count)] = {
        "Data/Audio/horror_woman_crying.wav",
        "Data/Audio/horror_distant_scream.wav",
        "Data/Audio/horror_phantom_footsteps.wav",
        "Data/Audio/horror_baby_crying.wav",
        "Data/Audio/horror_creepy_laugh.wav",
        "Data/Audio/horror_whisper.wav",
        "Data/Audio/horror_heavy_breathing.wav",
        "Data/Audio/horror_door_knock.wav",
        "Data/Audio/horror_static_voice.wav",
        "Data/Audio/horror_metal_scrape.wav"
    };
    for (int i = 0; i < static_cast<int>(HorrorEventSfx::Count); ++i)
    {
        sndHorrorEvents[i] = LoadSoundWithFallback(kHorrorEventPaths[i]);
    }

    // 不気味なアナログVHS調UI・シーン演出サウンドの読み込み＆生成
    sndTitleConfirm = LoadSoundWithFallback("Data/Audio/title_confirm_sting.wav");
    sndLampBuzz     = LoadSoundWithFallback("Data/Audio/title_lamp_buzz.wav");
    sndTapeInsert   = LoadSoundWithFallback("Data/Audio/tape_insert.wav");
    sndTapeGlitch   = LoadSoundWithFallback("Data/Audio/tape_seek_glitch.wav");
    sndTapeLock     = LoadSoundWithFallback("Data/Audio/tape_lock_ready.wav");

    sndMenuClick  = CreateHorrorUiSfx(0);
    sndMenuHover  = CreateHorrorUiSfx(3);
    sndMenuAdjust = CreateHorrorUiSfx(1);
    sndTestPulse  = CreateHorrorUiSfx(2);

    currentBgmType = 0;
    isBicycleRolling = false;
    isPausedDuck = false;
    footstepLeftSide = false;
    footstepCooldown = 0.0f;
    heartbeatTimer = 0.0f;
    targetTensionMix = 0.0f;
    currentTensionMix = 0.0f;
    targetRainMix = 0.0f;
    currentRainMix = 0.0f;
    targetWindMix = 0.0f;
    currentWindMix = 0.0f;
    roarCooldown = 0.0f;
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
    SafeDelete(sndLoadingBgm);
    SafeDelete(sndWindAmbience);
    SafeDelete(sndGameAmbience);
    SafeDelete(sndTensionLoop);
    SafeDelete(sndRainAmbience);
    SafeDelete(sndFootstep);
    SafeDelete(sndBicycleRoll);
    SafeDelete(sndHeadlightSwitch);
    SafeDelete(sndNewspaperPickup);
    SafeDelete(sndDeliverySuccess);
    SafeDelete(sndBatteryPickup);
    SafeDelete(sndBatteryReload);
    SafeDelete(sndFearHeartbeat);
    SafeDelete(sndMonsterRoar);
    SafeDelete(sndZombieScream);
    SafeDelete(sndMonsterJumpscare);
    SafeDelete(sndGameClearSting);
    SafeDelete(sndGameOverSting);
    SafeDelete(sndMenuClick);
    SafeDelete(sndMenuHover);
    SafeDelete(sndMenuAdjust);
    SafeDelete(sndTestPulse);
    SafeDelete(sndTitleConfirm);
    SafeDelete(sndLampBuzz);
    SafeDelete(sndTapeInsert);
    SafeDelete(sndTapeGlitch);
    SafeDelete(sndTapeLock);
    for (int i = 0; i < static_cast<int>(HorrorEventSfx::Count); ++i)
    {
        SafeDelete(sndHorrorEvents[i]);
    }

    currentBgmType = 0;
}

int AudioManager::CalcBgmVolume(float relativeScale) const
{
    if (masterVolume <= 0 || bgmVolume <= 0) return 0;
    int vol = static_cast<int>((static_cast<float>(masterVolume) * static_cast<float>(bgmVolume) * 255.0f * relativeScale) / 10000.0f);
    return std::clamp(vol, 0, 255);
}

int AudioManager::CalcSfxVolume(float relativeScale) const
{
    if (masterVolume <= 0 || sfxVolume <= 0) return 0;
    int vol = static_cast<int>((static_cast<float>(masterVolume) * static_cast<float>(sfxVolume) * 255.0f * relativeScale) / 10000.0f);
    return std::clamp(vol, 0, 255);
}

void AudioManager::Update(float dt)
{
    if (footstepCooldown > 0.0f) footstepCooldown -= dt;
    if (heartbeatTimer > 0.0f) heartbeatTimer -= dt;
    if (roarCooldown > 0.0f) roarCooldown -= dt;

    // 追跡緊張レイヤーの滑らかなクロスフェード補間
    float tensionSpeed = (targetTensionMix > currentTensionMix) ? 2.8f : 0.75f;
    currentTensionMix += (targetTensionMix - currentTensionMix) * (std::min)(1.0f, dt * tensionSpeed);
    if (currentTensionMix < 0.005f) currentTensionMix = 0.0f;

    // 雨天環境音レイヤーの滑らかな補間
    currentRainMix += (targetRainMix - currentRainMix) * (std::min)(1.0f, dt * 1.4f);
    if (currentRainMix < 0.005f) currentRainMix = 0.0f;

    // 深夜風環境音レイヤーの滑らかな補間
    currentWindMix += (targetWindMix - currentWindMix) * (std::min)(1.0f, dt * 1.8f);
    if (currentWindMix < 0.005f) currentWindMix = 0.0f;

    float pauseDuckScale = isPausedDuck ? 0.42f : 1.0f;

    // 基本BGM音量同期
    if (currentBgmType == 1 && sndTitleAmbience != -1 && CheckSoundMem(sndTitleAmbience))
    {
        ChangeVolumeSoundMem(CalcBgmVolume(pauseDuckScale), sndTitleAmbience);
    }
    else if (currentBgmType == 2 && sndGameAmbience != -1 && CheckSoundMem(sndGameAmbience))
    {
        float ambDuck = 1.0f - currentTensionMix * 0.30f;
        ChangeVolumeSoundMem(CalcBgmVolume(pauseDuckScale * ambDuck), sndGameAmbience);
    }
    else if (currentBgmType == 3 && sndLoadingBgm != -1 && CheckSoundMem(sndLoadingBgm))
    {
        ChangeVolumeSoundMem(CalcBgmVolume(0.88f), sndLoadingBgm);
    }

    // 動的追跡・緊迫レイヤー制御（ゲーム中のみ）
    if (sndTensionLoop != -1)
    {
        if (currentBgmType == 2 && currentTensionMix > 0.01f && !isPausedDuck)
        {
            int tVol = CalcBgmVolume(0.88f * currentTensionMix);
            if (!CheckSoundMem(sndTensionLoop))
            {
                ChangeVolumeSoundMem(tVol, sndTensionLoop);
                PlaySoundMem(sndTensionLoop, DX_PLAYTYPE_LOOP);
            }
            else
            {
                ChangeVolumeSoundMem(tVol, sndTensionLoop);
            }
        }
        else if (CheckSoundMem(sndTensionLoop))
        {
            if (currentBgmType != 2 || currentTensionMix <= 0.01f)
            {
                StopSoundMem(sndTensionLoop);
            }
            else
            {
                ChangeVolumeSoundMem(CalcBgmVolume(0.30f * currentTensionMix), sndTensionLoop);
            }
        }
    }

    // 雨天環境レイヤー制御（タイトル3D背景およびゲーム本編）
    if (sndRainAmbience != -1)
    {
        if ((currentBgmType == 2 || currentBgmType == 1) && currentRainMix > 0.01f)
        {
            float scale = (currentBgmType == 1) ? (0.42f * currentRainMix) : (0.75f * currentRainMix * pauseDuckScale);
            int rVol = CalcBgmVolume(scale);
            if (!CheckSoundMem(sndRainAmbience))
            {
                ChangeVolumeSoundMem(rVol, sndRainAmbience);
                PlaySoundMem(sndRainAmbience, DX_PLAYTYPE_LOOP);
            }
            else
            {
                ChangeVolumeSoundMem(rVol, sndRainAmbience);
            }
        }
        else if (CheckSoundMem(sndRainAmbience))
        {
            StopSoundMem(sndRainAmbience);
        }
    }

    // 風環境音レイヤー制御（タイトル路地・クリア余韻）
    if (sndWindAmbience != -1)
    {
        if ((currentBgmType == 1 || currentBgmType == 4) && currentWindMix > 0.01f)
        {
            int wVol = CalcBgmVolume(0.55f * currentWindMix);
            if (!CheckSoundMem(sndWindAmbience))
            {
                ChangeVolumeSoundMem(wVol, sndWindAmbience);
                PlaySoundMem(sndWindAmbience, DX_PLAYTYPE_LOOP);
            }
            else
            {
                ChangeVolumeSoundMem(wVol, sndWindAmbience);
            }
        }
        else if (CheckSoundMem(sndWindAmbience))
        {
            StopSoundMem(sndWindAmbience);
        }
    }

    // 自転車走行音の音量同期
    if (isBicycleRolling && sndBicycleRoll != -1 && CheckSoundMem(sndBicycleRoll))
    {
        if (isPausedDuck || currentBgmType != 2)
        {
            StopBicycleRoll();
        }
        else
        {
            ChangeVolumeSoundMem(CalcSfxVolume(0.70f), sndBicycleRoll);
        }
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

void AudioManager::SetPausedDucking(bool paused)
{
    isPausedDuck = paused;
    if (paused)
    {
        StopBicycleRoll();
        StopAllHorrorEvents();
    }
}

void AudioManager::SetHorrorTension(float chaseTension)
{
    targetTensionMix = std::clamp(chaseTension, 0.0f, 1.0f);
}

void AudioManager::SetRainAmbience(float rainLevel)
{
    targetRainMix = std::clamp(rainLevel, 0.0f, 1.0f);
}

void AudioManager::SetTitleWindLevel(float windLevel)
{
    targetWindMix = std::clamp(windLevel, 0.0f, 1.0f);
}

void AudioManager::PlayTitleBgm()
{
    if (sndTitleAmbience == -1)
    {
        sndTitleAmbience = LoadSoundWithFallback("Data/Audio/title_horror_bgm.wav");
        if (sndTitleAmbience == -1) sndTitleAmbience = LoadSoundWithFallback("Data/Audio/title_ambience.wav");
    }
    if (sndTitleAmbience == -1) return;

    if (currentBgmType == 1 && CheckSoundMem(sndTitleAmbience))
    {
        return;
    }

    if (sndGameAmbience != -1 && CheckSoundMem(sndGameAmbience)) StopSoundMem(sndGameAmbience);
    if (sndLoadingBgm != -1 && CheckSoundMem(sndLoadingBgm)) StopSoundMem(sndLoadingBgm);
    if (sndTensionLoop != -1 && CheckSoundMem(sndTensionLoop)) StopSoundMem(sndTensionLoop);
    if (sndRainAmbience != -1 && CheckSoundMem(sndRainAmbience)) StopSoundMem(sndRainAmbience);
    targetTensionMix = 0.0f;
    currentTensionMix = 0.0f;
    targetRainMix = 0.0f;
    currentRainMix = 0.0f;

    int vol = CalcBgmVolume();
    ChangeVolumeSoundMem(vol, sndTitleAmbience);
    PlaySoundMem(sndTitleAmbience, DX_PLAYTYPE_LOOP);
    ChangeVolumeSoundMem(vol, sndTitleAmbience);
    currentBgmType = 1;
}

void AudioManager::PlayLoadingBgm()
{
    if (sndLoadingBgm == -1)
    {
        sndLoadingBgm = LoadSoundWithFallback("Data/Audio/loading_tape_bgm.wav");
    }
    if (currentBgmType == 3 && sndLoadingBgm != -1 && CheckSoundMem(sndLoadingBgm))
    {
        return;
    }

    if (sndTitleAmbience != -1 && CheckSoundMem(sndTitleAmbience)) StopSoundMem(sndTitleAmbience);
    if (sndGameAmbience != -1 && CheckSoundMem(sndGameAmbience)) StopSoundMem(sndGameAmbience);
    if (sndTensionLoop != -1 && CheckSoundMem(sndTensionLoop)) StopSoundMem(sndTensionLoop);
    if (sndRainAmbience != -1 && CheckSoundMem(sndRainAmbience)) StopSoundMem(sndRainAmbience);
    if (sndWindAmbience != -1 && CheckSoundMem(sndWindAmbience)) StopSoundMem(sndWindAmbience);
    targetTensionMix = 0.0f;
    currentTensionMix = 0.0f;
    targetRainMix = 0.0f;
    currentRainMix = 0.0f;
    targetWindMix = 0.0f;
    currentWindMix = 0.0f;

    if (sndLoadingBgm != -1)
    {
        int vol = CalcBgmVolume(0.88f);
        ChangeVolumeSoundMem(vol, sndLoadingBgm);
        PlaySoundMem(sndLoadingBgm, DX_PLAYTYPE_LOOP);
        ChangeVolumeSoundMem(vol, sndLoadingBgm);
        currentBgmType = 3;
    }
}

void AudioManager::PlayClearWindAmbience()
{
    if (sndTitleAmbience != -1 && CheckSoundMem(sndTitleAmbience)) StopSoundMem(sndTitleAmbience);
    if (sndGameAmbience != -1 && CheckSoundMem(sndGameAmbience)) StopSoundMem(sndGameAmbience);
    if (sndLoadingBgm != -1 && CheckSoundMem(sndLoadingBgm)) StopSoundMem(sndLoadingBgm);
    if (sndTensionLoop != -1 && CheckSoundMem(sndTensionLoop)) StopSoundMem(sndTensionLoop);
    if (sndRainAmbience != -1 && CheckSoundMem(sndRainAmbience)) StopSoundMem(sndRainAmbience);
    targetTensionMix = 0.0f;
    currentTensionMix = 0.0f;
    targetRainMix = 0.0f;
    currentRainMix = 0.0f;

    if (sndWindAmbience != -1)
    {
        targetWindMix = 0.42f;
        currentWindMix = 0.42f;
        int vol = CalcBgmVolume(0.55f * 0.42f);
        ChangeVolumeSoundMem(vol, sndWindAmbience);
        if (!CheckSoundMem(sndWindAmbience))
        {
            PlaySoundMem(sndWindAmbience, DX_PLAYTYPE_LOOP);
        }
        currentBgmType = 4;
    }
}

void AudioManager::PlayGameAmbienceBgm()
{
    if (currentBgmType == 2 && sndGameAmbience != -1 && CheckSoundMem(sndGameAmbience))
    {
        return;
    }

    if (sndTitleAmbience != -1 && CheckSoundMem(sndTitleAmbience))
    {
        StopSoundMem(sndTitleAmbience);
    }
    if (sndLoadingBgm != -1 && CheckSoundMem(sndLoadingBgm))
    {
        StopSoundMem(sndLoadingBgm);
    }
    if (sndWindAmbience != -1 && CheckSoundMem(sndWindAmbience))
    {
        StopSoundMem(sndWindAmbience);
    }

    targetTensionMix = 0.0f;
    currentTensionMix = 0.0f;
    targetWindMix = 0.0f;
    currentWindMix = 0.0f;

    if (sndGameAmbience != -1)
    {
        int vol = CalcBgmVolume();
        ChangeVolumeSoundMem(vol, sndGameAmbience);
        PlaySoundMem(sndGameAmbience, DX_PLAYTYPE_LOOP);
        ChangeVolumeSoundMem(vol, sndGameAmbience);
        currentBgmType = 2;
    }
}

void AudioManager::StopBgm()
{
    if (sndTitleAmbience != -1 && CheckSoundMem(sndTitleAmbience)) StopSoundMem(sndTitleAmbience);
    if (sndLoadingBgm != -1 && CheckSoundMem(sndLoadingBgm)) StopSoundMem(sndLoadingBgm);
    if (sndWindAmbience != -1 && CheckSoundMem(sndWindAmbience)) StopSoundMem(sndWindAmbience);
    if (sndGameAmbience != -1 && CheckSoundMem(sndGameAmbience)) StopSoundMem(sndGameAmbience);
    if (sndTensionLoop != -1 && CheckSoundMem(sndTensionLoop)) StopSoundMem(sndTensionLoop);
    if (sndRainAmbience != -1 && CheckSoundMem(sndRainAmbience)) StopSoundMem(sndRainAmbience);
    StopAllHorrorEvents();
    currentBgmType = 0;
    targetTensionMix = 0.0f;
    currentTensionMix = 0.0f;
    targetRainMix = 0.0f;
    currentRainMix = 0.0f;
    targetWindMix = 0.0f;
    currentWindMix = 0.0f;
}

void AudioManager::PlayFootstep()
{
    if (footstepCooldown > 0.0f || sndFootstep == -1) return;

    footstepLeftSide = !footstepLeftSide;
    int pan = footstepLeftSide ? -26 : 26;
    int pitchFreq = 42200 + (std::rand() % 3800);
    SetFrequencySoundMem(pitchFreq, sndFootstep);
    SetPanSoundMem(pan, sndFootstep);
    ChangeVolumeSoundMem(CalcSfxVolume(0.84f), sndFootstep);
    PlaySoundMem(sndFootstep, DX_PLAYTYPE_BACK);
    footstepCooldown = 0.34f + static_cast<float>(std::rand() % 4) * 0.01f;
}

void AudioManager::PlayBicycleRoll(bool isMoving)
{
    if (sndBicycleRoll == -1) return;

    if (isMoving && !isPausedDuck)
    {
        if (!isBicycleRolling || !CheckSoundMem(sndBicycleRoll))
        {
            ChangeVolumeSoundMem(CalcSfxVolume(0.70f), sndBicycleRoll);
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

void AudioManager::PlayHeadlightSwitch(bool hasBattery)
{
    if (sndHeadlightSwitch == -1) return;
    int pitchFreq = hasBattery ? (43200 + std::rand() % 2000) : 34000;
    SetFrequencySoundMem(pitchFreq, sndHeadlightSwitch);
    ChangeVolumeSoundMem(CalcSfxVolume(hasBattery ? 0.85f : 0.60f), sndHeadlightSwitch);
    PlaySoundMem(sndHeadlightSwitch, DX_PLAYTYPE_BACK);
}

void AudioManager::PlayNewspaperPickup()
{
    if (sndNewspaperPickup == -1) return;
    int pitchFreq = 42400 + (std::rand() % 3400);
    SetFrequencySoundMem(pitchFreq, sndNewspaperPickup);
    ChangeVolumeSoundMem(CalcSfxVolume(0.90f), sndNewspaperPickup);
    PlaySoundMem(sndNewspaperPickup, DX_PLAYTYPE_BACK);
}

void AudioManager::PlayDeliverySuccess()
{
    if (sndDeliverySuccess != -1)
    {
        SetFrequencySoundMem(43600 + (std::rand() % 1400), sndDeliverySuccess);
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
    if (fearPercent <= 0.30f || sndFearHeartbeat == -1) return;

    // 恐怖度が高いほど鼓動の間隔が短縮（1.15秒〜0.45秒）し音程も切迫
    float interval = 1.15f - (fearPercent * 0.70f);
    if (heartbeatTimer <= 0.0f)
    {
        heartbeatTimer = interval;
        float relativeVol = 0.45f + (fearPercent * 0.55f);
        int freq = 42800 + static_cast<int>(fearPercent * 5200.0f);
        SetFrequencySoundMem(freq, sndFearHeartbeat);
        ChangeVolumeSoundMem(CalcSfxVolume(relativeVol), sndFearHeartbeat);
        PlaySoundMem(sndFearHeartbeat, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayMonsterRoar()
{
    if (sndMonsterRoar != -1 && roarCooldown <= 0.0f)
    {
        int pitchFreq = 42500 + (std::rand() % 3000);
        SetFrequencySoundMem(pitchFreq, sndMonsterRoar);
        ChangeVolumeSoundMem(CalcSfxVolume(1.0f), sndMonsterRoar);
        PlaySoundMem(sndMonsterRoar, DX_PLAYTYPE_BACK);
        roarCooldown = 0.65f;
    }
}

void AudioManager::PlayZombieScream()
{
    int h = (sndZombieScream != -1) ? sndZombieScream : sndMonsterRoar;
    if (h != -1)
    {
        int pitchFreq = 42800 + (std::rand() % 2600);
        SetFrequencySoundMem(pitchFreq, h);
        ChangeVolumeSoundMem(CalcSfxVolume(1.0f), h);
        PlaySoundMem(h, DX_PLAYTYPE_BACK);
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

void AudioManager::PlayGameClearSting()
{
    if (sndGameClearSting != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(0.92f), sndGameClearSting);
        PlaySoundMem(sndGameClearSting, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayGameOverSting()
{
    if (sndGameOverSting != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(0.95f), sndGameOverSting);
        PlaySoundMem(sndGameOverSting, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayMenuClick()
{
    if (sndMenuClick != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(0.90f), sndMenuClick);
        PlaySoundMem(sndMenuClick, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayMenuHover()
{
    if (sndMenuHover != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(0.45f), sndMenuHover);
        PlaySoundMem(sndMenuHover, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayMenuAdjust()
{
    if (sndMenuAdjust != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(0.90f), sndMenuAdjust);
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

void AudioManager::PlayTitleConfirm()
{
    if (sndTitleConfirm != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(0.92f), sndTitleConfirm);
        PlaySoundMem(sndTitleConfirm, DX_PLAYTYPE_BACK);
    }
    else
    {
        PlayMenuClick();
    }
}

void AudioManager::PlayLampFlickerBuzz(int pan)
{
    if (sndLampBuzz != -1)
    {
        int clampedPan = std::clamp(pan, -255, 255);
        int freq = 42800 + (std::rand() % 2600);
        SetFrequencySoundMem(freq, sndLampBuzz);
        ChangePanSoundMem(clampedPan, sndLampBuzz);
        ChangeVolumeSoundMem(CalcSfxVolume(0.58f), sndLampBuzz);
        PlaySoundMem(sndLampBuzz, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayTapeInsert()
{
    if (sndTapeInsert != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(0.92f), sndTapeInsert);
        PlaySoundMem(sndTapeInsert, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayLoadingStaticBurst()
{
    if (sndTapeGlitch != -1)
    {
        int freq = 43000 + (std::rand() % 2200);
        SetFrequencySoundMem(freq, sndTapeGlitch);
        ChangeVolumeSoundMem(CalcSfxVolume(0.66f), sndTapeGlitch);
        PlaySoundMem(sndTapeGlitch, DX_PLAYTYPE_BACK);
    }
}

void AudioManager::PlayLoadingComplete()
{
    if (sndTapeLock != -1)
    {
        ChangeVolumeSoundMem(CalcSfxVolume(0.88f), sndTapeLock);
        PlaySoundMem(sndTapeLock, DX_PLAYTYPE_BACK);
    }
}

bool AudioManager::AreSceneAudioAssetsLoaded() const
{
    return (sndTitleAmbience != -1)
        && (sndLoadingBgm != -1)
        && (sndWindAmbience != -1)
        && (sndTitleConfirm != -1)
        && (sndLampBuzz != -1)
        && (sndTapeInsert != -1)
        && (sndTapeGlitch != -1)
        && (sndTapeLock != -1)
        && (sndMenuHover != -1);
}

void AudioManager::PlaySpatialHorrorEvent(
    HorrorEventSfx kind,
    const VECTOR& listenerPos,
    const VECTOR& listenerFwd,
    const VECTOR& sourcePos,
    float maxRange,
    float gainScale
)
{
    int idx = static_cast<int>(kind);
    if (idx < 0 || idx >= static_cast<int>(HorrorEventSfx::Count)) return;
    int handle = sndHorrorEvents[idx];
    if (handle == -1) return;

    float dx = sourcePos.x - listenerPos.x;
    float dz = sourcePos.z - listenerPos.z;
    float dist = std::sqrt(dx * dx + dz * dz);
    if (maxRange <= 0.1f) maxRange = 28.0f;
    if (dist > maxRange) return;

    // 左手系 DirectX 座標 (forward=(fx,0,fz) -> right=(fz,0,-fx)) で左右パンを算出
    float fLen = std::sqrt(listenerFwd.x * listenerFwd.x + listenerFwd.z * listenerFwd.z);
    float fx = (fLen > 0.001f) ? (listenerFwd.x / fLen) : 0.0f;
    float fz = (fLen > 0.001f) ? (listenerFwd.z / fLen) : 1.0f;
    float rx = fz;
    float rz = -fx;

    float invD = (dist > 0.05f) ? (1.0f / dist) : 0.0f;
    float ndx = dx * invD;
    float ndz = dz * invD;
    float rightDot = ndx * rx + ndz * rz;
    float frontDot = ndx * fx + ndz * fz;

    int pan = std::clamp(static_cast<int>(rightDot * 230.0f), -255, 255);
    float distNorm = std::clamp(dist / maxRange, 0.0f, 1.0f);
    float atten = (1.0f - distNorm) * (1.0f - 0.35f * distNorm);
    float rearMuffle = (frontDot < -0.15f) ? 0.90f : 1.0f;
    float finalScale = std::clamp(atten * rearMuffle * gainScale, 0.12f, 1.0f);

    int rearPitchShift = (frontDot < -0.25f) ? -1100 : 0;
    int pitchFreq = 42800 + (std::rand() % 2400) + rearPitchShift;
    SetFrequencySoundMem(pitchFreq, handle);
    ChangePanSoundMem(pan, handle);
    ChangeVolumeSoundMem(CalcSfxVolume(finalScale), handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

void AudioManager::StopAllHorrorEvents()
{
    for (int i = 0; i < static_cast<int>(HorrorEventSfx::Count); ++i)
    {
        if (sndHorrorEvents[i] != -1 && CheckSoundMem(sndHorrorEvents[i]))
        {
            StopSoundMem(sndHorrorEvents[i]);
        }
    }
}

bool AudioManager::IsHorrorEventSfxLoaded(HorrorEventSfx kind) const
{
    int idx = static_cast<int>(kind);
    if (idx < 0 || idx >= static_cast<int>(HorrorEventSfx::Count)) return false;
    return sndHorrorEvents[idx] != -1;
}
