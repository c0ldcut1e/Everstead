#include "PistonHeadBlock.h"

#include "../Level.h"
#include "PistonBaseBlock.h"

PistonHeadBlock::PistonHeadBlock() : PistonPartBlock("assets/texture/block/piston_top.png")
{
    m_stickyProperty = AddProperty("sticky", 2);
}

void PistonHeadBlock::OnRemoved(Level *level, const BlockPos &pos, uint16_t data)
{
    if (level->IsClientSide())
    {
        return;
    }

    Direction *facing = GetFacing(data);
    BlockPos basePos(pos.x - facing->dx, pos.y - facing->dy, pos.z - facing->dz);
    uint16_t baseData     = level->GetBlockData(basePos);
    PistonBaseBlock *base = dynamic_cast<PistonBaseBlock *>(Block::ById(level->GetBlockId(basePos)));
    if (base && base->IsExtended(baseData) && base->GetFacing(baseData) == facing)
    {
        base->DropAsItem(level, basePos, baseData);
        level->SetBlock(basePos, Block::ByName("air"));
        level->RecordBlockChange(basePos);
    }
}

void PistonHeadBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    Direction *facing = GetFacing(level->GetBlockData(pos));
    BlockPos basePos(pos.x - facing->dx, pos.y - facing->dy, pos.z - facing->dz);
    Block *base = Block::ById(level->GetBlockId(basePos));
    if (dynamic_cast<PistonBaseBlock *>(base))
    {
        base->OnNeighborChanged(level, basePos);
    }
}

void PistonHeadBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    Direction *facing = GetFacing(data);
    out->push_back(MakeBox(facing, 12.0 / 16.0, 1.0, 0.5));
    out->push_back(MakeBox(facing, 0.0, 12.0 / 16.0, 2.0 / 16.0));
}

uint16_t PistonHeadBlock::WithSticky(uint16_t data, bool sticky) const
{
    return WithPropertyValue(data, m_stickyProperty, sticky ? 1 : 0);
}

int PistonHeadBlock::GetEndTexture(uint16_t data, bool front) const
{
    return front && GetPropertyValue(data, m_stickyProperty) == 1 ? m_stickyTopTexture : m_topTexture;
}
