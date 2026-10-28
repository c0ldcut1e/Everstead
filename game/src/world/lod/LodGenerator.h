#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

#include "../generation/WorldGenSettings.h"
#include "LodStore.h"

class ThreadPool;

class LodGenerator
{
public:
    static const int s_levelCount    = 7;
    static const int s_minSpacing    = 1;
    static const int s_tileCells     = 32;
    static const int s_maxCellHeight = 8;
    static const int s_maxDistance   = 8192;

    static LodGenerator *GetInstance();

    LodGenerator();
    ~LodGenerator();

    void Submit(std::function<void()> task);
    void SetChunkBacklog(int backlog);
    LodVolume BuildTile(uint32_t seed, const WorldGenSettings &settings, int level, int tileX, int tileZ, bool generate);
    LodVolume BuildVolume(int level, int tileX, int tileZ, bool keepGenerated);
    LodStore *GetStore();

    static int GetTileSize(int level);
    static int GetCellHeight(int level);

private:
    void Throttle() const;
    void Generate(uint32_t seed, const WorldGenSettings &settings, int level, int tileX, int tileZ, LodVolume *volume) const;

    LodStore m_store;
    ThreadPool *m_pool;
    int m_threadCount;
    std::atomic<int> m_chunkBacklog;
};
