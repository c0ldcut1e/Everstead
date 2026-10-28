#include "IceBlock.h"

#include "../Level.h"

IceBlock::IceBlock(const std::string &texturePath) : Block(true, texturePath)
{
    SetTransparent(true);
    SetTranslucent(true);
    SetRandomTicking(true);
    SetSlipperiness(0.98f);
}

void IceBlock::RandomTick(Level *level, const BlockPos &pos)
{
    if (level->GetBlockLightLevel(pos) <= s_meltLightLevel)
    {
        return;
    }

    level->SetBlock(pos, Block::ByName("water_flow"));
    level->RecordBlockChange(pos);
}
