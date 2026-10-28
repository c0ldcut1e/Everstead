#pragma once

#include "Block.h"

class StairBlock : public Block
{
public:
    enum eSTAIR_SHAPE : uint8_t
    {
        eStairShape_Straight   = 0,
        eStairShape_InnerLeft  = 1,
        eStairShape_InnerRight = 2,
        eStairShape_OuterLeft  = 3,
        eStairShape_OuterRight = 4,
    };

    StairBlock(const std::string &texturePath);

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void UpdateShape(Level *level, const BlockPos &pos) override;
    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    uint16_t GetItemData() const override;

private:
    static Direction *FacingDirection(int facing);
    static bool InHalf(int quadrantX, int quadrantZ, int facing);

    int GetFacing(uint16_t data) const;
    bool IsTopHalf(uint16_t data) const;
    bool GetNeighborStairs(Level *level, const BlockPos &pos, int facing, uint16_t *outData) const;
    bool CanTakeShape(Level *level, const BlockPos &pos, uint16_t data, int face) const;
    uint16_t WithShape(Level *level, const BlockPos &pos, uint16_t data) const;

    int m_facingProperty;
    int m_halfProperty;
    int m_shapeProperty;
};
