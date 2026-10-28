#include "AttributeMap.h"

AttributeMap::AttributeMap()
{
    const std::vector<const Attribute *> &attributes = Attribute::GetAll();
    m_instances.reserve(attributes.size());
    for (size_t i = 0; i < attributes.size(); i++)
    {
        m_instances.push_back(AttributeInstance(attributes[i]));
    }
}

AttributeInstance *AttributeMap::Get(const Attribute &attribute)
{
    return &m_instances[attribute.GetIndex()];
}

const AttributeInstance *AttributeMap::Get(const Attribute &attribute) const
{
    return &m_instances[attribute.GetIndex()];
}

double AttributeMap::GetValue(const Attribute &attribute) const
{
    return Get(attribute)->GetValue();
}

const std::vector<AttributeInstance> &AttributeMap::GetInstances() const
{
    return m_instances;
}
