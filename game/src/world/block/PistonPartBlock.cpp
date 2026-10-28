#include "PistonPartBlock.h"

#include <cstdlib>

static Direction *const s_pistonFacings[] = {Direction::down, Direction::up, Direction::north, Direction::south, Direction::west, Direction::east};

PistonPartBlock::PistonPartBlock(const std::string &texturePath) : Block(false, texturePath)
{
    SetBlocksLiquid(true);
    m_facingProperty     = AddProperty("facing", 6);
    m_topTexture         = AddTextureVariant("assets/texture/block/piston_top.png");
    m_stickyTopTexture   = AddTextureVariant("assets/texture/block/piston_top_sticky.png");
    m_innerTexture       = AddTextureVariant("assets/texture/block/piston_inner.png");
    m_bottomTexture      = AddTextureVariant("assets/texture/block/piston_bottom.png");
    m_sideTexture        = AddTextureVariant("assets/texture/block/piston_side.png");
    m_rotatedSideTexture = AddTextureVariant("assets/texture/block/piston_side.png#rotated");
    SetTransparent(true);
    SetCollidable(true);
    SetRenderShape(eRenderShape_BOXES);
    SetBreakSound("dig.stone");
}

// The side texture has the head strip along its top, so we rotate or flip it until that strip faces the same way as the piston
Block::UVRect PistonPartBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    *flipU            = false;
    Direction *facing = GetFacing(data);
    if (direction == facing || direction == Direction::Opposite(facing))
    {
        if (IsArm(box, facing))
        {
            return GetTextureVariantAtlasUVRect(m_sideTexture);
        }

        return GetTextureVariantAtlasUVRect(GetEndTexture(data, direction == facing));
    }

    int alongU = 0;
    int alongV = 0;
    GetFaceAxes(direction, facing, &alongU, &alongV);
    return GetTextureVariantAtlasUVRect(alongV != 0 ? (IsArm(box, facing) ? m_rotatedSideTexture : m_sideTexture)
                                                    : (IsArm(box, facing) ? m_sideTexture : m_rotatedSideTexture));
}

void PistonPartBlock::AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const
{
    Direction *facing = GetFacing(data);
    if (direction == facing || direction == Direction::Opposite(facing))
    {
        return;
    }

    int alongU = 0;
    int alongV = 0;
    GetFaceAxes(direction, facing, &alongU, &alongV);
    if (IsArm(box, facing))
    {
        if (alongV != 0)
        {
            *u0 = 12.0f / 16.0f;
            *u1 = 1.0f;
        }
        else
        {
            *v0 = 0.0f;
            *v1 = 4.0f / 16.0f;
        }

        return;
    }

    if (alongV > 0 || alongU < 0)
    {
        *u0 = 1.0f - *u0;
        *u1 = 1.0f - *u1;
        *v0 = 1.0f - *v0;
        *v1 = 1.0f - *v1;
    }
}

Direction *PistonPartBlock::GetFacing(uint16_t data) const
{
    return s_pistonFacings[GetPropertyValue(data, m_facingProperty)];
}

uint16_t PistonPartBlock::WithFacing(uint16_t data, Direction *facing) const
{
    for (int i = 0; i < 6; i++)
    {
        if (s_pistonFacings[i] == facing)
        {
            return WithPropertyValue(data, m_facingProperty, i);
        }
    }

    return data;
}

// The start and end are measured along the facing from the back of the block
AABB PistonPartBlock::MakeBox(Direction *facing, double start, double end, double halfWidth) const
{
    glm::dvec3 axis(std::abs(facing->dx), std::abs(facing->dy), std::abs(facing->dz));
    glm::dvec3 offset(facing->dx + facing->dy + facing->dz < 0 ? 1.0 - end : start);
    glm::dvec3 length(end - start);
    glm::dvec3 min = axis * offset + (1.0 - axis) * (0.5 - halfWidth);
    return AABB(min, min + axis * length + (1.0 - axis) * (halfWidth * 2.0));
}

bool PistonPartBlock::IsArm(const AABB &box, Direction *facing) const
{
    glm::dvec3 size = box.GetMax() - box.GetMin();
    return (facing->dx == 0 && size.x < 0.5) || (facing->dy == 0 && size.y < 0.5) || (facing->dz == 0 && size.z < 0.5);
}

// These follow the mesher, where side faces run v downwards and the u axis turns with the face
void PistonPartBlock::GetFaceAxes(Direction *direction, Direction *facing, int *alongU, int *alongV) const
{
    glm::ivec3 u(0);
    glm::ivec3 v(0, -1, 0);
    if (direction == Direction::south)
    {
        u = glm::ivec3(1, 0, 0);
    }
    else if (direction == Direction::north)
    {
        u = glm::ivec3(-1, 0, 0);
    }
    else if (direction == Direction::east)
    {
        u = glm::ivec3(0, 0, -1);
    }
    else if (direction == Direction::west)
    {
        u = glm::ivec3(0, 0, 1);
    }
    else
    {
        u = glm::ivec3(-1, 0, 0);
        v = glm::ivec3(0, 0, direction == Direction::up ? -1 : 1);
    }

    glm::ivec3 f(facing->dx, facing->dy, facing->dz);
    *alongU = u.x * f.x + u.y * f.y + u.z * f.z;
    *alongV = v.x * f.x + v.y * f.y + v.z * f.z;
}
