#pragma once

#include <cstdint>
#include <vector>

// This marks river cells at a 4 block resolution by zooming a coarse random parity grid and tracing its borders
class RiverLayer
{
public:
    explicit RiverLayer(uint32_t seed);

    bool IsRiver(int levelX, int levelZ) const;

private:
    struct Stage
    {
        int64_t seed;
    };

    Stage m_init;
    Stage m_zooms[6];
    Stage m_river;
    Stage m_smooth;

    std::vector<int> GetArea(int stage, int xo, int zo, int w, int h) const;
    std::vector<int> GetInitArea(int xo, int zo, int w, int h) const;
    std::vector<int> GetZoomArea(int stage, int xo, int zo, int w, int h) const;
    std::vector<int> GetRiverArea(int xo, int zo, int w, int h) const;
    std::vector<int> GetSmoothArea(int xo, int zo, int w, int h) const;

    static Stage MakeStage(int64_t mixup, int64_t worldSeed);

    static const int s_zoomCount = 6;
    static const int s_tileSize  = 16;
};
