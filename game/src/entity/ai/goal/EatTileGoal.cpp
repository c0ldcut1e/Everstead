#include "EatTileGoal.h"

#include <algorithm>
#include <cmath>

#include "../../../world/Level.h"
#include "../../../world/block/Block.h"
#include "../../animal/Sheep.h"

EatTileGoal::EatTileGoal(Sheep *sheep)
{
    m_sheep            = sheep;
    m_eatAnimationTick = 0;
    m_flags            = eGoalFlag_Move | eGoalFlag_Look | eGoalFlag_Jump;
}

bool EatTileGoal::CanUse()
{
    if (m_sheep->GetRandom().NextInt(0, m_sheep->IsBaby() ? 49 : 999) != 0)
    {
        return false;
    }

    const glm::dvec3 &position = m_sheep->GetPosition();
    BlockPos pos((int) floor(position.x), (int) floor(position.y), (int) floor(position.z));
    Level *level = m_sheep->GetLevel();
    return Block::ById(level->GetBlockId(pos)) == Block::ByName("grass") ||
           Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z))) == Block::ByName("grass_block");
}

bool EatTileGoal::CanContinueToUse()
{
    return m_eatAnimationTick > 0;
}

void EatTileGoal::Start()
{
    m_eatAnimationTick = Sheep::kEatAnimationTicks;
    m_sheep->GetLevel()->BroadcastEntityEvent(m_sheep, Sheep::kEatGrassEvent);
    m_sheep->GetNavigation()->Stop();
}

void EatTileGoal::Stop()
{
    m_eatAnimationTick = 0;
}

void EatTileGoal::Tick()
{
    m_eatAnimationTick = std::max(0, m_eatAnimationTick - 1);
    if (m_eatAnimationTick != 4)
    {
        return;
    }

    const glm::dvec3 &position = m_sheep->GetPosition();
    BlockPos pos((int) floor(position.x), (int) floor(position.y), (int) floor(position.z));
    BlockPos below(pos.x, pos.y - 1, pos.z);
    Level *level = m_sheep->GetLevel();
    Block *grass = Block::ByName("grass");
    if (Block::ById(level->GetBlockId(pos)) == grass)
    {
        level->PlayBlockBreakSound(pos, grass);
        level->SetBlock(pos, Block::ByName("air"));
        m_sheep->Ate();
    }
    else if (Block::ById(level->GetBlockId(below)) == Block::ByName("grass_block"))
    {
        level->PlayBlockBreakSound(below, Block::ByName("grass_block"));
        level->SetBlock(below, Block::ByName("dirt"));
        m_sheep->Ate();
    }
}

int EatTileGoal::GetEatAnimationTick() const
{
    return m_eatAnimationTick;
}
