#pragma once

#include "Block.h"

class SaplingBlock : public Block
{
public:
    enum eTreeType
    {
        eTreeType_Oak,
        eTreeType_Spruce,
        eTreeType_Birch,
        eTreeType_Acacia,
        eTreeType_Jungle,
        eTreeType_Cherry,
        eTreeType_DarkOak,
    };

    SaplingBlock(const std::string &texturePath, eTreeType treeType);

    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    void RandomTick(Level *level, const BlockPos &pos) override;
    void Grow(Level *level, const BlockPos &pos) const;

private:
    bool IsSameSapling(Level *level, int x, int y, int z) const;
    bool FindTwoByTwo(Level *level, const BlockPos &pos, int *offsetX, int *offsetZ) const;

    eTreeType m_treeType;
};
