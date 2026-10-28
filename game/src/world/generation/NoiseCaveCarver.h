#pragma once

#include <FastNoiseLite.h>

#include "WorldGenStage.h"

class Block;

class NoiseCaveCarver : public WorldGenStage
{
public:
    static inline const int s_minCaveY = 6;
    static const int s_lavaLevel       = 10;

    NoiseCaveCarver(uint32_t seed, bool caveDimension);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    static const int s_cellWidth  = 4;
    static const int s_cellHeight = 4;
    static const int s_gridX      = Chunk::s_sizeX / s_cellWidth + 1;
    static const int s_gridY      = Chunk::s_sizeY / s_cellHeight + 1;
    static const int s_gridZ      = Chunk::s_sizeZ / s_cellWidth + 1;

    static int GridIndex(int x, int y, int z);
    static double Sample(const float *grid, int x, int y, int z);
    int FindSurface(const Chunk &chunk, int x, int z, bool *underwater) const;
    bool ShouldCarve(const float *cheese, const float *spaghettiA, const float *spaghettiB, int x, int y, int z, double entrance, int surface,
                     bool underwater, double openness) const;
    double GetDripstoneOpenness(int x, int surface, int z) const;

    FastNoiseLite m_cheese;
    FastNoiseLite m_spaghettiA;
    FastNoiseLite m_spaghettiB;
    FastNoiseLite m_entrance;
    FastNoiseLite m_dripstone;
    bool m_caveDimension;
    int m_maxY;
    Block *m_air;
    Block *m_stone;
    Block *m_dirt;
    Block *m_grassBlock;
    Block *m_sand;
    Block *m_snowBlock;
    Block *m_packedIce;
    Block *m_water;
};
