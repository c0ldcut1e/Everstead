#include "EffectsExperimentBlock.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../Explosion.h"
#include "../Level.h"

EffectsExperimentBlock::EffectsExperimentBlock() : Block(true, "assets/texture/block/effects_experiment.png")
{}

bool EffectsExperimentBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return true;
}

// The stage ticks line up with experiment/sequence.ogg, so changing one means regenerating the sound
bool EffectsExperimentBlock::OnUse(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide() || !m_stages.emplace(pos, 0).second)
    {
        return true;
    }

    glm::dvec3 center(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5);
    Explosion(level, center, 10.0f).Experiment(eExplosionType_ExperimentLaunch, pos);
    level->PlaySound("experiment.sequence", center, 24.0f, 1.0f);
    level->ScheduleBlockForTick(pos, 20, 0);
    return true;
}

void EffectsExperimentBlock::OnBreak(Level *level, const BlockPos &pos)
{
    (void) level;

    m_stages.erase(pos);
}

void EffectsExperimentBlock::Tick(Level *level, const BlockPos &pos)
{
    auto stage = m_stages.find(pos);
    if (stage == m_stages.end())
    {
        return;
    }

    int bursts = 16;
    glm::dvec3 center(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5);
    stage->second++;
    if (stage->second <= bursts)
    {
        double angle    = level->GetRandom().NextDouble() * std::numbers::pi * 2.0;
        double distance = 5.0 + level->GetRandom().NextDouble() * 9.0;
        for (int i = 0; i < (stage->second > 8 ? 2 : 1); i++)
        {
            glm::dvec3 offset(cos(angle + i * std::numbers::pi) * distance, level->GetRandom().NextDouble() * 6.0 - 2.0,
                              sin(angle + i * std::numbers::pi) * distance);
            Explosion(level, center + offset, 3.0f + stage->second * 0.2f + level->GetRandom().NextFloat() * 1.5f)
                    .Experiment(eExplosionType_ExperimentBurst, pos);
        }

        level->ScheduleBlockForTick(pos, stage->second == bursts ? 15 : std::max(13 - stage->second, 2), 0);
    }
    else if (stage->second == bursts + 1)
    {
        Explosion(level, center, 14.0f).Experiment(eExplosionType_ExperimentPull, pos);
        level->ScheduleBlockForTick(pos, 40, 0);
    }
    else
    {
        Explosion(level, center, 16.0f).Experiment(eExplosionType_ExperimentFinale, pos);
        m_stages.erase(pos);
        level->SetBlock(pos, Block::ByName("air"));
        level->RecordBlockChange(pos);
    }
}
