#include "OpenableBlock.h"

#include "../../utils/Random.h"
#include "../Level.h"

OpenableBlock::OpenableBlock(bool solid, const std::string &texturePath) : Block(solid, texturePath)
{}

void OpenableBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    Block::OnNeighborChanged(level, pos);
    if (level->IsClientSide() || Block::ById(level->GetBlockId(pos)) != this)
    {
        return;
    }

    uint16_t data = level->GetBlockData(pos);
    bool powered  = IsPowered(level, pos, data);
    if (powered != (GetPropertyValue(data, m_poweredProperty) != 0))
    {
        SetOpen(level, pos, powered, powered);
    }
}

bool OpenableBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return true;
}

bool OpenableBlock::OnUse(Level *level, const BlockPos &pos)
{
    uint16_t data  = level->GetBlockData(pos);
    BlockPos other = GetOtherPos(pos, data);
    if (other != pos && Block::ById(level->GetBlockId(other)) != this)
    {
        return false;
    }

    SetOpen(level, pos, !IsOpen(data), GetPropertyValue(data, m_poweredProperty) != 0);
    return true;
}

bool OpenableBlock::IsOpen(uint16_t data) const
{
    return GetPropertyValue(data, m_openProperty) != 0;
}

uint16_t OpenableBlock::WithPlacementPower(Level *level, const BlockPos &pos, uint16_t data) const
{
    int powered = IsPowered(level, pos, data) ? 1 : 0;
    return WithPropertyValue(WithPropertyValue(data, m_poweredProperty, powered), m_openProperty, powered);
}

bool OpenableBlock::IsPowered(Level *level, const BlockPos &pos, uint16_t data) const
{
    BlockPos other = GetOtherPos(pos, data);
    return level->GetBestNeighborSignal(pos) > 0 || (other != pos && level->GetBestNeighborSignal(other) > 0);
}

void OpenableBlock::SetOpen(Level *level, const BlockPos &pos, bool open, bool powered)
{
    bool wasOpen      = IsOpen(level->GetBlockData(pos));
    BlockPos parts[2] = {pos, GetOtherPos(pos, level->GetBlockData(pos))};
    for (int i = 0; i < (parts[1] != pos ? 2 : 1); i++)
    {
        uint16_t data = WithPropertyValue(level->GetBlockData(parts[i]), m_openProperty, open ? 1 : 0);
        level->SetBlockData(parts[i], WithPropertyValue(data, m_poweredProperty, powered ? 1 : 0));
        level->RecordBlockChange(parts[i]);
    }

    if (open == wasOpen)
    {
        return;
    }

    glm::dvec3 center((double) pos.x + 0.5, (double) pos.y + 0.5, (double) pos.z + 0.5);
    level->PlaySound(open ? "random.door_open" : "random.door_close", center, 1.0f, level->GetRandom().NextFloat() * 0.1f + 0.9f);
}
