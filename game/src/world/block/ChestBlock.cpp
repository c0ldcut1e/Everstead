#include "ChestBlock.h"

#include <cmath>

#include "../ServerLevel.h"
#include "../blockentity/ChestBlockEntity.h"
#include "../inventory/CompoundContainer.h"

static Direction *s_partnerDirections[] = {nullptr, Direction::north, Direction::south, Direction::west, Direction::east};

ChestBlock::ChestBlock() : Block(true, "assets/texture/block/planks_oak.png")
{
    m_facingProperty  = AddProperty("facing", 4);
    m_partnerProperty = AddProperty("partner", 5);
    SetTransparent(true);
    SetRenderShape(eRenderShape_BOXES);
    SetEntityRendered(true);
    SetBreakSound("dig.wood");
}

bool ChestBlock::IsSolidAt(Level *level, int x, int y, int z)
{
    Block *block = Block::ById(level->GetBlockId(BlockPos(x, y, z)));
    return block->GetRenderShape() == eRenderShape_CUBE && block->IsSolid() && !block->IsLiquid() && !block->IsTransparent();
}

int ChestBlock::FindPartner(Level *level, const BlockPos &pos) const
{
    for (int i = 1; i < 5; i++)
    {
        Direction *direction = s_partnerDirections[i];
        if (Block::ById(level->GetBlockId(BlockPos(pos.x + direction->dx, pos.y, pos.z + direction->dz))) == this)
        {
            return i;
        }
    }

    return 0;
}

bool ChestBlock::IsFullChest(Level *level, const BlockPos &pos) const
{
    return Block::ById(level->GetBlockId(pos)) == this && FindPartner(level, pos) != 0;
}

void ChestBlock::SetFacing(Level *level, const BlockPos &pos, int facing)
{
    uint16_t data =
            WithPropertyValue(WithPropertyValue(level->GetBlockData(pos), m_facingProperty, facing), m_partnerProperty, FindPartner(level, pos));
    if (data != level->GetBlockData(pos))
    {
        level->SetBlockData(pos, data);
        level->RecordBlockChange(pos);
    }
}

void ChestBlock::RecalcLockDir(Level *level, const BlockPos &pos)
{
    int x     = pos.x;
    int y     = pos.y;
    int z     = pos.z;
    bool n    = IsSolidAt(level, x, y, z - 1);
    bool s    = IsSolidAt(level, x, y, z + 1);
    bool w    = IsSolidAt(level, x - 1, y, z);
    bool e    = IsSolidAt(level, x + 1, y, z);
    int other = FindPartner(level, pos);

    int facing = 2;
    if (other == 1 || other == 2)
    {
        int partnerZ = other == 1 ? z - 1 : z + 1;
        bool w2      = IsSolidAt(level, x - 1, y, partnerZ);
        bool e2      = IsSolidAt(level, x + 1, y, partnerZ);
        facing       = GetFacing(level->GetBlockData(BlockPos(x, y, partnerZ))) == 2 ? 2 : 3;
        if ((w || w2) && !e && !e2)
        {
            facing = 3;
        }

        if ((e || e2) && !w && !w2)
        {
            facing = 2;
        }
    }
    else if (other == 3 || other == 4)
    {
        int partnerX = other == 3 ? x - 1 : x + 1;
        bool n2      = IsSolidAt(level, partnerX, y, z - 1);
        bool s2      = IsSolidAt(level, partnerX, y, z + 1);
        facing       = GetFacing(level->GetBlockData(BlockPos(partnerX, y, z))) == 0 ? 0 : 1;
        if ((n || n2) && !s && !s2)
        {
            facing = 1;
        }

        if ((s || s2) && !n && !n2)
        {
            facing = 0;
        }
    }
    else
    {
        facing = 1;
        if (s && !n)
        {
            facing = 0;
        }

        if (w && !e)
        {
            facing = 3;
        }

        if (e && !w)
        {
            facing = 2;
        }
    }

    SetFacing(level, pos, facing);
}

void ChestBlock::OnPlace(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide())
    {
        return;
    }

    int placedFacing = GetFacing(level->GetBlockData(pos));
    RecalcLockDir(level, pos);
    for (int i = 1; i < 5; i++)
    {
        Direction *direction = s_partnerDirections[i];
        BlockPos neighbor(pos.x + direction->dx, pos.y, pos.z + direction->dz);
        if (Block::ById(level->GetBlockId(neighbor)) == this)
        {
            RecalcLockDir(level, neighbor);
        }
    }

    int partner = FindPartner(level, pos);
    if (partner == 0 || ((partner == 1 || partner == 2) && placedFacing >= 2) || ((partner == 3 || partner == 4) && placedFacing < 2))
    {
        if (partner != 0)
        {
            Direction *direction = s_partnerDirections[partner];
            SetFacing(level, BlockPos(pos.x + direction->dx, pos.y, pos.z + direction->dz), placedFacing);
        }

        SetFacing(level, pos, placedFacing);
    }
}

void ChestBlock::OnBreak(Level *level, const BlockPos &pos)
{
    ((ServerLevel *) level)->DropContainerItems(pos, (ChestBlockEntity *) level->GetBlockEntity(pos));
}

const char *ChestBlock::GetBlockEntityId() const
{
    return "chest";
}

bool ChestBlock::HasAnalogOutput() const
{
    return true;
}

int ChestBlock::GetAnalogOutput(Level *level, const BlockPos &pos) const
{
    ChestBlockEntity *chest = (ChestBlockEntity *) level->GetBlockEntity(pos);
    Direction *partner      = GetPartner(level->GetBlockData(pos));
    if (!partner)
    {
        return chest->GetRedstoneSignal();
    }

    return CompoundContainer(chest, (ChestBlockEntity *) level->GetBlockEntity(BlockPos(pos.x + partner->dx, pos.y, pos.z + partner->dz)))
            .GetRedstoneSignal();
}

void ChestBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (!level->IsClientSide())
    {
        SetFacing(level, pos, GetFacing(level->GetBlockData(pos)));
    }
}

void ChestBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    glm::dvec3 min(1.0 / 16.0, 0.0, 1.0 / 16.0);
    glm::dvec3 max(15.0 / 16.0, 14.0 / 16.0, 15.0 / 16.0);
    Direction *partner = GetPartner(data);
    if (partner == Direction::north)
    {
        min.z = 0.0;
    }
    else if (partner == Direction::south)
    {
        max.z = 1.0;
    }
    else if (partner == Direction::west)
    {
        min.x = 0.0;
    }
    else if (partner == Direction::east)
    {
        max.x = 1.0;
    }

    out->push_back(AABB(min, max));
}

uint16_t ChestBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) face;
    (void) hitPos;

    int facing = 0;
    if (fabs(look.x) > fabs(look.z))
    {
        facing = look.x > 0.0 ? 2 : 3;
    }
    else
    {
        facing = look.z > 0.0 ? 0 : 1;
    }

    return WithPropertyValue(WithPropertyValue(0, m_facingProperty, facing), m_partnerProperty, FindPartner(level, pos));
}

uint16_t ChestBlock::GetItemData() const
{
    return WithPropertyValue(0, m_facingProperty, 1);
}

bool ChestBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return true;
}

bool ChestBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;

    if (Block::ById(level->GetBlockId(pos)) == this)
    {
        return true;
    }

    int chestCount = 0;
    for (int i = 1; i < 5; i++)
    {
        Direction *direction = s_partnerDirections[i];
        BlockPos neighbor(pos.x + direction->dx, pos.y, pos.z + direction->dz);
        if (Block::ById(level->GetBlockId(neighbor)) != this)
        {
            continue;
        }

        if (IsFullChest(level, neighbor))
        {
            return false;
        }

        chestCount++;
    }

    return chestCount <= 1;
}

int ChestBlock::GetFacing(uint16_t data) const
{
    return GetPropertyValue(data, m_facingProperty);
}

Direction *ChestBlock::GetPartner(uint16_t data) const
{
    return s_partnerDirections[GetPropertyValue(data, m_partnerProperty)];
}

Direction *ChestBlock::FindPartnerDirection(Level *level, const BlockPos &pos) const
{
    return s_partnerDirections[FindPartner(level, pos)];
}

bool ChestBlock::IsBlocked(Level *level, const BlockPos &pos) const
{
    return IsSolidAt(level, pos.x, pos.y + 1, pos.z);
}
