#pragma once

#include <vector>

#include "TextureAtlas.h"

class BlockAtlas
{
public:
    static BlockAtlas *GetInstance();

    void Build();
    void TickAnimations(double deltaTime, const std::vector<TextureAtlas::Region> &visibleBlockRegions,
                        const std::vector<TextureAtlas::Region> &visibleFlowRegions);

    TextureAtlas *GetBlockAtlas();
    TextureAtlas *GetFlowAtlas();

private:
    TextureAtlas m_blockAtlas;
    TextureAtlas m_flowAtlas;
};
