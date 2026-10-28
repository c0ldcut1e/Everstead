#pragma once

#include <vector>

#include "AttributeInstance.h"

class AttributeMap
{
public:
    AttributeMap();

    AttributeInstance *Get(const Attribute &attribute);
    const AttributeInstance *Get(const Attribute &attribute) const;
    double GetValue(const Attribute &attribute) const;
    const std::vector<AttributeInstance> &GetInstances() const;

private:
    std::vector<AttributeInstance> m_instances;
};
