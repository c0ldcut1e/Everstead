#include "VineBlock.h"

#include "../../utils/Random.h"
#include "../Level.h"
#include "../chunk/Chunk.h"

static Direction *const s_vineSides[] = {Direction::north, Direction::east, Direction::south, Direction::west};

static int SideIndex(Direction *direction)
{
    for (int i = 0; i < 4; i++)
    {
        if (s_vineSides[i] == direction)
        {
            return i;
        }
    }

    return -1;
}

static BlockPos Offset(const BlockPos &pos, Direction *direction)
{
    return BlockPos(pos.x + direction->dx, pos.y + direction->dy, pos.z + direction->dz);
}

VineBlock::VineBlock(const std::string &texturePath) : Block(false, texturePath)
{
    m_sideProperties[0] = AddProperty("north", 2);
    m_sideProperties[1] = AddProperty("east", 2);
    m_sideProperties[2] = AddProperty("south", 2);
    m_sideProperties[3] = AddProperty("west", 2);
    m_upProperty        = AddProperty("up", 2);
    SetTransparent(true);
    SetRenderShape(eRenderShape_BOXES);
    SetReplaceable(true);
    SetRandomTicking(true);
    SetClimbable(true);
    SetFlatItem(true);
}

bool VineBlock::CanAttachTo(const Block *block)
{
    return block && block->IsSolid() && !block->IsLiquid() && block->GetRenderShape() == eRenderShape_CUBE;
}

bool VineBlock::HasFace(uint16_t data, int side) const
{
    return GetPropertyValue(data, m_sideProperties[side]) != 0;
}

uint16_t VineBlock::WithFace(uint16_t data, int side, bool value) const
{
    return WithPropertyValue(data, m_sideProperties[side], value ? 1 : 0);
}

bool VineBlock::HasAnySide(uint16_t data) const
{
    for (int i = 0; i < 4; i++)
    {
        if (HasFace(data, i))
        {
            return true;
        }
    }

    return false;
}

void VineBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    if (GetPropertyValue(data, m_upProperty) != 0)
    {
        out->push_back(AABB(glm::dvec3(0.0, 1.0 - kThickness, 0.0), glm::dvec3(1.0, 1.0, 1.0)));
    }

    if (HasFace(data, 0))
    {
        out->push_back(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(1.0, 1.0, kThickness)));
    }

    if (HasFace(data, 1))
    {
        out->push_back(AABB(glm::dvec3(1.0 - kThickness, 0.0, 0.0), glm::dvec3(1.0, 1.0, 1.0)));
    }

    if (HasFace(data, 2))
    {
        out->push_back(AABB(glm::dvec3(0.0, 0.0, 1.0 - kThickness), glm::dvec3(1.0, 1.0, 1.0)));
    }

    if (HasFace(data, 3))
    {
        out->push_back(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(kThickness, 1.0, 1.0)));
    }

    if (out->empty())
    {
        out->push_back(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(1.0, 1.0, kThickness)));
    }
}

Block *VineBlock::GetPlacementBlock(Direction *face)
{
    return face == Direction::up ? nullptr : this;
}

uint16_t VineBlock::GetPlacementData(Direction *placedAgainst) const
{
    if (placedAgainst == Direction::down)
    {
        return WithPropertyValue(0, m_upProperty, 1);
    }

    int side = SideIndex(Direction::Opposite(placedAgainst));
    return side >= 0 ? WithFace(0, side, true) : 0;
}

Direction *VineBlock::GetAttachment(uint16_t data) const
{
    for (int i = 0; i < 4; i++)
    {
        if (HasFace(data, i))
        {
            return s_vineSides[i];
        }
    }

    return Direction::up;
}

bool VineBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    if (Block::ById(level->GetBlockId(pos)) == this)
    {
        uint16_t surviving = GetSurvivingFaces(level, pos, level->GetBlockData(pos));
        return HasAnySide(surviving) || GetPropertyValue(surviving, m_upProperty) != 0;
    }

    return attachment && CanAttachTo(Block::ById(level->GetBlockId(Offset(pos, attachment))));
}

uint16_t VineBlock::GetSurvivingFaces(Level *level, const BlockPos &pos, uint16_t data) const
{
    BlockPos abovePos   = Offset(pos, Direction::up);
    Block *above        = Block::ById(level->GetBlockId(abovePos));
    uint16_t aboveData  = above == this ? level->GetBlockData(abovePos) : 0;
    bool canAttachAbove = CanAttachTo(above);
    uint16_t result     = WithPropertyValue(data, m_upProperty, canAttachAbove && GetPropertyValue(data, m_upProperty) != 0 ? 1 : 0);
    for (int i = 0; i < 4; i++)
    {
        if (!HasFace(data, i))
        {
            continue;
        }

        bool supported = CanAttachTo(Block::ById(level->GetBlockId(Offset(pos, s_vineSides[i])))) || (above == this && HasFace(aboveData, i));
        result         = WithFace(result, i, supported);
    }

    return result;
}

void VineBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide())
    {
        return;
    }

    uint16_t data      = level->GetBlockData(pos);
    uint16_t surviving = GetSurvivingFaces(level, pos, data);
    if (surviving == data)
    {
        return;
    }

    if (!HasAnySide(surviving) && GetPropertyValue(surviving, m_upProperty) == 0)
    {
        level->SetBlock(pos, Block::ByName("air"));
    }
    else
    {
        level->SetBlockData(pos, surviving);
    }

    level->RecordBlockChange(pos);
}

uint16_t VineBlock::KeepRandomSides(Random *random, uint16_t data) const
{
    uint16_t result = 0;
    for (int i = 0; i < 4; i++)
    {
        if (HasFace(data, i) && (random->NextUInt() & 1) != 0)
        {
            result = WithFace(result, i, true);
        }
    }

    return result;
}

bool VineBlock::HasTooManyNearby(Level *level, const BlockPos &pos) const
{
    int remaining = kMaxNearbyVine;
    for (int x = pos.x - kSpreadRadius; x <= pos.x + kSpreadRadius; x++)
    {
        for (int z = pos.z - kSpreadRadius; z <= pos.z + kSpreadRadius; z++)
        {
            for (int y = pos.y - 1; y <= pos.y + 1; y++)
            {
                if (Block::ById(level->GetBlockId(BlockPos(x, y, z))) == this && --remaining <= 0)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void VineBlock::RandomTick(Level *level, const BlockPos &pos)
{
    if ((level->GetRandom().NextUInt() & 3) != 0 || !level->IsAreaLoaded(pos, kSpreadRadius + 1))
    {
        return;
    }

    Grow(level, pos, level->GetBlockData(pos));
}

void VineBlock::Grow(Level *level, const BlockPos &pos, uint16_t data)
{
    Block *air         = Block::ByName("air");
    bool canSpread     = !HasTooManyNearby(level, pos);
    int directionIndex = (int) (level->GetRandom().NextUInt() % 6);

    if (directionIndex == 4)
    {
        BlockPos abovePos = Offset(pos, Direction::up);
        if (!canSpread || abovePos.y >= Chunk::s_sizeY || Block::ById(level->GetBlockId(abovePos)) != air)
        {
            return;
        }

        uint16_t grown = 0;
        for (int i = 0; i < 4; i++)
        {
            if (HasFace(data, i) && (level->GetRandom().NextUInt() & 1) != 0 &&
                CanAttachTo(Block::ById(level->GetBlockId(Offset(abovePos, s_vineSides[i])))))
            {
                grown = WithFace(grown, i, true);
            }
        }

        if (HasAnySide(grown))
        {
            level->SetBlock(abovePos, this, nullptr, grown);
            level->RecordBlockChange(abovePos);
        }

        return;
    }

    if (directionIndex < 4)
    {
        if (HasFace(data, directionIndex) || !canSpread)
        {
            return;
        }

        Direction *direction = s_vineSides[directionIndex];
        BlockPos targetPos   = Offset(pos, direction);
        Block *target        = Block::ById(level->GetBlockId(targetPos));
        if (target != air)
        {
            if (CanAttachTo(target))
            {
                level->SetBlockData(pos, WithFace(data, directionIndex, true));
                level->RecordBlockChange(pos);
            }

            return;
        }

        int clockwise        = (directionIndex + 1) & 3;
        int counterClockwise = (directionIndex + 3) & 3;
        int opposite         = (directionIndex + 2) & 3;
        BlockPos cornerCw    = Offset(targetPos, s_vineSides[clockwise]);
        BlockPos cornerCcw   = Offset(targetPos, s_vineSides[counterClockwise]);
        bool hasCw           = HasFace(data, clockwise);
        bool hasCcw          = HasFace(data, counterClockwise);

        if (hasCw && CanAttachTo(Block::ById(level->GetBlockId(cornerCw))))
        {
            level->SetBlock(targetPos, this, nullptr, WithFace(0, clockwise, true));
            level->RecordBlockChange(targetPos);
        }
        else if (hasCcw && CanAttachTo(Block::ById(level->GetBlockId(cornerCcw))))
        {
            level->SetBlock(targetPos, this, nullptr, WithFace(0, counterClockwise, true));
            level->RecordBlockChange(targetPos);
        }
        else if (hasCw && Block::ById(level->GetBlockId(cornerCw)) == air &&
                 CanAttachTo(Block::ById(level->GetBlockId(Offset(pos, s_vineSides[clockwise])))))
        {
            level->SetBlock(cornerCw, this, nullptr, WithFace(0, opposite, true));
            level->RecordBlockChange(cornerCw);
        }
        else if (hasCcw && Block::ById(level->GetBlockId(cornerCcw)) == air &&
                 CanAttachTo(Block::ById(level->GetBlockId(Offset(pos, s_vineSides[counterClockwise])))))
        {
            level->SetBlock(cornerCcw, this, nullptr, WithFace(0, opposite, true));
            level->RecordBlockChange(cornerCcw);
        }
        else if (CanAttachTo(Block::ById(level->GetBlockId(Offset(targetPos, Direction::up)))))
        {
            level->SetBlock(targetPos, this, nullptr, WithPropertyValue(0, m_upProperty, 1));
            level->RecordBlockChange(targetPos);
        }

        return;
    }

    BlockPos belowPos = Offset(pos, Direction::down);
    if (belowPos.y <= 1)
    {
        return;
    }

    Block *below = Block::ById(level->GetBlockId(belowPos));
    if (below == air)
    {
        uint16_t grown = KeepRandomSides(&level->GetRandom(), data);
        if (HasAnySide(grown))
        {
            level->SetBlock(belowPos, this, nullptr, grown);
            level->RecordBlockChange(belowPos);
        }
    }
    else if (below == this)
    {
        uint16_t belowData = level->GetBlockData(belowPos);
        uint16_t merged    = belowData | KeepRandomSides(&level->GetRandom(), data);
        if (merged != belowData)
        {
            level->SetBlockData(belowPos, merged);
            level->RecordBlockChange(belowPos);
        }
    }
}
