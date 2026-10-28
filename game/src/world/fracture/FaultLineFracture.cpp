#include "FaultLineFracture.h"

#include <glm/common.hpp>

#include "../../entity/player/ServerPlayer.h"
#include "../../entity/player/ServerPlayerGameMode.h"
#include "FractureTraits.h"

FaultLineFracture::FaultLineFracture() : FractureTrait("fault_line", FractureTraits::eGearType_Tool, 2)
{}

int FaultLineFracture::OnHarvest(ServerPlayer *player, const Harvest &harvest)
{
    if (harvest.cascade || harvest.level->GetRandom().NextFloat() >= 0.2f)
    {
        return 0;
    }

    glm::dvec3 center(harvest.pos.x + 0.5, harvest.pos.y + 0.5, harvest.pos.z + 0.5);
    glm::dvec3 look = center - (player->GetPosition() + glm::dvec3(0.0, player->GetEyeHeight(), 0.0));
    glm::dvec3 away = glm::abs(look);
    BlockPos step(0, 0, 0);
    if (away.x >= away.y && away.x >= away.z)
    {
        step.x = look.x > 0.0 ? 1 : -1;
    }
    else if (away.y >= away.z)
    {
        step.y = look.y > 0.0 ? 1 : -1;
    }
    else
    {
        step.z = look.z > 0.0 ? 1 : -1;
    }

    int length = harvest.level->GetRandom().NextInt(2, 3);
    int broken = 0;
    while (broken < length &&
           player->GetGameMode()->BreakExtraBlock(
                   harvest.level,
                   BlockPos(harvest.pos.x + step.x * (broken + 1), harvest.pos.y + step.y * (broken + 1), harvest.pos.z + step.z * (broken + 1)),
                   harvest.held))
    {
        broken++;
    }

    if (broken > 0)
    {
        PlaySound(harvest.level, center, 1.0f);
    }

    return broken;
}
