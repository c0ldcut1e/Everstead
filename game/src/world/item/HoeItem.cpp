#include "HoeItem.h"

#include "../../utils/Direction.h"
#include "../../utils/Random.h"
#include "../Level.h"
#include "../block/Block.h"
#include "../chunk/Chunk.h"

HoeItem::HoeItem()
{}

bool HoeItem::CanUseOn(Level *level, const BlockPos &pos, Direction *face) const
{
    if (face == Direction::down || pos.y + 1 >= Chunk::s_sizeY || level->GetBlockId(BlockPos(pos.x, pos.y + 1, pos.z)) != 0)
    {
        return false;
    }

    Block *block = Block::ById(level->GetBlockId(pos));
    return block == Block::ByName("grass_block") || block == Block::ByName("dirt");
}

void HoeItem::UseOn(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &look) const
{
    (void) face;
    (void) look;

    Block *farmland = Block::ByName("farmland");
    if (!level->IsClientSide())
    {
        int variants      = 1;
        std::string sound = farmland->GetStepSound(&variants);
        level->PlaySound(sound + std::to_string(level->GetRandom().NextInt(1, variants)), glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 1.0f,
                         0.8f);
    }

    level->SetBlock(pos, farmland);
    level->RecordBlockChange(pos);
}
