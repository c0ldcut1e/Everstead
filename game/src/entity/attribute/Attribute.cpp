#include "Attribute.h"

#include <algorithm>

const Attribute Attribute::s_movementSpeed("movement_speed", 0.1, 0.0, 1024.0);
const Attribute Attribute::s_flyingSpeed("flying_speed", 0.05, 0.0, 1024.0);
const Attribute Attribute::s_jumpStrength("jump_strength", 0.42, 0.0, 32.0);
const Attribute Attribute::s_gravity("gravity", 0.08, -1.0, 1.0);
const Attribute Attribute::s_sneakingSpeed("sneaking_speed", 0.3, 0.0, 1.0);
const Attribute Attribute::s_blockInteractionRange("block_interaction_range", 4.5, 0.0, 64.0);
const Attribute Attribute::s_armor("armor", 0.0, 0.0, 30.0);
const Attribute Attribute::s_attackDamage("attack_damage", 1.0, 0.0, 2048.0);
const Attribute Attribute::s_attackKnockback("attack_knockback", 0.0, 0.0, 5.0);
const Attribute Attribute::s_knockbackResistance("knockback_resistance", 0.0, 0.0, 1.0);
const Attribute Attribute::s_maxHealth("max_health", 20.0, 1.0, 1024.0);
const Attribute Attribute::s_stepHeight("step_height", 0.6, 0.0, 10.0);
const Attribute Attribute::s_safeFallDistance("safe_fall_distance", 3.0, -1024.0, 1024.0);
const Attribute Attribute::s_fallDamageMultiplier("fall_damage_multiplier", 1.0, 0.0, 100.0);
const Attribute Attribute::s_entityInteractionRange("entity_interaction_range", 3.0, 0.0, 64.0);
const Attribute Attribute::s_blockBreakSpeed("block_break_speed", 1.0, 0.0, 1024.0);
const Attribute Attribute::s_oxygenBonus("oxygen_bonus", 0.0, 0.0, 1024.0);

Attribute::Attribute(const ResourceLocation &location, double defaultValue, double minimum, double maximum)
{
    m_location     = location;
    m_defaultValue = defaultValue;
    m_minimum      = minimum;
    m_maximum      = maximum;
}

const std::vector<const Attribute *> &Attribute::GetAll()
{
    static const std::vector<const Attribute *> attributes = {&s_movementSpeed,
                                                              &s_flyingSpeed,
                                                              &s_jumpStrength,
                                                              &s_gravity,
                                                              &s_sneakingSpeed,
                                                              &s_blockInteractionRange,
                                                              &s_armor,
                                                              &s_attackDamage,
                                                              &s_attackKnockback,
                                                              &s_knockbackResistance,
                                                              &s_maxHealth,
                                                              &s_stepHeight,
                                                              &s_safeFallDistance,
                                                              &s_fallDamageMultiplier,
                                                              &s_entityInteractionRange,
                                                              &s_blockBreakSpeed,
                                                              &s_oxygenBonus};
    return attributes;
}

const Attribute *Attribute::ByName(const ResourceLocation &location)
{
    const std::vector<const Attribute *> &attributes = GetAll();
    for (size_t i = 0; i < attributes.size(); i++)
    {
        if (attributes[i]->m_location == location)
        {
            return attributes[i];
        }
    }

    return nullptr;
}

const ResourceLocation &Attribute::GetResourceLocation() const
{
    return m_location;
}

std::string Attribute::GetDescriptionId() const
{
    return m_location.ToLanguageKey("attribute.name");
}

size_t Attribute::GetIndex() const
{
    const std::vector<const Attribute *> &attributes = GetAll();
    return (size_t) (std::find(attributes.begin(), attributes.end(), this) - attributes.begin());
}

double Attribute::GetDefaultValue() const
{
    return m_defaultValue;
}

double Attribute::Sanitize(double value) const
{
    return std::clamp(value, m_minimum, m_maximum);
}
