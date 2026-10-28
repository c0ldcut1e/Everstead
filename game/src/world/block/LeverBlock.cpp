#include "LeverBlock.h"

#include <cmath>

#include "../Level.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

static Direction *const s_leverFaces[] = {Direction::down, Direction::up, Direction::north, Direction::south, Direction::west, Direction::east};

LeverBlock::LeverBlock() : SwitchBlock("assets/texture/block/lever.png")
{
    m_faceProperty    = AddProperty("face", 6);
    m_axisProperty    = AddProperty("axis", 2);
    m_poweredProperty = AddProperty("powered", 2);
    SetSecondaryTexture("assets/texture/block/cobblestone.png");
    SetTransparent(true);
    SetCollidable(false);
    SetRenderShape(eRenderShape_BOXES);
    SetNeedsAttachedSupport(true);
    SetFlatItem(true);
}

void LeverBlock::GetFrame(uint16_t data, glm::dvec3 *out, glm::dvec3 *along, glm::dvec3 *across) const
{
    Direction *face = s_leverFaces[GetPropertyValue(data, m_faceProperty)];
    *out            = glm::dvec3(face->dx, face->dy, face->dz);
    *along          = glm::dvec3(0.0, 1.0, 0.0);
    if (face->dy != 0)
    {
        *along = GetPropertyValue(data, m_axisProperty) == 0 ? glm::dvec3(1.0, 0.0, 0.0) : glm::dvec3(0.0, 0.0, 1.0);
    }

    *across = glm::cross(*out, *along);
}

AABB LeverBlock::GetFrameBox(uint16_t data, double depth, double halfAlong, double halfAcross) const
{
    glm::dvec3 out;
    glm::dvec3 along;
    glm::dvec3 across;
    GetFrame(data, &out, &along, &across);
    glm::dvec3 base = glm::dvec3(0.5) - out * 0.5;
    glm::dvec3 a    = base - along * halfAlong - across * halfAcross;
    glm::dvec3 b    = base + out * depth + along * halfAlong + across * halfAcross;
    return AABB(glm::min(a, b), glm::max(a, b));
}

void LeverBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    const double p = 1.0 / 16.0;
    out->push_back(GetFrameBox(data, 8.0 * p, 4.0 * p, 4.0 * p));
}

void LeverBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    const double p = 1.0 / 16.0;
    out->push_back(GetFrameBox(data, 3.0 * p, 4.0 * p, 3.0 * p));
}

void LeverBlock::GetRenderPlanes(uint16_t data, std::vector<RenderPlane> *out) const
{
    const double p = 1.0 / 16.0;
    glm::dvec3 outward;
    glm::dvec3 along;
    glm::dvec3 across;
    GetFrame(data, &outward, &along, &across);
    glm::dvec3 handle = glm::normalize(outward + (IsPowered(data) ? -along : along));
    glm::dvec3 side   = glm::cross(handle, across);
    glm::dvec3 start  = glm::dvec3(0.5) - outward * 0.5 + outward * p;
    glm::dvec3 end    = start + handle * (10.0 * p);
    float u0          = 7.0f / 16.0f;
    float u1          = 9.0f / 16.0f;
    float v0          = 6.0f / 16.0f;
    out->push_back(RenderPlane{start + side * p, end + side * p, across * p, u0, v0, u1, 1.0f});
    out->push_back(RenderPlane{start - side * p, end - side * p, across * p, u0, v0, u1, 1.0f});
    out->push_back(RenderPlane{start + across * p, end + across * p, side * p, u0, v0, u1, 1.0f});
    out->push_back(RenderPlane{start - across * p, end - across * p, side * p, u0, v0, u1, 1.0f});
    out->push_back(RenderPlane{end - side * p, end + side * p, across * p, u0, v0, u1, 8.0f / 16.0f});
}

Block::UVRect LeverBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    (void) data;
    (void) box;
    (void) direction;
    *flipU = false;
    return GetSecondaryAtlasUVRect();
}

uint16_t LeverBlock::GetPlacementData(Direction *placedAgainst) const
{
    for (int i = 0; i < 6; i++)
    {
        if (s_leverFaces[i] == placedAgainst)
        {
            return WithPropertyValue(0, m_faceProperty, i);
        }
    }

    return WithPropertyValue(0, m_faceProperty, 1);
}

uint16_t LeverBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) level;
    (void) pos;
    (void) hitPos;
    return WithPropertyValue(GetPlacementData(face), m_axisProperty, std::abs(look.x) > std::abs(look.z) ? 0 : 1);
}

Direction *LeverBlock::GetAttachment(uint16_t data) const
{
    return Direction::Opposite(s_leverFaces[GetPropertyValue(data, m_faceProperty)]);
}

uint16_t LeverBlock::GetItemData() const
{
    return WithPropertyValue(0, m_faceProperty, 1);
}

bool LeverBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return true;
}

bool LeverBlock::OnUse(Level *level, const BlockPos &pos)
{
    bool powered = !IsPowered(level->GetBlockData(pos));
    SetPowered(level, pos, powered);

    glm::dvec3 center((double) pos.x + 0.5, (double) pos.y + 0.5, (double) pos.z + 0.5);
    level->PlaySound("random.click", center, 0.3f, powered ? 0.6f : 0.5f);
    return true;
}
