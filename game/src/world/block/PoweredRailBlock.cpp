#include "PoweredRailBlock.h"

#include "../Level.h"

PoweredRailBlock::PoweredRailBlock(const std::string &texturePath, const std::string &poweredTexturePath)
    : RailBlock(texturePath, poweredTexturePath, false)
{}

void PoweredRailBlock::OnPlace(Level *level, const BlockPos &pos)
{
    RailBlock::OnPlace(level, pos);
    UpdatePower(level, pos);
}

void PoweredRailBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    RailBlock::OnNeighborChanged(level, pos);
    if (level->GetBlockId(pos) == GetId())
    {
        UpdatePower(level, pos);
    }
}

void PoweredRailBlock::UpdatePower(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide())
    {
        return;
    }

    uint16_t data = level->GetBlockData(pos);
    bool powered =
            level->GetBestNeighborSignal(pos) > 0 || FindPoweredSignal(level, pos, data, true, 0) || FindPoweredSignal(level, pos, data, false, 0);
    if (powered == IsPowered(data))
    {
        return;
    }

    SetPowered(level, pos, powered);
    level->NotifyBlockNeighbors(pos);
    level->NotifyBlockNeighbors(BlockPos(pos.x, pos.y - 1, pos.z));
    int shape = GetShape(data);
    if (shape >= 2 && shape <= 5)
    {
        level->NotifyBlockNeighbors(BlockPos(pos.x, pos.y + 1, pos.z));
    }
}

// Power travels along at most 8 rails of the same axis away from the powered one
bool PoweredRailBlock::FindPoweredSignal(Level *level, BlockPos pos, uint16_t data, bool forward, int depth) const
{
    if (depth >= 8)
    {
        return false;
    }

    int shape       = GetShape(data);
    bool checkBelow = true;
    if (shape == 0 || shape == 4 || shape == 5)
    {
        pos.z += forward ? 1 : -1;
        if ((shape == 4 && !forward) || (shape == 5 && forward))
        {
            pos.y++;
            checkBelow = false;
        }

        shape = 0;
    }
    else
    {
        pos.x += forward ? -1 : 1;
        if ((shape == 2 && !forward) || (shape == 3 && forward))
        {
            pos.y++;
            checkBelow = false;
        }

        shape = 1;
    }

    return IsPoweredRailWithSignal(level, pos, forward, depth, shape) ||
           (checkBelow && IsPoweredRailWithSignal(level, BlockPos(pos.x, pos.y - 1, pos.z), forward, depth, shape));
}

bool PoweredRailBlock::IsPoweredRailWithSignal(Level *level, const BlockPos &pos, bool forward, int depth, int axisShape) const
{
    if (level->GetBlockId(pos) != GetId())
    {
        return false;
    }

    uint16_t data = level->GetBlockData(pos);
    int shape     = GetShape(data);
    bool alongX   = shape >= 1 && shape <= 3;
    if (alongX != (axisShape == 1) || !IsPowered(data))
    {
        return false;
    }

    return level->GetBestNeighborSignal(pos) > 0 || FindPoweredSignal(level, pos, data, forward, depth + 1);
}
