#include "AnvilBlock.h"

#include <cmath>

AnvilBlock::AnvilBlock(const std::string &topPath, const std::string &damagedName) : GravityBlock("assets/texture/block/anvil.png")
{
    m_facingProperty = AddProperty("facing", 4);
    m_top            = AddTextureVariant(topPath);
    m_rotatedTop     = AddTextureVariant(topPath + "#rotated");
    m_damagedName    = damagedName;
    SetTransparent(true);
    SetRenderShape(eRenderShape_BOXES);
    SetBreakSound("dig.stone", 4);
    SetPlaceSound("random.anvil_land", 0, 0.8f);
}

uint16_t AnvilBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) level;
    (void) pos;
    (void) face;
    (void) hitPos;

    int facing = 0;
    if (fabs(look.x) > fabs(look.z))
    {
        facing = look.x > 0.0 ? 1 : 0;
    }
    else
    {
        facing = look.z > 0.0 ? 2 : 3;
    }

    return WithPropertyValue(0, m_facingProperty, facing);
}

uint16_t AnvilBlock::GetItemData() const
{
    return WithPropertyValue(0, m_facingProperty, 0);
}

void AnvilBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    GetRenderBoxes(data, out);
}

void AnvilBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    out->push_back(AABB(glm::dvec3(2.0, 0.0, 2.0) / 16.0, glm::dvec3(14.0, 4.0, 14.0) / 16.0));
    if (GetPropertyValue(data, m_facingProperty) >= 2)
    {
        out->push_back(AABB(glm::dvec3(3.0, 4.0, 4.0) / 16.0, glm::dvec3(13.0, 5.0, 12.0) / 16.0));
        out->push_back(AABB(glm::dvec3(4.0, 5.0, 6.0) / 16.0, glm::dvec3(12.0, 10.0, 10.0) / 16.0));
        out->push_back(AABB(glm::dvec3(0.0, 10.0, 3.0) / 16.0, glm::dvec3(16.0, 16.0, 13.0) / 16.0));
    }
    else
    {
        out->push_back(AABB(glm::dvec3(4.0, 4.0, 3.0) / 16.0, glm::dvec3(12.0, 5.0, 13.0) / 16.0));
        out->push_back(AABB(glm::dvec3(6.0, 5.0, 4.0) / 16.0, glm::dvec3(10.0, 10.0, 12.0) / 16.0));
        out->push_back(AABB(glm::dvec3(3.0, 10.0, 0.0) / 16.0, glm::dvec3(13.0, 16.0, 16.0) / 16.0));
    }
}

Block::UVRect AnvilBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    *flipU = false;
    if (direction == Direction::up && box.GetMax().y == 1.0)
    {
        return GetTextureVariantAtlasUVRect(GetPropertyValue(data, m_facingProperty) >= 2 ? m_rotatedTop : m_top);
    }

    return GetAtlasUVRect(direction);
}

bool AnvilBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return true;
}

Block *AnvilBlock::GetDamaged() const
{
    if (m_damagedName.empty())
    {
        return nullptr;
    }

    return Block::ByName(m_damagedName);
}
