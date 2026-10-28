#include "BambooSaplingBlock.h"

#include "../../utils/Random.h"
#include "../Level.h"
#include "BambooBlock.h"

BambooSaplingBlock::BambooSaplingBlock() : Block(false, "assets/texture/block/bamboo_stage0.png")
{
    SetTransparent(true);
    SetCollidable(false);
    SetRenderShape(eRenderShape_CROSS);
    SetCrossJitter(eCrossJitter_None);
    SetRandomTicking(true);
    SetBreakSound("bamboo.sapling_place", 6);
    SetPlaceSound("bamboo.sapling_place", 6, 0.8f);
    SetAABB(AABB(glm::dvec3(4.0 / 16.0, 0.0, 4.0 / 16.0), glm::dvec3(12.0 / 16.0, 12.0 / 16.0, 12.0 / 16.0)));
}

void BambooSaplingBlock::OnPlace(Level *level, const BlockPos &pos)
{
    BambooBlock *bamboo = (BambooBlock *) Block::ByName("bamboo");
    BlockPos below(pos.x, pos.y - 1, pos.z);
    Block *belowBlock = Block::ById(level->GetBlockId(below));
    if (belowBlock == bamboo || belowBlock == this)
    {
        level->SetBlock(pos, bamboo, nullptr, bamboo->GetStateData(1, BambooBlock::eLeaves_None, 0));
        level->RecordBlockChange(pos);
    }
}

void BambooSaplingBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    BambooBlock *bamboo = (BambooBlock *) Block::ByName("bamboo");
    BlockPos above(pos.x, pos.y + 1, pos.z);
    if (!level->IsClientSide() && Block::ById(level->GetBlockId(above)) == bamboo)
    {
        level->SetBlock(pos, bamboo, nullptr, bamboo->GetStateData(bamboo->GetAge(level->GetBlockData(above)), BambooBlock::eLeaves_None, 0));
        level->RecordBlockChange(pos);
        return;
    }

    Block::OnNeighborChanged(level, pos);
}

bool BambooSaplingBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;

    Block *below = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)));
    return below == Block::ByName("bamboo") || below == this || BambooBlock::CanPlantOn(below);
}

void BambooSaplingBlock::RandomTick(Level *level, const BlockPos &pos)
{
    BlockPos above(pos.x, pos.y + 1, pos.z);
    if (level->IsClientSide() || level->GetRandom().NextInt(0, 2) != 0 || level->GetBlockId(above) != 0 || level->GetRawBrightness(above) < 9)
    {
        return;
    }

    BambooBlock *bamboo = (BambooBlock *) Block::ByName("bamboo");
    level->SetBlock(above, bamboo, nullptr, bamboo->GetStateData(0, BambooBlock::eLeaves_Small, 0));
    level->RecordBlockChange(above);
}
