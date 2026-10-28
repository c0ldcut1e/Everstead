#pragma once

#include "../../utils/AABB.h"

class Culler
{
public:
    virtual ~Culler() = default;

    virtual bool TestAABB(const AABB &aabb) const = 0;
};
