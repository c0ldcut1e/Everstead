#pragma once

#include "Block.h"

class TargetBlock : public Block
{
public:
    TargetBlock();

    void OnPlace(Level *level, const BlockPos &pos) override;
    void Tick(Level *level, const BlockPos &pos) override;
    void OnProjectileHit(Level *level, const BlockPos &pos, Entity *projectile, const glm::dvec3 &hitPos, Direction *face) override;
    int GetSignal(uint16_t data, Direction *direction) const override;
    bool IsSignalSource() const override;

private:
    void SetPower(Level *level, const BlockPos &pos, int power);

    int m_powerProperty;
};
