#pragma once

#include "Block.h"

class RedstoneLampBlock : public Block
{
public:
    RedstoneLampBlock(bool lit);

    void SetToggled(RedstoneLampBlock *toggled);
    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void Tick(Level *level, const BlockPos &pos) override;

private:
    bool m_lit;
    RedstoneLampBlock *m_toggled;
};
