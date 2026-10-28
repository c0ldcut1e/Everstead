#include "DetectorRailBlock.h"

#include "../../entity/item/Minecart.h"
#include "../Level.h"

DetectorRailBlock::DetectorRailBlock(const std::string &texturePath, const std::string &poweredTexturePath)
    : RailBlock(texturePath, poweredTexturePath, false)
{}

void DetectorRailBlock::Tick(Level *level, const BlockPos &pos)
{
    if (IsPowered(level->GetBlockData(pos)))
    {
        CheckPressed(level, pos);
    }
}

void DetectorRailBlock::OnRemoved(Level *level, const BlockPos &pos, uint16_t data)
{
    if (IsPowered(data))
    {
        level->NotifyBlockNeighbors(pos);
        level->NotifyBlockNeighbors(BlockPos(pos.x, pos.y - 1, pos.z));
    }
}

int DetectorRailBlock::GetSignal(uint16_t data, Direction *direction) const
{
    (void) direction;

    return IsPowered(data) ? 15 : 0;
}

int DetectorRailBlock::GetDirectSignal(uint16_t data, Direction *direction) const
{
    return IsPowered(data) && direction == Direction::down ? 15 : 0;
}

bool DetectorRailBlock::IsSignalSource() const
{
    return true;
}

void DetectorRailBlock::OnMinecartInside(Level *level, const BlockPos &pos)
{
    if (!IsPowered(level->GetBlockData(pos)))
    {
        CheckPressed(level, pos);
    }
}

void DetectorRailBlock::CheckPressed(Level *level, const BlockPos &pos)
{
    const double inset = 2.0 / 16.0;
    AABB area(glm::dvec3(pos.x + inset, pos.y, pos.z + inset), glm::dvec3(pos.x + 1.0 - inset, pos.y + 1.0 - inset, pos.z + 1.0 - inset));
    bool pressed                                         = false;
    const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
    for (size_t i = 0; i < entities.size() && !pressed; i++)
    {
        pressed = dynamic_cast<Minecart *>(entities[i].get()) && entities[i]->GetAABB().Intersects(area);
    }

    if (pressed != IsPowered(level->GetBlockData(pos)))
    {
        SetPowered(level, pos, pressed);
        level->NotifyBlockNeighbors(pos);
        level->NotifyBlockNeighbors(BlockPos(pos.x, pos.y - 1, pos.z));
    }

    if (pressed)
    {
        level->ScheduleBlockForTick(pos, 20, 0);
    }
}
