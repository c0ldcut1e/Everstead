#include "RiverLayer.h"

namespace
{
    const uint64_t k_mul = 6364136223846793005ULL;
    const uint64_t k_add = 1442695040888963407ULL;

    int64_t Mix(int64_t value, int64_t salt)
    {
        uint64_t v = (uint64_t) value;
        v *= v * k_mul + k_add;
        v += (uint64_t) salt;
        return (int64_t) v;
    }

    struct Rng
    {
        int64_t seed;
        int64_t value;

        void Init(int64_t x, int64_t z)
        {
            value = Mix(Mix(Mix(Mix(seed, x), z), x), z);
        }

        int Next(int max)
        {
            int result = (int) ((value >> 24) % max);
            if (result < 0)
            {
                result += max;
            }

            value = Mix(value, seed);
            return result;
        }
    };

    int PickTwo(Rng &rng, int a, int b)
    {
        return rng.Next(2) == 0 ? a : b;
    }

    int PickMode(Rng &rng, int a, int b, int c, int d)
    {
        if (b == c && c == d)
            return b;
        if (a == b && a == c)
            return a;
        if (a == b && a == d)
            return a;
        if (a == c && a == d)
            return a;
        if (a == b && c != d)
            return a;
        if (a == c && b != d)
            return a;
        if (a == d && b != c)
            return a;
        if (b == c && a != d)
            return b;
        if (b == d && a != c)
            return b;
        if (c == d && a != b)
            return c;

        int s = rng.Next(4);
        if (s == 0)
            return a;
        if (s == 1)
            return b;
        if (s == 2)
            return c;
        return d;
    }

    struct TileCache
    {
        const RiverLayer *owner = nullptr;
        int tileX               = 0;
        int tileZ               = 0;
        std::vector<int> cells;
    };
} // namespace

RiverLayer::Stage RiverLayer::MakeStage(int64_t mixup, int64_t worldSeed)
{
    int64_t salt = mixup;
    salt         = Mix(salt, mixup);
    salt         = Mix(salt, mixup);
    salt         = Mix(salt, mixup);

    Stage stage;
    stage.seed = Mix(Mix(Mix(worldSeed, salt), salt), salt);
    return stage;
}

RiverLayer::RiverLayer(uint32_t seed)
{
    int64_t worldSeed = (int64_t) seed;
    m_init            = MakeStage(100, worldSeed);
    for (int i = 0; i < s_zoomCount; i++)
    {
        m_zooms[i] = MakeStage(1000 + i, worldSeed);
    }

    m_river  = MakeStage(1, worldSeed);
    m_smooth = MakeStage(1000, worldSeed);
}

std::vector<int> RiverLayer::GetArea(int stage, int xo, int zo, int w, int h) const
{
    if (stage == 0)
    {
        return GetInitArea(xo, zo, w, h);
    }

    return GetZoomArea(stage - 1, xo, zo, w, h);
}

std::vector<int> RiverLayer::GetInitArea(int xo, int zo, int w, int h) const
{
    std::vector<int> result(w * h);
    Rng rng{m_init.seed, 0};
    for (int z = 0; z < h; z++)
    {
        for (int x = 0; x < w; x++)
        {
            rng.Init(x + xo, z + zo);
            result[x + z * w] = rng.Next(2) + 2;
        }
    }

    return result;
}

std::vector<int> RiverLayer::GetZoomArea(int stage, int xo, int zo, int w, int h) const
{
    int px             = xo >> 1;
    int pz             = zo >> 1;
    int pw             = (w >> 1) + 3;
    int ph             = (h >> 1) + 3;
    std::vector<int> p = GetArea(stage, px, pz, pw, ph);

    int ww = pw << 1;
    std::vector<int> tmp(ww * (ph << 1));
    Rng rng{m_zooms[stage].seed, 0};
    for (int z = 0; z < ph - 1; z++)
    {
        int pp = (z << 1) * ww;
        int ul = p[z * pw];
        int dl = p[(z + 1) * pw];
        for (int x = 0; x < pw - 1; x++)
        {
            rng.Init((int64_t) (x + px) << 1, (int64_t) (z + pz) << 1);
            int ur       = p[(x + 1) + z * pw];
            int dr       = p[(x + 1) + (z + 1) * pw];
            tmp[pp]      = ul;
            tmp[pp + ww] = PickTwo(rng, ul, dl);
            pp++;
            tmp[pp]      = PickTwo(rng, ul, ur);
            tmp[pp + ww] = PickMode(rng, ul, ur, dl, dr);
            pp++;
            ul = ur;
            dl = dr;
        }
    }

    std::vector<int> result(w * h);
    for (int z = 0; z < h; z++)
    {
        int src = (z + (zo & 1)) * ww + (xo & 1);
        for (int x = 0; x < w; x++)
        {
            result[x + z * w] = tmp[src + x];
        }
    }

    return result;
}

std::vector<int> RiverLayer::GetRiverArea(int xo, int zo, int w, int h) const
{
    int pw             = w + 2;
    std::vector<int> p = GetArea(s_zoomCount, xo - 1, zo - 1, pw, h + 2);
    std::vector<int> result(w * h);
    for (int z = 0; z < h; z++)
    {
        for (int x = 0; x < w; x++)
        {
            int l             = p[x + (z + 1) * pw];
            int r             = p[(x + 2) + (z + 1) * pw];
            int u             = p[(x + 1) + z * pw];
            int d             = p[(x + 1) + (z + 2) * pw];
            int c             = p[(x + 1) + (z + 1) * pw];
            result[x + z * w] = (c != l || c != r || c != u || c != d) ? 1 : 0;
        }
    }

    return result;
}

std::vector<int> RiverLayer::GetSmoothArea(int xo, int zo, int w, int h) const
{
    int pw             = w + 2;
    std::vector<int> p = GetRiverArea(xo - 1, zo - 1, pw, h + 2);
    std::vector<int> result(w * h);
    Rng rng{m_smooth.seed, 0};
    for (int z = 0; z < h; z++)
    {
        for (int x = 0; x < w; x++)
        {
            int l = p[x + (z + 1) * pw];
            int r = p[(x + 2) + (z + 1) * pw];
            int u = p[(x + 1) + z * pw];
            int d = p[(x + 1) + (z + 2) * pw];
            int c = p[(x + 1) + (z + 1) * pw];
            if (l == r && u == d)
            {
                rng.Init(x + xo, z + zo);
                c = rng.Next(2) == 0 ? l : u;
            }
            else if (l == r)
            {
                c = l;
            }
            else if (u == d)
            {
                c = u;
            }

            result[x + z * w] = c;
        }
    }

    return result;
}

bool RiverLayer::IsRiver(int levelX, int levelZ) const
{
    thread_local TileCache s_cache;

    int quartX = levelX >> 2;
    int quartZ = levelZ >> 2;
    int tileX  = quartX >> 4;
    int tileZ  = quartZ >> 4;
    if (s_cache.owner != this || s_cache.tileX != tileX || s_cache.tileZ != tileZ || s_cache.cells.empty())
    {
        s_cache.cells = GetSmoothArea(tileX * s_tileSize, tileZ * s_tileSize, s_tileSize, s_tileSize);
        s_cache.owner = this;
        s_cache.tileX = tileX;
        s_cache.tileZ = tileZ;
    }

    return s_cache.cells[(quartX & (s_tileSize - 1)) + (quartZ & (s_tileSize - 1)) * s_tileSize] != 0;
}
