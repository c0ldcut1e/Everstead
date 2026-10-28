#pragma once

#include "Block.h"

class LeavesBlock : public Block
{
public:
    LeavesBlock(const std::string &texturePath);

    void RandomTick(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;

    uint16_t GetNaturalData() const;

    static bool IsLeaves(const Block *block);
    static bool IsLog(const Block *block);
    static void FlagDecayAround(Level *level, const BlockPos &pos);

private:
    static constexpr int kSupportDistance = 6;

    bool IsSupported(Level *level, const BlockPos &pos) const;

    int m_naturalProperty;
    int m_checkDecayProperty;
};
