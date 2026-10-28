#pragma once

#include "Block.h"

class FurnaceBlock : public Block
{
public:
    FurnaceBlock(const std::string &sidePath, const std::string &topPath, const std::string &frontPath, bool container, bool lit);

    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    uint16_t GetItemData() const override;
    UVRect GetAtlasUVRect(Direction *direction, uint16_t data) const override;
    bool IsUsable(uint16_t data) const override;
    void OnBreak(Level *level, const BlockPos &pos) override;
    const char *GetBlockEntityId() const override;
    bool HasAnalogOutput() const override;
    int GetAnalogOutput(Level *level, const BlockPos &pos) const override;
    void AnimateTick(Level *level, const BlockPos &pos, Random *random) override;

private:
    int m_facingProperty;
    bool m_container;
    bool m_lit;
};
