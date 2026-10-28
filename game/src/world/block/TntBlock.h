#pragma once

#include "Block.h"

class TntBlock : public Block
{
public:
    TntBlock();

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void Tick(Level *level, const BlockPos &pos) override;
    void OnExploded(Level *level, const BlockPos &pos) override;
    bool DropsFromExplosion() const override;

    static void Ignite(Level *level, const BlockPos &pos);

private:
    void IgniteIfPowered(Level *level, const BlockPos &pos);
};
