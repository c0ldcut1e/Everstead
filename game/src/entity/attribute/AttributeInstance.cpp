#include "AttributeInstance.h"

static const char *s_operationNames[] = {"add_value", "add_multiplied_base", "add_multiplied_total"};

AttributeInstance::AttributeInstance(const Attribute *attribute)
{
    m_attribute = attribute;
    m_baseValue = attribute->GetDefaultValue();
    m_modifiers = {};
}

const char *AttributeInstance::GetOperationName(eATTRIBUTE_OPERATION operation)
{
    return s_operationNames[operation];
}

bool AttributeInstance::ParseOperation(const std::string &name, eATTRIBUTE_OPERATION *operation)
{
    for (uint8_t i = 0; i < 3; i++)
    {
        if (name == s_operationNames[i])
        {
            *operation = (eATTRIBUTE_OPERATION) i;
            return true;
        }
    }

    return false;
}

const Attribute *AttributeInstance::GetAttribute() const
{
    return m_attribute;
}

double AttributeInstance::GetBaseValue() const
{
    return m_baseValue;
}

void AttributeInstance::SetBaseValue(double value)
{
    m_baseValue = value;
}

double AttributeInstance::GetValue() const
{
    double value = m_baseValue;
    for (size_t i = 0; i < m_modifiers.size(); i++)
    {
        if (m_modifiers[i].operation == eAttributeOperation_AddValue)
        {
            value += m_modifiers[i].amount;
        }
    }

    double result = value;
    for (size_t i = 0; i < m_modifiers.size(); i++)
    {
        if (m_modifiers[i].operation == eAttributeOperation_AddMultipliedBase)
        {
            result += value * m_modifiers[i].amount;
        }
    }

    for (size_t i = 0; i < m_modifiers.size(); i++)
    {
        if (m_modifiers[i].operation == eAttributeOperation_AddMultipliedTotal)
        {
            result *= 1.0 + m_modifiers[i].amount;
        }
    }

    return m_attribute->Sanitize(result);
}

const std::vector<AttributeModifier> &AttributeInstance::GetModifiers() const
{
    return m_modifiers;
}

const AttributeModifier *AttributeInstance::FindModifier(const std::string &id) const
{
    for (size_t i = 0; i < m_modifiers.size(); i++)
    {
        if (m_modifiers[i].id == id)
        {
            return &m_modifiers[i];
        }
    }

    return nullptr;
}

bool AttributeInstance::AddModifier(const AttributeModifier &modifier)
{
    if (FindModifier(modifier.id))
    {
        return false;
    }

    m_modifiers.push_back(modifier);
    return true;
}

bool AttributeInstance::RemoveModifier(const std::string &id)
{
    for (size_t i = 0; i < m_modifiers.size(); i++)
    {
        if (m_modifiers[i].id == id)
        {
            m_modifiers.erase(m_modifiers.begin() + (long) i);
            return true;
        }
    }

    return false;
}

void AttributeInstance::SetModifiers(const std::vector<AttributeModifier> &modifiers)
{
    m_modifiers = modifiers;
}
