#include "FractureTrait.h"

#include <memory>

#include "../../network/packet/s2c/ParticleS2CPacket.h"
#include "../../network/packet/s2c/WorldEffectS2CPacket.h"
#include "../ServerLevel.h"

FractureTrait::FractureTrait(const ResourceLocation &id, uint8_t gearTypes, int weight)
{
    m_id        = id;
    m_gearTypes = gearTypes;
    m_weight    = weight;
}

const ResourceLocation &FractureTrait::GetId() const
{
    return m_id;
}

uint8_t FractureTrait::GetGearTypes() const
{
    return m_gearTypes;
}

int FractureTrait::GetWeight() const
{
    return m_weight;
}

FractureTrait::State *FractureTrait::CreateState()
{
    return nullptr;
}

void FractureTrait::Tick(ServerPlayer *player, State *state)
{
    (void) player;
    (void) state;
}

int FractureTrait::ModifyAttackDamage(ServerPlayer *player, int damage)
{
    (void) player;
    return damage;
}

int FractureTrait::GetAttackWear()
{
    return 0;
}

bool FractureTrait::OnAttack(ServerPlayer *player, const Hit &hit)
{
    (void) player;
    (void) hit;
    return false;
}

bool FractureTrait::PreventHurt(ServerPlayer *player, eDAMAGE_SOURCE source, int pieces)
{
    (void) player;
    (void) source;
    (void) pieces;
    return false;
}

void FractureTrait::OnHurt(ServerPlayer *player, int pieces)
{
    (void) player;
    (void) pieces;
}

bool FractureTrait::OnArrowHit(ServerPlayer *player, Arrow *arrow, int pieces)
{
    (void) player;
    (void) arrow;
    (void) pieces;
    return false;
}

bool FractureTrait::OnArrowLand(Arrow *arrow, Direction *face)
{
    (void) arrow;
    (void) face;
    return false;
}

bool FractureTrait::PreventBreak(ServerPlayer *player, Level *level, const BlockPos &pos)
{
    (void) player;
    (void) level;
    (void) pos;
    return false;
}

int FractureTrait::OnHarvest(ServerPlayer *player, const Harvest &harvest)
{
    (void) player;
    (void) harvest;
    return 0;
}

int FractureTrait::ModifyWear(int amount, Random *random)
{
    (void) random;
    return amount;
}

int FractureTrait::OnPickupExperience(ServerPlayer *player, ItemInstance *item, int amount)
{
    (void) player;
    (void) item;
    return amount;
}

void FractureTrait::SendEffect(Level *level, WorldEffects::eWORLD_EFFECT_TYPE type, const glm::dvec3 &center, float radius)
{
    ((ServerLevel *) level)->SendToNearby(center, 64.0, std::shared_ptr<Packet>(new WorldEffectS2CPacket(type, center, radius)));
}

void FractureTrait::SendParticles(Level *level, ePARTICLE_TYPE type, const glm::dvec3 &center, const glm::dvec3 &spread, float speed, int count)
{
    ((ServerLevel *) level)->SendToNearby(center, 64.0, std::shared_ptr<Packet>(new ParticleS2CPacket(type, center, spread, speed, count)));
}

void FractureTrait::PlaySound(Level *level, const glm::dvec3 &position, float volume)
{
    level->PlaySound("fracture." + m_id.GetPath(), position, volume, 0.9f + level->GetRandom().NextFloat() * 0.2f);
}
