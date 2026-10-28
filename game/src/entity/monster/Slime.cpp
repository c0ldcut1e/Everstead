#include "Slime.h"

#include <cmath>
#include <numbers>

#include "../../tag/CompoundTag.h"
#include "../../world/Level.h"
#include "../../world/item/Item.h"
#include "../../world/loot/LootTable.h"
#include "../../world/particle/ParticleSpawnParams.h"
#include "../ai/control/MoveControl.h"
#include "../player/Player.h"

#include <glm/gtx/norm.hpp>

Slime::Slime(Level *level) : Monster(level)
{
    m_squish       = 0.0f;
    m_oldSquish    = 0.0f;
    m_targetSquish = 0.0f;
    m_jumpDelay    = level->GetRandom().NextInt(10, 29);
    m_size         = 0;
    m_wasOnGround  = false;
    SetSlimeSize(1 << level->GetRandom().NextInt(0, 2));
}

eENTITY_TYPE Slime::GetType()
{
    return s_type;
}

void Slime::Load(const CompoundTag *tag)
{
    Monster::Load(tag);
    SetSlimeSize(tag->GetInt("Size") + 1);
}

void Slime::Save(CompoundTag *tag) const
{
    Monster::Save(tag);
    tag->PutInt("Size", m_size - 1);
}

void Slime::GetMobData(MobData *data) const
{
    Monster::GetMobData(data);
    data->variant = (uint8_t) m_size;
}

void Slime::ApplyMobData(const MobData &data)
{
    if (data.variant != m_size)
    {
        SetSlimeSize(data.variant);
    }

    Monster::ApplyMobData(data);
}

int Slime::GetSlimeSize() const
{
    return m_size;
}

void Slime::SetSlimeSize(int size)
{
    m_size = size;
    m_attributes.Get(Attribute::s_maxHealth)->SetBaseValue(size * size);
    SetSize(0.6f * size, 0.6f * size);
    SetHealth(GetMaxHealth());
}

float Slime::GetSquish(float partialTick) const
{
    return m_oldSquish + (m_squish - m_oldSquish) * partialTick;
}

void Slime::ServerTick()
{
    Random &random = m_level->GetRandom();
    if (m_health > 0)
    {
        if (m_onGround && !m_wasOnGround)
        {
            const Icon &icon = Item::ByName("slime_ball")->GetIcon();
            for (int i = 0; i < m_size * 8; i++)
            {
                double angle = random.NextFloat() * std::numbers::pi * 2.0;
                double dist  = m_size * 0.5 * (random.NextFloat() * 0.5 + 0.5);
                ParticleSpawnParams params;
                params.type      = eParticleType_ItemCrack;
                params.position  = glm::dvec3(m_position.x + sin(angle) * dist, m_aabb.GetMin().y, m_position.z + cos(angle) * dist);
                params.region    = {icon.GetU0(), icon.GetV0(), icon.GetU1(), icon.GetV1()};
                params.hasRegion = true;
                m_level->SpawnParticle(params);
            }

            if (m_size > 2)
            {
                m_level->PlaySound(GetHurtSound(), m_position, GetSoundVolume(), ((random.NextFloat() - random.NextFloat()) * 0.2f + 1.0f) / 0.8f);
            }
        }

        Player *player = m_level->FindNearestPlayer(m_position, 0.6 * m_size);
        if (m_size > 1 && player && CanSee(player) && player->Hurt(eDamageSource_Mob, m_size, this))
        {
            m_level->PlaySound("mob.slime.attack" + std::to_string(random.NextInt(1, 2)), m_position, 1.0f,
                               (random.NextFloat() - random.NextFloat()) * 0.2f + 1.0f);
        }
    }
    else if (m_deathTime == 19)
    {
        Split();
    }

    m_wasOnGround = m_onGround;
    Monster::ServerTick();
}

void Slime::ServerAiStep()
{
    Random &random = m_level->GetRandom();
    Player *player = m_level->FindNearestPlayer(m_position, 16.0);
    if (player)
    {
        glm::dvec3 delta = player->GetPosition() - m_position;
        SetRotation(MoveControl::RotLerp(m_yaw, (float) glm::degrees(atan2(delta.z, delta.x)), 10.0f), 0.0f);
    }

    m_speed = GetAttributeValue(Attribute::s_movementSpeed);
    if (m_onGround && m_jumpDelay-- <= 0)
    {
        m_jumpDelay = random.NextInt(10, 29);
        if (player)
        {
            m_jumpDelay /= 3;
        }

        SetJumping(true);
        if (m_size > 1)
        {
            m_level->PlaySound(GetHurtSound(), m_position, GetSoundVolume(), ((random.NextFloat() - random.NextFloat()) * 0.2f + 1.0f) * 0.8f);
        }

        m_strafeInput  = 1.0 - random.NextFloat() * 2.0;
        m_forwardInput = m_size;
    }
    else
    {
        SetJumping(false);
        if (m_onGround)
        {
            m_strafeInput  = 0.0;
            m_forwardInput = 0.0;
        }
    }
}

void Slime::ClientTick()
{
    m_squish += (m_targetSquish - m_squish) * 0.5f;
    m_oldSquish      = m_squish;
    bool wasOnGround = m_grounded;
    Monster::ClientTick();
    if (m_grounded && !wasOnGround)
    {
        m_targetSquish = -0.5f;
    }
    else if (!m_grounded && wasOnGround)
    {
        m_targetSquish = 1.0f;
    }

    m_targetSquish *= 0.6f;
}

std::string Slime::GetHurtSound() const
{
    return "mob.slime.say" + std::to_string(GetRandom().NextInt(1, 5));
}

std::string Slime::GetDeathSound() const
{
    return GetHurtSound();
}

float Slime::GetSoundVolume() const
{
    return 0.4f * m_size;
}

void Slime::GetLootProperties(std::vector<LootProperty> *properties) const
{
    properties->push_back({"size", m_size});
}

int Slime::GetExperienceReward()
{
    return m_size;
}

void Slime::Split()
{
    if (m_size <= 1)
    {
        return;
    }

    Random &random = m_level->GetRandom();
    int count      = random.NextInt(2, 4);
    for (int i = 0; i < count; i++)
    {
        std::unique_ptr<Slime> child = std::make_unique<Slime>(m_level);
        child->SetSlimeSize(m_size / 2);
        child->SetPosition(m_position + glm::dvec3((i % 2 - 0.5) * m_size / 4.0, 0.5, (i / 2 - 0.5) * m_size / 4.0));
        child->SetRotation(random.NextFloat() * 360.0f, 0.0f);
        m_level->AddEntity(std::move(child));
    }
}
