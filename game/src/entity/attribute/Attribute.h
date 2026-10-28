#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../../core/ResourceLocation.h"

class Attribute
{
public:
    Attribute(const ResourceLocation &location, double defaultValue, double minimum, double maximum);

    static const Attribute s_movementSpeed;
    static const Attribute s_flyingSpeed;
    static const Attribute s_jumpStrength;
    static const Attribute s_gravity;
    static const Attribute s_sneakingSpeed;
    static const Attribute s_blockInteractionRange;
    static const Attribute s_armor;
    static const Attribute s_attackDamage;
    static const Attribute s_attackKnockback;
    static const Attribute s_knockbackResistance;
    static const Attribute s_maxHealth;
    static const Attribute s_stepHeight;
    static const Attribute s_safeFallDistance;
    static const Attribute s_fallDamageMultiplier;
    static const Attribute s_entityInteractionRange;
    static const Attribute s_blockBreakSpeed;
    static const Attribute s_oxygenBonus;

    static const std::vector<const Attribute *> &GetAll();
    static const Attribute *ByName(const ResourceLocation &location);

    const ResourceLocation &GetResourceLocation() const;
    std::string GetDescriptionId() const;
    size_t GetIndex() const;
    double GetDefaultValue() const;
    double Sanitize(double value) const;

private:
    ResourceLocation m_location;
    double m_defaultValue;
    double m_minimum;
    double m_maximum;
};
