#pragma once

#include "../../utils/Random.h"
#include "../chunk/Chunk.h"

class Block;

class LakeFeature
{
public:
    static const int s_width  = 16;
    static const int s_height = 8;

    struct Plan
    {
        bool active;
        int baseY;
        bool grid[s_width * s_width * s_height];
    };

    LakeFeature();

    void BuildPlan(Random &random, int baseY, Plan *plan) const;
    bool Covers(const Plan &plan, int x, int z) const;
    void Place(Chunk &chunk, const Plan &plan, Block *liquid, Random &random) const;

private:
    static int Index(int x, int y, int z);
    bool IsEdge(const Plan &plan, int x, int y, int z) const;

    Block *m_air;
    Block *m_stone;
    Block *m_dirt;
    Block *m_grassBlock;
};
