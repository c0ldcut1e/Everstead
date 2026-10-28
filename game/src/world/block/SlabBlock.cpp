#include "SlabBlock.h"

#include <cmath>

SlabBlock::SlabBlock(const std::string &texturePath, Block *fullBlock) : Block(true, texturePath)
{
    m_fullBlock    = fullBlock;
    m_halfProperty = AddProperty("half", 2);
    SetTransparent(true);
    SetRenderShape(eRenderShape_BOXES);
}

void SlabBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    double minY = IsTopHalf(data) ? 0.5 : 0.0;
    out->push_back(AABB(glm::dvec3(0.0, minY, 0.0), glm::dvec3(1.0, minY + 0.5, 1.0)));
}

uint16_t SlabBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) level;
    (void) look;

    bool top = face == Direction::down || (face != Direction::up && hitPos.y - (double) pos.y > 0.5);
    return WithPropertyValue(0, m_halfProperty, top ? 1 : 0);
}

Block *SlabBlock::GetCombinedBlock(uint16_t existingData, uint16_t placedData) const
{
    return IsTopHalf(existingData) != IsTopHalf(placedData) ? m_fullBlock : nullptr;
}

bool SlabBlock::IsTopHalf(uint16_t data) const
{
    return GetPropertyValue(data, m_halfProperty) == 1;
}
