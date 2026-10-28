#pragma once

#include "Block.h"

class Random;

class PointedDripstoneBlock : public Block
{
public:
    enum eTHICKNESS : uint8_t
    {
        eThickness_TIP_MERGE = 0,
        eThickness_TIP       = 1,
        eThickness_FRUSTUM   = 2,
        eThickness_MIDDLE    = 3,
        eThickness_BASE      = 4,
        eThickness_COUNT     = 5,
    };

    PointedDripstoneBlock(const std::string &texturePrefix, bool hangingOnly = false);

    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void UpdateShape(Level *level, const BlockPos &pos) override;
    void Tick(Level *level, const BlockPos &pos) override;
    void RandomTick(Level *level, const BlockPos &pos) override;
    void AnimateTick(Level *level, const BlockPos &pos, Random *random) override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    Direction *GetAttachment(uint16_t data) const override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    AABB GetPlacedAABB(const BlockPos &pos, uint16_t data) const override;
    void GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const override;
    UVRect GetCrossAtlasUVRect(uint16_t data) const override;
    int GetCrossTextureVariant(uint16_t data) const override;

    Direction *GetTipDirection(uint16_t data) const;
    int GetThickness(uint16_t data) const;
    bool IsPointingAt(Level *level, const BlockPos &pos, Direction *tipDirection) const;
    uint16_t MakeState(Direction *tipDirection, int thickness) const;
    int CalculateThickness(Level *level, const BlockPos &pos, Direction *tipDirection) const;
    void SetLandSound(const std::string &landSound, int variants);
    const std::string &GetLandSound() const;
    int GetLandSoundVariants() const;
    void OnHeatPlaced(Level *level, const BlockPos &heatPos, int emission);

private:
    bool IsSupported(Level *level, const BlockPos &pos, Direction *tipDirection) const;
    bool IsNearHeat(Level *level, const BlockPos &pos) const;
    void Fall(Level *level, const BlockPos &pos, uint16_t data);

    int m_directionProperty;
    int m_thicknessProperty;
    int m_variants[2][eThickness_COUNT];
    bool m_hangingOnly;
    std::string m_landSound;
    int m_landSoundVariants;
};
