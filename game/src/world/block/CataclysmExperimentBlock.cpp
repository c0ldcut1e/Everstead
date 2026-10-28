#include "CataclysmExperimentBlock.h"

#include "../Level.h"

CataclysmExperimentBlock::CataclysmExperimentBlock(const std::string &theme, eEXPLOSION_TYPE charge, eEXPLOSION_TYPE blast, int chargeTicks,
                                                   float blastRadius)
    : Block(true, "assets/texture/block/" + theme + "_experiment.png")
{
    m_sound       = "experiment." + theme;
    m_charge      = charge;
    m_blast       = blast;
    m_chargeTicks = chargeTicks;
    m_blastRadius = blastRadius;
}

bool CataclysmExperimentBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return true;
}

// The charge length lines up with the blast in the theme's sound, so changing it means regenerating the sound
bool CataclysmExperimentBlock::OnUse(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide() || !m_charging.insert(pos).second)
    {
        return true;
    }

    glm::dvec3 center(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5);
    Explosion(level, center, 8.0f).Experiment(m_charge, pos);
    level->PlaySound(m_sound, center, 24.0f, 1.0f);
    level->ScheduleBlockForTick(pos, m_chargeTicks, 0);
    return true;
}

void CataclysmExperimentBlock::OnBreak(Level *level, const BlockPos &pos)
{
    (void) level;

    m_charging.erase(pos);
}

void CataclysmExperimentBlock::Tick(Level *level, const BlockPos &pos)
{
    if (!m_charging.erase(pos))
    {
        return;
    }

    Explosion(level, glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), m_blastRadius).Experiment(m_blast, pos);
    level->SetBlock(pos, Block::ByName("air"));
    level->RecordBlockChange(pos);
}
