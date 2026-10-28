#pragma once

#include <string>
#include <unordered_set>

#include "../Explosion.h"
#include "Block.h"

class CataclysmExperimentBlock : public Block
{
public:
    CataclysmExperimentBlock(const std::string &theme, eEXPLOSION_TYPE charge, eEXPLOSION_TYPE blast, int chargeTicks, float blastRadius);

    bool IsUsable(uint16_t data) const override;
    bool OnUse(Level *level, const BlockPos &pos) override;
    void OnBreak(Level *level, const BlockPos &pos) override;
    void Tick(Level *level, const BlockPos &pos) override;

private:
    std::string m_sound;
    eEXPLOSION_TYPE m_charge;
    eEXPLOSION_TYPE m_blast;
    int m_chargeTicks;
    float m_blastRadius;
    std::unordered_set<BlockPos> m_charging;
};
