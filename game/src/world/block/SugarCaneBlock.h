#pragma once

#include "Block.h"

class SugarCaneBlock : public Block
{
public:
    SugarCaneBlock(const std::string &texturePath);

    void RandomTick(Level *level, const BlockPos &pos) override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;

private:
    int m_ageProperty;
};
