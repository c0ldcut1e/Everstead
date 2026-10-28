#include "RedstoneLampBlock.h"

#include "../Level.h"

RedstoneLampBlock::RedstoneLampBlock(bool lit)
    : Block(true, lit ? "assets/texture/block/redstone_lamp_on.png" : "assets/texture/block/redstone_lamp.png")
{
    m_lit     = lit;
    m_toggled = nullptr;
    SetBreakSound("random.glass", 3);
    SetPlaceSound("step.stone", 6, 0.8f);
    if (lit)
    {
        SetLightEmission(15);
        SetLightColor(0xFF, 0xC8, 0x80);
    }
}

void RedstoneLampBlock::SetToggled(RedstoneLampBlock *toggled)
{
    m_toggled = toggled;
}

void RedstoneLampBlock::OnPlace(Level *level, const BlockPos &pos)
{
    if (!level->IsClientSide() && !m_lit && level->GetBestNeighborSignal(pos) > 0)
    {
        level->SetBlock(pos, m_toggled, nullptr, level->GetBlockData(pos));
        level->RecordBlockChange(pos);
    }
}

void RedstoneLampBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    Block::OnNeighborChanged(level, pos);
    if (level->IsClientSide() || Block::ById(level->GetBlockId(pos)) != this)
    {
        return;
    }

    bool powered = level->GetBestNeighborSignal(pos) > 0;
    if (m_lit && !powered)
    {
        level->ScheduleBlockForTick(pos, 4, 0);
    }
    else if (!m_lit && powered)
    {
        level->SetBlock(pos, m_toggled, nullptr, level->GetBlockData(pos));
        level->RecordBlockChange(pos);
    }
}

void RedstoneLampBlock::Tick(Level *level, const BlockPos &pos)
{
    if (m_lit && level->GetBestNeighborSignal(pos) == 0)
    {
        level->SetBlock(pos, m_toggled, nullptr, level->GetBlockData(pos));
        level->RecordBlockChange(pos);
    }
}
