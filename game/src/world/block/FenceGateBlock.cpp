#include "FenceGateBlock.h"

#include "../Level.h"

#include <cmath>

FenceGateBlock::FenceGateBlock(const std::string &texturePath) : OpenableBlock(true, texturePath)
{
    m_facingProperty  = AddProperty("facing", 4);
    m_openProperty    = AddProperty("open", 2);
    m_poweredProperty = AddProperty("powered", 2);
    SetTransparent(true);
    SetRenderShape(eRenderShape_BOXES);
    SetBreakSound("dig.wood");
}

// Boxes are built facing south and spanning x, then turned to the stored facing
AABB FenceGateBlock::Rotate(uint16_t data, const AABB &box) const
{
    const glm::dvec3 &min = box.GetMin();
    const glm::dvec3 &max = box.GetMax();
    double x0             = min.x;
    double x1             = max.x;
    double z0             = min.z;
    double z1             = max.z;

    int facing = GetPropertyValue(data, m_facingProperty);
    if (facing == 0)
    {
        x0 = 1.0 - max.x;
        x1 = 1.0 - min.x;
        z0 = 1.0 - max.z;
        z1 = 1.0 - min.z;
    }
    else if (facing == 1)
    {
        x0 = min.z;
        x1 = max.z;
        z0 = 1.0 - max.x;
        z1 = 1.0 - min.x;
    }
    else if (facing == 3)
    {
        x0 = 1.0 - max.z;
        x1 = 1.0 - min.z;
        z0 = min.x;
        z1 = max.x;
    }

    return AABB(glm::dvec3(x0, min.y, z0), glm::dvec3(x1, max.y, z1));
}

// Selection keeps the closed outline so an open gate can still be aimed at through its gap
void FenceGateBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    out->push_back(Rotate(data, AABB(glm::dvec3(0.0, 0.0, 0.375), glm::dvec3(1.0, 1.0, 0.625))));
}

void FenceGateBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    const double p = 1.0 / 16.0;
    out->push_back(Rotate(data, AABB(glm::dvec3(0.0, 5.0 * p, 7.0 * p), glm::dvec3(2.0 * p, 1.0, 9.0 * p))));
    out->push_back(Rotate(data, AABB(glm::dvec3(14.0 * p, 5.0 * p, 7.0 * p), glm::dvec3(1.0, 1.0, 9.0 * p))));

    if (!IsOpen(data))
    {
        out->push_back(Rotate(data, AABB(glm::dvec3(2.0 * p, 6.0 * p, 7.0 * p), glm::dvec3(14.0 * p, 9.0 * p, 9.0 * p))));
        out->push_back(Rotate(data, AABB(glm::dvec3(2.0 * p, 12.0 * p, 7.0 * p), glm::dvec3(14.0 * p, 15.0 * p, 9.0 * p))));
        out->push_back(Rotate(data, AABB(glm::dvec3(6.0 * p, 9.0 * p, 7.0 * p), glm::dvec3(10.0 * p, 12.0 * p, 9.0 * p))));
        return;
    }

    static const double s_leafX[2][2] = {{0.0, 2.0}, {14.0, 16.0}};
    for (int leaf = 0; leaf < 2; leaf++)
    {
        double minX = s_leafX[leaf][0] * p;
        double maxX = s_leafX[leaf][1] * p;
        out->push_back(Rotate(data, AABB(glm::dvec3(minX, 6.0 * p, 9.0 * p), glm::dvec3(maxX, 9.0 * p, 13.0 * p))));
        out->push_back(Rotate(data, AABB(glm::dvec3(minX, 12.0 * p, 9.0 * p), glm::dvec3(maxX, 15.0 * p, 13.0 * p))));
        out->push_back(Rotate(data, AABB(glm::dvec3(minX, 6.0 * p, 13.0 * p), glm::dvec3(maxX, 15.0 * p, 15.0 * p))));
    }
}

void FenceGateBlock::GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const
{
    if (IsOpen(data))
    {
        return;
    }

    out->push_back(Rotate(data, AABB(glm::dvec3(0.0, 0.0, 0.375), glm::dvec3(1.0, 1.5, 0.625))));
}

uint16_t FenceGateBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) face;
    (void) hitPos;

    int facing = 0;
    if (std::fabs(look.x) > std::fabs(look.z))
    {
        facing = look.x > 0.0 ? 1 : 3;
    }
    else
    {
        facing = look.z > 0.0 ? 2 : 0;
    }

    return WithPlacementPower(level, pos, WithPropertyValue(0, m_facingProperty, facing));
}
