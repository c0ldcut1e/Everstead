#include "WeightedPressurePlateBlock.h"

#include <algorithm>
#include <cmath>

#include "../../core/EversteadServer.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../network/PlayerList.h"
#include "../ServerLevel.h"

WeightedPressurePlateBlock::WeightedPressurePlateBlock(const std::string &texturePath, int maxWeight) : Block(false, texturePath)
{
    SetBlocksLiquid(true);
    m_maxWeight     = maxWeight;
    m_powerProperty = AddProperty("power", 16);
    SetTransparent(true);
    SetCollidable(false);
    SetRenderShape(eRenderShape_BOXES);
    SetNeedsSupportBelow(true);
}

void WeightedPressurePlateBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    const double p = 1.0 / 16.0;
    double height  = GetPropertyValue(data, m_powerProperty) > 0 ? 0.5 * p : p;
    out->push_back(AABB(glm::dvec3(p, 0.0, p), glm::dvec3(15.0 * p, height, 15.0 * p)));
}

void WeightedPressurePlateBlock::OnRemoved(Level *level, const BlockPos &pos, uint16_t data)
{
    if (GetPropertyValue(data, m_powerProperty) > 0)
    {
        NotifySignalNeighbors(level, pos);
    }
}

int WeightedPressurePlateBlock::GetSignalStrength(Level *level, const BlockPos &pos) const
{
    AABB area(glm::dvec3(pos.x + 0.125, pos.y, pos.z + 0.125), glm::dvec3(pos.x + 0.875, pos.y + 0.25, pos.z + 0.875));
    int count                                            = 0;
    const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i] && entities[i]->GetAABB().Intersects(area))
        {
            count++;
        }
    }

    const std::vector<std::unique_ptr<ServerPlayer>> &players = ((ServerLevel *) level)->GetServer()->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        if (players[i]->GetLevel() == level && players[i]->GetAABB().Intersects(area))
        {
            count++;
        }
    }

    return (int) ceil((float) std::min(count, m_maxWeight) / (float) m_maxWeight * 15.0f);
}

void WeightedPressurePlateBlock::UpdateSignal(Level *level, const BlockPos &pos)
{
    uint16_t data = level->GetBlockData(pos);
    int previous  = GetPropertyValue(data, m_powerProperty);
    int power     = GetSignalStrength(level, pos);
    if (power != previous)
    {
        level->SetBlockData(pos, WithPropertyValue(data, m_powerProperty, power));
        level->RecordBlockChange(pos);
        NotifySignalNeighbors(level, pos);
    }

    glm::dvec3 center(pos.x + 0.5, pos.y + 0.1, pos.z + 0.5);
    if (previous == 0 && power > 0)
    {
        level->PlaySound("random.click", center, 0.3f, 0.6f);
    }
    else if (previous > 0 && power == 0)
    {
        level->PlaySound("random.click", center, 0.3f, 0.5f);
    }

    if (power > 0)
    {
        level->ScheduleBlockForTick(pos, 10, 0);
    }
}

void WeightedPressurePlateBlock::OnEntityInside(Level *level, const BlockPos &pos)
{
    if (!level->IsClientSide() && GetPropertyValue(level->GetBlockData(pos), m_powerProperty) == 0)
    {
        UpdateSignal(level, pos);
    }
}

void WeightedPressurePlateBlock::Tick(Level *level, const BlockPos &pos)
{
    if (GetPropertyValue(level->GetBlockData(pos), m_powerProperty) > 0)
    {
        UpdateSignal(level, pos);
    }
}

int WeightedPressurePlateBlock::GetSignal(uint16_t data, Direction *direction) const
{
    (void) direction;

    return GetPropertyValue(data, m_powerProperty);
}

int WeightedPressurePlateBlock::GetDirectSignal(uint16_t data, Direction *direction) const
{
    return direction == Direction::down ? GetPropertyValue(data, m_powerProperty) : 0;
}

bool WeightedPressurePlateBlock::IsSignalSource() const
{
    return true;
}

void WeightedPressurePlateBlock::NotifySignalNeighbors(Level *level, const BlockPos &pos)
{
    level->NotifyBlockNeighbors(pos);
    level->NotifyBlockNeighbors(BlockPos(pos.x, pos.y - 1, pos.z));
}
