#pragma once

#include <vector>

#include "PistonPartBlock.h"

class PistonHeadBlock : public PistonPartBlock
{
public:
    PistonHeadBlock();

    void OnRemoved(Level *level, const BlockPos &pos, uint16_t data) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    uint16_t WithSticky(uint16_t data, bool sticky) const;

protected:
    int GetEndTexture(uint16_t data, bool front) const override;

private:
    int m_stickyProperty;
};
