#include "DiggerItem.h"

DiggerItem::DiggerItem()
{
    m_toolType = Block::eToolType_None;
    m_level    = 0;
    m_speed    = 1.0f;
}

void DiggerItem::SetTier(Block::eTOOL_TYPE toolType, int level, float speed)
{
    m_toolType = toolType;
    m_level    = level;
    m_speed    = speed;
}

float DiggerItem::GetDestroySpeed(Block *block) const
{
    return block->GetToolType() == m_toolType ? m_speed : 1.0f;
}

bool DiggerItem::CanHarvest(Block *block) const
{
    return block->GetToolType() == m_toolType && m_level >= block->GetHarvestLevel();
}

int DiggerItem::GetMineDamage(Block *block) const
{
    return block->GetDestroyTime() != 0.0f ? 1 : 0;
}

int DiggerItem::GetHitDamage() const
{
    return 2;
}
