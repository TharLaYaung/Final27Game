#include "Ground.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace
{
    struct GroundAssets
    {
        bool initialized = false;
        int grassTex = -1;
        int asphaltTex = -1;
        int curbTex = -1;
        int roadMarkTex = -1;
        int manholeTex = -1;
        int puddleTex = -1;

        std::vector<VERTEX3D> grassTris;
        std::vector<VERTEX3D> asphaltTris;
        std::vector<VERTEX3D> curbTris;
        std::vector<VERTEX3D> roadMarkTris;
        std::vector<VERTEX3D> manholeTris;
        std::vector<VERTEX3D> puddleTris;
    };

    GroundAssets g_assets;

    uint32_t Hash2D(int x, int y, uint32_t seed)
    {
        uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u + seed * 1442695041u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return h ^ (h >> 16);
    }

    float HashToFloat(uint32_t h)
    {
        return static_cast<float>(h & 0xFFFFu) / 65535.0f;
    }

    float SmoothTileNoise(float u, float v, int period, uint32_t seed)
    {
        float px = u * static_cast<float>(period);
        float py = v * static_cast<float>(period);
        int x0 = static_cast<int>(floorf(px));
        int y0 = static_cast<int>(floorf(py));
        float fx = px - static_cast<float>(x0);
        float fy = py - static_cast<float>(y0);

        float sx = fx * fx * (3.0f - 2.0f * fx);
        float sy = fy * fy * (3.0f - 2.0f * fy);

        auto wrap = [period](int k) {
            int r = k % period;
            return r < 0 ? r + period : r;
        };

        int ix0 = wrap(x0);
        int ix1 = wrap(x0 + 1);
        int iy0 = wrap(y0);
        int iy1 = wrap(y0 + 1);

        float n00 = HashToFloat(Hash2D(ix0, iy0, seed));
        float n10 = HashToFloat(Hash2D(ix1, iy0, seed));
        float n01 = HashToFloat(Hash2D(ix0, iy1, seed));
        float n11 = HashToFloat(Hash2D(ix1, iy1, seed));

        float nx0 = n00 + (n10 - n00) * sx;
        float nx1 = n01 + (n11 - n01) * sx;
        return nx0 + (nx1 - nx0) * sy;
    }

    float PeriodicFBM(float u, float v, int baseFreq, int octaves, uint32_t seed)
    {
        float sum = 0.0f;
        float amp = 0.5f;
        float norm = 0.0f;
        int freq = baseFreq;
        for (int i = 0; i < octaves; ++i)
        {
            sum += amp * SmoothTileNoise(u, v, freq, seed + static_cast<uint32_t>(i * 97));
            norm += amp;
            amp *= 0.5f;
            freq *= 2;
        }
        return (norm > 0.0f) ? (sum / norm) : 0.0f;
    }

    template <typename PixelFn>
    int CreateProceduralTexture(int size, PixelFn fn)
    {
        int soft = MakeARGB8ColorSoftImage(size, size);
        if (soft == -1) return -1;

        for (int y = 0; y < size; ++y)
        {
            float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(size);
            for (int x = 0; x < size; ++x)
            {
                float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(size);
                int r = 0, g = 0, b = 0, a = 255;
                fn(x, y, u, v, r, g, b, a);
                DrawPixelSoftImage(
                    soft, x, y,
                    std::clamp(r, 0, 255),
                    std::clamp(g, 0, 255),
                    std::clamp(b, 0, 255),
                    std::clamp(a, 0, 255)
                );
            }
        }

        int tex = CreateGraphFromSoftImage(soft);
        DeleteSoftImage(soft);
        return tex;
    }

    int BuildAsphaltTexture()
    {
        return CreateProceduralTexture(256, [](int x, int y, float u, float v, int& r, int& g, int& b, int& a) {
            float coarse = PeriodicFBM(u, v, 6, 3, 101);
            float damp = PeriodicFBM(u, v, 3, 2, 211);
            float grain = HashToFloat(Hash2D(x, y, 307));
            float sparkle = (grain > 0.94f) ? 26.0f : (grain - 0.5f) * 22.0f;

            // 微細な舗装ひび割れ線（低頻度）
            float crackWave = PeriodicFBM(u, v, 4, 2, 419);
            float crack = fabsf(crackWave - 0.5f);
            float crackShade = (crack < 0.016f) ? -22.0f * (1.0f - crack / 0.016f) : 0.0f;

            // 湿った濃いめのパッチで反射差を出す
            float wetness = (damp < 0.44f) ? (0.44f - damp) * 32.0f : 0.0f;
            float base = 88.0f + (coarse - 0.5f) * 26.0f + sparkle + crackShade - wetness;

            r = static_cast<int>(base * 0.95f);
            g = static_cast<int>(base * 0.98f);
            b = static_cast<int>(base * 1.07f);
            a = 255;
        });
    }

    int BuildGrassTexture()
    {
        return CreateProceduralTexture(256, [](int x, int y, float u, float v, int& r, int& g, int& b, int& a) {
            float patch = PeriodicFBM(u, v, 4, 3, 503);
            float blade = PeriodicFBM(u * 2.0f, v * 0.65f, 16, 2, 607);
            float speck = (HashToFloat(Hash2D(x, y, 709)) - 0.5f) * 16.0f;

            float gBase = 64.0f + (patch - 0.5f) * 28.0f + (blade - 0.5f) * 24.0f + speck;
            // 土が混じる部分を控えめに追加
            float soil = (patch < 0.36f) ? (0.36f - patch) * 45.0f : 0.0f;
            r = static_cast<int>(gBase * 0.72f + soil * 0.55f);
            g = static_cast<int>(gBase * 0.98f - soil * 0.15f);
            b = static_cast<int>(gBase * 0.74f - soil * 0.10f);
            a = 255;
        });
    }

    int BuildCurbTexture()
    {
        return CreateProceduralTexture(256, [](int x, int y, float u, float v, int& r, int& g, int& b, int& a) {
            float weather = PeriodicFBM(u, v, 5, 3, 811);
            float grit = (HashToFloat(Hash2D(x, y, 919)) - 0.5f) * 20.0f;
            float joint = (u < 0.025f || u > 0.975f) ? -34.0f : 0.0f;
            float shade = 136.0f + (weather - 0.5f) * 28.0f + grit + joint;
            r = static_cast<int>(shade * 0.96f);
            g = static_cast<int>(shade * 0.99f);
            b = static_cast<int>(shade * 1.04f);
            a = 255;
        });
    }

    int BuildRoadMarkTexture()
    {
        return CreateProceduralTexture(128, [](int x, int y, float u, float v, int& r, int& g, int& b, int& a) {
            float wear = PeriodicFBM(u, v, 5, 2, 1021);
            float grit = HashToFloat(Hash2D(x, y, 1103));
            float edgeU = (std::min)(u, 1.0f - u);
            float edgeV = (std::min)(v, 1.0f - v);
            float feather = (std::min)(1.0f, (std::min)(edgeU, edgeV) * 14.0f);

            float paint = (wear > 0.28f) ? 1.0f : (wear / 0.28f) * 0.55f;
            float val = 195.0f + (grit - 0.5f) * 26.0f;
            r = static_cast<int>(val * 0.98f);
            g = static_cast<int>(val);
            b = static_cast<int>(val * 1.03f);
            a = static_cast<int>(215.0f * feather * paint);
        });
    }

    int BuildManholeTexture()
    {
        return CreateProceduralTexture(128, [](int x, int y, float u, float v, int& r, int& g, int& b, int& a) {
            float dx = u - 0.5f;
            float dy = v - 0.5f;
            float dist = sqrtf(dx * dx + dy * dy);
            float ang = atan2f(dy, dx);

            if (dist > 0.485f)
            {
                r = g = b = a = 0;
                return;
            }

            float alpha = (dist > 0.46f) ? (0.485f - dist) / 0.025f : 1.0f;
            float rust = PeriodicFBM(u, v, 6, 2, 1201);
            float grit = (HashToFloat(Hash2D(x, y, 1301)) - 0.5f) * 14.0f;

            // 外周リム・同心円溝・放射模様
            float ringOuter = (dist > 0.415f && dist < 0.465f) ? 16.0f : 0.0f;
            float groove1 = (fabsf(dist - 0.34f) < 0.015f || fabsf(dist - 0.18f) < 0.014f) ? -28.0f : 0.0f;
            float lugs = (dist > 0.19f && dist < 0.40f && sinf(ang * 12.0f) > 0.35f) ? 14.0f : 0.0f;

            float base = 92.0f + ringOuter + groove1 + lugs + (rust - 0.5f) * 18.0f + grit;
            r = static_cast<int>(base + rust * 14.0f);
            g = static_cast<int>(base * 0.96f);
            b = static_cast<int>(base * 0.92f);
            a = static_cast<int>(255.0f * std::clamp(alpha, 0.0f, 1.0f));
        });
    }

    int BuildPuddleTexture()
    {
        return CreateProceduralTexture(128, [](int, int, float u, float v, int& r, int& g, int& b, int& a) {
            float dx = (u - 0.5f) * 2.0f;
            float dy = (v - 0.5f) * 2.0f;
            float warp = PeriodicFBM(u, v, 3, 3, 1409);
            float dist = sqrtf(dx * dx + dy * dy) + (warp - 0.5f) * 0.38f;

            if (dist >= 0.88f)
            {
                r = g = b = a = 0;
                return;
            }

            float t = 1.0f - std::clamp(dist / 0.88f, 0.0f, 1.0f);
            float smooth = t * t * (3.0f - 2.0f * t);

            // 濡れた鏡面風の暗い水たまり色（夜空・街灯を淡く映す）
            r = 72;
            g = 88;
            b = 114;
            a = static_cast<int>(155.0f * smooth);
        });
    }

    VERTEX3D MakeVertex(
        float x, float y, float z,
        VECTOR norm,
        COLOR_U8 dif, COLOR_U8 spc,
        float u, float v)
    {
        VERTEX3D vert{};
        vert.pos = VGet(x, y, z);
        vert.norm = norm;
        vert.dif = dif;
        vert.spc = spc;
        vert.u = u;
        vert.v = v;
        vert.su = u;
        vert.sv = v;
        return vert;
    }

    void AppendHorizontalGrid(
        std::vector<VERTEX3D>& out,
        float x1, float z1, float x2, float z2,
        float y, float step, float uvScale,
        COLOR_U8 difCenter, COLOR_U8 difEdge, float edgeFadeWidth,
        COLOR_U8 spc)
    {
        int nx = (std::max)(1, static_cast<int>(ceilf((x2 - x1) / step)));
        int nz = (std::max)(1, static_cast<int>(ceilf((z2 - z1) / step)));
        VECTOR up = VGet(0.0f, 1.0f, 0.0f);

        auto colorAt = [&](float px, float pz) -> COLOR_U8 {
            if (edgeFadeWidth <= 0.0f) return difCenter;
            float minD = (std::min)((std::min)(px - x1, x2 - px), (std::min)(pz - z1, z2 - pz));
            float t = std::clamp(minD / edgeFadeWidth, 0.0f, 1.0f);
            return GetColorU8(
                static_cast<BYTE>(difEdge.r + (difCenter.r - difEdge.r) * t),
                static_cast<BYTE>(difEdge.g + (difCenter.g - difEdge.g) * t),
                static_cast<BYTE>(difEdge.b + (difCenter.b - difEdge.b) * t),
                difCenter.a
            );
        };

        for (int iz = 0; iz < nz; ++iz)
        {
            float za = z1 + (z2 - z1) * (static_cast<float>(iz) / static_cast<float>(nz));
            float zb = z1 + (z2 - z1) * (static_cast<float>(iz + 1) / static_cast<float>(nz));
            for (int ix = 0; ix < nx; ++ix)
            {
                float xa = x1 + (x2 - x1) * (static_cast<float>(ix) / static_cast<float>(nx));
                float xb = x1 + (x2 - x1) * (static_cast<float>(ix + 1) / static_cast<float>(nx));

                VERTEX3D v00 = MakeVertex(xa, y, za, up, colorAt(xa, za), spc, xa * uvScale, za * uvScale);
                VERTEX3D v10 = MakeVertex(xb, y, za, up, colorAt(xb, za), spc, xb * uvScale, za * uvScale);
                VERTEX3D v11 = MakeVertex(xb, y, zb, up, colorAt(xb, zb), spc, xb * uvScale, zb * uvScale);
                VERTEX3D v01 = MakeVertex(xa, y, zb, up, colorAt(xa, zb), spc, xa * uvScale, zb * uvScale);

                out.push_back(v00);
                out.push_back(v01);
                out.push_back(v11);

                out.push_back(v00);
                out.push_back(v11);
                out.push_back(v10);
            }
        }
    }

    void AppendUnitQuad(
        std::vector<VERTEX3D>& out,
        float x1, float z1, float x2, float z2,
        float y, COLOR_U8 dif, COLOR_U8 spc, float angle = 0.0f)
    {
        VECTOR up = VGet(0.0f, 1.0f, 0.0f);
        float cx = (x1 + x2) * 0.5f;
        float cz = (z1 + z2) * 0.5f;
        float hx = (x2 - x1) * 0.5f;
        float hz = (z2 - z1) * 0.5f;
        float ca = cosf(angle);
        float sa = sinf(angle);

        auto rot = [&](float lx, float lz) {
            return VGet(cx + lx * ca - lz * sa, y, cz + lx * sa + lz * ca);
        };

        VECTOR p00 = rot(-hx, -hz);
        VECTOR p10 = rot(hx, -hz);
        VECTOR p11 = rot(hx, hz);
        VECTOR p01 = rot(-hx, hz);

        VERTEX3D v00 = MakeVertex(p00.x, p00.y, p00.z, up, dif, spc, 0.0f, 0.0f);
        VERTEX3D v10 = MakeVertex(p10.x, p10.y, p10.z, up, dif, spc, 1.0f, 0.0f);
        VERTEX3D v11 = MakeVertex(p11.x, p11.y, p11.z, up, dif, spc, 1.0f, 1.0f);
        VERTEX3D v01 = MakeVertex(p01.x, p01.y, p01.z, up, dif, spc, 0.0f, 1.0f);

        out.push_back(v00);
        out.push_back(v01);
        out.push_back(v11);

        out.push_back(v00);
        out.push_back(v11);
        out.push_back(v10);
    }

    void AppendCurbBlock(std::vector<VERTEX3D>& out, float cx, float cz, float hw, float hd, float height)
    {
        float x0 = cx - hw, x1 = cx + hw;
        float z0 = cz - hd, z1 = cz + hd;
        float y0 = 0.005f, y1 = height;
        float step = 2.0f;

        COLOR_U8 topDif = GetColorU8(136, 142, 152, 255);
        COLOR_U8 sideDif = GetColorU8(94, 100, 110, 255);
        COLOR_U8 spc = GetColorU8(32, 34, 38, 255);

        // 上面
        AppendHorizontalGrid(out, x0, z0, x1, z1, y1, step, 0.55f, topDif, topDif, 0.0f, spc);

        // 長手方向に応じた細分化側面ヘルパー
        auto appendSide = [&](float ax, float az, float bx, float bz, VECTOR norm) {
            float dx = bx - ax;
            float dz = bz - az;
            float len = sqrtf(dx * dx + dz * dz);
            int segs = (std::max)(1, static_cast<int>(ceilf(len / step)));
            for (int i = 0; i < segs; ++i)
            {
                float t0 = static_cast<float>(i) / static_cast<float>(segs);
                float t1 = static_cast<float>(i + 1) / static_cast<float>(segs);
                float sx0 = ax + dx * t0, sz0 = az + dz * t0;
                float sx1 = ax + dx * t1, sz1 = az + dz * t1;
                float u0 = len * t0 * 0.55f;
                float u1 = len * t1 * 0.55f;

                VERTEX3D a0 = MakeVertex(sx0, y0, sz0, norm, sideDif, spc, u0, 0.0f);
                VERTEX3D a1 = MakeVertex(sx1, y0, sz1, norm, sideDif, spc, u1, 0.0f);
                VERTEX3D b1 = MakeVertex(sx1, y1, sz1, norm, topDif, spc, u1, 0.35f);
                VERTEX3D b0 = MakeVertex(sx0, y1, sz0, norm, topDif, spc, u0, 0.35f);

                out.push_back(a0);
                out.push_back(a1);
                out.push_back(b1);

                out.push_back(a0);
                out.push_back(b1);
                out.push_back(b0);
            }
        };

        appendSide(x0, z1, x1, z1, VGet(0.0f, 0.0f, 1.0f));
        appendSide(x1, z0, x0, z0, VGet(0.0f, 0.0f, -1.0f));
        appendSide(x1, z1, x1, z0, VGet(1.0f, 0.0f, 0.0f));
        appendSide(x0, z0, x0, z1, VGet(-1.0f, 0.0f, 0.0f));
    }

    void AppendDashedCenterline(std::vector<VERTEX3D>& out, float a0, float a1, float fixedCoord, bool horizontal)
    {
        COLOR_U8 markDif = GetColorU8(170, 176, 185, 255);
        COLOR_U8 spc = GetColorU8(30, 32, 36, 255);
        const float yMark = 0.0125f;
        const float halfW = 0.065f;
        const float dashLen = 2.4f;
        const float gapLen = 2.2f;

        for (float pos = a0; pos + 0.6f < a1; pos += dashLen + gapLen)
        {
            float endPos = (std::min)(pos + dashLen, a1);
            if (horizontal)
            {
                AppendUnitQuad(out, pos, fixedCoord - halfW, endPos, fixedCoord + halfW, yMark, markDif, spc);
            }
            else
            {
                AppendUnitQuad(out, fixedCoord - halfW, pos, fixedCoord + halfW, endPos, yMark, markDif, spc);
            }
        }
    }

    void AppendCrosswalk(std::vector<VERTEX3D>& out, float cx, float cz, bool horizontalRoad)
    {
        COLOR_U8 markDif = GetColorU8(180, 186, 196, 255);
        COLOR_U8 spc = GetColorU8(30, 32, 36, 255);
        const float yMark = 0.0125f;

        for (int i = -2; i <= 2; ++i)
        {
            float offset = static_cast<float>(i) * 0.92f;
            if (horizontalRoad)
            {
                AppendUnitQuad(out, cx - 1.0f, cz + offset - 0.22f, cx + 1.0f, cz + offset + 0.22f, yMark, markDif, spc);
            }
            else
            {
                AppendUnitQuad(out, cx + offset - 0.22f, cz - 1.0f, cx + offset + 0.22f, cz + 1.0f, yMark, markDif, spc);
            }
        }
    }

    void BuildGroundGeometry()
    {
        g_assets.grassTex = BuildGrassTexture();
        g_assets.asphaltTex = BuildAsphaltTexture();
        g_assets.curbTex = BuildCurbTexture();
        g_assets.roadMarkTex = BuildRoadMarkTexture();
        g_assets.manholeTex = BuildManholeTexture();
        g_assets.puddleTex = BuildPuddleTexture();

        // 1. 芝・土ベース地面（中心街区は細かく2.5m分割、外縁は15m分割）
        COLOR_U8 grassDif = GetColorU8(95, 112, 96, 255);
        COLOR_U8 grassAoDif = GetColorU8(52, 62, 54, 255);
        COLOR_U8 zeroSpc = GetColorU8(10, 12, 10, 255);
        const float GSIZE = 120.0f;

        // 外周4領域＋中央詳細領域を重なりなく敷き詰め
        AppendHorizontalGrid(g_assets.grassTris, -42.0f, -40.0f, 42.0f, 46.0f, 0.0f, 2.5f, 0.35f, grassDif, grassAoDif, 3.0f, zeroSpc);
        AppendHorizontalGrid(g_assets.grassTris, -GSIZE, -GSIZE, GSIZE, -40.0f, 0.0f, 16.0f, 0.22f, grassAoDif, grassAoDif, 0.0f, zeroSpc);
        AppendHorizontalGrid(g_assets.grassTris, -GSIZE, 46.0f, GSIZE, GSIZE, 0.0f, 16.0f, 0.22f, grassAoDif, grassAoDif, 0.0f, zeroSpc);
        AppendHorizontalGrid(g_assets.grassTris, -GSIZE, -40.0f, -42.0f, 46.0f, 0.0f, 16.0f, 0.22f, grassAoDif, grassAoDif, 0.0f, zeroSpc);
        AppendHorizontalGrid(g_assets.grassTris, 42.0f, -40.0f, GSIZE, 46.0f, 0.0f, 16.0f, 0.22f, grassAoDif, grassAoDif, 0.0f, zeroSpc);

        // 2. 重なりのないアスファルト道路網（y=0.010f、2.0m分割、濡れたスペキュラ付き）
        COLOR_U8 roadCenterDif = GetColorU8(122, 126, 138, 255);
        COLOR_U8 roadEdgeDif = GetColorU8(78, 82, 92, 255);
        COLOR_U8 roadSpc = GetColorU8(95, 104, 120, 255);
        const float yRoad = 0.010f;
        const float rUv = 0.42f;

        // 西通り & 東通り（南大通り角まで貫通）
        AppendHorizontalGrid(g_assets.asphaltTris, -28.0f, -33.0f, -22.0f, 18.0f, yRoad, 1.8f, rUv, roadCenterDif, roadEdgeDif, 1.0f, roadSpc);
        AppendHorizontalGrid(g_assets.asphaltTris, 22.0f, -33.0f, 28.0f, 18.0f, yRoad, 1.8f, rUv, roadCenterDif, roadEdgeDif, 1.0f, roadSpc);
        // 南大通り・中央横通り・北横通り（東西通りに綺麗に接合）
        AppendHorizontalGrid(g_assets.asphaltTris, -22.0f, -33.0f, 22.0f, -27.0f, yRoad, 1.8f, rUv, roadCenterDif, roadEdgeDif, 1.0f, roadSpc);
        AppendHorizontalGrid(g_assets.asphaltTris, -22.0f, -13.0f, 22.0f, -7.0f, yRoad, 1.8f, rUv, roadCenterDif, roadEdgeDif, 1.0f, roadSpc);
        AppendHorizontalGrid(g_assets.asphaltTris, -22.0f, 12.0f, 22.0f, 18.0f, yRoad, 1.8f, rUv, roadCenterDif, roadEdgeDif, 1.0f, roadSpc);
        // 北路地
        AppendHorizontalGrid(g_assets.asphaltTris, -3.0f, 18.0f, 3.0f, 40.0f, yRoad, 1.8f, rUv, roadCenterDif, roadEdgeDif, 1.0f, roadSpc);

        // 3. 立体縁石ブロック（元の座標・高さを維持）
        AppendCurbBlock(g_assets.curbTris, -28.12f, -6.0f, 0.12f, 24.0f, 0.07f);
        AppendCurbBlock(g_assets.curbTris, 28.12f, -6.0f, 0.12f, 24.0f, 0.07f);
        AppendCurbBlock(g_assets.curbTris, 0.0f, -33.12f, 28.0f, 0.12f, 0.07f);
        AppendCurbBlock(g_assets.curbTris, -15.5f, 18.12f, 12.5f, 0.12f, 0.07f);
        AppendCurbBlock(g_assets.curbTris, 15.5f, 18.12f, 12.5f, 0.12f, 0.07f);
        AppendCurbBlock(g_assets.curbTris, 0.0f, 40.12f, 3.2f, 0.12f, 0.07f);
        AppendCurbBlock(g_assets.curbTris, -3.12f, 27.5f, 0.12f, 12.5f, 0.07f);
        AppendCurbBlock(g_assets.curbTris, 3.12f, 27.5f, 0.12f, 12.5f, 0.07f);

        // 4. 道路白線（外側線・破線センターライン・横断歩道・停止線）
        COLOR_U8 edgeLineDif = GetColorU8(165, 172, 180, 255);
        COLOR_U8 edgeSpc = GetColorU8(28, 30, 34, 255);
        const float yLine = 0.012f;

        // 外側車道線
        AppendUnitQuad(g_assets.roadMarkTris, -21.85f, -32.85f, 21.85f, -32.72f, yLine, edgeLineDif, edgeSpc);
        AppendUnitQuad(g_assets.roadMarkTris, -21.85f, -27.28f, 21.85f, -27.15f, yLine, edgeLineDif, edgeSpc);
        AppendUnitQuad(g_assets.roadMarkTris, -27.85f, -26.8f, -27.72f, 11.8f, yLine, edgeLineDif, edgeSpc);
        AppendUnitQuad(g_assets.roadMarkTris, -22.28f, -26.8f, -22.15f, -13.2f, yLine, edgeLineDif, edgeSpc);
        AppendUnitQuad(g_assets.roadMarkTris, -22.28f, -6.8f, -22.15f, 11.8f, yLine, edgeLineDif, edgeSpc);
        AppendUnitQuad(g_assets.roadMarkTris, 27.72f, -26.8f, 27.85f, 11.8f, yLine, edgeLineDif, edgeSpc);
        AppendUnitQuad(g_assets.roadMarkTris, 22.15f, -26.8f, 22.28f, -13.2f, yLine, edgeLineDif, edgeSpc);
        AppendUnitQuad(g_assets.roadMarkTris, 22.15f, -6.8f, 22.28f, 11.8f, yLine, edgeLineDif, edgeSpc);
        AppendUnitQuad(g_assets.roadMarkTris, -2.85f, 18.2f, -2.72f, 39.2f, yLine, edgeLineDif, edgeSpc);
        AppendUnitQuad(g_assets.roadMarkTris, 2.72f, 18.2f, 2.85f, 39.2f, yLine, edgeLineDif, edgeSpc);

        // 中央破線
        AppendDashedCenterline(g_assets.roadMarkTris, -20.5f, 20.5f, -30.0f, true);
        AppendDashedCenterline(g_assets.roadMarkTris, -20.5f, 20.5f, -10.0f, true);
        AppendDashedCenterline(g_assets.roadMarkTris, -20.5f, 20.5f, 15.0f, true);
        AppendDashedCenterline(g_assets.roadMarkTris, -25.5f, -14.5f, -25.0f, false);
        AppendDashedCenterline(g_assets.roadMarkTris, -5.5f, 10.5f, -25.0f, false);
        AppendDashedCenterline(g_assets.roadMarkTris, -25.5f, -14.5f, 25.0f, false);
        AppendDashedCenterline(g_assets.roadMarkTris, -5.5f, 10.5f, 25.0f, false);

        // 交差点横断歩道＆停止線
        AppendCrosswalk(g_assets.roadMarkTris, -20.8f, -30.0f, true);
        AppendCrosswalk(g_assets.roadMarkTris, 20.8f, -30.0f, true);
        AppendCrosswalk(g_assets.roadMarkTris, -25.0f, -25.8f, false);
        AppendCrosswalk(g_assets.roadMarkTris, -25.0f, 10.8f, false);
        AppendCrosswalk(g_assets.roadMarkTris, 25.0f, 10.8f, false);
        AppendUnitQuad(g_assets.roadMarkTris, -2.6f, 18.5f, 2.6f, 18.85f, yLine, edgeLineDif, edgeSpc);

        // 5. 鋳鉄製マンホール蓋（y=0.013f）
        COLOR_U8 mhDif = GetColorU8(150, 152, 160, 255);
        COLOR_U8 mhSpc = GetColorU8(130, 136, 146, 255);
        const float yMh = 0.013f;
        AppendUnitQuad(g_assets.manholeTris, -12.6f, -30.6f, -11.4f, -29.4f, yMh, mhDif, mhSpc, 0.3f);
        AppendUnitQuad(g_assets.manholeTris, -25.6f, -19.2f, -24.4f, -18.0f, yMh, mhDif, mhSpc, 1.1f);
        AppendUnitQuad(g_assets.manholeTris, -25.6f, 3.0f, -24.4f, 4.2f, yMh, mhDif, mhSpc, 0.7f);
        AppendUnitQuad(g_assets.manholeTris, 24.4f, -4.0f, 25.6f, -2.8f, yMh, mhDif, mhSpc, 2.0f);
        AppendUnitQuad(g_assets.manholeTris, -0.6f, 14.4f, 0.6f, 15.6f, yMh, mhDif, mhSpc, 0.2f);
        AppendUnitQuad(g_assets.manholeTris, -0.6f, 28.0f, 0.6f, 29.2f, yMh, mhDif, mhSpc, 1.4f);

        // 6. 夜露・雨上がり水たまり（街灯下や路肩の反射ポイント、y=0.014f）
        COLOR_U8 pudDif = GetColorU8(95, 110, 128, 255);
        COLOR_U8 pudSpc = GetColorU8(245, 250, 255, 255);
        const float yPud = 0.014f;
        AppendUnitQuad(g_assets.puddleTris, -7.2f, -29.8f, -3.4f, -27.4f, yPud, pudDif, pudSpc, 0.22f);
        AppendUnitQuad(g_assets.puddleTris, -25.5f, -27.5f, -21.8f, -24.8f, yPud, pudDif, pudSpc, -0.35f);
        AppendUnitQuad(g_assets.puddleTris, -27.6f, -14.5f, -24.6f, -10.8f, yPud, pudDif, pudSpc, 0.45f);
        AppendUnitQuad(g_assets.puddleTris, -24.8f, 9.5f, -21.5f, 13.0f, yPud, pudDif, pudSpc, -0.25f);
        AppendUnitQuad(g_assets.puddleTris, -8.5f, -10.5f, -4.8f, -7.4f, yPud, pudDif, pudSpc, 0.18f);
        AppendUnitQuad(g_assets.puddleTris, 21.8f, 9.2f, 25.4f, 12.6f, yPud, pudDif, pudSpc, 0.60f);
        AppendUnitQuad(g_assets.puddleTris, -2.5f, 21.5f, 1.4f, 24.8f, yPud, pudDif, pudSpc, -0.40f);

        g_assets.initialized = true;
    }

    void DrawBatch(const std::vector<VERTEX3D>& verts, int texHandle, int blendMode = DX_BLENDMODE_NOBLEND, int blendParam = 255)
    {
        if (verts.empty()) return;
        int polyNum = static_cast<int>(verts.size()) / 3;
        SetDrawBlendMode(blendMode, blendParam);
        DrawPolygon3D(verts.data(), polyNum, texHandle, TRUE);
    }
}

void DrawGround()
{
    if (!g_assets.initialized)
    {
        BuildGroundGeometry();
    }

    int prevDrawMode = GetDrawMode();
    SetUseLighting(TRUE);
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetDrawMode(DX_DRAWMODE_BILINEAR);

    SetTextureAddressMode(DX_TEXADDRESS_WRAP);
    DrawBatch(g_assets.grassTris, g_assets.grassTex);
    DrawBatch(g_assets.asphaltTris, g_assets.asphaltTex);
    DrawBatch(g_assets.curbTris, g_assets.curbTex);

    SetTextureAddressMode(DX_TEXADDRESS_CLAMP);
    // 路面標示・マンホール・水たまりは微妙なZオフセット上に透過合成
    SetWriteZBuffer3D(FALSE);
    DrawBatch(g_assets.roadMarkTris, g_assets.roadMarkTex, DX_BLENDMODE_ALPHA, 255);
    DrawBatch(g_assets.manholeTris, g_assets.manholeTex, DX_BLENDMODE_ALPHA, 255);
    DrawBatch(g_assets.puddleTris, g_assets.puddleTex, DX_BLENDMODE_ALPHA, 255);
    SetWriteZBuffer3D(TRUE);

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetDrawMode(prevDrawMode);
}

void GroundFinalize()
{
    int* handles[] = {
        &g_assets.grassTex,
        &g_assets.asphaltTex,
        &g_assets.curbTex,
        &g_assets.roadMarkTex,
        &g_assets.manholeTex,
        &g_assets.puddleTex
    };
    for (int* h : handles)
    {
        if (*h != -1)
        {
            DeleteGraph(*h);
            *h = -1;
        }
    }
    g_assets.grassTris.clear();
    g_assets.asphaltTris.clear();
    g_assets.curbTris.clear();
    g_assets.roadMarkTris.clear();
    g_assets.manholeTris.clear();
    g_assets.puddleTris.clear();
    g_assets.initialized = false;
}
