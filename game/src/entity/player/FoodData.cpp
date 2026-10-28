#include "FoodData.h"

#include <algorithm>
#include <cmath>

#include "../../tag/CompoundTag.h"
#include "../../world/Level.h"
#include "ServerPlayer.h"

FoodData::FoodData()
{
    m_foodLevel       = kMaxFoodLevel;
    m_saturationLevel = 5.0f;
    m_exhaustionLevel = 0.0f;
    m_tickTimer       = 0;
    m_healProgress    = 0.0f;
}

void FoodData::Eat(int nutrition, float saturationModifier)
{
    m_foodLevel       = std::min(nutrition + m_foodLevel, kMaxFoodLevel);
    m_saturationLevel = std::min(m_saturationLevel + (float) nutrition * saturationModifier * 2.0f, (float) m_foodLevel);
}

void FoodData::Tick(ServerPlayer *player)
{
    int difficulty = player->GetLevel()->GetDifficulty();
    if (m_exhaustionLevel > 4.0f)
    {
        m_exhaustionLevel -= 4.0f;
        if (m_saturationLevel > 0.0f)
        {
            m_saturationLevel = std::max(m_saturationLevel - 1.0f, 0.0f);
        }
        else if (difficulty > 0)
        {
            m_foodLevel = std::max(m_foodLevel - 1, 0);
        }
    }

    int health               = player->GetHealth();
    bool naturalRegeneration = player->GetLevel()->GetGameRules().GetBool(eGameRule_NaturalRegeneration);
    if (naturalRegeneration && m_saturationLevel > 0.0f && m_foodLevel >= kMaxFoodLevel && health < player->GetMaxHealth())
    {
        m_tickTimer++;
        if (m_tickTimer >= 10)
        {
            float saturation = std::min(m_saturationLevel, 6.0f);
            // Health is whole points, so we carry over the part of a point that low saturation heals
            m_healProgress += saturation / 6.0f;
            if (m_healProgress >= 1.0f)
            {
                player->Heal((int) m_healProgress);
                m_healProgress -= floorf(m_healProgress);
            }

            AddExhaustion(saturation);
            m_tickTimer = 0;
        }
    }
    else if (naturalRegeneration && m_foodLevel >= 18 && health < player->GetMaxHealth())
    {
        m_tickTimer++;
        if (m_tickTimer >= 80)
        {
            player->Heal(1);
            AddExhaustion(6.0f);
            m_tickTimer = 0;
        }
    }
    else if (m_foodLevel <= 0)
    {
        m_tickTimer++;
        if (m_tickTimer >= 80)
        {
            if (health > 10 || difficulty >= 3 || (health > 1 && difficulty >= 2))
            {
                player->Hurt(eDamageSource_Starve, 1, nullptr);
            }

            m_tickTimer = 0;
        }
    }
    else
    {
        m_tickTimer = 0;
    }
}

void FoodData::AddExhaustion(float amount)
{
    m_exhaustionLevel = std::min(m_exhaustionLevel + amount, 40.0f);
}

bool FoodData::NeedsFood() const
{
    return m_foodLevel < kMaxFoodLevel;
}

void FoodData::Read(const CompoundTag *tag)
{
    m_foodLevel       = tag->GetInt("foodLevel");
    m_tickTimer       = tag->GetInt("foodTickTimer");
    m_saturationLevel = tag->GetFloat("foodSaturationLevel");
    m_exhaustionLevel = tag->GetFloat("foodExhaustionLevel");
}

void FoodData::Write(CompoundTag *tag) const
{
    tag->PutInt("foodLevel", m_foodLevel);
    tag->PutInt("foodTickTimer", m_tickTimer);
    tag->PutFloat("foodSaturationLevel", m_saturationLevel);
    tag->PutFloat("foodExhaustionLevel", m_exhaustionLevel);
}

int FoodData::GetFoodLevel() const
{
    return m_foodLevel;
}

void FoodData::SetFoodLevel(int foodLevel)
{
    m_foodLevel = foodLevel;
}

float FoodData::GetSaturationLevel() const
{
    return m_saturationLevel;
}

void FoodData::SetSaturationLevel(float saturationLevel)
{
    m_saturationLevel = saturationLevel;
}
