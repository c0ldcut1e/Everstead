#include "ShovelItem.h"

#include "../../utils/Direction.h"
#include "../../utils/Random.h"
#include "../Level.h"
#include "../block/Block.h"
#include "../chunk/Chunk.h"

ShovelItem::ShovelItem()
{}

bool ShovelItem::CanUseOn(Level *level, const BlockPos &pos, Direction *face) const
{
    if (face == Direction::down || pos.y + 1 >= Chunk::s_sizeY || level->GetBlockId(BlockPos(pos.x, pos.y + 1, pos.z)) != 0)
    {
        return false;
    }

    Block *block = Block::ById(level->GetBlockId(pos));
    return block == Block::ByName("grass_block") || block == Block::ByName("dirt") || block == Block::ByName("coarse_dirt") ||
           block == Block::ByName("podzol");
}

void ShovelItem::UseOn(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &look) const
{
    (void) face;
    (void) look;

    Block *dirtPath = Block::ByName("dirt_path");
    if (!level->IsClientSide())
    {
        int variants      = 1;
        std::string sound = dirtPath->GetStepSound(&variants);
        level->PlaySound(sound + std::to_string(level->GetRandom().NextInt(1, variants)), glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 1.0f,
                         1.0f);
    }

    level->SetBlock(pos, dirtPath);
    level->RecordBlockChange(pos);
}
