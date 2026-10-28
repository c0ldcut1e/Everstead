#include "LivingEntity.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

#include "../utils/Random.h"
#include "../utils/math/Mth.h"
#include "../world/Level.h"
#include "../world/block/Block.h"
#include "../world/block/FarmlandBlock.h"
#include "../world/block/PointedDripstoneBlock.h"
#include "../world/chunk/Chunk.h"
#include "../world/particle/ParticleSpawnParams.h"

#include <glm/trigonometric.hpp>

const int LivingEntity::s_swingDuration          = 6;
const float LivingEntity::s_maxHeadYaw           = 75.0f;
const float LivingEntity::s_moveThresholdSquared = 0.0025f;

LivingEntity::LivingEntity(Level *level) : Entity(level)
{
    m_swinging           = false;
    m_swingTime          = 0;
    m_attackAnim         = 0.0f;
    m_attackAnimOld      = 0.0f;
    m_fallDistance       = 0.0f;
    m_health             = 20;
    m_lastHealth         = 20;
    m_lastHurt           = 0;
    m_invulnerableTime   = 0;
    m_hurtTime           = 0;
    m_critTime           = 0;
    m_deathTime          = 0;
    m_airSupply          = s_totalAirSupply;
    m_fireTicks          = 0;
    m_damageSpill        = 0;
    m_limbSwing          = 0.0f;
    m_limbSwingOld       = 0.0f;
    m_limbSwingAmount    = 0.0f;
    m_limbSwingAmountOld = 0.0f;
    m_bodyYaw            = 0.0f;
    m_bodyYawOld         = 0.0f;
    m_headYaw            = 0.0f;
    m_headYawOld         = 0.0f;
    m_headYawChanged     = false;
    m_stepDistance       = 0.0f;
    m_nextStep           = 1;
}

LivingEntity::~LivingEntity()
{}

eENTITY_TYPE LivingEntity::GetType()
{
    return s_type;
}

void LivingEntity::TickHurt()
{
    if (m_invulnerableTime > 0)
    {
        m_invulnerableTime--;
    }

    if (m_hurtTime > 0)
    {
        m_hurtTime--;
    }
}

void LivingEntity::PushEntities()
{
    const AABB &box = GetAABB();
    std::vector<Entity *> entities;
    m_level->GetPushableEntities(AABB(box.GetMin() - glm::dvec3(0.2, 0.0, 0.2), box.GetMax() + glm::dvec3(0.2, 0.0, 0.2)), this, &entities);
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i] != m_vehicle && entities[i]->GetVehicle() != this && entities[i]->GetType() != eEntityType_RemotePlayer &&
            CanPush(entities[i]))
        {
            entities[i]->Push(this);
        }
    }

    TickCramming();
}

bool LivingEntity::CanPush(Entity *entity)
{
    PlayerTeam *team               = GetTeam();
    PlayerTeam *otherTeam          = entity->GetTeam();
    eTEAM_COLLISION_RULE rule      = team ? team->collisionRule : eTeamCollisionRule_Always;
    eTEAM_COLLISION_RULE otherRule = otherTeam ? otherTeam->collisionRule : eTeamCollisionRule_Always;
    if (rule == eTeamCollisionRule_Never || otherRule == eTeamCollisionRule_Never)
    {
        return false;
    }

    bool allied = team && team == otherTeam;
    if ((rule == eTeamCollisionRule_PushOwnTeam || otherRule == eTeamCollisionRule_PushOwnTeam) && allied)
    {
        return false;
    }

    return (rule != eTeamCollisionRule_PushOtherTeams && otherRule != eTeamCollisionRule_PushOtherTeams) || allied;
}

void LivingEntity::TickCramming()
{
    int maxCramming = m_level->GetGameRules().GetInt(eGameRule_MaxEntityCramming);
    if (m_level->IsClientSide() || maxCramming <= 0 || m_level->GetRandom().NextInt(0, 3) != 0)
    {
        return;
    }

    std::vector<Entity *> entities;
    m_level->GetPushableEntities(GetAABB(), this, &entities);
    int count = 0;
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (!entities[i]->GetVehicle() && CanPush(entities[i]))
        {
            count++;
        }
    }

    if (count >= maxCramming)
    {
        Hurt(eDamageSource_Cramming, 6, nullptr);
    }
}

void LivingEntity::TickCrit()
{
    Random &random = m_level->GetRandom();

    if (m_critTime <= 0)
    {
        return;
    }

    m_critTime--;
    const AABB &box = GetAABB();
    glm::dvec3 size = box.GetMax() - box.GetMin();
    for (int i = 0; i < 16; i++)
    {
        double xa = random.NextFloat() * 2.0f - 1.0f;
        double ya = random.NextFloat() * 2.0f - 1.0f;
        double za = random.NextFloat() * 2.0f - 1.0f;
        if (xa * xa + ya * ya + za * za > 1.0)
        {
            continue;
        }

        ParticleSpawnParams params;
        params.type = eParticleType_Crit;
        params.position =
                glm::dvec3(m_position.x + xa * size.x / 4.0, box.GetMin().y + size.y / 2.0 + ya * size.y / 4.0, m_position.z + za * size.z / 4.0);
        params.velocity    = glm::dvec3(xa, ya + 0.2, za);
        params.hasVelocity = true;
        m_level->SpawnParticle(params);
    }
}

void LivingEntity::TickDeath()
{
    Random &random = m_level->GetRandom();

    if (m_health > 0)
    {
        m_deathTime = 0;
        return;
    }

    if (m_deathTime >= 20)
    {
        return;
    }

    m_deathTime++;
    if (m_deathTime < 20)
    {
        return;
    }

    const AABB &box = GetAABB();
    glm::dvec3 size = box.GetMax() - box.GetMin();
    for (int i = 0; i < 20; i++)
    {
        ParticleSpawnParams params;
        params.type        = eParticleType_Explode;
        params.position    = glm::dvec3(m_position.x + random.NextFloat() * size.x * 2.0 - size.x, box.GetMin().y + random.NextFloat() * size.y,
                                        m_position.z + random.NextFloat() * size.x * 2.0 - size.x);
        params.velocity    = glm::dvec3(random.NextGaussian() * 0.02, random.NextGaussian() * 0.02, random.NextGaussian() * 0.02);
        params.hasVelocity = true;
        m_level->SpawnParticle(params);
    }
}

void LivingEntity::Swing()
{
    if (CanStartSwing())
    {
        m_swingTime = -1;
        m_swinging  = true;
    }
}

bool LivingEntity::CanStartSwing() const
{
    return !m_swinging || m_swingTime >= s_swingDuration / 2 || m_swingTime < 0;
}

void LivingEntity::UpdateSwing()
{
    m_attackAnimOld = m_attackAnim;

    if (m_swinging)
    {
        m_swingTime++;
        if (m_swingTime >= s_swingDuration)
        {
            m_swinging = false;
        }

        m_attackAnim = (float) m_swingTime / (float) s_swingDuration;
    }
    else
    {
        m_swingTime     = 0;
        m_attackAnim    = 0.0f;
        m_attackAnimOld = 0.0f;
    }
}

float LivingEntity::GetAttackAnim(float partialTicks) const
{
    float diff = m_attackAnim - m_attackAnimOld;
    if (diff < 0.0f)
    {
        diff += 1.0f;
    }

    return m_attackAnimOld + diff * partialTicks;
}

int LivingEntity::GetHealth() const
{
    return m_health;
}

void LivingEntity::SetHealth(int health)
{
    m_health = std::clamp(health, 0, GetMaxHealth());
}

float LivingEntity::GetFallDistance() const
{
    return m_fallDistance;
}

void LivingEntity::SetFallDistance(float fallDistance)
{
    m_fallDistance = fallDistance;
}

int LivingEntity::GetMaxHealth() const
{
    return (int) GetAttributeValue(Attribute::s_maxHealth);
}

int LivingEntity::GetLastHealth() const
{
    return m_lastHealth;
}

int LivingEntity::GetInvulnerableTime() const
{
    return m_invulnerableTime;
}

void LivingEntity::SetInvulnerableTime(int ticks)
{
    m_invulnerableTime = ticks;
}

float LivingEntity::GetHurtAnim(float partialTicks) const
{
    if (m_hurtTime <= 0)
    {
        return 0.0f;
    }

    float hurt = ((float) m_hurtTime - partialTicks) / (float) s_hurtDuration;
    return sinf(hurt * hurt * hurt * hurt * std::numbers::pi_v<float>);
}

int LivingEntity::GetHurtTime() const
{
    return m_hurtTime;
}

int LivingEntity::GetDeathTime() const
{
    return m_deathTime;
}

void LivingEntity::AnimateHurt()
{
    m_invulnerableTime = s_invulnerableDuration;
    m_hurtTime         = s_hurtDuration;
}

void LivingEntity::AnimateCrit()
{
    m_critTime = 3;
}

int LivingEntity::GetAirSupply() const
{
    return m_airSupply;
}

bool LivingEntity::TickAirSupply()
{
    if (m_health <= 0 || !IsInWater() || !m_level->IsLiquidAt(m_position + glm::dvec3(0.0, GetEyeHeight(), 0.0), false))
    {
        m_airSupply = s_totalAirSupply;
        return false;
    }

    if (m_level->GetRandom().NextInt(0, (int) GetAttributeValue(Attribute::s_oxygenBonus)) == 0)
    {
        m_airSupply--;
    }

    if (m_airSupply == -20)
    {
        m_airSupply = 0;
        return true;
    }

    return false;
}

uint32_t LivingEntity::MultiplyTint(uint32_t color, uint32_t tint)
{
    uint32_t r = (((color >> 16) & 0xFF) * ((tint >> 16) & 0xFF)) / 255;
    uint32_t g = (((color >> 8) & 0xFF) * ((tint >> 8) & 0xFF)) / 255;
    uint32_t b = ((color & 0xFF) * (tint & 0xFF)) / 255;
    return (color & 0xFF000000) | (r << 16) | (g << 8) | b;
}

void LivingEntity::UpdateFall(const glm::dvec3 &position, double moveY, bool grounded)
{
    const float minFallDistance = 3.0f;

    if (m_flying || IsInWater() || IsOnLadder() || IsTouchingBlock(Block::ByName("sweet_berry_bush")))
    {
        m_fallDistance = 0.0f;
        return;
    }

    if (!grounded)
    {
        if (moveY < 0.0)
        {
            m_fallDistance -= (float) moveY;
        }
        return;
    }

    float fallDistance = m_fallDistance;
    m_fallDistance     = 0.0f;
    if (fallDistance <= minFallDistance)
    {
        return;
    }

    int blockX   = (int) floor(position.x);
    int blockY   = (int) floor(position.y - 0.2);
    int blockZ   = (int) floor(position.z);
    Block *block = Block::ById(m_level->GetBlockId(BlockPos(blockX, blockY, blockZ)));
    if (!block || block == Block::ById(0) || block->IsLiquid())
    {
        return;
    }

    uint32_t color     = 0xFF999999;
    ChunkPos chunkPos  = ChunkPos(Mth::FloorDiv(blockX, Chunk::s_sizeX), Mth::FloorDiv(blockZ, Chunk::s_sizeZ));
    const Chunk *chunk = m_level->GetChunk(chunkPos);
    if (chunk)
    {
        color = MultiplyTint(color, block->ResolveTint(Direction::up, m_level, chunk, Mth::FloorMod(blockX, Chunk::s_sizeX),
                                                       Mth::FloorMod(blockZ, Chunk::s_sizeZ)));
    }

    float scale = std::min(0.2f + ceilf(fallDistance - minFallDistance) / 15.0f, 2.5f);

    Block::UVRect rect = block->GetAtlasUVRect(Direction::up);
    ParticleSpawnParams params;
    params.type      = eParticleType_Terrain;
    params.position  = glm::dvec3(position.x, position.y + 0.1, position.z);
    params.dustSpeed = 0.15;
    params.region    = {rect.u0, rect.v0, rect.u1, rect.v1};
    params.color     = color;
    params.amount    = (int) (150.0f * scale);
    params.hasRegion = true;
    params.hasColor  = true;
    m_level->SpawnParticle(params);
}

void LivingEntity::UpdateStepSound(const glm::dvec3 &moved, bool grounded)
{
    const float stepVolume = 0.15f;

    if (IsSneaking())
    {
        return;
    }

    bool onLadder   = IsOnLadder();
    double distance = moved.x * moved.x + moved.y * moved.y + moved.z * moved.z;

    m_stepDistance += (float) sqrt(distance) * 0.6f;
    if ((!grounded && !onLadder) || m_flying || m_stepDistance <= (float) m_nextStep)
    {
        return;
    }

    BlockPos below((int) floor(m_position.x), (int) floor(m_position.y - 0.2), (int) floor(m_position.z));
    if (onLadder)
    {
        below = BlockPos((int) floor(m_position.x), (int) floor(m_aabb.GetMin().y), (int) floor(m_position.z));
    }

    Block *block = Block::ById(m_level->GetBlockId(below));
    if (!block || block == Block::ById(0) || block->IsLiquid())
    {
        return;
    }

    m_nextStep = (int) m_stepDistance + 1;

    int variants      = 5;
    std::string sound = "step.ladder";
    if (!onLadder || block != Block::ByName("ladder"))
    {
        sound = block->GetStepSound(&variants);
    }
    m_level->PlaySound(sound + std::to_string(m_level->GetRandom().NextInt(1, variants)), m_position, stepVolume, 1.0f);
}

int LivingEntity::GetArmorValue() const
{
    return std::min((int) GetAttributeValue(Attribute::s_armor), 20);
}

bool LivingEntity::Hurt(eDAMAGE_SOURCE source, int damage, LivingEntity *attacker)
{
    if (m_health <= 0)
    {
        return false;
    }

    // Damage during the second half of invulnerability only applies the amount above the last hit
    if (m_invulnerableTime > s_invulnerableDuration / 2)
    {
        if (damage <= m_lastHurt)
        {
            return false;
        }

        SetHealth(m_health - AbsorbDamage(source, damage - m_lastHurt));
        m_lastHurt = damage;
        OnHurt(source);
        if (m_health <= 0)
        {
            Die(source, attacker);
        }

        return false;
    }

    m_lastHurt   = damage;
    m_lastHealth = m_health;
    AnimateHurt();
    SetHealth(m_health - AbsorbDamage(source, damage));
    OnHurt(source);
    m_level->OnEntityHurt(this);
    std::string sound = m_health <= 0 ? GetDeathSound() : GetHurtSound();
    if (!sound.empty())
    {
        m_level->PlaySound(sound, m_position, GetSoundVolume(), GetVoicePitch());
    }

    if (m_health <= 0)
    {
        Die(source, attacker);
    }

    return true;
}

void LivingEntity::Heal(int amount)
{
    if (m_health <= 0)
    {
        return;
    }

    SetHealth(std::min(m_health + amount, GetMaxHealth()));
    m_invulnerableTime = std::max(m_invulnerableTime, s_invulnerableDuration / 2);
}

void LivingEntity::Knockback(const glm::dvec3 &source, const glm::dvec3 &push)
{
    Random &random = m_level->GetRandom();

    double sourceX = source.x - m_position.x;
    double sourceZ = source.z - m_position.z;
    while (sourceX * sourceX + sourceZ * sourceZ < 1.0e-4)
    {
        sourceX = (random.NextDouble() - random.NextDouble()) * 0.01;
        sourceZ = (random.NextDouble() - random.NextDouble()) * 0.01;
    }

    double distance = sqrt(sourceX * sourceX + sourceZ * sourceZ);
    m_velocity      = glm::dvec3(m_velocity.x / 2.0 - sourceX / distance * 0.4 + push.x, std::min(m_velocity.y / 2.0 + 0.4, 0.4) + push.y,
                                 m_velocity.z / 2.0 - sourceZ / distance * 0.4 + push.z);
}

void LivingEntity::SetOnFire(int seconds)
{
    m_fireTicks = std::max(m_fireTicks, seconds * 20);
}

void LivingEntity::TickFire()
{
    Random &random = m_level->GetRandom();

    if (m_fireTicks > 0)
    {
        if (m_fireTicks % 20 == 0)
        {
            Hurt(eDamageSource_OnFire, 1, nullptr);
        }

        m_fireTicks--;
    }

    UpdateAABB();
    if (IsInLava())
    {
        Hurt(eDamageSource_Lava, 4, nullptr);
        SetOnFire(15);
        m_fallDistance *= 0.5f;
    }

    BlockPos feet((int) floor(m_position.x), (int) floor(m_position.y), (int) floor(m_position.z));
    BlockPos head(feet.x, feet.y + 1, feet.z);
    BlockPos below(feet.x, (int) floor(m_position.y - 0.2), feet.z);
    if (m_onGround && !IsSneaking() && Block::ById(m_level->GetBlockId(below)) == Block::ByName("magma"))
    {
        Hurt(eDamageSource_HotFloor, 1, nullptr);
    }

    bool wet = IsInWater() || (feet.y >= 0 && feet.y < Chunk::s_sizeY && (m_level->IsRainingAt(feet) || m_level->IsRainingAt(head)));
    if (IsTouchingBlock(Block::ByName("prickly_vine")))
    {
        Hurt(eDamageSource_PricklyVine, 1, nullptr);
    }

    Block *sweetBerryBush = Block::ByName("sweet_berry_bush");
    if ((fabs(m_position.x - m_oldPosition.x) >= 0.003 || fabs(m_position.z - m_oldPosition.z) >= 0.003) &&
        IsTouchingBlock(sweetBerryBush, sweetBerryBush->FindProperty("age"), 1))
    {
        Hurt(eDamageSource_SweetBerryBush, 1, nullptr);
    }

    if (IsTouchingBlock(Block::ByName("cactus")))
    {
        Hurt(eDamageSource_Cactus, 1, nullptr);
    }

    if (IsTouchingBlock(Block::ByName("fire")))
    {
        Hurt(eDamageSource_InFire, 1, nullptr);
        if (!wet)
        {
            SetOnFire(8);
        }
    }

    if (wet && m_fireTicks > 0)
    {
        m_level->PlaySound("random.fizz", m_position, 0.7f, 1.6f + (random.NextFloat() - random.NextFloat()) * 0.4f);
        m_fireTicks = 0;
    }
}

void LivingEntity::CauseFallDamage(float distance)
{
    Random &random = m_level->GetRandom();

    BlockPos below((int) floor(m_position.x), (int) floor(m_position.y - 0.2), (int) floor(m_position.z));
    Block *block                   = Block::ById(m_level->GetBlockId(below));
    PointedDripstoneBlock *pointed = dynamic_cast<PointedDripstoneBlock *>(block);
    uint16_t data                  = m_level->GetBlockData(below);
    eDAMAGE_SOURCE source          = eDamageSource_Fall;
    float multiplier               = block->GetFallDamageMultiplier(this);
    if (pointed && pointed->GetTipDirection(data) == Direction::up && pointed->GetThickness(data) == PointedDripstoneBlock::eThickness_TIP)
    {
        source = eDamageSource_Stalagmite;
        multiplier *= 2.0f;
        distance += 2.5f;
    }

    FarmlandBlock *farmland = dynamic_cast<FarmlandBlock *>(block);
    if (farmland)
    {
        farmland->OnFallOn(m_level, below, distance);
    }

    int damage = (int) ceilf((distance - (float) GetAttributeValue(Attribute::s_safeFallDistance)) * multiplier *
                             (float) GetAttributeValue(Attribute::s_fallDamageMultiplier));
    if (damage <= 0)
    {
        return;
    }

    std::string fallSound = damage > 4 ? "damage.fallbig" + std::to_string(random.NextInt(1, 2)) : "damage.fallsmall";
    m_level->PlaySound(fallSound, m_position, 1.0f, 1.0f);
    Hurt(source, damage, nullptr);

    if (block == Block::ById(0))
    {
        return;
    }

    int variants      = 5;
    std::string sound = block->GetStepSound(&variants);
    m_level->PlaySound(sound + std::to_string(random.NextInt(1, variants)), m_position, 0.5f, 0.75f);
}

bool LivingEntity::IsArmorDamage(eDAMAGE_SOURCE source)
{
    return source == eDamageSource_InFire || source == eDamageSource_Lava || source == eDamageSource_Player || source == eDamageSource_Explosion ||
           source == eDamageSource_FallingStalactite || source == eDamageSource_Thrown || source == eDamageSource_Mob ||
           source == eDamageSource_Arrow || source == eDamageSource_LightningBolt;
}

int LivingEntity::AbsorbDamage(eDAMAGE_SOURCE source, int damage)
{
    if (!IsArmorDamage(source))
    {
        return damage;
    }

    int absorbed  = damage * (25 - GetArmorValue()) + m_damageSpill;
    m_damageSpill = absorbed % 25;
    return absorbed / 25;
}

void LivingEntity::OnHurt(eDAMAGE_SOURCE source)
{
    (void) source;
}

std::string LivingEntity::GetHurtSound() const
{
    return "damage.hurtflesh" + std::to_string(m_level->GetRandom().NextInt(1, 3));
}

std::string LivingEntity::GetDeathSound() const
{
    return GetHurtSound();
}

float LivingEntity::GetSoundVolume() const
{
    return 1.0f;
}

float LivingEntity::GetVoicePitch() const
{
    Random &random = m_level->GetRandom();

    return (random.NextFloat() - random.NextFloat()) * 0.2f + 1.0f;
}

void LivingEntity::Die(eDAMAGE_SOURCE source, LivingEntity *attacker)
{
    (void) source;
    (void) attacker;
    m_fireTicks    = 0;
    m_fallDistance = 0.0f;
}

void LivingEntity::ApplyNetworkState(const glm::dvec3 &position, float yaw, float pitch)
{
    if (!m_hasNetworkState)
    {
        m_bodyYaw    = yaw;
        m_bodyYawOld = yaw;
        m_headYaw    = yaw;
        m_headYawOld = yaw;
    }

    Entity::ApplyNetworkState(position, yaw, pitch);
}

void LivingEntity::ApplyRenderState(const glm::dvec3 &position, float yaw, float pitch)
{
    m_lerpSteps = 0;
    if (!m_hasNetworkState)
    {
        m_hasNetworkState = true;
        m_bodyYaw         = yaw;
        m_bodyYawOld      = yaw;
        m_headYaw         = yaw;
        m_headYawOld      = yaw;
        SetPosition(position);
        SetRotation(yaw, pitch);
        StoreOld();
        UpdateAABB();
        return;
    }

    SetPosition(position);
    SetRotation(yaw, pitch);
}

void LivingEntity::SetInstantHeadRotation(float yaw, float pitch)
{
    m_headYaw    = yaw;
    m_headYawOld = yaw;
    m_pitch      = pitch;
    m_oldPitch   = pitch;
}

void LivingEntity::ApplyHeadYaw(float yaw)
{
    m_headYawOld = m_headYaw;
    m_headYaw    = yaw;
    while (m_headYaw - m_headYawOld <= -180.0f)
    {
        m_headYawOld -= 360.0f;
    }

    while (m_headYaw - m_headYawOld > 180.0f)
    {
        m_headYawOld += 360.0f;
    }

    m_headYawChanged = true;
}

float LivingEntity::GetHeadYaw() const
{
    return m_headYaw;
}

void LivingEntity::TickRemote()
{
    if (m_headYawChanged)
    {
        m_headYawChanged = false;
    }
    else
    {
        m_headYawOld = m_headYaw;
    }

    Entity::TickRemote();
}

void LivingEntity::UpdateLimbSwing(float horizontalSpeed)
{
    m_limbSwingOld       = m_limbSwing;
    m_limbSwingAmountOld = m_limbSwingAmount;

    float targetAmount = std::clamp(horizontalSpeed * 4.0f, 0.0f, 1.0f);
    m_limbSwingAmount += (targetAmount - m_limbSwingAmount) * 0.4f;
    m_limbSwing += m_limbSwingAmount;
}

float LivingEntity::GetRenderBodyYaw(float partialTicks) const
{
    float delta = m_bodyYaw - m_bodyYawOld;
    while (delta <= -180.0f)
    {
        delta += 360.0f;
    }

    while (delta > 180.0f)
    {
        delta -= 360.0f;
    }

    return m_bodyYawOld + delta * partialTicks;
}

float LivingEntity::GetRenderHeadYaw(float partialTicks) const
{
    return m_headYawOld + (m_headYaw - m_headYawOld) * partialTicks;
}

float LivingEntity::GetLimbSwing() const
{
    return m_limbSwing;
}

float LivingEntity::GetLimbSwingOld() const
{
    return m_limbSwingOld;
}

float LivingEntity::GetLimbSwingAmount() const
{
    return m_limbSwingAmount;
}

float LivingEntity::GetLimbSwingAmountOld() const
{
    return m_limbSwingAmountOld;
}

void LivingEntity::UpdateBodyYaw(float deltaX, float deltaZ)
{
    m_bodyYawOld = m_bodyYaw;

    float targetBodyYaw = m_bodyYaw;
    if (m_attackAnim > 0.0f)
    {
        targetBodyYaw = m_yaw;
    }
    else if (deltaX * deltaX + deltaZ * deltaZ > s_moveThresholdSquared)
    {
        targetBodyYaw = glm::degrees(atan2f(deltaZ, deltaX));
    }

    float toTarget = targetBodyYaw - m_bodyYaw;
    while (toTarget < -180.0f)
    {
        toTarget += 360.0f;
    }

    while (toTarget >= 180.0f)
    {
        toTarget -= 360.0f;
    }

    m_bodyYaw += toTarget * 0.3f;

    float headBodyDelta = m_headYaw - m_bodyYaw;
    while (headBodyDelta < -180.0f)
    {
        headBodyDelta += 360.0f;
    }

    while (headBodyDelta >= 180.0f)
    {
        headBodyDelta -= 360.0f;
    }

    if (headBodyDelta < -s_maxHeadYaw)
    {
        headBodyDelta = -s_maxHeadYaw;
    }

    if (headBodyDelta >= s_maxHeadYaw)
    {
        headBodyDelta = s_maxHeadYaw;
    }

    m_bodyYaw = m_headYaw - headBodyDelta;
    if (headBodyDelta * headBodyDelta > 2500.0f)
    {
        m_bodyYaw += headBodyDelta * 0.2f;
    }
}
