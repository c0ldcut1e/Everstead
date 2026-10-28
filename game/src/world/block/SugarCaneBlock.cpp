#include "SugarCaneBlock.h"

#include "../Level.h"

SugarCaneBlock::SugarCaneBlock(const std::string &texturePath) : Block(false, texturePath)
{
    SetBlocksLiquid(true);
    m_ageProperty = AddProperty("age", 16);
    SetCollidable(false);
    SetRenderShape(eRenderShape_CROSS);
    SetCrossJitter(eCrossJitter_None);
    SetAABB(AABB(glm::dvec3(0.125, 0.0, 0.125), glm::dvec3(0.875, 1.0, 0.875)));
    SetNeedsSupportBelow(true);
    SetRandomTicking(true);
}

void SugarCaneBlock::RandomTick(Level *level, const BlockPos &pos)
{
    BlockPos above(pos.x, pos.y + 1, pos.z);
    if (level->IsClientSide() || level->GetBlockId(above) != Block::ByName("air")->GetId())
    {
        return;
    }

    int height = 1;
    while (level->GetBlockId(BlockPos(pos.x, pos.y - height, pos.z)) == GetId())
    {
        height++;
    }

    if (height >= 3)
    {
        return;
    }

    uint16_t data = level->GetBlockData(pos);
    int age       = GetPropertyValue(data, m_ageProperty);
    if (age == 15)
    {
        level->SetBlock(above, this);
        level->RecordBlockChange(above);
        level->SetBlockData(pos, WithPropertyValue(data, m_ageProperty, 0));
    }
    else
    {
        level->SetBlockData(pos, WithPropertyValue(data, m_ageProperty, age + 1));
    }

    level->RecordBlockChange(pos);
}

bool SugarCaneBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;

    Block *below = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)));
    if (below == this)
    {
        return true;
    }

    if (below != Block::ByName("grass_block") && below != Block::ByName("dirt") && below != Block::ByName("sand") &&
        below != Block::ByName("red_sand"))
    {
        return false;
    }

    Direction *sides[] = {Direction::north, Direction::south, Direction::west, Direction::east};
    for (int i = 0; i < 4; i++)
    {
        Block *side = Block::ById(level->GetBlockId(BlockPos(pos.x + sides[i]->dx, pos.y - 1, pos.z + sides[i]->dz)));
        if (side == Block::ByName("water_still") || side == Block::ByName("water_flow"))
        {
            return true;
        }
    }

    return false;
}
