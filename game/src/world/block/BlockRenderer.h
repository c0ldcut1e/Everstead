#pragma once

#include <cstdint>

#include "Block.h"

class BlockRenderer
{
public:
    static bool IsWaterBlock(Block *block, Block *waterStillBlock, Block *waterFlowBlock);
    static bool TesselateWaterInWorld(Block *block, Block *neighbor, Block *waterStillBlock, Block *waterFlowBlock);
    static bool TesselateInWorld(Block *block, Block *neighbor, Block *waterStillBlock, Block *waterFlowBlock);

    static void RenderBlockAsItem(Block *block, uint32_t color);
    static void RenderBlockAsEntity(Block *block, uint16_t data, uint32_t color);

    static float xOff;
    static float yOff;
    static float zOff;

private:
    static void RenderBlock(Block *block, uint16_t data, bool item, uint32_t color);
};
