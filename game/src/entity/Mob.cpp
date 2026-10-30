#include "Mob.h"

#include <cmath>
#include <limits>
#include <memory>
#include <utility>

#include "../utils/HitResult.h"
#include "../world/block/Block.h"
#include "../world/block/FenceBlock.h"
#include "../world/block/WallBlock.h"
#include "../world/item/Item.h"
#include "../world/item/ItemInstance.h"
#include "../world/loot/LootTables.h"
#include "EntityFactory.h"
#include "item/ExperienceOrb.h"
#include "item/ItemEntity.h"
#include "item/LeashKnot.h"
#include "player/GameType.h"
#include "player/Player.h"

#include <glm/gtx/norm.hpp>

Mob::Mob(Level *level)
    : LivingEntity(level), m_moveControl(this), m_jumpControl(this), m_lookControl(this), m_bodyControl(this), m_navigation(this, level)
{
    m_width                  = 0.6f;
    m_height                 = 1.8f;
    m_speed                  = 0.0;
    m_forwardInput           = 0.0;
    m_strafeInput            = 0.0;
    m_noActionTime           = 0;
    m_ambientSoundTime       = 0;
    m_lastHurtByMobId        = -1;
    m_lastHurtByMobTime      = 0;
    m_lastHurtByMobTimestamp = 0;
    m_targetId               = -1;
    m_attackTime             = 0;
    m_lastHurtByPlayerTime   = 0;
    m_sentData               = MobData();
    m_leashHolderId          = -1;
    m_leashRestoreTime       = 0;
    m_stepHeight             = 0.5;
    SetSize(m_width, m_height);
}

Mob::~Mob()
{}

eENTITY_TYPE Mob::GetType()
{
    return s_type;
}

void Mob::Tick()
{
    if (m_level->IsClientSide())
    {
        ClientTick();
    }
    else
    {
        ServerTick();
    }
}

void Mob::ServerTick()
{
    TickHurt();
    TickFire();
    SetSharedFlag(eEntityFlag_OnFire, m_fireTicks > 0);
    if (m_health > 0 && TickAirSupply())
    {
        Hurt(eDamageSource_Drown, 2, nullptr);
    }

    if (m_position.y < -64.0)
    {
        Hurt(eDamageSource_OutOfWorld, 4, nullptr);
    }

    if (m_health > 0)
    {
        m_noActionTime++;
        if (RemoveWhenFarAway() && m_leashHolderId < 0 && !m_leashTag)
        {
            Player *player = m_level->FindNearestPlayer(m_position, std::numeric_limits<double>::infinity());
            if (player)
            {
                double distSq = glm::distance2(player->GetPosition(), m_position);
                if (distSq > 128.0 * 128.0 || (m_noActionTime > 600 && m_level->GetRandom().NextInt(0, 799) == 0 && distSq > 32.0 * 32.0))
                {
                    m_level->RemoveEntity(this);
                    return;
                }
            }
        }

        if (m_level->FindNearestPlayer(m_position, 32.0))
        {
            m_noActionTime = 0;
        }

        if (m_attackTime > 0)
        {
            m_attackTime--;
        }

        ServerAiStep();

        double oldY = m_position.y;
        if (m_vehicle)
        {
            m_velocity = glm::dvec3();
            m_vehicle->PositionRider(this);
        }
        else
        {
            SetMoveInput(m_strafeInput * 0.98, m_forwardInput * 0.98);
            Entity::Tick();
            PushEntities();
        }

        if (m_vehicle || IsInWater() || IsOnLadder())
        {
            m_fallDistance = 0.0f;
        }
        else if (!m_onGround)
        {
            if (m_position.y < oldY)
            {
                m_fallDistance += (float) (oldY - m_position.y);
            }
        }
        else
        {
            float fallDistance = m_fallDistance;
            m_fallDistance     = 0.0f;
            CauseFallDamage(fallDistance);
        }

        m_bodyControl.Tick();
        if (m_level->GetRandom().NextInt(0, 999) < m_ambientSoundTime++)
        {
            m_ambientSoundTime = -GetAmbientSoundInterval();
            PlayAmbientSound();
        }
    }
    else
    {
        m_deathTime++;
        if (m_deathTime == 20)
        {
            if (m_lastHurtByPlayerTime > 0 && CanDropLoot())
            {
                ExperienceOrb::SpawnOrbs(m_level, m_position, GetExperienceReward());
            }

            m_level->RemoveEntity(this);
            return;
        }
    }

    if (m_lastHurtByMobTime > 0)
    {
        m_lastHurtByMobTime--;
    }

    if (m_lastHurtByPlayerTime > 0)
    {
        m_lastHurtByPlayerTime--;
    }

    MobData data;
    GetMobData(&data);
    if (data.health != m_sentData.health || data.flags != m_sentData.flags || data.baby != m_sentData.baby || data.inLove != m_sentData.inLove ||
        data.variant != m_sentData.variant || data.leashHolderId != m_sentData.leashHolderId)
    {
        m_sentData = data;
        m_level->OnMobDataChanged(this);
    }
}

void Mob::ServerAiStep()
{
    m_targetSelector.Tick();
    m_goalSelector.Tick();
    TickLeash();
    // A mob rider steers its vehicle, so the vehicle's own path would fight the target the rider hands it
    if (m_passengers.empty() || !dynamic_cast<Mob *>(m_passengers[0]))
    {
        m_navigation.Tick();
    }

    Mob *vehicle = dynamic_cast<Mob *>(m_vehicle);
    if (vehicle && m_moveControl.HasWanted())
    {
        glm::dvec3 wanted = m_moveControl.GetWantedPosition();
        vehicle->m_moveControl.SetWantedPosition(wanted.x, wanted.y, wanted.z, m_moveControl.GetSpeed());
    }

    m_moveControl.Tick();
    m_lookControl.Tick();
    m_jumpControl.Tick();
}

void Mob::ClientTick()
{
    if (!m_hasNetworkState)
    {
        ApplyNetworkState(m_position, m_yaw, m_pitch);
    }

    m_totalTickCount++;
    TickHurt();
    TickCrit();
    TickDeath();
    TickRemote();
    if (m_vehicle)
    {
        m_vehicle->PositionRider(this);
    }

    float deltaX          = (float) (m_position.x - m_oldPosition.x);
    float deltaZ          = (float) (m_position.z - m_oldPosition.z);
    float horizontalSpeed = sqrtf(deltaX * deltaX + deltaZ * deltaZ);
    UpdateLimbSwing(horizontalSpeed);
    m_walkDist += horizontalSpeed * 0.6f;
    UpdateStepSound(m_position - m_oldPosition, m_grounded);
    UpdateBodyYaw(deltaX, deltaZ);
    UpdateSwing();
}

bool Mob::IsPickable() const
{
    return m_health > 0 && Entity::IsPickable();
}

double Mob::GetEyeHeight() const
{
    return m_height * 0.85f;
}

bool Mob::Hurt(eDAMAGE_SOURCE source, int damage, LivingEntity *attacker)
{
    if (m_level->IsClientSide())
    {
        return false;
    }

    m_noActionTime = 0;
    if (attacker)
    {
        m_lastHurtByMobId        = attacker->GetId();
        m_lastHurtByMobTime      = 100;
        m_lastHurtByMobTimestamp = m_level->GetGameTime();
        if (dynamic_cast<Player *>(attacker))
        {
            m_lastHurtByPlayerTime = 100;
        }
    }

    return LivingEntity::Hurt(source, damage, attacker);
}

void Mob::Die(eDAMAGE_SOURCE source, LivingEntity *attacker)
{
    LivingEntity::Die(source, attacker);
    m_deathTime = 0;
    if (m_leashHolderId >= 0 || m_leashTag)
    {
        DropLeash(true);
    }

    if (CanDropLoot())
    {
        DropDeathLoot();
    }
}

void Mob::Load(const CompoundTag *tag)
{
    Entity::Load(tag);
    if (tag->Contains("Health"))
    {
        SetHealth(tag->GetShort("Health"));
    }

    if (tag->Contains("Fire"))
    {
        m_fireTicks = tag->GetShort("Fire");
    }

    if (tag->Contains("Air"))
    {
        m_airSupply = tag->GetShort("Air");
    }

    if (tag->Contains("FallDistance"))
    {
        m_fallDistance = tag->GetFloat("FallDistance");
    }

    if (tag->Contains("HurtTime"))
    {
        m_hurtTime = tag->GetShort("HurtTime");
    }

    if (tag->Contains("DeathTime"))
    {
        m_deathTime = tag->GetShort("DeathTime");
    }

    if (tag->Contains("Leash"))
    {
        m_leashTag.reset((CompoundTag *) tag->GetCompound("Leash")->Clone().release());
    }
}

void Mob::Save(CompoundTag *tag) const
{
    Entity::Save(tag);
    tag->PutShort("Health", (int16_t) m_health);
    tag->PutShort("Fire", (int16_t) m_fireTicks);
    tag->PutShort("Air", (int16_t) m_airSupply);
    tag->PutFloat("FallDistance", m_fallDistance);
    tag->PutShort("HurtTime", (int16_t) m_hurtTime);
    tag->PutShort("DeathTime", (int16_t) m_deathTime);
    if (m_leashTag)
    {
        tag->Put("Leash", m_leashTag->Clone());
        return;
    }

    Entity *holder = GetLeashHolder();
    if (!holder)
    {
        return;
    }

    std::unique_ptr<CompoundTag> leash(new CompoundTag());
    if (holder->GetType() == eEntityType_LeashKnot)
    {
        BlockPos pos = ((LeashKnot *) holder)->GetFencePos();
        leash->PutInt("X", pos.x);
        leash->PutInt("Y", pos.y);
        leash->PutInt("Z", pos.z);
    }
    else
    {
        const uint32_t *uniqueId = holder->GetUniqueID().GetData();
        leash->PutIntArray("UUID", {(int) uniqueId[0], (int) uniqueId[1], (int) uniqueId[2], (int) uniqueId[3], (int) uniqueId[4]});
    }

    tag->Put("Leash", std::move(leash));
}

bool Mob::Interact(Player *player)
{
    (void) player;
    return false;
}

bool Mob::InteractLeash(Player *player)
{
    if (m_leashHolderId == player->GetId())
    {
        if (!m_level->IsClientSide())
        {
            DropLeash(!GameType::HasInfiniteItems(player->GetGameType()));
        }

        return true;
    }

    ItemInstance *heldItem = player->GetInventory()->GetSelectedItem();
    if (heldItem->IsEmpty() || heldItem->GetItem() != Item::ByName("lead") || !CanBeLeashed())
    {
        return false;
    }

    if (!m_level->IsClientSide())
    {
        SetLeashHolder(player);
        if (!GameType::HasInfiniteItems(player->GetGameType()))
        {
            heldItem->ConsumeOne();
        }
    }

    return true;
}

bool Mob::CanBeLeashed() const
{
    return m_leashHolderId < 0 && !m_leashTag;
}

int Mob::GetLeashHolderId() const
{
    return m_leashHolderId;
}

Entity *Mob::GetLeashHolder() const
{
    return m_leashHolderId < 0 ? nullptr : m_level->GetEntityById(m_leashHolderId);
}

void Mob::SetLeashHolder(Entity *holder)
{
    m_leashHolderId = holder->GetId();
    m_leashTag.reset();
}

void Mob::DropLeash(bool dropItem)
{
    m_leashHolderId = -1;
    m_leashTag.reset();
    if (dropItem)
    {
        SpawnAtLocation(Item::ByName("lead"), 1);
    }
}

void Mob::TickLeash()
{
    if (m_leashTag)
    {
        RestoreLeash();
    }

    if (m_leashHolderId < 0)
    {
        return;
    }

    Entity *holder       = GetLeashHolder();
    LivingEntity *living = dynamic_cast<LivingEntity *>(holder);
    if (!holder || (living && living->GetHealth() <= 0))
    {
        DropLeash(true);
        return;
    }

    glm::dvec3 offset = holder->GetPosition() - m_position;
    double distance   = glm::length(offset);
    if (distance > 10.0)
    {
        DropLeash(true);
        return;
    }

    if (distance > 4.0)
    {
        m_navigation.MoveTo(holder, 1.0);
    }

    if (distance > 6.0)
    {
        glm::dvec3 direction = offset / distance;
        m_velocity += direction * glm::abs(direction) * 0.4;
    }
}

void Mob::RestoreLeash()
{
    if (m_leashTag->Contains("UUID"))
    {
        std::vector<int> data = m_leashTag->GetIntArray("UUID");
        Player *player        = m_level->FindNearestPlayer(m_position, 10.0);
        if (player && data.size() == 5 &&
            player->GetUniqueID() == UniqueID((uint32_t) data[0], (uint32_t) data[1], (uint32_t) data[2], (uint32_t) data[3], (uint32_t) data[4]))
        {
            SetLeashHolder(player);
        }
        else if (++m_leashRestoreTime > 200)
        {
            DropLeash(true);
        }

        return;
    }

    BlockPos pos(m_leashTag->GetInt("X"), m_leashTag->GetInt("Y"), m_leashTag->GetInt("Z"));
    Block *block = Block::ById(m_level->GetBlockId(pos));
    if (dynamic_cast<FenceBlock *>(block) && !dynamic_cast<WallBlock *>(block))
    {
        SetLeashHolder(LeashKnot::GetOrCreate(m_level, pos));
    }
    else
    {
        DropLeash(true);
    }
}

void Mob::FinalizeSpawn()
{}

bool Mob::CanAddPassenger(Entity *passenger) const
{
    (void) passenger;
    return m_passengers.empty();
}

void Mob::PositionRider(Entity *passenger)
{
    passenger->SetPosition(m_position + glm::dvec3(0.0, GetRideHeight(), 0.0));
}

bool Mob::IsBaby() const
{
    return false;
}

float Mob::GetWalkTargetValue(const BlockPos &pos) const
{
    (void) pos;
    return 0.0f;
}

void Mob::GetMobData(MobData *data) const
{
    data->health        = m_health;
    data->flags         = m_sharedFlags;
    data->baby          = false;
    data->inLove        = false;
    data->variant       = 0;
    data->leashHolderId = m_leashHolderId;
}

void Mob::ApplyMobData(const MobData &data)
{
    SetHealth(data.health);
    SetSharedFlags(data.flags);
    m_leashHolderId = data.leashHolderId;
}

bool Mob::DoHurtTarget(LivingEntity *target)
{
    if (!target->Hurt(eDamageSource_Mob, (int) GetAttributeValue(Attribute::s_attackDamage), this))
    {
        return false;
    }

    target->Knockback(m_position, glm::dvec3());
    return true;
}

GoalSelector *Mob::GetGoalSelector()
{
    return &m_goalSelector;
}

LivingEntity *Mob::GetTarget() const
{
    return dynamic_cast<LivingEntity *>(m_level->GetEntityById(m_targetId));
}

void Mob::SetTarget(LivingEntity *target)
{
    m_targetId = target ? target->GetId() : -1;
}

bool Mob::CanAttack(LivingEntity *target) const
{
    if (target == this || target->GetHealth() <= 0)
    {
        return false;
    }

    Player *player = dynamic_cast<Player *>(target);
    return !player || GameType::CanHurtPlayer(player->GetGameType());
}

bool Mob::CanSee(LivingEntity *entity) const
{
    glm::dvec3 eye(m_position.x, m_position.y + GetEyeHeight(), m_position.z);
    glm::dvec3 delta = glm::dvec3(entity->GetPosition().x, entity->GetPosition().y + entity->GetEyeHeight(), entity->GetPosition().z) - eye;
    float distance   = (float) glm::length(delta);
    std::unique_ptr<HitResult> hit(m_level->Clip(eye, delta, distance, false));
    return hit->IsMiss() || hit->GetDistance() >= distance;
}

float Mob::GetBrightness() const
{
    return m_level->GetBrightness(BlockPos((int) floor(m_position.x), (int) floor(m_position.y + m_height * 0.66), (int) floor(m_position.z)));
}

float Mob::GetWidth() const
{
    return m_width;
}

int Mob::GetAttackTime() const
{
    return m_attackTime;
}

void Mob::SetAttackTime(int attackTime)
{
    m_attackTime = attackTime;
}

MoveControl *Mob::GetMoveControl()
{
    return &m_moveControl;
}

JumpControl *Mob::GetJumpControl()
{
    return &m_jumpControl;
}

LookControl *Mob::GetLookControl()
{
    return &m_lookControl;
}

PathNavigation *Mob::GetNavigation()
{
    return &m_navigation;
}

Random &Mob::GetRandom() const
{
    return m_level->GetRandom();
}

int Mob::GetNoActionTime() const
{
    return m_noActionTime;
}

LivingEntity *Mob::GetLastHurtByMob() const
{
    if (m_lastHurtByMobTime <= 0)
    {
        return nullptr;
    }

    return dynamic_cast<LivingEntity *>(m_level->GetEntityById(m_lastHurtByMobId));
}

int64_t Mob::GetLastHurtByMobTimestamp() const
{
    return m_lastHurtByMobTimestamp;
}

bool Mob::GetRandomPos(int horizontal, int vertical, glm::dvec3 *pos)
{
    float bestValue = -99999.0f;
    bool found      = false;
    BlockPos origin((int) floor(m_position.x), (int) floor(m_position.y), (int) floor(m_position.z));
    for (int i = 0; i < 10; i++)
    {
        BlockPos target(origin.x + m_level->GetRandom().NextInt(0, horizontal * 2 - 1) - horizontal,
                        origin.y + m_level->GetRandom().NextInt(0, vertical * 2 - 1) - vertical,
                        origin.z + m_level->GetRandom().NextInt(0, horizontal * 2 - 1) - horizontal);
        float value = GetWalkTargetValue(target);
        if (value > bestValue)
        {
            bestValue = value;
            *pos      = glm::dvec3(target.x, target.y, target.z);
            found     = true;
        }
    }

    return found;
}

void Mob::SetForwardInput(double forward)
{
    m_forwardInput = forward;
}

void Mob::SetSpeed(double speed)
{
    m_speed        = speed * GetAttributeValue(Attribute::s_movementSpeed);
    m_forwardInput = m_speed;
}

double Mob::GetMovementSpeed() const
{
    return m_speed;
}

float Mob::GetBodyYaw() const
{
    return m_bodyYaw;
}

void Mob::SetBodyYaw(float yaw)
{
    m_bodyYaw = yaw;
}

void Mob::SetHeadYaw(float yaw)
{
    m_headYaw = yaw;
}

void Mob::SetSize(float width, float height)
{
    m_width       = width;
    m_height      = height;
    m_boundingBox = AABB(glm::dvec3(-width / 2.0f, 0.0, -width / 2.0f), glm::dvec3(width / 2.0f, height, width / 2.0f));
    UpdateAABB();
}

void Mob::PlayAmbientSound()
{
    std::string sound = GetAmbientSound();
    if (!sound.empty())
    {
        m_level->PlaySound(sound, m_position, GetSoundVolume(), GetVoicePitch());
    }
}

std::string Mob::GetAmbientSound() const
{
    return "";
}

int Mob::GetAmbientSoundInterval() const
{
    return 80;
}

void Mob::DropDeathLoot()
{
    const LootTable *table = LootTables::ForEntity(EntityFactory::GetResourceLocation(GetType()));
    if (!table)
    {
        return;
    }

    LootContext context = {&m_level->GetRandom(), {}, nullptr, m_lastHurtByPlayerTime > 0, GetSharedFlag(eEntityFlag_OnFire)};
    GetLootProperties(&context.properties);
    std::vector<ItemInstance> items;
    table->GetRandomItems(context, &items);
    for (size_t i = 0; i < items.size(); i++)
    {
        SpawnAtLocation(items[i].GetItem(), items[i].GetCount());
    }
}

void Mob::GetLootProperties(std::vector<LootProperty> *properties) const
{
    (void) properties;
}

int Mob::GetExperienceReward()
{
    return 0;
}

bool Mob::RemoveWhenFarAway() const
{
    return false;
}

bool Mob::CanDropLoot() const
{
    return !IsBaby();
}

double Mob::GetRideHeight() const
{
    return m_height * 0.75;
}

float Mob::GetVoicePitch() const
{
    float pitch = LivingEntity::GetVoicePitch();
    return IsBaby() ? pitch + 0.5f : pitch;
}

void Mob::SpawnAtLocation(Item *item, int count)
{
    std::unique_ptr<ItemEntity> itemEntity(new ItemEntity(m_level));
    itemEntity->Sync(m_position, glm::dvec3(m_level->GetRandom().NextDouble() * 0.2 - 0.1, 0.2, m_level->GetRandom().NextDouble() * 0.2 - 0.1),
                     ItemInstance(item, count), 0, 10);
    m_level->AddEntity(std::move(itemEntity));
}
