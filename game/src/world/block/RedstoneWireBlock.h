#pragma once

#include "Block.h"

class RedstoneWireBlock : public Block
{
public:
    RedstoneWireBlock();

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnRemoved(Level *level, const BlockPos &pos, uint16_t data) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void UpdateShape(Level *level, const BlockPos &pos) override;
    bool IsUsable(uint16_t data) const override;
    bool OnUse(Level *level, const BlockPos &pos) override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;
    uint32_t GetDataTint(uint16_t data) const override;
    void AnimateTick(Level *level, const BlockPos &pos, Random *random) override;
    int GetSignal(uint16_t data, Direction *direction) const override;
    int GetDirectSignal(uint16_t data, Direction *direction) const override;
    bool IsSignalSource() const override;
    int GetWirePower(Level *level, const BlockPos &pos) const;

private:
    enum eSIDE_CONNECTION
    {
        eSideConnection_NONE,
        eSideConnection_SIDE,
        eSideConnection_UP
    };

    int GetSideConnection(Level *level, const BlockPos &pos, Direction *direction) const;
    uint16_t GetConnections(Level *level, const BlockPos &pos, uint16_t data) const;
    // A single connection also counts its opposite side, so a dead end runs straight through the block
    void GetConnectedSides(uint16_t data, bool *connected) const;
    void UpdatePower(Level *level, const BlockPos &pos);
    void NotifyWireNeighbors(Level *level, const BlockPos &pos);

    static Direction *const s_sides[4];

    int m_powerProperty;
    int m_sideProperties[4];
    int m_dotProperty;
    int m_lineVariant;
    int m_verticalLineVariant;
    // We turn this off while a wire reads its neighbors, otherwise it would see its own power coming back through them
    bool m_shouldSignal;
};
