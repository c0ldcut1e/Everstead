#include "Arrow.h"

#include <algorithm>
#include <cmath>

#include "../../core/EversteadServer.h"
#include "../../network/PlayerList.h"
#include "../../network/packet/s2c/TakeItemEntityS2CPacket.h"
#include "../../tag/CompoundTag.h"
#include "../../utils/HitResult.h"
#include "../../world/ServerLevel.h"
#include "../../world/block/Block.h"
#include "../../world/fracture/FractureTrait.h"
#include "../../world/item/Item.h"
#include "../../world/particle/ParticleSpawnParams.h"
#include "../item/ItemEntity.h"
#include "../player/GameType.h"
#include "../player/ServerPlayer.h"

#include <glm/common.hpp>
#include <glm/trigonometric.hpp>

Arrow::Arrow(Level *level) : Entity(level)
{
    m_ownerId            = 0;
    m_critical           = false;
    m_creativeOnlyPickup = false;
    m_inGround           = false;
    m_stuckBlock         = 0;
    m_life               = 0;
    m_flightTime         = 0;
    m_shake              = 0;
    m_collecting         = false;
    m_collectorIndex     = 0;
    m_collectAge         = 0;
    m_homingId           = 0;
    m_returning          = false;
    m_noGravity          = true;
    m_boundingBox        = {glm::dvec3(-0.25, 0.0, -0.25), glm::dvec3(0.25, 0.5, 0.25)};
}

eENTITY_TYPE Arrow::GetType()
{
    return s_type;
}

void Arrow::Tick()
{
    Random &random = m_level->GetRandom();

    m_totalTickCount++;
    if (m_collecting)
    {
        m_collectAge++;
        if (m_collectAge >= ItemEntity::s_collectTicks)
        {
            m_level->RemoveEntity(this);
        }

        return;
    }

    if (m_position.y < -64.0)
    {
        m_level->RemoveEntity(this);
        return;
    }

    if (m_shake > 0)
    {
        m_shake--;
    }

    if (m_inGround)
    {
        if (!m_level->IsClientSide())
        {
            TickInGround();
        }

        return;
    }

    if (m_returning)
    {
        if (!m_level->IsClientSide())
        {
            TickReturn();
            return;
        }

        SetPosition(m_position + m_velocity);
        FaceVelocity();
        return;
    }

    if (m_homingId != 0 && !m_level->IsClientSide())
    {
        TickHoming();
    }

    m_flightTime++;
    double length        = glm::length(m_velocity);
    glm::dvec3 direction = glm::normalize(m_velocity);
    HitResult *hit       = m_level->Clip(m_position, m_velocity, (float) length, false, false, true);
    bool hitBlock        = hit->IsBlock();
    double nearest       = hitBlock ? hit->GetDistance() : length;
    Direction *face      = nullptr;
    glm::dvec3 hitPos    = hit->GetHitPos();
    if (hitBlock)
    {
        m_stuckPos = hit->GetBlockPos();
        face       = hit->GetBlockFace();
    }

    delete hit;

    glm::dvec3 end = m_position + m_velocity;
    AABB sweep(glm::dvec3(std::min(m_position.x, end.x), std::min(m_position.y, end.y), std::min(m_position.z, end.z)),
               glm::dvec3(std::max(m_position.x, end.x), std::max(m_position.y, end.y), std::max(m_position.z, end.z)));
    std::vector<Entity *> entities;
    m_level->GetPushableEntities(sweep.Inflated(1.0), this, &entities);
    LivingEntity *hitEntity = nullptr;
    for (size_t i = 0; i < entities.size(); i++)
    {
        LivingEntity *entity = (LivingEntity *) entities[i];
        if (!entity->IsPickable() || (entity->GetId() == m_ownerId && m_flightTime < 5))
        {
            continue;
        }

        double distance;
        if (entity->GetAABB().Inflated(0.3).Clip(m_position, direction, nearest, &distance))
        {
            hitEntity = entity;
            nearest   = distance;
        }
    }

    if (hitEntity)
    {
        if (!m_level->IsClientSide())
        {
            int damage = (int) ceil(length * 2.0);
            if (m_critical)
            {
                damage += random.NextInt(0, damage / 2 + 1);
            }

            LivingEntity *owner  = dynamic_cast<LivingEntity *>(m_level->GetEntityById(m_ownerId));
            ServerPlayer *player = dynamic_cast<ServerPlayer *>(hitEntity);
            if (player && player != owner)
            {
                std::vector<std::pair<FractureTrait *, int>> fractures = player->GetArmorFractures();
                for (size_t i = 0; i < fractures.size(); i++)
                {
                    if (fractures[i].first->OnArrowHit(player, this, fractures[i].second))
                    {
                        return;
                    }
                }
            }

            if (hitEntity->Hurt(eDamageSource_Arrow, damage, owner))
            {
                hitEntity->Knockback(m_position, glm::dvec3());
                m_level->PlaySound("random.bowhit" + std::to_string(random.NextInt(1, 4)), m_position, 1.0f,
                                   1.2f / (random.NextFloat() * 0.2f + 0.9f));
                m_level->RemoveEntity(this);
                return;
            }

            m_velocity   = m_velocity * -0.1;
            m_flightTime = 0;
            SetRotation(m_yaw + 180.0f, m_pitch);
            ((ServerLevel *) m_level)->SendEntity(this, nullptr);
        }
        else
        {
            m_velocity = glm::dvec3();
        }

        return;
    }

    if (hitBlock)
    {
        SetPosition(m_position + direction * (nearest - 0.05));
        if (!m_level->IsClientSide())
        {
            for (size_t i = 0; i < m_fractures.size(); i++)
            {
                if (m_fractures[i]->OnArrowLand(this, face))
                {
                    return;
                }
            }
        }

        m_velocity = glm::dvec3();
        m_inGround = true;
        m_shake    = 7;
        m_critical = false;
        if (!m_level->IsClientSide())
        {
            m_stuckBlock = m_level->GetBlockId(m_stuckPos);
            Block::ById(m_stuckBlock)->OnProjectileHit(m_level, m_stuckPos, this, hitPos, face);
            m_level->PlaySound("random.bowhit" + std::to_string(random.NextInt(1, 4)), m_position, 1.0f, 1.2f / (random.NextFloat() * 0.2f + 0.9f));
            ((ServerLevel *) m_level)->SendEntity(this, nullptr);
        }

        return;
    }

    if (m_critical && m_level->IsClientSide())
    {
        for (int i = 0; i < 4; i++)
        {
            ParticleSpawnParams params;
            params.type        = eParticleType_Crit;
            params.position    = m_position + m_velocity * (i / 4.0);
            params.velocity    = glm::dvec3(-m_velocity.x, -m_velocity.y + 0.2, -m_velocity.z);
            params.hasVelocity = true;
            m_level->SpawnParticle(params);
        }
    }

    SetPosition(m_position + m_velocity);
    FaceVelocity();
    m_velocity *= (IsInWater() ? 0.8 : 0.99);
    m_velocity.y -= 0.05;
}

int Arrow::GetUpdateInterval() const
{
    return 20;
}

bool Arrow::SendsVelocity() const
{
    return true;
}

void Arrow::TickInGround()
{
    if (m_level->GetBlockId(m_stuckPos) != m_stuckBlock)
    {
        m_inGround   = false;
        m_life       = 0;
        m_flightTime = 0;
        ((ServerLevel *) m_level)->SendEntity(this, nullptr);
        return;
    }

    m_life++;
    if (m_life >= 1200)
    {
        m_level->RemoveEntity(this);
        return;
    }

    if (m_shake == 0)
    {
        TryPickup();
    }
}

void Arrow::TryPickup()
{
    ServerPlayer *player = (ServerPlayer *) ((ServerLevel *) m_level)->FindNearestPlayer(m_position, 3.0);
    if (!player || (m_creativeOnlyPickup && !GameType::HasInfiniteItems(player->GetGameType())))
    {
        return;
    }

    const AABB &playerBox = player->GetAABB();
    AABB reach(playerBox.GetMin() - glm::dvec3(1.0, 0.5, 1.0), playerBox.GetMax() + glm::dvec3(1.0, 0.5, 1.0));
    if (!reach.Intersects(GetAABB()))
    {
        return;
    }

    GiveTo(player);
}

void Arrow::TickReturn()
{
    ServerPlayer *owner = dynamic_cast<ServerPlayer *>(m_level->GetEntityById(m_ownerId));
    if (!owner)
    {
        m_returning = false;
        ((ServerLevel *) m_level)->SendEntity(this, nullptr);
        return;
    }

    glm::dvec3 delta = owner->GetPosition() + glm::dvec3(0.0, owner->GetEyeHeight() - 0.3, 0.0) - m_position;
    double distance  = glm::length(delta);
    if (distance < 1.5)
    {
        if (!GiveTo(owner))
        {
            m_returning = false;
            m_velocity  = glm::dvec3();
            ((ServerLevel *) m_level)->SendEntity(this, nullptr);
        }

        return;
    }

    m_velocity = delta / distance * std::min(distance, 1.2);
    SetPosition(m_position + m_velocity);
    FaceVelocity();
    ((ServerLevel *) m_level)->SendEntity(this, nullptr);
}

void Arrow::TickHoming()
{
    LivingEntity *target = dynamic_cast<LivingEntity *>(m_level->GetEntityById(m_homingId));
    if (!target || target->GetHealth() <= 0)
    {
        return;
    }

    const AABB &box    = target->GetAABB();
    glm::dvec3 desired = glm::normalize((box.GetMin() + box.GetMax()) * 0.5 - m_position) * std::max(glm::length(m_velocity), 1.0);
    m_velocity         = glm::mix(m_velocity, desired, 0.35);
    ((ServerLevel *) m_level)->SendEntity(this, nullptr);
}

bool Arrow::GiveTo(ServerPlayer *player)
{
    Random &random = m_level->GetRandom();

    if (!m_creativeOnlyPickup)
    {
        ItemInstance item(Item::ByName("arrow"));
        player->GetInventory()->AddItem(item);
        if (!item.IsEmpty())
        {
            return false;
        }

        player->GetServer()->GetPlayerList()->SendInventoryTo(player);
    }

    player->GetServer()->GetPlayerList()->SendToLevel(m_level,
                                                      std::shared_ptr<Packet>(new TakeItemEntityS2CPacket(GetId(), player->GetPlayerIndex(), 1)));
    m_level->PlaySound("random.pop", m_position, 0.2f, ((random.NextFloat() - random.NextFloat()) * 0.7f + 1.0f) * 2.0f);
    m_level->RemoveEntity(this);
    return true;
}

bool Arrow::IsPickable() const
{
    return false;
}

double Arrow::GetRenderDistanceScale() const
{
    return 4.0;
}

// The server owns the stuck state and resends the arrow whenever it changes, so the client never sticks or loosens on its own
void Arrow::ReadSpawnData(int data)
{
    m_ownerId   = data >> 3;
    m_critical  = (data & 1) != 0;
    m_inGround  = (data & 2) != 0;
    m_returning = (data & 4) != 0;
}

int Arrow::GetSpawnData()
{
    return (m_ownerId << 3) | (m_returning ? 4 : 0) | (m_inGround ? 2 : 0) | (m_critical ? 1 : 0);
}

void Arrow::Load(const CompoundTag *tag)
{
    Entity::Load(tag);
    if (tag->Contains("InGround"))
    {
        m_inGround = tag->GetBoolean("InGround");
    }

    if (tag->Contains("StuckX") && tag->Contains("StuckY") && tag->Contains("StuckZ"))
    {
        m_stuckPos = BlockPos(tag->GetInt("StuckX"), tag->GetInt("StuckY"), tag->GetInt("StuckZ"));
    }

    if (tag->Contains("StuckBlock"))
    {
        m_stuckBlock = (uint32_t) tag->GetInt("StuckBlock");
    }

    if (tag->Contains("Life"))
    {
        m_life = tag->GetShort("Life");
    }

    if (tag->Contains("Shake"))
    {
        m_shake = tag->GetByte("Shake");
    }

    if (tag->Contains("Critical"))
    {
        m_critical = tag->GetBoolean("Critical");
    }

    if (tag->Contains("CreativeOnlyPickup"))
    {
        m_creativeOnlyPickup = tag->GetBoolean("CreativeOnlyPickup");
    }
}

void Arrow::Save(CompoundTag *tag) const
{
    Entity::Save(tag);
    tag->PutBoolean("InGround", m_inGround);
    tag->PutInt("StuckX", m_stuckPos.x);
    tag->PutInt("StuckY", m_stuckPos.y);
    tag->PutInt("StuckZ", m_stuckPos.z);
    tag->PutInt("StuckBlock", (int) m_stuckBlock);
    tag->PutShort("Life", (int16_t) m_life);
    tag->PutByte("Shake", (int8_t) m_shake);
    tag->PutBoolean("Critical", m_critical);
    tag->PutBoolean("CreativeOnlyPickup", m_creativeOnlyPickup);
}

void Arrow::Shoot(LivingEntity *owner, float power, bool critical, bool creativeOnlyPickup)
{
    Random &random = m_level->GetRandom();

    m_ownerId            = owner->GetId();
    m_critical           = critical;
    m_creativeOnlyPickup = creativeOnlyPickup;
    glm::dvec3 front     = glm::normalize(owner->GetFront());
    glm::dvec3 side      = glm::normalize(glm::cross(front, glm::dvec3(0.0, 1.0, 0.0))) * 0.16;
    glm::dvec3 direction =
            glm::dvec3(front.x + random.NextGaussian() * 0.0075, front.y + random.NextGaussian() * 0.0075, front.z + random.NextGaussian() * 0.0075);
    SetPosition(owner->GetPosition() + glm::dvec3(0.0, owner->GetEyeHeight() - 0.1, 0.0) + side);
    SetVelocity(direction * (power * 3.0));
    FaceVelocity();
}

void Arrow::ShootAt(LivingEntity *owner, LivingEntity *target, float velocity, float inaccuracy)
{
    Random &random = m_level->GetRandom();

    m_ownerId             = owner->GetId();
    m_creativeOnlyPickup  = true;
    const AABB &targetBox = target->GetAABB();
    double y              = owner->GetPosition().y + owner->GetEyeHeight() - 0.1;
    double dx             = target->GetPosition().x - owner->GetPosition().x;
    double dy             = targetBox.GetMin().y + (targetBox.GetMax().y - targetBox.GetMin().y) / 3.0 - y;
    double dz             = target->GetPosition().z - owner->GetPosition().z;
    double horizontal     = sqrt(dx * dx + dz * dz);
    if (horizontal < 1.0e-7)
    {
        return;
    }

    SetPosition(glm::dvec3(owner->GetPosition().x + dx / horizontal, y, owner->GetPosition().z + dz / horizontal));
    glm::dvec3 direction = glm::normalize(glm::dvec3(dx, dy + horizontal * 0.2, dz));
    direction = glm::dvec3(direction.x + random.NextGaussian() * 0.0075 * inaccuracy, direction.y + random.NextGaussian() * 0.0075 * inaccuracy,
                           direction.z + random.NextGaussian() * 0.0075 * inaccuracy);
    SetVelocity(direction * (double) velocity);
    FaceVelocity();
}

void Arrow::Launch(const glm::dvec3 &position, const glm::dvec3 &velocity)
{
    SetPosition(position);
    SetVelocity(velocity);
    FaceVelocity();
}

void Arrow::Redirect(Entity *owner, const glm::dvec3 &velocity)
{
    m_velocity   = velocity;
    m_ownerId    = owner->GetId();
    m_flightTime = 0;
    FaceVelocity();
    ((ServerLevel *) m_level)->SendEntity(this, nullptr);
}

void Arrow::SetFractures(const std::vector<FractureTrait *> &fractures)
{
    m_fractures = fractures;
}

void Arrow::HomeOn(LivingEntity *target, const glm::dvec3 &velocity)
{
    m_homingId   = target->GetId();
    m_velocity   = velocity;
    m_flightTime = 0;
    FaceVelocity();
    ((ServerLevel *) m_level)->SendEntity(this, nullptr);
}

int Arrow::GetHomingTargetId() const
{
    return m_homingId;
}

void Arrow::ReturnToOwner()
{
    m_returning = true;
    ((ServerLevel *) m_level)->SendEntity(this, nullptr);
}

void Arrow::FaceVelocity()
{
    float yaw   = glm::degrees((float) atan2(m_velocity.z, m_velocity.x));
    float pitch = glm::degrees((float) atan2(m_velocity.y, sqrt(m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z)));
    SetRotation(yaw, pitch);
}

float Arrow::GetShake(float partialTicks) const
{
    return m_shake - partialTicks;
}

void Arrow::StartCollect(uint8_t playerIndex)
{
    m_collecting     = true;
    m_collectorIndex = playerIndex;
    m_collectAge     = 0;
}

bool Arrow::IsCollecting() const
{
    return m_collecting;
}

int Arrow::GetOwnerId() const
{
    return m_ownerId;
}

uint8_t Arrow::GetCollectorIndex() const
{
    return m_collectorIndex;
}

float Arrow::GetCollectProgress(float partialTicks) const
{
    float progress = ((float) m_collectAge + partialTicks) / (float) ItemEntity::s_collectTicks;
    return progress > 1.0f ? 1.0f : progress * progress;
}
