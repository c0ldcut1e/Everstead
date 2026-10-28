#pragma once

#include "GravityBlock.h"

class AnvilBlock : public GravityBlock
{
public:
    AnvilBlock(const std::string &topPath, const std::string &damagedName);

    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    uint16_t GetItemData() const override;
    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;
    bool IsUsable(uint16_t data) const override;

    Block *GetDamaged() const;

private:
    int m_facingProperty;
    int m_top;
    int m_rotatedTop;
    std::string m_damagedName;
};
