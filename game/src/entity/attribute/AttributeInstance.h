#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Attribute.h"

enum eATTRIBUTE_OPERATION : uint8_t
{
    eAttributeOperation_AddValue           = 0,
    eAttributeOperation_AddMultipliedBase  = 1,
    eAttributeOperation_AddMultipliedTotal = 2,
};

struct AttributeModifier
{
    std::string id;
    double amount;
    eATTRIBUTE_OPERATION operation;
};

class AttributeInstance
{
public:
    explicit AttributeInstance(const Attribute *attribute);

    static const char *GetOperationName(eATTRIBUTE_OPERATION operation);
    static bool ParseOperation(const std::string &name, eATTRIBUTE_OPERATION *operation);

    const Attribute *GetAttribute() const;
    double GetBaseValue() const;
    void SetBaseValue(double value);
    double GetValue() const;

    const std::vector<AttributeModifier> &GetModifiers() const;
    const AttributeModifier *FindModifier(const std::string &id) const;
    bool AddModifier(const AttributeModifier &modifier);
    bool RemoveModifier(const std::string &id);
    void SetModifiers(const std::vector<AttributeModifier> &modifiers);

private:
    const Attribute *m_attribute;
    double m_baseValue;
    std::vector<AttributeModifier> m_modifiers;
};
