#include "TearFracture.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "../../entity/item/ItemEntity.h"
#include "../../entity/player/ServerPlayer.h"
#include "../ServerLevel.h"
#include "FractureTraits.h"

TearFracture::TearFracture() : FractureTrait("tear", FractureTraits::eGearType_Weapon, 1)
{}

FractureTrait::State *TearFracture::CreateState()
{
    return new TearState();
}

void TearFracture::Tick(ServerPlayer *player, State *state)
{
    TearState *tear = (TearState *) state;
    if (tear->ticks <= 0)
    {
        return;
    }

    tear->ticks--;
    ServerLevel *level                                   = (ServerLevel *) player->GetLevel();
    glm::dvec3 target                                    = player->GetPosition() + glm::dvec3(0.0, 0.5, 0.0);
    const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i]->GetType() != eEntityType_Item || glm::distance(entities[i]->GetPosition(), tear->position) > 6.0)
        {
            continue;
        }

        ItemEntity *item  = (ItemEntity *) entities[i].get();
        glm::dvec3 toward = target - item->GetPosition();
        double distance   = glm::length(toward);
        if (distance < 0.5)
        {
            continue;
        }

        item->SetPickupDelay(0);
        item->SetVelocity(toward / distance * std::min(distance * 0.15, 0.6) + glm::dvec3(0.0, 0.04, 0.0));
        // Clients only simulate items locally, so we resend the pulled ones to keep them in sync
        if ((tear->ticks + item->GetId()) % 4 == 0)
        {
            level->SendEntity(item, nullptr);
        }
    }

    if (tear->ticks % 5 == 0)
    {
        SendParticles(level, eParticleType_Rift, tear->position, glm::dvec3(0.5), 0.05f, 6);
    }
}

bool TearFracture::OnAttack(ServerPlayer *player, const Hit &hit)
{
    if (hit.target->GetHealth() > 0 || hit.target->GetLevel()->GetRandom().NextFloat() >= 0.15f)
    {
        return false;
    }

    TearState *state = (TearState *) player->GetFractureState(this);
    state->position  = hit.center;
    state->ticks     = 52;
    SendEffect(player->GetLevel(), WorldEffects::eWorldEffectType_FractureTear, hit.center, 0.9f);
    PlaySound(player->GetLevel(), hit.center, 1.0f);
    return false;
}
