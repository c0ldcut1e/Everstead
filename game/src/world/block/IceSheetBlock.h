#pragma once

#include "Block.h"

class IceSheetBlock : public Block
{
public:
    IceSheetBlock(const std::string &texturePath);

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetSelectionBoxes(const Level *level, const BlockPos &pos, uint16_t data, std::vector<AABB> *out) const override;
    void GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    uint16_t GetPlacementData(Direction *placedAgainst) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    Block *GetCombinedBlock(uint16_t existingData, uint16_t placedData) const override;
    Direction *GetAttachment(uint16_t data) const override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;

    static bool CanAttachTo(const Block *block);
    uint16_t WithFace(uint16_t data, Direction *side) const;
    bool HasFace(uint16_t data, Direction *side) const;

private:
    static constexpr float kThickness = 0.01f;
    static constexpr int kFaceCount   = 6;

    uint16_t GetSurvivingFaces(Level *level, const BlockPos &pos, uint16_t data) const;

    int m_faceProperties[kFaceCount];
};
