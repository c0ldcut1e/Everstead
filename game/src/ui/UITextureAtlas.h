#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../rendering/BufferedImage.h"
#include "../rendering/Texture.h"

class UITextureAtlas
{
public:
    struct Region
    {
        float u0;
        float v0;
        float u1;
        float v1;
        int pixelWidth;
        int pixelHeight;
    };

    static Texture *GetTexture();
    static const Region &GetRegion(const std::string &path);
    static float GetU(const Region &region, float u);
    static float GetV(const Region &region, float v);

private:
    struct BuildEntry
    {
        std::string path;
        BufferedImage image;
        int atlasX;
        int atlasY;
    };

    static void EnsureBuilt();
    static void RegisterTexturePath(const std::string &path);
    static void Build();

    static bool s_built;
    static std::vector<std::string> s_paths;
    static std::unordered_map<std::string, Region> s_regions;
    static std::shared_ptr<Texture> s_texture;
};
