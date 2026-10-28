#pragma once

#include "Block.h"

class DirtPathBlock : public Block
{
public:
    DirtPathBlock(const std::string &sidePath, const std::string &topPath, const std::string &bottomPath);

    void OnNeighborChanged(Level *level, const BlockPos &pos) override;

    static bool IsCoveredBySolid(Level *level, const BlockPos &pos);
};
