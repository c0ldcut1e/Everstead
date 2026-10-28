#pragma once

#include "Block.h"

class Random;

class VineBlock : public Block
{
public:
    VineBlock(const std::string &texturePath);

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void RandomTick(Level *level, const BlockPos &pos) override;
    Block *GetPlacementBlock(Direction *face) override;
    uint16_t GetPlacementData(Direction *placedAgainst) const override;
    Direction *GetAttachment(uint16_t data) const override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;

private:
    static constexpr float kThickness   = 0.05f;
    static constexpr int kSpreadRadius  = 4;
    static constexpr int kMaxNearbyVine = 5;

    static bool CanAttachTo(const Block *block);

    bool HasFace(uint16_t data, int side) const;
    uint16_t WithFace(uint16_t data, int side, bool value) const;
    bool HasAnySide(uint16_t data) const;
    uint16_t GetSurvivingFaces(Level *level, const BlockPos &pos, uint16_t data) const;
    uint16_t KeepRandomSides(Random *random, uint16_t data) const;
    bool HasTooManyNearby(Level *level, const BlockPos &pos) const;
    void Grow(Level *level, const BlockPos &pos, uint16_t data);

    int m_sideProperties[4];
    int m_upProperty;
};
