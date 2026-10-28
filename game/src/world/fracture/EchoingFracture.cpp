#include "EchoingFracture.h"

#include "../Level.h"
#include "../block/Block.h"
#include "FractureTraits.h"

EchoingFracture::EchoingFracture() : FractureTrait("echoing", FractureTraits::eGearType_Tool, 5)
{}

bool EchoingFracture::PreventBreak(ServerPlayer *player, Level *level, const BlockPos &pos)
{
    (void) player;
    if (level->GetRandom().NextFloat() >= 0.05f)
    {
        return false;
    }

    level->PlaySound("fracture.echoing_fail", glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 0.4f, 0.9f + level->GetRandom().NextFloat() * 0.2f);
    return true;
}

int EchoingFracture::OnHarvest(ServerPlayer *player, const Harvest &harvest)
{
    (void) player;
    if (harvest.level->GetRandom().NextFloat() >= 0.15f)
    {
        return 0;
    }

    glm::dvec3 center(harvest.pos.x + 0.5, harvest.pos.y + 0.5, harvest.pos.z + 0.5);
    harvest.block->DropAsItem(harvest.level, harvest.pos, harvest.data, harvest.held);
    SendParticles(harvest.level, eParticleType_RiftSpark, center, glm::dvec3(0.3), 0.1f, 8);
    PlaySound(harvest.level, center, 0.4f);
    return 0;
}
