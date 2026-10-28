#include "FurnaceBlockEntity.h"

#include "../ServerLevel.h"
#include "../block/Block.h"

FurnaceBlockEntity::FurnaceBlockEntity(Level *level, const BlockPos &pos) : BlockEntity(level, pos)
{
    m_swappingBlock = false;
}

const char *FurnaceBlockEntity::GetId() const
{
    return "furnace";
}

void FurnaceBlockEntity::Tick()
{
    if (m_level->IsClientSide())
    {
        return;
    }

    bool wasLit = IsLit();
    if (!FurnaceContainer::Tick())
    {
        return;
    }

    ServerLevel *level = (ServerLevel *) m_level;
    level->MarkBlockChunkDirty(m_pos);
    if (wasLit != IsLit())
    {
        m_swappingBlock = true;
        level->SetBlock(m_pos, Block::ByName(IsLit() ? "lit_furnace" : "furnace"), nullptr, level->GetBlockData(m_pos));
        m_swappingBlock = false;
        level->RecordBlockChange(m_pos);
    }
}

void FurnaceBlockEntity::Save(CompoundTag *tag) const
{
    BlockEntity::Save(tag);
    FurnaceContainer::Save(tag);
}

void FurnaceBlockEntity::Load(const CompoundTag *tag)
{
    FurnaceContainer::Load(tag);
}

bool FurnaceBlockEntity::IsSwappingBlock() const
{
    return m_swappingBlock;
}
