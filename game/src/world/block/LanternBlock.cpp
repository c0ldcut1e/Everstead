#include "LanternBlock.h"

#include <cmath>

#include "../Level.h"
#include "ChainBlock.h"

LanternBlock::LanternBlock(const std::string &name, uint8_t r, uint8_t g, uint8_t b) : Block(false, "assets/texture/block/" + name + ".png")
{
    SetBlocksLiquid(true);
    m_hangingProperty = AddProperty("hanging", 2);
    SetTransparent(true);
    SetCollidable(true);
    SetRenderShape(eRenderShape_BOXES);
    SetFlatItem(true);
    SetLightEmission(15);
    SetLightColor(r, g, b);
    SetBreakSound("dig.stone");
}

bool LanternBlock::IsHanging(uint16_t data) const
{
    return GetPropertyValue(data, m_hangingProperty) != 0;
}

void LanternBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    double lift = IsHanging(data) ? 1.0 / 16.0 : 0.0;
    out->push_back(AABB(glm::dvec3(5.0 / 16.0, lift, 5.0 / 16.0), glm::dvec3(11.0 / 16.0, 7.0 / 16.0 + lift, 11.0 / 16.0)));
    out->push_back(AABB(glm::dvec3(6.0 / 16.0, 7.0 / 16.0 + lift, 6.0 / 16.0), glm::dvec3(10.0 / 16.0, 9.0 / 16.0 + lift, 10.0 / 16.0)));
}

void LanternBlock::GetRenderPlanes(uint16_t data, std::vector<RenderPlane> *out) const
{
    double width = 1.5 / 16.0 / std::sqrt(2.0);
    if (IsHanging(data))
    {
        out->push_back(RenderPlane{glm::dvec3(0.5, 11.0 / 16.0, 0.5), glm::dvec3(0.5, 15.0 / 16.0, 0.5), glm::dvec3(width, 0.0, -width),
                                   11.0f / 16.0f, 1.0f / 16.0f, 14.0f / 16.0f, 5.0f / 16.0f});
        out->push_back(RenderPlane{glm::dvec3(0.5, 10.0 / 16.0, 0.5), glm::dvec3(0.5, 1.0, 0.5), glm::dvec3(width, 0.0, width), 11.0f / 16.0f,
                                   6.0f / 16.0f, 14.0f / 16.0f, 12.0f / 16.0f});
    }
    else
    {
        out->push_back(RenderPlane{glm::dvec3(0.5, 9.0 / 16.0, 0.5), glm::dvec3(0.5, 11.0 / 16.0, 0.5), glm::dvec3(width, 0.0, -width), 11.0f / 16.0f,
                                   1.0f / 16.0f, 14.0f / 16.0f, 3.0f / 16.0f});
        out->push_back(RenderPlane{glm::dvec3(0.5, 9.0 / 16.0, 0.5), glm::dvec3(0.5, 11.0 / 16.0, 0.5), glm::dvec3(width, 0.0, width), 11.0f / 16.0f,
                                   10.0f / 16.0f, 14.0f / 16.0f, 12.0f / 16.0f});
    }
}

bool LanternBlock::HasAmbientOcclusion() const
{
    return false;
}

void LanternBlock::AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const
{
    (void) data;

    bool body     = box.GetMax().x - box.GetMin().x > 5.0 / 16.0;
    bool vertical = direction->dy != 0;
    if (body)
    {
        *u0 = 0.0f;
        *u1 = 6.0f / 16.0f;
        *v0 = vertical ? 9.0f / 16.0f : 2.0f / 16.0f;
        *v1 = vertical ? 15.0f / 16.0f : 9.0f / 16.0f;
    }
    else
    {
        *u0 = 1.0f / 16.0f;
        *u1 = 5.0f / 16.0f;
        *v0 = vertical ? 10.0f / 16.0f : 0.0f;
        *v1 = vertical ? 14.0f / 16.0f : 2.0f / 16.0f;
    }
}

uint16_t LanternBlock::GetPlacementData(Direction *placedAgainst) const
{
    return WithPropertyValue(0, m_hangingProperty, placedAgainst == Direction::down ? 1 : 0);
}

Direction *LanternBlock::GetAttachment(uint16_t data) const
{
    return IsHanging(data) ? Direction::up : Direction::down;
}

bool LanternBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    BlockPos supportPos(pos.x + attachment->dx, pos.y + attachment->dy, pos.z + attachment->dz);
    Block *support       = Block::ById(level->GetBlockId(supportPos));
    uint16_t supportData = level->GetBlockData(supportPos);
    if (support == Block::ByName("chain"))
    {
        return ((ChainBlock *) support)->GetAxis(supportData) == 0;
    }

    return support->IsFaceSturdy(supportData, Direction::Opposite(attachment));
}
