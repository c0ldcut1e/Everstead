#include "PistonBaseBlock.h"

#include <algorithm>

#include "../../utils/Random.h"
#include "../Level.h"
#include "../ServerLevel.h"
#include "../blockentity/PistonMovingBlockEntity.h"
#include "MovingPistonBlock.h"
#include "PistonHeadBlock.h"

PistonBaseBlock::PistonBaseBlock(bool sticky)
    : PistonPartBlock(sticky ? "assets/texture/block/piston_top_sticky.png" : "assets/texture/block/piston_top.png")
{
    m_sticky           = sticky;
    m_extendedProperty = AddProperty("extended", 2);
}

void PistonBaseBlock::OnPlace(Level *level, const BlockPos &pos)
{
    CheckState(level, pos);
}

void PistonBaseBlock::OnRemoved(Level *level, const BlockPos &pos, uint16_t data)
{
    if (level->IsClientSide() || !IsExtended(data))
    {
        return;
    }

    Direction *facing = GetFacing(data);
    BlockPos front(pos.x + facing->dx, pos.y + facing->dy, pos.z + facing->dz);
    Block *block = Block::ById(level->GetBlockId(front));
    if (block == Block::ByName("piston_head"))
    {
        level->SetBlock(front, Block::ByName("air"));
        level->RecordBlockChange(front);
    }
    else if (block == Block::ByName("moving_piston") && ((PistonMovingBlockEntity *) level->GetBlockEntity(front))->IsSource())
    {
        level->SetBlock(front, Block::ByName("air"));
        level->RecordBlockChange(front);
    }
}

void PistonBaseBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    CheckState(level, pos);
}

void PistonBaseBlock::CheckState(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide())
    {
        return;
    }

    uint16_t data     = level->GetBlockData(pos);
    bool powered      = IsPowered(level, pos, GetFacing(data));
    Direction *facing = GetFacing(data);
    if (powered && !IsExtended(data))
    {
        PushStructure structure;
        structure.pistonPos     = pos;
        structure.moveDirection = facing;
        if (ResolvePushStructure(level, &structure, BlockPos(pos.x + facing->dx, pos.y + facing->dy, pos.z + facing->dz)))
        {
            level->AddBlockEvent(pos, this, 0);
        }
    }
    else if (!powered && IsExtended(data))
    {
        // Event 2 makes a sticky piston drop a block it is still pushing instead of pulling it back
        BlockPos pulled(pos.x + facing->dx * 2, pos.y + facing->dy * 2, pos.z + facing->dz * 2);
        int id = 1;
        if (Block::ById(level->GetBlockId(pulled)) == Block::ByName("moving_piston"))
        {
            PistonMovingBlockEntity *blockEntity = (PistonMovingBlockEntity *) level->GetBlockEntity(pulled);
            if (blockEntity->GetFacing() == facing && blockEntity->IsExtending() &&
                (blockEntity->GetProgress() < 0.5f || blockEntity->GetLastTicked() == level->GetGameTime() ||
                 ((ServerLevel *) level)->IsHandlingBlockTicks()))
            {
                id = 2;
            }
        }

        level->AddBlockEvent(pos, this, id);
    }
}

void PistonBaseBlock::TriggerEvent(Level *level, const BlockPos &pos, int id)
{
    uint16_t data = level->GetBlockData(pos);
    bool powered  = IsPowered(level, pos, GetFacing(data));
    if (id == 0 && powered)
    {
        Extend(level, pos, data);
    }
    else if (id != 0 && !powered)
    {
        Retract(level, pos, data, id);
    }
}

// We also check the block above the piston because pistons react to power one block up
bool PistonBaseBlock::IsPowered(Level *level, const BlockPos &pos, Direction *facing) const
{
    Direction *const directions[] = {Direction::down, Direction::up, Direction::north, Direction::south, Direction::west, Direction::east};
    for (int i = 0; i < 6; i++)
    {
        Direction *direction = directions[i];
        if (direction != facing &&
            level->GetSignal(BlockPos(pos.x + direction->dx, pos.y + direction->dy, pos.z + direction->dz), Direction::Opposite(direction)) > 0)
        {
            return true;
        }
    }

    BlockPos above(pos.x, pos.y + 1, pos.z);
    for (int i = 1; i < 6; i++)
    {
        Direction *direction = directions[i];
        if (level->GetSignal(BlockPos(above.x + direction->dx, above.y + direction->dy, above.z + direction->dz), Direction::Opposite(direction)) > 0)
        {
            return true;
        }
    }

    return false;
}

bool PistonBaseBlock::ResolvePushStructure(Level *level, PushStructure *structure, const BlockPos &start) const
{
    if (start.y < 0 || start.y >= Chunk::s_sizeY)
    {
        return false;
    }

    Block *block = Block::ById(level->GetBlockId(start));
    if (block == Block::ByName("air"))
    {
        return true;
    }

    if (!CanPush(block, level->GetBlockData(start)))
    {
        return false;
    }

    if (IsDestroyedByPush(block))
    {
        structure->toDestroy.push_back(start);
        return true;
    }

    if (!AddBlockLine(level, structure, start))
    {
        return false;
    }

    Block *slime = Block::ByName("slime_block");
    for (size_t i = 0; i < structure->toMove.size(); i++)
    {
        if (Block::ById(level->GetBlockId(structure->toMove[i])) == slime && !AddBranchingBlocks(level, structure, structure->toMove[i]))
        {
            return false;
        }
    }

    return true;
}

// We collect slime pulled along behind the origin first, then the blocks ahead of it, keeping the list ordered back to front
bool PistonBaseBlock::AddBlockLine(Level *level, PushStructure *structure, const BlockPos &origin) const
{
    if (origin.y < 0 || origin.y >= Chunk::s_sizeY)
    {
        return true;
    }

    Block *air                    = Block::ByName("air");
    Block *slime                  = Block::ByName("slime_block");
    Block *block                  = Block::ById(level->GetBlockId(origin));
    std::vector<BlockPos> &toMove = structure->toMove;
    if (block == air || !CanPush(block, level->GetBlockData(origin)) || IsDestroyedByPush(block) || origin == structure->pistonPos ||
        std::find(toMove.begin(), toMove.end(), origin) != toMove.end())
    {
        return true;
    }

    Direction *move = structure->moveDirection;
    int count       = 1;
    if (count + toMove.size() > 12)
    {
        return false;
    }

    while (block == slime)
    {
        BlockPos behind(origin.x - move->dx * count, origin.y - move->dy * count, origin.z - move->dz * count);
        if (behind.y < 0 || behind.y >= Chunk::s_sizeY)
        {
            break;
        }

        block = Block::ById(level->GetBlockId(behind));
        if (block == air || !CanPush(block, level->GetBlockData(behind)) || IsDestroyedByPush(block) || behind == structure->pistonPos)
        {
            break;
        }

        count++;
        if (count + toMove.size() > 12)
        {
            return false;
        }
    }

    int added = 0;
    for (int i = count - 1; i >= 0; i--)
    {
        toMove.push_back(BlockPos(origin.x - move->dx * i, origin.y - move->dy * i, origin.z - move->dz * i));
        added++;
    }

    for (int step = 1;; step++)
    {
        BlockPos ahead(origin.x + move->dx * step, origin.y + move->dy * step, origin.z + move->dz * step);
        std::vector<BlockPos>::iterator found = std::find(toMove.begin(), toMove.end(), ahead);
        if (found != toMove.end())
        {
            // This line runs into blocks we already have, so it moves in front of them to keep the back to front order
            size_t collision = found - toMove.begin();
            std::vector<BlockPos> reordered(toMove.begin(), toMove.begin() + collision);
            reordered.insert(reordered.end(), toMove.end() - added, toMove.end());
            reordered.insert(reordered.end(), toMove.begin() + collision, toMove.end() - added);
            toMove = reordered;
            for (size_t i = 0; i <= collision + added; i++)
            {
                if (Block::ById(level->GetBlockId(toMove[i])) == slime && !AddBranchingBlocks(level, structure, toMove[i]))
                {
                    return false;
                }
            }

            return true;
        }

        if (ahead.y < 0 || ahead.y >= Chunk::s_sizeY)
        {
            return false;
        }

        block = Block::ById(level->GetBlockId(ahead));
        if (block == air)
        {
            return true;
        }

        if (!CanPush(block, level->GetBlockData(ahead)) || ahead == structure->pistonPos)
        {
            return false;
        }

        if (IsDestroyedByPush(block))
        {
            structure->toDestroy.push_back(ahead);
            return true;
        }

        if (toMove.size() >= 12)
        {
            return false;
        }

        toMove.push_back(ahead);
        added++;
    }
}

bool PistonBaseBlock::AddBranchingBlocks(Level *level, PushStructure *structure, const BlockPos &from) const
{
    Direction *const directions[] = {Direction::down, Direction::up, Direction::north, Direction::south, Direction::west, Direction::east};
    Direction *move               = structure->moveDirection;
    for (int i = 0; i < 6; i++)
    {
        Direction *direction = directions[i];
        if ((direction->dx != 0 && move->dx != 0) || (direction->dy != 0 && move->dy != 0) || (direction->dz != 0 && move->dz != 0))
        {
            continue;
        }

        if (!AddBlockLine(level, structure, BlockPos(from.x + direction->dx, from.y + direction->dy, from.z + direction->dz)))
        {
            return false;
        }
    }

    return true;
}

// Every origin is cleared before anything is placed because slime can move blocks into spots that are not being vacated
bool PistonBaseBlock::MoveBlocks(Level *level, const BlockPos &pos, Direction *facing, bool extending) const
{
    PushStructure structure;
    structure.pistonPos     = pos;
    structure.moveDirection = extending ? facing : Direction::Opposite(facing);
    int startDistance       = extending ? 1 : 2;
    if (!ResolvePushStructure(level, &structure,
                              BlockPos(pos.x + facing->dx * startDistance, pos.y + facing->dy * startDistance, pos.z + facing->dz * startDistance)))
    {
        return false;
    }

    Block *air = Block::ByName("air");
    for (size_t i = 0; i < structure.toDestroy.size(); i++)
    {
        const BlockPos &destroyed = structure.toDestroy[i];
        Block::ById(level->GetBlockId(destroyed))->DropAsItem(level, destroyed, level->GetBlockData(destroyed));
        level->SetBlockAndDataNoNeighborUpdates(destroyed, air, 0);
        level->RecordBlockChange(destroyed);
    }

    std::vector<Block *> blocks;
    std::vector<uint16_t> blockData;
    for (size_t i = 0; i < structure.toMove.size(); i++)
    {
        blocks.push_back(Block::ById(level->GetBlockId(structure.toMove[i])));
        blockData.push_back(level->GetBlockData(structure.toMove[i]));
        level->SetBlockAndDataNoNeighborUpdates(structure.toMove[i], air, 0);
        level->RecordBlockChange(structure.toMove[i]);
    }

    Direction *move           = structure.moveDirection;
    MovingPistonBlock *moving = (MovingPistonBlock *) Block::ByName("moving_piston");
    for (size_t i = structure.toMove.size(); i > 0; i--)
    {
        const BlockPos &from = structure.toMove[i - 1];
        moving->Place(level, BlockPos(from.x + move->dx, from.y + move->dy, from.z + move->dz), blocks[i - 1], blockData[i - 1], facing, extending,
                      false);
    }

    for (size_t i = 0; i < structure.toDestroy.size(); i++)
    {
        level->NotifyBlockNeighbors(structure.toDestroy[i]);
    }

    for (size_t i = 0; i < structure.toMove.size(); i++)
    {
        const BlockPos &from = structure.toMove[i];
        level->NotifyBlockNeighbors(from);
        level->NotifyBlockNeighbors(BlockPos(from.x + move->dx, from.y + move->dy, from.z + move->dz));
    }

    return true;
}

bool PistonBaseBlock::CanPush(Block *block, uint16_t data) const
{
    if (block == Block::ByName("obsidian") || block->GetDestroyTime() < 0.0f || block->GetBlockEntityId() || block == Block::ByName("piston_head"))
    {
        return false;
    }

    PistonBaseBlock *piston = dynamic_cast<PistonBaseBlock *>(block);
    return !piston || !piston->IsExtended(data);
}

bool PistonBaseBlock::IsDestroyedByPush(Block *block) const
{
    return block->IsReplaceable() || !block->IsCollidable();
}

void PistonBaseBlock::Extend(Level *level, const BlockPos &pos, uint16_t data)
{
    Direction *facing = GetFacing(data);
    if (!MoveBlocks(level, pos, facing, true))
    {
        return;
    }

    level->SetBlockData(pos, WithExtended(data, true));
    level->RecordBlockChange(pos);

    MovingPistonBlock *moving = (MovingPistonBlock *) Block::ByName("moving_piston");
    PistonHeadBlock *head     = (PistonHeadBlock *) Block::ByName("piston_head");
    BlockPos front(pos.x + facing->dx, pos.y + facing->dy, pos.z + facing->dz);
    moving->Place(level, front, head, head->WithSticky(head->WithFacing(0, facing), m_sticky), facing, true, true);

    level->NotifyBlockNeighbors(pos);
    level->NotifyBlockNeighbors(front);
    level->PlaySound("tile.piston.out", glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 0.5f, level->GetRandom().NextFloat() * 0.25f + 0.6f);
}

void PistonBaseBlock::Retract(Level *level, const BlockPos &pos, uint16_t data, int id)
{
    Direction *facing         = GetFacing(data);
    MovingPistonBlock *moving = (MovingPistonBlock *) Block::ByName("moving_piston");
    Block *air                = Block::ByName("air");
    BlockPos front(pos.x + facing->dx, pos.y + facing->dy, pos.z + facing->dz);
    if (Block::ById(level->GetBlockId(front)) == moving)
    {
        ((PistonMovingBlockEntity *) level->GetBlockEntity(front))->Finish();
    }

    moving->Place(level, pos, this, WithExtended(data, false), facing, false, true);
    level->NotifyBlockNeighbors(pos);
    // The head goes away without neighbor updates before anything is pulled, so the pull never sees it
    if (Block::ById(level->GetBlockId(front)) == Block::ByName("piston_head"))
    {
        level->SetBlockAndDataNoNeighborUpdates(front, air, 0);
        level->RecordBlockChange(front);
    }

    if (m_sticky)
    {
        BlockPos pulled(front.x + facing->dx, front.y + facing->dy, front.z + facing->dz);
        Block *pulledBlock = Block::ById(level->GetBlockId(pulled));
        bool spat          = false;
        if (pulledBlock == moving)
        {
            PistonMovingBlockEntity *blockEntity = (PistonMovingBlockEntity *) level->GetBlockEntity(pulled);
            if (blockEntity->GetFacing() == facing && blockEntity->IsExtending())
            {
                blockEntity->Finish();
                spat = true;
            }
        }

        if (!spat && id == 1 && pulledBlock != air && CanPush(pulledBlock, level->GetBlockData(pulled)) && !IsDestroyedByPush(pulledBlock))
        {
            MoveBlocks(level, pos, facing, false);
        }
    }

    level->PlaySound("tile.piston.in", glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 0.5f, level->GetRandom().NextFloat() * 0.15f + 0.6f);
}

void PistonBaseBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    if (!IsExtended(data))
    {
        out->push_back(AABB(glm::dvec3(0.0), glm::dvec3(1.0)));
        return;
    }

    Direction *facing = GetFacing(data);
    out->push_back(MakeBox(facing, 0.0, 12.0 / 16.0, 0.5));
    out->push_back(MakeBox(facing, 12.0 / 16.0, 1.0, 2.0 / 16.0));
}

uint16_t PistonBaseBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos,
                                            const glm::dvec3 &look) const
{
    (void) level;
    (void) pos;
    (void) face;
    (void) hitPos;
    glm::dvec3 absLook = glm::abs(look);
    Direction *facing  = nullptr;
    if (absLook.y >= absLook.x && absLook.y >= absLook.z)
    {
        facing = look.y > 0.0 ? Direction::down : Direction::up;
    }
    else if (absLook.x >= absLook.z)
    {
        facing = look.x > 0.0 ? Direction::west : Direction::east;
    }
    else
    {
        facing = look.z > 0.0 ? Direction::north : Direction::south;
    }

    return WithFacing(0, facing);
}

uint16_t PistonBaseBlock::GetItemData() const
{
    return WithFacing(0, Direction::up);
}

bool PistonBaseBlock::IsSticky() const
{
    return m_sticky;
}

bool PistonBaseBlock::IsExtended(uint16_t data) const
{
    return GetPropertyValue(data, m_extendedProperty) == 1;
}

uint16_t PistonBaseBlock::WithExtended(uint16_t data, bool extended) const
{
    return WithPropertyValue(data, m_extendedProperty, extended ? 1 : 0);
}

int PistonBaseBlock::GetEndTexture(uint16_t data, bool front) const
{
    if (!front)
    {
        return m_bottomTexture;
    }

    if (IsExtended(data))
    {
        return m_innerTexture;
    }

    return m_sticky ? m_stickyTopTexture : m_topTexture;
}
