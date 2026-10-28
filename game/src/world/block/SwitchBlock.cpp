#include "SwitchBlock.h"

#include "../Level.h"

SwitchBlock::SwitchBlock(const std::string &texturePath) : Block(false, texturePath)
{}

void SwitchBlock::OnRemoved(Level *level, const BlockPos &pos, uint16_t data)
{
    if (IsPowered(data))
    {
        NotifySignalNeighbors(level, pos, data);
    }
}

int SwitchBlock::GetSignal(uint16_t data, Direction *direction) const
{
    (void) direction;

    return IsPowered(data) ? 15 : 0;
}

int SwitchBlock::GetDirectSignal(uint16_t data, Direction *direction) const
{
    return IsPowered(data) && direction == GetAttachment(data) ? 15 : 0;
}

bool SwitchBlock::IsSignalSource() const
{
    return true;
}

bool SwitchBlock::IsPowered(uint16_t data) const
{
    return GetPropertyValue(data, m_poweredProperty) != 0;
}

void SwitchBlock::SetPowered(Level *level, const BlockPos &pos, bool powered)
{
    uint16_t data = WithPropertyValue(level->GetBlockData(pos), m_poweredProperty, powered ? 1 : 0);
    level->SetBlockData(pos, data);
    level->RecordBlockChange(pos);
    if (!level->IsClientSide())
    {
        NotifySignalNeighbors(level, pos, data);
    }
}

// The attached block carries a direct signal, so its neighbors have to hear about the change too
void SwitchBlock::NotifySignalNeighbors(Level *level, const BlockPos &pos, uint16_t data)
{
    Direction *attachment = GetAttachment(data);
    level->NotifyBlockNeighbors(pos);
    level->NotifyBlockNeighbors(BlockPos(pos.x + attachment->dx, pos.y + attachment->dy, pos.z + attachment->dz));
}
