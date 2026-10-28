#pragma once

#include "../../utils/Random.h"
#include "../chunk/Chunk.h"

class Block;

class MegaSpruceTreeFeature
{
public:
    MegaSpruceTreeFeature();

    bool Place(Chunk &chunk, Random &random, int x, int y, int z, bool tallCrown) const;
    void PlaceGenerated(Chunk &chunk, Random &random, int x, int y, int z, bool tallCrown) const;

    static const int k_maxHeight = 32;

private:
    static bool IsInside(int x, int y, int z);
    void SetIfReplaceable(Chunk &chunk, int x, int y, int z, Block *block) const;
    void PlaceLeavesLayer(Chunk &chunk, int x, int y, int z, int width) const;
    void Grow(Chunk &chunk, Random &random, int x, int y, int z, int height, bool tallCrown) const;
    void PlacePodzolCircle(Chunk &chunk, int x, int y, int z) const;
    void PlacePodzolAt(Chunk &chunk, int x, int y, int z) const;

    Block *m_air;
    Block *m_log;
    Block *m_leaves;
    Block *m_dirt;
    Block *m_grassBlock;
    Block *m_coarseDirt;
    Block *m_podzol;
};
