#pragma once

#include "../../utils/Random.h"
#include "../chunk/Chunk.h"

class Block;
class Direction;

class SwampTreeFeature
{
public:
    SwampTreeFeature();

    void Place(Chunk &chunk, Random &random, int x, int y, int z) const;

private:
    static bool IsInside(int x, int y, int z);
    void SetIfReplaceable(Chunk &chunk, int x, int y, int z, Block *block) const;
    void HangVine(Chunk &chunk, int x, int y, int z, Direction *direction) const;

    Block *m_air;
    Block *m_log;
    Block *m_leaves;
    Block *m_vine;
    Block *m_dirt;
    Block *m_grassBlock;
};
