#include "GravityBlock.h"

#include "../../entity/block/FallingBlock.h"

#include "../Level.h"
#include "../chunk/Chunk.h"

static const int s_fallDelay = 2;

GravityBlock::GravityBlock(const std::string &texturePath) : Block(true, texturePath)
{}

// Falling only starts from block updates, so generated floating blocks stay put until something touches them
void GravityBlock::OnPlace(Level *level, const BlockPos &pos)
{
    level->ScheduleBlockForTick(pos, s_fallDelay, 0);
}

void GravityBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide() || pos.y <= 0 || !CanFallThrough(Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)))))
    {
        return;
    }

    level->ScheduleBlockForTick(pos, s_fallDelay, 0);
}

void GravityBlock::Tick(Level *level, const BlockPos &pos)
{
    BlockPos landing(pos.x, pos.y - 1, pos.z);
    if (landing.y < 0 || !CanFallThrough(Block::ById(level->GetBlockId(landing))))
    {
        return;
    }

    uint16_t data = level->GetBlockData(pos);
    std::unique_ptr<Entity> fallingBlock(new FallingBlock(level));
    fallingBlock->SetPosition(glm::dvec3(pos.x + 0.5, pos.y, pos.z + 0.5));
    fallingBlock->ReadSpawnData(((int) level->GetBlockId(pos) << 16) | (int) data);
    level->AddEntity(std::move(fallingBlock));
    level->SetBlock(pos, Block::ByName("air"));
    level->RecordBlockChange(pos);
}

bool GravityBlock::CanFallThrough(Block *block)
{
    static Block *air = Block::ByName("air");
    return !block || block == air || block->IsLiquid() || block->IsReplaceable();
}

// Blocks without collision such as torches are fallen into, and the landing breaks the falling block into an item
bool GravityBlock::CanFallInto(Block *block)
{
    return CanFallThrough(block) || !block->IsCollidable();
}
