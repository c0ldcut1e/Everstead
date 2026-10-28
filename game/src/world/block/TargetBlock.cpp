#include "TargetBlock.h"

#include <algorithm>
#include <cmath>

#include "../../entity/projectile/Arrow.h"
#include "../Level.h"

TargetBlock::TargetBlock() : Block(true, "assets/texture/block/target_side.png")
{
    m_powerProperty = AddProperty("power", 16);
    SetBreakSound("dig.grass");
    SetFaceTexture(Direction::up, "assets/texture/block/target_top.png");
    SetFaceTexture(Direction::down, "assets/texture/block/target_top.png");
}

// A powered target loses its scheduled tick when a piston moves it
void TargetBlock::OnPlace(Level *level, const BlockPos &pos)
{
    if (!level->IsClientSide() && GetPropertyValue(level->GetBlockData(pos), m_powerProperty) > 0)
    {
        SetPower(level, pos, 0);
    }
}

void TargetBlock::Tick(Level *level, const BlockPos &pos)
{
    if (GetPropertyValue(level->GetBlockData(pos), m_powerProperty) > 0)
    {
        SetPower(level, pos, 0);
    }
}

void TargetBlock::OnProjectileHit(Level *level, const BlockPos &pos, Entity *projectile, const glm::dvec3 &hitPos, Direction *face)
{
    if (GetPropertyValue(level->GetBlockData(pos), m_powerProperty) > 0)
    {
        return;
    }

    double x = std::abs(hitPos.x - floor(hitPos.x) - 0.5);
    double y = std::abs(hitPos.y - floor(hitPos.y) - 0.5);
    double z = std::abs(hitPos.z - floor(hitPos.z) - 0.5);
    double distance;
    if (face->dy != 0)
    {
        distance = std::max(x, z);
    }
    else if (face->dz != 0)
    {
        distance = std::max(x, y);
    }
    else
    {
        distance = std::max(y, z);
    }

    SetPower(level, pos, std::max(1, (int) ceil(15.0 * std::clamp((0.5 - distance) / 0.5, 0.0, 1.0))));
    level->ScheduleBlockForTick(pos, dynamic_cast<Arrow *>(projectile) ? 20 : 8, 0);
}

int TargetBlock::GetSignal(uint16_t data, Direction *direction) const
{
    (void) direction;

    return GetPropertyValue(data, m_powerProperty);
}

bool TargetBlock::IsSignalSource() const
{
    return true;
}

void TargetBlock::SetPower(Level *level, const BlockPos &pos, int power)
{
    level->SetBlockData(pos, WithPropertyValue(level->GetBlockData(pos), m_powerProperty, power));
    level->RecordBlockChange(pos);
    level->NotifyBlockNeighbors(pos);
}
