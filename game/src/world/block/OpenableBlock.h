#pragma once

#include "Block.h"

class OpenableBlock : public Block
{
public:
    OpenableBlock(bool solid, const std::string &texturePath);

    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    bool IsUsable(uint16_t data) const override;
    bool OnUse(Level *level, const BlockPos &pos) override;

protected:
    bool IsOpen(uint16_t data) const;
    uint16_t WithPlacementPower(Level *level, const BlockPos &pos, uint16_t data) const;

    // Subclasses add these themselves because the property order decides the saved data layout
    int m_openProperty;
    int m_poweredProperty;

private:
    bool IsPowered(Level *level, const BlockPos &pos, uint16_t data) const;
    void SetOpen(Level *level, const BlockPos &pos, bool open, bool powered);
};
