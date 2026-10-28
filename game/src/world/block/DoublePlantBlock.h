#pragma once

#include "Block.h"

class DoublePlantBlock : public Block
{
public:
    DoublePlantBlock(const std::string &lowerTexturePath, const std::string &upperTexturePath);

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnClientPlace(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    BlockPos GetOtherPos(const BlockPos &pos, uint16_t data) const override;
    UVRect GetCrossAtlasUVRect(uint16_t data) const override;
    uint16_t GetUpperData() const;

private:
    bool IsUpper(uint16_t data) const;

    int m_halfProperty;
};
