#pragma once

#include "../../utils/Random.h"
#include "../chunk/Chunk.h"

class Block;
class Direction;

class JungleTreeFeature
{
public:
    JungleTreeFeature();

    void Place(Chunk &chunk, Random &random, int x, int y, int z) const;
    bool PlaceMega(Chunk &chunk, Random &random, int x, int y, int z) const;
    void PlaceBush(Chunk &chunk, Random &random, int x, int y, int z) const;

private:
    static bool IsInside(int x, int y, int z);
    bool IsAir(Chunk &chunk, int x, int y, int z) const;
    void SetIfReplaceable(Chunk &chunk, int x, int y, int z, Block *block) const;
    void SetLeaves(Chunk &chunk, int x, int y, int z, Block *leaves) const;
    void PlaceVine(Chunk &chunk, int x, int y, int z, Direction *direction) const;
    void HangVine(Chunk &chunk, int x, int y, int z, Direction *direction) const;
    void HangLeafVines(Chunk &chunk, Random &random, int minX, int minY, int minZ, int maxX, int maxY, int maxZ) const;
    void PlaceDirt(Chunk &chunk, int x, int y, int z) const;

    Block *m_air;
    Block *m_log;
    Block *m_leaves;
    Block *m_oakLeaves;
    Block *m_vine;
    Block *m_cocoa;
    Block *m_dirt;
    Block *m_grassBlock;
};
