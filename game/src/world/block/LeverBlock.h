#pragma once

#include "SwitchBlock.h"

class LeverBlock : public SwitchBlock
{
public:
    LeverBlock();

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderPlanes(uint16_t data, std::vector<RenderPlane> *out) const override;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    uint16_t GetPlacementData(Direction *placedAgainst) const override;
    Direction *GetAttachment(uint16_t data) const override;
    uint16_t GetItemData() const override;
    bool IsUsable(uint16_t data) const override;
    bool OnUse(Level *level, const BlockPos &pos) override;

private:
    // Out is the face normal, along runs the way the handle swings and across is the hinge axis
    void GetFrame(uint16_t data, glm::dvec3 *out, glm::dvec3 *along, glm::dvec3 *across) const;
    AABB GetFrameBox(uint16_t data, double depth, double halfAlong, double halfAcross) const;

    int m_faceProperty;
    int m_axisProperty;
};
