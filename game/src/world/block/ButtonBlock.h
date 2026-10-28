#pragma once

#include "SwitchBlock.h"

class ButtonBlock : public SwitchBlock
{
public:
    ButtonBlock(const std::string &texturePath, int pressTicks);

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetItemBoxes(std::vector<AABB> *out) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    uint16_t GetPlacementData(Direction *placedAgainst) const override;
    Direction *GetAttachment(uint16_t data) const override;
    uint16_t GetItemData() const override;
    bool IsUsable(uint16_t data) const override;
    bool OnUse(Level *level, const BlockPos &pos) override;
    void Tick(Level *level, const BlockPos &pos) override;

private:
    int m_faceProperty;
    int m_pressTicks;
};
