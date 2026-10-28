#include "OverflowFracture.h"

#include "../../entity/item/ExperienceOrb.h"
#include "FractureTraits.h"

OverflowFracture::OverflowFracture() : FractureTrait("overflow", FractureTraits::eGearType_Tool, 10)
{}

int OverflowFracture::OnHarvest(ServerPlayer *player, const Harvest &harvest)
{
    (void) player;
    if (harvest.level->GetRandom().NextFloat() >= 0.1f)
    {
        return 0;
    }

    ExperienceOrb::SpawnOrbs(harvest.level, glm::dvec3(harvest.pos.x + 0.5, harvest.pos.y + 0.5, harvest.pos.z + 0.5),
                             harvest.level->GetRandom().NextInt(1, 3));
    return 0;
}
