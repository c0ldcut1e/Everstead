#pragma once

#include "ChunkMesher.h"

class ChunkMesherGreedyMesher : private ChunkMesher
{
public:
    static void Greedy2D(std::vector<MaskCell> &mask, int width, int height, bool useGreedy, std::vector<GreedyRect> *outRects);
};
