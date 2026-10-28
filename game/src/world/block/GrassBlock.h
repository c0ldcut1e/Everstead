#pragma once

#include "Block.h"

class GrassBlock : public Block
{
public:
    GrassBlock(const std::string &texturePath);

    void RandomTick(Level *level, const BlockPos &pos) override;
    UVRect GetSnowySideAtlasUVRect() const;

private:
    static constexpr int kMinSurviveLight = 4;
    static constexpr int kMinSpreadLight  = 9;
    static constexpr int kSpreadAttempts  = 4;

    static bool IsSmothered(Level *level, const BlockPos &pos);

    int m_snowySide;
};
