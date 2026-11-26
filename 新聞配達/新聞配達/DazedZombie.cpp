#include "DazedZombie.h"
#include "AudioManager.h"
#include "HorrorUI.h"
#include "WorldCollision.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>

namespace
{
    // 行動・アニメーション設定定数
    constexpr float TRIGGER_DISTANCE = 8.5f;         // 茫然から絶叫へ移行する接近距離 (m)
    constexpr float CLOSE_OVERRIDE_DISTANCE = 4.5f;  // 至近距離での強制反応距離 (m)
    constexpr float SCREAM_ANIM_SPEED = 1.35f;       // 絶叫アニメーション再生倍率
    constexpr float INJURED_RUN_SPEED = 6.2f;        // 負傷走行の追跡移動速度 (m/s)
    constexpr float INJURED_RUN_ANIM_SPEED = 1.25f;  // 負傷走行アニメーション再生倍率
    constexpr float MIN_RUN_TIME_BEFORE_VANISH = 0.70f;
    constexpr float MAX_RUN_DURATION = 3.0f;         // 追跡後に消失するまでの最大時間 (s)
    constexpr float VANISH_PROXIMITY = 1.75f;        // プレイヤー至近到達による消失距離 (m)
    constexpr float MIN_CAMERA_STOP_DIST = 0.95f;    // カメラ内へのめり込み防止距離 (m)
    constexpr float VANISH_DURATION = 0.20f;         // 突然の霧消演出時間 (s)
    constexpr float MIN_RESPAWN_PLAYER_DIST = 16.0f; // 別地点への再出現時・プレイヤーとの最小離隔距離 (m)
    constexpr float INITIAL_SPAWN_DELAY = 0.25f;     // ゲーム開始直後の初回配置遅延 (s)

    // 候補パスからモデルを順次探索して読み込むヘルパー
    int LoadZombieModelWithFallback(const char* relPath)
    {
        const char* prefixes[] = {
            "",
            "新聞配達/",
            "../新聞配達/",
            "新聞配達/新聞配達/",
            "../../新聞配達/"
        };

        char fullPath[260];
        for (const char* prefix : prefixes)
        {
            snprintf(fullPath, sizeof(fullPath), "%s%s", prefix, relPath);
            int handle = MV1LoadModel(fullPath);
            if (handle != -1)
            {
                return handle;
            }
        }
        return -1;
    }

    // モデルの有効アニメーションをアタッチして総再生時間を取得
    int AttachPrimaryAnim(int handle, float& outTotalTime)
    {
        if (handle == -1)
        {
            outTotalTime = 0.0f;
            return -1;
        }

        int animNum = MV1GetAnimNum(handle);
        int attachIdx = -1;
        if (animNum > 1)
        {
            attachIdx = MV1AttachAnim(handle, 1, -1, FALSE);
            outTotalTime = MV1GetAnimTotalTime(handle, 1);
        }
        else if (animNum == 1)
        {
            attachIdx = MV1AttachAnim(handle, 0, -1, FALSE);
            outTotalTime = MV1GetAnimTotalTime(handle, 0);
        }
        return attachIdx;
    }
}

DazedZombie::DazedZombie()
    : dazedModelHandle(-1)
    , screamModelHandle(-1)
    , runModelHandle(-1)
    , dazedAnimAttach(-1)
    , dazedAnimTotal(250.0f)
    , screamAnimAttach(-1)
    , screamAnimTotal(84.0f)
    , runAnimAttach(-1)
    , runAnimTotal(19.0f)
    , lightHandle(-1)
    , lightsSuppressed(false)
    , state(DazedZombieState::WaitingSpawn)
    , animPlayTime(0.0f)
    , stateTimer(INITIAL_SPAWN_DELAY)
    , vanishOpacity(1.0f)
    , position(VGet(0.0f, -999.0f, 0.0f))
    , yawAngle(0.0f)
    , scale(0.01f)
    , lastSpawnIndex(-1)
    , vfxEmitTimer(0.0f)
    , shockwaveActive(false)
    , shockwaveCenter(VGet(0.0f, 0.0f, 0.0f))
    , shockwaveAge(0.0f)
    , vfxWispTex(-1)
    , vfxRingTex(-1)
{
    ClearVfx();
    // 街全域の道路・路地・交差点からランダム出現候補地点（27箇所）を登録
    spawnCandidates = {
        // 南大通り (Z = -30付近)
        VGet(-18.0f, 0.0f, -29.0f),
        VGet(-9.5f,  0.0f, -31.0f),
        VGet( 9.5f,  0.0f, -29.0f),
        VGet( 19.0f, 0.0f, -31.0f),
        VGet(-24.5f, 0.0f, -29.5f),
        VGet( 24.5f, 0.0f, -29.5f),

        // 西通り (X = -25付近)
        VGet(-25.5f, 0.0f, -21.0f),
        VGet(-24.5f, 0.0f, -12.0f),
        VGet(-25.0f, 0.0f,  -3.0f),
        VGet(-25.5f, 0.0f,   6.0f),
        VGet(-25.0f, 0.0f,  15.0f),

        // 東通り (X = +25付近)
        VGet( 25.5f, 0.0f, -21.0f),
        VGet( 24.5f, 0.0f, -12.0f),
        VGet( 25.0f, 0.0f,  -3.0f),
        VGet( 25.5f, 0.0f,   6.0f),
        VGet( 25.0f, 0.0f,  15.0f),

        // 中央横通り (Z = -10付近)
        VGet(-15.0f, 0.0f, -10.5f),
        VGet( -5.0f, 0.0f, -10.0f),
        VGet(  5.0f, 0.0f, -10.0f),
        VGet( 15.0f, 0.0f, -10.5f),

        // 北横通り (Z = +15付近)
        VGet(-15.0f, 0.0f,  14.5f),
        VGet( -5.0f, 0.0f,  15.5f),
        VGet(  5.0f, 0.0f,  14.5f),
        VGet( 15.0f, 0.0f,  15.5f),

        // 北路地・行き止まり奥 (X = 0付近)
        VGet( -1.2f, 0.0f,  23.0f),
        VGet(  1.2f, 0.0f,  30.0f),
        VGet( -0.5f, 0.0f,  36.5f)
    };
}

DazedZombie::~DazedZombie()
{
    Finalize();
}

void DazedZombie::ConfigureModelMaterials(int handle)
{
    if (handle == -1) return;

    MV1SetScale(handle, VGet(scale, scale, scale));

    int matNum = MV1GetMaterialNum(handle);
    for (int m = 0; m < matNum; ++m)
    {
        if (m == 0)
        {
            // 素肌部分（顔・腕・脚）：青白い腐敗皮膚色でライトや影が立体的に乗る設定
            MV1SetMaterialDifColor(handle, m, GetColorF(0.82f, 0.72f, 0.68f, 1.0f));
            MV1SetMaterialAmbColor(handle, m, GetColorF(0.16f, 0.14f, 0.14f, 1.0f));
            MV1SetMaterialSpcColor(handle, m, GetColorF(0.45f, 0.40f, 0.40f, 1.0f));
            MV1SetMaterialEmiColor(handle, m, GetColorF(0.030f, 0.025f, 0.025f, 1.0f));
        }
        else
        {
            // 衣服・髪部分：暗い汚れ色で素肌とのコントラストと陰影を際立たせる
            MV1SetMaterialDifColor(handle, m, GetColorF(0.30f, 0.25f, 0.26f, 1.0f));
            MV1SetMaterialAmbColor(handle, m, GetColorF(0.08f, 0.07f, 0.08f, 1.0f));
            MV1SetMaterialSpcColor(handle, m, GetColorF(0.18f, 0.16f, 0.18f, 1.0f));
            MV1SetMaterialEmiColor(handle, m, GetColorF(0.015f, 0.012f, 0.015f, 1.0f));
        }
    }
}

bool DazedZombie::Initialize()
{
    Reset();

    if (lightHandle == -1)
    {
        lightHandle = CreatePointLightHandle(VGet(0.0f, -999.0f, 0.0f), 6.5f, 1.0f, 0.14f, 0.04f);
    }
    if (lightHandle != -1)
    {
        SetLightEnableHandle(lightHandle, FALSE);
    }

    // 既に読み込み済みなら重複ロードを回避
    if (dazedModelHandle != -1 && screamModelHandle != -1 && runModelHandle != -1)
    {
        return true;
    }

    // 1. 座り込み茫然モデル (Sitting Dazed (1).mv1)
    if (dazedModelHandle == -1)
    {
        dazedModelHandle = LoadZombieModelWithFallback("Data/Model/Sitting Dazed (1).mv1");
        if (dazedModelHandle != -1)
        {
            dazedAnimAttach = AttachPrimaryAnim(dazedModelHandle, dazedAnimTotal);
            ConfigureModelMaterials(dazedModelHandle);
        }
    }

    // 2. 威嚇絶叫モデル (Zombie Scream (1).mv1)
    if (screamModelHandle == -1)
    {
        screamModelHandle = LoadZombieModelWithFallback("Data/Model/Zombie Scream (1).mv1");
        if (screamModelHandle != -1)
        {
            screamAnimAttach = AttachPrimaryAnim(screamModelHandle, screamAnimTotal);
            ConfigureModelMaterials(screamModelHandle);
        }
    }

    // 3. 負傷走行モデル (Injured Run.mv1)
    if (runModelHandle == -1)
    {
        runModelHandle = LoadZombieModelWithFallback("Data/Model/Injured Run.mv1");
        if (runModelHandle != -1)
        {
            runAnimAttach = AttachPrimaryAnim(runModelHandle, runAnimTotal);
            ConfigureModelMaterials(runModelHandle);
        }
    }

    return (dazedModelHandle != -1 && screamModelHandle != -1 && runModelHandle != -1);
}

void DazedZombie::Reset()
{
    state = DazedZombieState::WaitingSpawn;
    animPlayTime = 0.0f;
    stateTimer = INITIAL_SPAWN_DELAY;
    vanishOpacity = 1.0f;
    position = VGet(0.0f, -999.0f, 0.0f);
    yawAngle = 0.0f;
    lastSpawnIndex = -1;
    lightsSuppressed = false;

    int handles[3] = { dazedModelHandle, screamModelHandle, runModelHandle };
    for (int h : handles)
    {
        if (h != -1)
        {
            MV1SetOpacityRate(h, 1.0f);
            MV1SetPosition(h, VGet(0.0f, -999.0f, 0.0f));
        }
    }

    if (lightHandle != -1)
    {
        SetLightEnableHandle(lightHandle, FALSE);
    }
    ClearVfx();
}

void DazedZombie::EnsureVfxTextures()
{
    if (vfxWispTex == -1)
    {
        const int sz = 64;
        int soft = MakeARGB8ColorSoftImage(sz, sz);
        if (soft != -1)
        {
            for (int y = 0; y < sz; ++y)
            {
                float dy = (static_cast<float>(y) + 0.5f) / static_cast<float>(sz) * 2.0f - 1.0f;
                for (int x = 0; x < sz; ++x)
                {
                    float dx = (static_cast<float>(x) + 0.5f) / static_cast<float>(sz) * 2.0f - 1.0f;
                    float r = std::sqrt(dx * dx + dy * dy);
                    if (r >= 1.0f) { DrawPixelSoftImage(soft, x, y, 0, 0, 0, 0); continue; }
                    float fade = 1.0f - r;
                    float val = std::exp(-r * r * 4.2f) * fade * fade;
                    DrawPixelSoftImage(soft, x, y, 255, 255, 255, std::clamp(static_cast<int>(255.0f * val), 0, 255));
                }
            }
            vfxWispTex = CreateGraphFromSoftImage(soft);
            DeleteSoftImage(soft);
        }
    }
    if (vfxRingTex == -1)
    {
        const int sz = 128;
        int soft = MakeARGB8ColorSoftImage(sz, sz);
        if (soft != -1)
        {
            for (int y = 0; y < sz; ++y)
            {
                float dy = (static_cast<float>(y) + 0.5f) / static_cast<float>(sz) * 2.0f - 1.0f;
                for (int x = 0; x < sz; ++x)
                {
                    float dx = (static_cast<float>(x) + 0.5f) / static_cast<float>(sz) * 2.0f - 1.0f;
                    float r = std::sqrt(dx * dx + dy * dy);
                    if (r >= 1.0f) { DrawPixelSoftImage(soft, x, y, 0, 0, 0, 0); continue; }
                    float ang = std::atan2(dy, dx);
                    float wave = 0.78f + 0.22f * std::sin(ang * 9.0f);
                    float ring = std::exp(-std::pow((r - 0.84f) / 0.055f, 2.0f)) * wave * (1.0f - r * 0.35f);
                    DrawPixelSoftImage(soft, x, y, 255, 255, 255, std::clamp(static_cast<int>(255.0f * ring), 0, 255));
                }
            }
            vfxRingTex = CreateGraphFromSoftImage(soft);
            DeleteSoftImage(soft);
        }
    }
}

void DazedZombie::FinalizeVfxTextures()
{
    if (vfxWispTex != -1) { DeleteGraph(vfxWispTex); vfxWispTex = -1; }
    if (vfxRingTex != -1) { DeleteGraph(vfxRingTex); vfxRingTex = -1; }
}

void DazedZombie::ClearVfx()
{
    for (int i = 0; i < MAX_VFX_PARTICLES; ++i)
    {
        vfxParticles[i].active = false;
        vfxParticles[i].age = 0.0f;
        vfxParticles[i].maxAge = 1.0f;
        vfxParticles[i].size = 0.2f;
        vfxParticles[i].isEmber = false;
        vfxParticles[i].pos = VGet(0.0f, 0.0f, 0.0f);
        vfxParticles[i].vel = VGet(0.0f, 0.0f, 0.0f);
    }
    vfxEmitTimer = 0.0f;
    shockwaveActive = false;
    shockwaveAge = 0.0f;
}

void DazedZombie::EmitVfxParticle(const VECTOR& p, const VECTOR& v, float pSize, float maxAge, bool isEmber)
{
    for (int i = 0; i < MAX_VFX_PARTICLES; ++i)
    {
        if (!vfxParticles[i].active)
        {
            vfxParticles[i].pos = p;
            vfxParticles[i].vel = v;
            vfxParticles[i].size = pSize;
            vfxParticles[i].age = 0.0f;
            vfxParticles[i].maxAge = maxAge;
            vfxParticles[i].isEmber = isEmber;
            vfxParticles[i].active = true;
            return;
        }
    }
}

void DazedZombie::EmitScreamAndVanishBurst(bool isVanish)
{
    int count = isVanish ? 30 : 18;
    for (int i = 0; i < count; ++i)
    {
        float ang = static_cast<float>(std::rand() % 628) / 100.0f;
        float spd = (isVanish ? 1.4f : 1.8f) + static_cast<float>(std::rand() % 220) / 100.0f;
        float vy = 0.5f + static_cast<float>(std::rand() % 180) / 100.0f;
        VECTOR sp = VGet(
            position.x + std::cos(ang) * 0.30f,
            position.y + 0.55f + static_cast<float>(std::rand() % 110) / 100.0f,
            position.z + std::sin(ang) * 0.30f
        );
        VECTOR sv = VGet(std::cos(ang) * spd, vy, std::sin(ang) * spd);
        bool ember = (i % 3 != 0);
        EmitVfxParticle(sp, sv, ember ? 0.14f : 0.72f, ember ? 0.65f : 1.05f, ember);
    }
}

void DazedZombie::UpdateVfx(float dt)
{
    if (shockwaveActive)
    {
        shockwaveAge += dt;
        if (shockwaveAge >= 0.55f) shockwaveActive = false;
    }

    for (int i = 0; i < MAX_VFX_PARTICLES; ++i)
    {
        VfxParticle& pt = vfxParticles[i];
        if (!pt.active) continue;
        pt.age += dt;
        if (pt.age >= pt.maxAge)
        {
            pt.active = false;
            continue;
        }
        pt.pos.x += pt.vel.x * dt;
        pt.pos.y += pt.vel.y * dt;
        pt.pos.z += pt.vel.z * dt;
        pt.vel.x *= (1.0f - (std::min)(0.8f, dt * 1.8f));
        pt.vel.z *= (1.0f - (std::min)(0.8f, dt * 1.8f));
    }

    if (!IsVisible()) return;

    vfxEmitTimer += dt;
    if (state == DazedZombieState::SittingDazed && vfxEmitTimer >= 0.14f)
    {
        vfxEmitTimer = 0.0f;
        float rx = (static_cast<float>(std::rand() % 200) / 100.0f - 1.0f) * 0.45f;
        float rz = (static_cast<float>(std::rand() % 200) / 100.0f - 1.0f) * 0.45f;
        EmitVfxParticle(
            VGet(position.x + rx, position.y + 0.18f, position.z + rz),
            VGet(rx * 0.15f, 0.48f, rz * 0.15f),
            0.65f, 1.35f, false
        );
    }
    else if ((state == DazedZombieState::Scream || state == DazedZombieState::InjuredRun) && vfxEmitTimer >= 0.055f)
    {
        vfxEmitTimer = 0.0f;
        float rx = (static_cast<float>(std::rand() % 200) / 100.0f - 1.0f) * 0.40f;
        float rz = (static_cast<float>(std::rand() % 200) / 100.0f - 1.0f) * 0.40f;
        bool ember = (std::rand() % 2 == 0);
        EmitVfxParticle(
            VGet(position.x + rx, position.y + (ember ? 0.85f : 0.25f), position.z + rz),
            VGet(rx * 0.4f, ember ? 0.95f : 0.55f, rz * 0.4f),
            ember ? 0.12f : 0.72f, ember ? 0.55f : 0.95f, ember
        );
    }
}

void DazedZombie::DrawVfx()
{
    EnsureVfxTextures();
    if (vfxWispTex == -1) return;

    SetUseLighting(FALSE);
    SetWriteZBuffer3D(FALSE);
    SetTextureAddressMode(DX_TEXADDRESS_CLAMP);

    // 絶叫衝撃波リング
    if (shockwaveActive && vfxRingTex != -1)
    {
        float t = std::clamp(shockwaveAge / 0.55f, 0.0f, 1.0f);
        float r = 0.35f + 4.2f * (1.0f - (1.0f - t) * (1.0f - t));
        int alpha = static_cast<int>(155.0f * (1.0f - t));
        SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
        COLOR_U8 c = GetColorU8(230, 60, 55, 255);
        COLOR_U8 z = GetColorU8(0, 0, 0, 0);
        auto mk = [&](float x, float zPos, float u, float v) {
            VERTEX3D vert{};
            vert.pos = VGet(x, shockwaveCenter.y, zPos);
            vert.norm = VGet(0.0f, 1.0f, 0.0f);
            vert.dif = c; vert.spc = z;
            vert.u = u; vert.v = v; vert.su = u; vert.sv = v;
            return vert;
        };
        VERTEX3D ringVerts[6] = {
            mk(shockwaveCenter.x - r, shockwaveCenter.z - r, 0.0f, 0.0f),
            mk(shockwaveCenter.x - r, shockwaveCenter.z + r, 0.0f, 1.0f),
            mk(shockwaveCenter.x + r, shockwaveCenter.z + r, 1.0f, 1.0f),
            mk(shockwaveCenter.x - r, shockwaveCenter.z - r, 0.0f, 0.0f),
            mk(shockwaveCenter.x + r, shockwaveCenter.z + r, 1.0f, 1.0f),
            mk(shockwaveCenter.x + r, shockwaveCenter.z - r, 1.0f, 0.0f)
        };
        DrawPolygon3D(ringVerts, 2, vfxRingTex, TRUE);
    }

    // 瘴気ウィスプ・火の粉パーティクル
    for (int i = 0; i < MAX_VFX_PARTICLES; ++i)
    {
        const VfxParticle& pt = vfxParticles[i];
        if (!pt.active) continue;
        float t = std::clamp(pt.age / pt.maxAge, 0.0f, 1.0f);
        float fade = std::sin(t * 3.14159f);
        if (pt.isEmber)
        {
            SetDrawBright(245, 75, 55);
            SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(210.0f * fade));
            DrawBillboard3D(pt.pos, 0.5f, 0.5f, pt.size * (1.0f - t * 0.4f), 0.0f, vfxWispTex, TRUE);
        }
        else
        {
            bool redMood = (state == DazedZombieState::Scream || state == DazedZombieState::InjuredRun || state == DazedZombieState::Vanishing);
            if (redMood) SetDrawBright(90, 32, 36);
            else SetDrawBright(115, 132, 158);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(85.0f * fade));
            DrawBillboard3D(pt.pos, 0.5f, 0.5f, pt.size * (0.85f + t * 0.75f), t * 1.4f, vfxWispTex, TRUE);
        }
    }

    SetDrawBright(255, 255, 255);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetWriteZBuffer3D(TRUE);
    SetUseLighting(TRUE);
}

int DazedZombie::GetActiveModelHandle() const
{
    switch (state)
    {
    case DazedZombieState::SittingDazed:
        return dazedModelHandle;
    case DazedZombieState::Scream:
        return screamModelHandle;
    case DazedZombieState::InjuredRun:
    case DazedZombieState::Vanishing:
        return runModelHandle;
    default:
        return -1;
    }
}

int DazedZombie::GetActiveAnimAttachIndex() const
{
    switch (state)
    {
    case DazedZombieState::SittingDazed:
        return dazedAnimAttach;
    case DazedZombieState::Scream:
        return screamAnimAttach;
    case DazedZombieState::InjuredRun:
    case DazedZombieState::Vanishing:
        return runAnimAttach;
    default:
        return -1;
    }
}

float DazedZombie::GetActiveAnimTotalTime() const
{
    switch (state)
    {
    case DazedZombieState::SittingDazed:
        return dazedAnimTotal;
    case DazedZombieState::Scream:
        return screamAnimTotal;
    case DazedZombieState::InjuredRun:
    case DazedZombieState::Vanishing:
        return runAnimTotal;
    default:
        return 0.0f;
    }
}

bool DazedZombie::IsActive() const
{
    return (state != DazedZombieState::WaitingSpawn);
}

bool DazedZombie::IsVisible() const
{
    return IsActive() && (GetActiveModelHandle() != -1);
}

const char* DazedZombie::GetStateName() const
{
    switch (state)
    {
    case DazedZombieState::WaitingSpawn: return "WAITING_RESPAWN";
    case DazedZombieState::SittingDazed: return "SITTING_DAZED";
    case DazedZombieState::Scream:       return "SCREAMING";
    case DazedZombieState::InjuredRun:   return "INJURED_RUN";
    case DazedZombieState::Vanishing:    return "VANISHING";
    default:                             return "UNKNOWN";
    }
}

void DazedZombie::SetLightsSuppressed(bool suppressed)
{
    lightsSuppressed = suppressed;
    if (suppressed && lightHandle != -1)
    {
        SetLightEnableHandle(lightHandle, FALSE);
    }
}

void DazedZombie::AdvanceLoopingAnim(float dt, float speedMult)
{
    int handle = GetActiveModelHandle();
    int attachIdx = GetActiveAnimAttachIndex();
    float totalTime = GetActiveAnimTotalTime();
    if (handle == -1 || attachIdx == -1 || totalTime <= 0.0f)
    {
        return;
    }

    animPlayTime += dt * 30.0f * speedMult;
    if (animPlayTime >= totalTime)
    {
        animPlayTime = std::fmod(animPlayTime, totalTime);
    }
    MV1SetAttachAnimTime(handle, attachIdx, animPlayTime);
}

void DazedZombie::LookAtTarget(const VECTOR& targetPos, float blendRate)
{
    float dx = targetPos.x - position.x;
    float dz = targetPos.z - position.z;
    if (dx * dx + dz * dz < 0.0001f)
    {
        return;
    }

    float desiredYaw = std::atan2(dx, dz) + MODEL_FORWARD_OFFSET;
    if (blendRate < 0.0f) blendRate = 0.0f;
    if (blendRate >= 0.999f)
    {
        yawAngle = desiredYaw;
        return;
    }

    float diff = desiredYaw - yawAngle;
    while (diff > DX_PI_F) diff -= DX_TWO_PI_F;
    while (diff < -DX_PI_F) diff += DX_TWO_PI_F;
    yawAngle += diff * blendRate;
}

bool DazedZombie::SpawnAtRandomLocation(const VECTOR& playerPos)
{
    if (spawnCandidates.empty())
    {
        return false;
    }

    std::vector<int> validIndices;
    std::vector<int> fallbackIndices;

    for (int i = 0; i < static_cast<int>(spawnCandidates.size()); ++i)
    {
        if (i == lastSpawnIndex && spawnCandidates.size() > 1)
        {
            continue;
        }

        const VECTOR& cand = spawnCandidates[i];
        if (WorldCollision::Instance().CheckCollision(cand, 0.70f))
        {
            continue;
        }

        float dx = cand.x - playerPos.x;
        float dz = cand.z - playerPos.z;
        float distXZ = std::sqrt(dx * dx + dz * dz);

        fallbackIndices.push_back(i);
        if (distXZ >= MIN_RESPAWN_PLAYER_DIST)
        {
            validIndices.push_back(i);
        }
    }

    const std::vector<int>& pool = !validIndices.empty() ? validIndices : fallbackIndices;
    if (pool.empty())
    {
        return false;
    }

    int pickedIdx = pool[std::rand() % pool.size()];
    VECTOR chosenPos = spawnCandidates[pickedIdx];

    // 路肩・道路範囲内の微小なランダム位置ゆらぎを加える（衝突がなければ採用）
    float jx = (static_cast<float>(std::rand() % 200) / 100.0f - 1.0f) * 1.1f;
    float jz = (static_cast<float>(std::rand() % 200) / 100.0f - 1.0f) * 1.1f;
    VECTOR jitteredPos = VGet(chosenPos.x + jx, 0.0f, chosenPos.z + jz);
    if (!WorldCollision::Instance().CheckCollision(jitteredPos, 0.70f))
    {
        chosenPos = jitteredPos;
    }

    lastSpawnIndex = pickedIdx;
    position = chosenPos;
    position.y = 0.0f;

    // 座り込み時はランダムな向き（わずかに路地・通路側へ向く不気味な角度）で待機
    yawAngle = (static_cast<float>(std::rand() % 628) / 100.0f);
    state = DazedZombieState::SittingDazed;
    stateTimer = 0.0f;
    vanishOpacity = 1.0f;
    animPlayTime = (dazedAnimTotal > 1.0f)
        ? static_cast<float>(std::rand() % static_cast<int>(dazedAnimTotal))
        : 0.0f;

    return true;
}

void DazedZombie::Update(float dt, const VECTOR& playerPos, float& outFearDelta)
{
    outFearDelta = 0.0f;

    if (dazedModelHandle == -1 || screamModelHandle == -1 || runModelHandle == -1)
    {
        return;
    }

    float dx = playerPos.x - position.x;
    float dz = playerPos.z - position.z;
    float distXZ = std::sqrt(dx * dx + dz * dz);

    switch (state)
    {
    case DazedZombieState::WaitingSpawn:
    {
        stateTimer -= dt;
        if (stateTimer <= 0.0f)
        {
            if (!SpawnAtRandomLocation(playerPos))
            {
                stateTimer = 1.0f;
            }
        }
        break;
    }

    case DazedZombieState::SittingDazed:
    {
        // 座り込み茫然アニメーションをループ再生
        AdvanceLoopingAnim(dt, 1.0f);

        // プレイヤーが十分近づいたか判定（壁越し誤反応防止の視線チェック付き）
        bool isCloseEnough = false;
        if (distXZ <= CLOSE_OVERRIDE_DISTANCE)
        {
            isCloseEnough = true;
        }
        else if (distXZ <= TRIGGER_DISTANCE)
        {
            VECTOR zEye = VGet(position.x, position.y + 0.85f, position.z);
            VECTOR pEye = VGet(playerPos.x, playerPos.y - 0.1f, playerPos.z);
            if (!WorldCollision::Instance().CheckSweep(zEye, pEye, 0.25f))
            {
                isCloseEnough = true;
            }
        }

        if (isCloseEnough)
        {
            state = DazedZombieState::Scream;
            stateTimer = 0.0f;
            animPlayTime = 0.0f;
            LookAtTarget(playerPos, 1.0f);
            AudioManager::Instance().PlayZombieScream();
            HorrorUI::Instance().TriggerHorrorGlitchBurst(0.85f, 0.45f);
            shockwaveActive = true;
            shockwaveCenter = VGet(position.x, 0.05f, position.z);
            shockwaveAge = 0.0f;
            EmitScreamAndVanishBurst(false);
            outFearDelta += 0.06f;
        }
        break;
    }

    case DazedZombieState::Scream:
    {
        // 絶叫モーションを1回通して再生しながらプレイヤー方向を見据える
        stateTimer += dt;
        LookAtTarget(playerPos, dt * 8.0f);

        if (screamAnimAttach != -1 && screamAnimTotal > 0.0f)
        {
            animPlayTime += dt * 30.0f * SCREAM_ANIM_SPEED;
            if (animPlayTime >= screamAnimTotal)
            {
                MV1SetAttachAnimTime(screamModelHandle, screamAnimAttach, screamAnimTotal - 0.01f);
                // 絶叫完了後、即座に負傷走行（Injured Run）でプレイヤーへ突進開始
                state = DazedZombieState::InjuredRun;
                stateTimer = 0.0f;
                animPlayTime = 0.0f;
                LookAtTarget(playerPos, 1.0f);
            }
            else
            {
                MV1SetAttachAnimTime(screamModelHandle, screamAnimAttach, animPlayTime);
            }
        }
        else
        {
            state = DazedZombieState::InjuredRun;
            stateTimer = 0.0f;
            animPlayTime = 0.0f;
        }
        break;
    }

    case DazedZombieState::InjuredRun:
    {
        stateTimer += dt;

        // 負傷走行アニメーションのループ再生
        AdvanceLoopingAnim(dt, INJURED_RUN_ANIM_SPEED);

        // プレイヤーへ向かって追跡移動（障害物めり込み解決付き）
        LookAtTarget(playerPos, dt * 10.0f);
        if (distXZ > MIN_CAMERA_STOP_DIST)
        {
            float invD = 1.0f / distXZ;
            float step = INJURED_RUN_SPEED * dt;
            if (step > distXZ - MIN_CAMERA_STOP_DIST)
            {
                step = distXZ - MIN_CAMERA_STOP_DIST;
            }
            position.x += dx * invD * step;
            position.z += dz * invD * step;
            WorldCollision::Instance().ResolveCollision(position, 0.40f);
            position.y = 0.0f;
        }

        outFearDelta += 0.05f * dt;

        // プレイヤーへ迫り切った瞬間、または一定時間突進したところで突然消失
        bool reachedClose = (stateTimer >= MIN_RUN_TIME_BEFORE_VANISH && distXZ <= VANISH_PROXIMITY);
        bool runTimedOut = (stateTimer >= MAX_RUN_DURATION);
        bool escapedFar = (distXZ >= 28.0f);

        if (reachedClose || runTimedOut || escapedFar)
        {
            state = DazedZombieState::Vanishing;
            stateTimer = 0.0f;
            vanishOpacity = 1.0f;
            EmitScreamAndVanishBurst(true);
            HorrorUI::Instance().TriggerHorrorGlitchBurst(0.55f, 0.28f);
            if (reachedClose)
            {
                outFearDelta += 0.05f;
            }
        }
        break;
    }

    case DazedZombieState::Vanishing:
    {
        stateTimer += dt;

        // 走りモーションを継続しつつ、突如かき消えるように透明化
        AdvanceLoopingAnim(dt, INJURED_RUN_ANIM_SPEED);

        float t = stateTimer / VANISH_DURATION;
        if (t >= 1.0f)
        {
            // 完全に消失し、待機後に別のランダム地点へ再出現
            if (runModelHandle != -1)
            {
                MV1SetOpacityRate(runModelHandle, 1.0f);
                MV1SetPosition(runModelHandle, VGet(0.0f, -999.0f, 0.0f));
            }
            if (lightHandle != -1)
            {
                SetLightEnableHandle(lightHandle, FALSE);
            }
            position = VGet(0.0f, -999.0f, 0.0f);
            state = DazedZombieState::WaitingSpawn;
            stateTimer = 2.5f + static_cast<float>(std::rand() % 250) / 100.0f;
            vanishOpacity = 1.0f;
        }
        else
        {
            // 一瞬のちらつきと急速フェードアウトで神隠し的な消失を表現
            float flicker = ((std::rand() % 4) == 0) ? 0.35f : 1.0f;
            vanishOpacity = (1.0f - t) * flicker;
        }
        break;
    }
    }

    // 補助ポイントライトの位置・色同期（視認性確保のためプレイヤー近接時のみ有効）
    if (lightHandle != -1)
    {
        if (!lightsSuppressed && IsVisible() && distXZ <= 22.0f)
        {
            float facingAngle = yawAngle - MODEL_FORWARD_OFFSET;
            float lightOffset = 1.35f;
            float lightY = (state == DazedZombieState::SittingDazed) ? 1.55f : 1.90f;
            SetLightPositionHandle(lightHandle, VGet(
                position.x + std::sin(facingAngle) * lightOffset,
                position.y + lightY,
                position.z + std::cos(facingAngle) * lightOffset
            ));
            SetLightEnableHandle(lightHandle, TRUE);

            if (state == DazedZombieState::Scream || state == DazedZombieState::InjuredRun)
            {
                SetLightRangeAttenHandle(lightHandle, 8.0f, 1.0f, 0.10f, 0.025f);
                SetLightDifColorHandle(lightHandle, GetColorF(0.95f, 0.58f, 0.54f, 1.0f));
                SetLightSpcColorHandle(lightHandle, GetColorF(0.55f, 0.30f, 0.30f, 1.0f));
            }
            else if (state == DazedZombieState::Vanishing)
            {
                SetLightRangeAttenHandle(lightHandle, 6.0f, 1.0f, 0.16f, 0.04f);
                SetLightDifColorHandle(lightHandle, GetColorF(0.60f * vanishOpacity, 0.40f * vanishOpacity, 0.50f * vanishOpacity, 1.0f));
                SetLightSpcColorHandle(lightHandle, GetColorF(0.30f * vanishOpacity, 0.25f * vanishOpacity, 0.30f * vanishOpacity, 1.0f));
            }
            else
            {
                // 座り込み茫然時：暗がりにうずくまる不気味な青白い微光
                SetLightRangeAttenHandle(lightHandle, 5.5f, 1.0f, 0.16f, 0.04f);
                SetLightDifColorHandle(lightHandle, GetColorF(0.52f, 0.58f, 0.68f, 1.0f));
                SetLightSpcColorHandle(lightHandle, GetColorF(0.30f, 0.35f, 0.42f, 1.0f));
            }
        }
        else
        {
            SetLightEnableHandle(lightHandle, FALSE);
        }
    }

    UpdateVfx(dt);
}

void DazedZombie::Draw()
{
    int handle = GetActiveModelHandle();
    if (handle != -1 && IsVisible())
    {
        float opacity = (state == DazedZombieState::Vanishing) ? vanishOpacity : 1.0f;
        MV1SetOpacityRate(handle, opacity);
        MV1SetPosition(handle, position);
        MV1SetRotationXYZ(handle, VGet(0.0f, yawAngle, 0.0f));
        MV1SetScale(handle, VGet(scale, scale, scale));
        MV1DrawModel(handle);

        if (opacity < 1.0f)
        {
            MV1SetOpacityRate(handle, 1.0f);
        }
    }

    DrawVfx();
}

void DazedZombie::Finalize()
{
    FinalizeVfxTextures();
    if (dazedModelHandle != -1)
    {
        if (dazedAnimAttach != -1) MV1DetachAnim(dazedModelHandle, dazedAnimAttach);
        MV1DeleteModel(dazedModelHandle);
        dazedModelHandle = -1;
        dazedAnimAttach = -1;
    }
    if (screamModelHandle != -1)
    {
        if (screamAnimAttach != -1) MV1DetachAnim(screamModelHandle, screamAnimAttach);
        MV1DeleteModel(screamModelHandle);
        screamModelHandle = -1;
        screamAnimAttach = -1;
    }
    if (runModelHandle != -1)
    {
        if (runAnimAttach != -1) MV1DetachAnim(runModelHandle, runAnimAttach);
        MV1DeleteModel(runModelHandle);
        runModelHandle = -1;
        runAnimAttach = -1;
    }
    if (lightHandle != -1)
    {
        DeleteLightHandle(lightHandle);
        lightHandle = -1;
    }
    state = DazedZombieState::WaitingSpawn;
}
