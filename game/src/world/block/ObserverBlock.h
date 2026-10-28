#pragma once

#include "Block.h"

class ObserverBlock : public Block
{
public:
    ObserverBlock();

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnRemoved(Level *level, const BlockPos &pos, uint16_t data) override;
    void OnNeighborStateChanged(Level *level, const BlockPos &pos, Direction *direction) override;
    void Tick(Level *level, const BlockPos &pos) override;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;
    void AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    uint16_t GetItemData() const override;
    int GetSignal(uint16_t data, Direction *direction) const override;
    int GetDirectSignal(uint16_t data, Direction *direction) const override;
    bool IsSignalSource() const override;
    bool CanConnectRedstone(uint16_t data, Direction *direction) const override;

    Direction *GetFacing(uint16_t data) const;
    bool IsPowered(uint16_t data) const;

private:
    uint16_t WithFacing(uint16_t data, Direction *facing) const;
    void UpdateNeighborsInFront(Level *level, const BlockPos &pos, uint16_t data) const;
    glm::dvec3 ToModel(Direction *facing, const glm::dvec3 &world) const;
    void GetFaceUV(uint16_t data, const AABB &box, Direction *direction, int *texture, float *u0, float *v0, float *u1, float *v1) const;

    int m_facingProperty;
    int m_poweredProperty;
    int m_frontTexture;
    int m_backTexture;
    int m_backOnTexture;
    int m_sideTexture;
    int m_topTexture;
};
