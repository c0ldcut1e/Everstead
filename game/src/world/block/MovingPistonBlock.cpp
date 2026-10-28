#include "MovingPistonBlock.h"

#include <memory>

#include "../../core/EversteadServer.h"
#include "../../network/PlayerList.h"
#include "../../network/packet/s2c/MovingPistonS2CPacket.h"
#include "../../tag/CompoundTag.h"
#include "../ServerLevel.h"
#include "../blockentity/PistonMovingBlockEntity.h"

MovingPistonBlock::MovingPistonBlock() : Block(false, "assets/texture/block/piston_side.png")
{
    SetBlocksLiquid(true);
    SetTransparent(true);
    SetCollidable(true);
    SetRenderShape(eRenderShape_BOXES);
}

void MovingPistonBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    (void) out;
}

void MovingPistonBlock::GetPlacedBoxes(const Level *level, const BlockPos &pos, uint16_t data, std::vector<AABB> *out) const
{
    GetPlacedCollisionBoxes(level, pos, data, out);
}

void MovingPistonBlock::GetSelectionBoxes(const Level *level, const BlockPos &pos, uint16_t data, std::vector<AABB> *out) const
{
    GetPlacedCollisionBoxes(level, pos, data, out);
    for (size_t i = 0; i < out->size(); i++)
    {
        (*out)[i] = (*out)[i].Translated(glm::dvec3(pos.x, pos.y, pos.z));
    }
}

// The client can get the block before the packet that creates its block entity
void MovingPistonBlock::GetPlacedCollisionBoxes(const Level *level, const BlockPos &pos, uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>>::const_iterator found = level->GetBlockEntities().find(pos);
    if (found == level->GetBlockEntities().end())
    {
        return;
    }

    PistonMovingBlockEntity *blockEntity = (PistonMovingBlockEntity *) found->second.get();
    Block *block                         = blockEntity->GetBlock();
    if (!block->IsCollidable())
    {
        return;
    }

    std::vector<AABB> boxes;
    block->GetCollisionBoxes(blockEntity->GetData(), &boxes);
    // A retracting piston base stays in place while only its head slides back
    glm::dvec3 offset = blockEntity->IsSource() && !blockEntity->IsExtending() ? glm::dvec3(0.0) : blockEntity->GetOffset(1.0f);
    for (size_t i = 0; i < boxes.size(); i++)
    {
        out->push_back(boxes[i].Translated(offset));
    }
}

const char *MovingPistonBlock::GetBlockEntityId() const
{
    return "piston";
}

void MovingPistonBlock::Place(Level *level, const BlockPos &pos, Block *block, uint16_t data, Direction *facing, bool extending, bool source)
{
    level->SetBlockAndDataNoNeighborUpdates(pos, this, 0);
    level->RecordBlockChange(pos);
    PistonMovingBlockEntity *blockEntity = (PistonMovingBlockEntity *) level->GetBlockEntity(pos);
    blockEntity->Init(block, data, facing, extending, source);

    std::unique_ptr<CompoundTag> tag(new CompoundTag());
    blockEntity->Save(tag.get());
    ((ServerLevel *) level)
            ->GetServer()
            ->GetPlayerList()
            ->SendToLevel(level, std::shared_ptr<Packet>(new MovingPistonS2CPacket(pos, std::move(tag))));
}
