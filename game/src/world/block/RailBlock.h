#pragma once

#include "Block.h"

class RailBlock : public Block
{
public:
    RailBlock(const std::string &texturePath, const std::string &variantTexturePath, bool canCurve);

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderPlanes(uint16_t data, std::vector<RenderPlane> *out) const override;
    UVRect GetAtlasUVRect(Direction *direction, uint16_t data) const override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;

    int GetShape(uint16_t data) const;
    bool IsPowered(uint16_t data) const;
    static bool IsRail(Block *block);

protected:
    void SetPowered(Level *level, const BlockPos &pos, bool powered);

    int m_shapeProperty;
    int m_poweredProperty;

private:
    struct Track
    {
        RailBlock *block;
        BlockPos pos;
        std::vector<BlockPos> connections;
    };

    void LoadTrack(Level *level, const BlockPos &pos, Track *out) const;
    bool FindTrack(Level *level, const BlockPos &pos, Track *out) const;
    void SetConnections(Track *track, int shape) const;
    bool ConnectsTo(const Track &track, const Track &other) const;
    bool HasConnection(const Track &track, int x, int z) const;
    void RemoveSoftConnections(Level *level, Track *track) const;
    bool CanConnectTo(const Track &track, const Track &other) const;
    void ConnectTo(Level *level, Track *track, const Track &other) const;
    bool HasNeighborTrack(Level *level, const Track &track, const BlockPos &pos) const;
    int CountPotentialConnections(Level *level, const BlockPos &pos) const;
    int SlopeShape(Level *level, const BlockPos &pos, int shape) const;
    void PlaceTrack(Level *level, const BlockPos &pos, bool first) const;
    bool IsRailAt(Level *level, const BlockPos &pos) const;
    bool HasTopSupport(Level *level, const BlockPos &pos) const;

    int m_variantTexture;
    bool m_canCurve;
};
