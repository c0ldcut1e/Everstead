#pragma once

#include <cstdint>

#include "../../core/ResourceLocation.h"
#include "../../entity/LivingEntity.h"
#include "../../utils/Random.h"
#include "../WorldEffects.h"
#include "../block/BlockPos.h"
#include "../particle/ParticleType.h"

#include <glm/vec3.hpp>

class Arrow;
class Block;
class Direction;
class ItemInstance;
class Level;
class ServerPlayer;

class FractureTrait
{
public:
    struct State
    {
        virtual ~State() = default;
    };

    struct Hit
    {
        LivingEntity *target;
        glm::dvec3 center;
        int damage;
        bool missed;
    };

    struct Harvest
    {
        Level *level;
        BlockPos pos;
        Block *block;
        uint16_t data;
        ItemInstance *held;
        bool cascade;
    };

    FractureTrait(const ResourceLocation &id, uint8_t gearTypes, int weight);
    virtual ~FractureTrait() = default;

    const ResourceLocation &GetId() const;
    uint8_t GetGearTypes() const;
    int GetWeight() const;

    // States live on the player, so this is only called the first time a player needs one
    virtual State *CreateState();
    virtual void Tick(ServerPlayer *player, State *state);
    virtual int ModifyAttackDamage(ServerPlayer *player, int damage);
    virtual int GetAttackWear();
    // Returning true replaces the normal knockback of the hit
    virtual bool OnAttack(ServerPlayer *player, const Hit &hit);
    // Armor hooks get how many worn pieces carry this trait so their chances can stack
    virtual bool PreventHurt(ServerPlayer *player, eDAMAGE_SOURCE source, int pieces);
    virtual void OnHurt(ServerPlayer *player, int pieces);
    virtual bool OnArrowHit(ServerPlayer *player, Arrow *arrow, int pieces);
    // Returning true keeps the arrow flying instead of sticking into the block
    virtual bool OnArrowLand(Arrow *arrow, Direction *face);
    virtual bool PreventBreak(ServerPlayer *player, Level *level, const BlockPos &pos);
    // This returns how many extra blocks were broken so the caller can wear the tool for them
    virtual int OnHarvest(ServerPlayer *player, const Harvest &harvest);
    virtual int ModifyWear(int amount, Random *random);
    // This returns how much of the picked up experience is left after the trait spends some on the item
    virtual int OnPickupExperience(ServerPlayer *player, ItemInstance *item, int amount);

protected:
    void SendEffect(Level *level, WorldEffects::eWORLD_EFFECT_TYPE type, const glm::dvec3 &center, float radius);
    void SendParticles(Level *level, ePARTICLE_TYPE type, const glm::dvec3 &center, const glm::dvec3 &spread, float speed, int count);
    void PlaySound(Level *level, const glm::dvec3 &position, float volume);

    ResourceLocation m_id;
    uint8_t m_gearTypes;
    int m_weight;
};
