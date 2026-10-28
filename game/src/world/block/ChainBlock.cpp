#include "ChainBlock.h"

#include <cmath>

ChainBlock::ChainBlock() : Block(false, "assets/texture/block/chain.png")
{
    SetBlocksLiquid(true);
    m_chainAxisProperty = AddProperty("axis", 3);
    SetTransparent(true);
    SetCollidable(true);
    SetRenderShape(eRenderShape_BOXES);
    SetFlatItem(true);
    SetBreakSound("dig.stone");
}

int ChainBlock::GetAxis(uint16_t data) const
{
    return GetPropertyValue(data, m_chainAxisProperty);
}

void ChainBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    double low  = 6.5 / 16.0;
    double high = 9.5 / 16.0;
    int axis    = GetAxis(data);
    if (axis == 1)
    {
        out->push_back(AABB(glm::dvec3(0.0, low, low), glm::dvec3(1.0, high, high)));
    }
    else if (axis == 2)
    {
        out->push_back(AABB(glm::dvec3(low, low, 0.0), glm::dvec3(high, high, 1.0)));
    }
    else
    {
        out->push_back(AABB(glm::dvec3(low, 0.0, low), glm::dvec3(high, 1.0, high)));
    }
}

void ChainBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    (void) out;
}

void ChainBlock::GetRenderPlanes(uint16_t data, std::vector<RenderPlane> *out) const
{
    double width = 1.5 / 16.0 / std::sqrt(2.0);
    int axis     = GetAxis(data);
    glm::dvec3 start(0.5, 0.0, 0.5);
    glm::dvec3 end(0.5, 1.0, 0.5);
    glm::dvec3 diagonals[2] = {glm::dvec3(width, 0.0, -width), glm::dvec3(width, 0.0, width)};
    if (axis == 1)
    {
        start        = glm::dvec3(0.0, 0.5, 0.5);
        end          = glm::dvec3(1.0, 0.5, 0.5);
        diagonals[0] = glm::dvec3(0.0, width, -width);
        diagonals[1] = glm::dvec3(0.0, width, width);
    }
    else if (axis == 2)
    {
        start        = glm::dvec3(0.5, 0.5, 0.0);
        end          = glm::dvec3(0.5, 0.5, 1.0);
        diagonals[0] = glm::dvec3(width, -width, 0.0);
        diagonals[1] = glm::dvec3(width, width, 0.0);
    }

    out->push_back(RenderPlane{start, end, diagonals[0], 0.0f, 0.0f, 3.0f / 16.0f, 1.0f});
    out->push_back(RenderPlane{start, end, diagonals[1], 3.0f / 16.0f, 0.0f, 6.0f / 16.0f, 1.0f});
}

bool ChainBlock::HasAmbientOcclusion() const
{
    return false;
}

uint16_t ChainBlock::GetPlacementData(Direction *placedAgainst) const
{
    int axis = 0;
    if (placedAgainst->dx != 0)
    {
        axis = 1;
    }
    else if (placedAgainst->dz != 0)
    {
        axis = 2;
    }

    return WithPropertyValue(0, m_chainAxisProperty, axis);
}
