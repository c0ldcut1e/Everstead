#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "BufferedImage.h"
#include "Texture.h"

class TextureAtlas
{
public:
    static const int s_mipLevelCount = 4;

    struct Region
    {
        float u0;
        float v0;
        float u1;
        float v1;
    };

    TextureAtlas();

    void Build(const std::vector<std::string> &paths, const std::unordered_map<std::string, BufferedImage> &images = {});
    void UpdateAnimations(double deltaTime, const std::vector<Region> &visibleRegions);
    void MarkVisible(const Region &region);

    bool Has(const std::string &path) const;
    bool IsAnimated(const Region &region) const;
    Region Get(const std::string &path) const;
    Texture *GetTexture() const;
    int GetCellWidth() const;
    int GetCellHeight() const;
    int GetColumns() const;
    int GetRows() const;

private:
    struct AnimationFrame
    {
        int index;
        int time;
    };

    struct AtlasBuildEntry
    {
        std::string path;
        BufferedImage image;
        bool animated;
        bool rotated;
        bool interpolate;
        int frameTime;
        int frameCount;
        int frameWidth;
        int frameHeight;
        int atlasX;
        int atlasY;
        std::vector<AnimationFrame> frames;
    };

    struct AnimationState
    {
        int frameTime;
        int frameCount;
        int frameWidth;
        int frameHeight;
        int atlasX;
        int atlasY;
        int frameIndex;
        double accumulator;
        bool interpolate;
        int lastWeight;
        std::vector<AnimationFrame> frames;
        Region region;
        std::vector<uint8_t> sourcePixels;
        std::vector<uint8_t> blendPixels;
    };

    std::string BuildSidecarJsonPath(const std::string &texturePath) const;
    bool ReadTextFile(const std::string &path, std::string *outText) const;
    bool LoadAnimationMetadata(const std::string &texturePath, int *outFrameTime, bool *outInterpolate, std::vector<AnimationFrame> *outFrames) const;

    std::unordered_map<std::string, Region> m_regions;
    std::unordered_map<std::string, AnimationState> m_animations;
    std::vector<Region> m_animatedRegions;
    std::vector<Region> m_markedRegions;
    std::shared_ptr<Texture> m_texture;
    int m_cellWidth;
    int m_cellHeight;
    int m_columns;
    int m_rows;
};
