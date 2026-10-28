#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UITextureAtlas.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <stdexcept>

#include "../core/Logger.h"
#include "../rendering/TextureRepository.h"

bool UITextureAtlas::s_built = false;
std::vector<std::string> UITextureAtlas::s_paths;
std::unordered_map<std::string, UITextureAtlas::Region> UITextureAtlas::s_regions;
std::shared_ptr<Texture> UITextureAtlas::s_texture;

Texture *UITextureAtlas::GetTexture()
{
    EnsureBuilt();
    return s_texture.get();
}

const UITextureAtlas::Region &UITextureAtlas::GetRegion(const std::string &path)
{
    EnsureBuilt();

    auto it = s_regions.find(path);
    if (it == s_regions.end())
    {
        RegisterTexturePath(path);
        Build();
        it = s_regions.find(path);
    }

    if (it == s_regions.end())
    {
        throw std::runtime_error("missing ui atlas region");
    }

    return it->second;
}

float UITextureAtlas::GetU(const Region &region, float u)
{
    return region.u0 + (region.u1 - region.u0) * u;
}

float UITextureAtlas::GetV(const Region &region, float v)
{
    return region.v0 + (region.v1 - region.v0) * v;
}

void UITextureAtlas::EnsureBuilt()
{
    if (s_built)
    {
        return;
    }

    for (const std::filesystem::directory_entry &entry : std::filesystem::recursive_directory_iterator("assets/texture/ui"))
    {
        std::string path = entry.path().generic_string();
        if (entry.is_regular_file() && entry.path().extension() == ".png" && path.find("/panorama/") == std::string::npos)
        {
            RegisterTexturePath(path);
        }
    }

    Build();
    s_built = true;
}

void UITextureAtlas::RegisterTexturePath(const std::string &path)
{
    if (path.empty() || std::find(s_paths.begin(), s_paths.end(), path) != s_paths.end())
    {
        return;
    }

    s_paths.push_back(path);
}

void UITextureAtlas::Build()
{
    s_regions.clear();
    s_texture.reset();

    std::vector<BuildEntry> entries;
    entries.reserve(s_paths.size());

    int width = 1024;

    for (size_t i = 0; i < s_paths.size(); i++)
    {
        BuildEntry entry;
        entry.path   = s_paths[i];
        entry.atlasX = 0;
        entry.atlasY = 0;
        try
        {
            entry.image.Load(entry.path.c_str());
        }
        catch (const std::exception &exception)
        {
            Logger::LogError("UI texture load failed path=%s error=%s", entry.path.c_str(), exception.what());
            entry.image = TextureRepository::CreateMissingImage();
        }
        catch (...)
        {
            Logger::LogError("UI texture load failed path=%s", entry.path.c_str());
            entry.image = TextureRepository::CreateMissingImage();
        }

        width = std::max(width, entry.image.GetPixelWidth());
        entries.push_back(entry);
    }

    if (entries.empty())
    {
        throw std::runtime_error("failed to build ui atlas");
    }

    std::sort(entries.begin(), entries.end(),
              [](const BuildEntry &a, const BuildEntry &b)
              {
                  return a.image.GetPixelHeight() > b.image.GetPixelHeight();
              });

    int shelfX      = 0;
    int shelfY      = 0;
    int shelfHeight = 0;
    for (size_t i = 0; i < entries.size(); i++)
    {
        if (shelfX + entries[i].image.GetPixelWidth() > width)
        {
            shelfX = 0;
            shelfY += shelfHeight + 1;
            shelfHeight = 0;
        }

        entries[i].atlasX = shelfX;
        entries[i].atlasY = shelfY;
        shelfX += entries[i].image.GetPixelWidth() + 1;
        shelfHeight = std::max(shelfHeight, entries[i].image.GetPixelHeight());
    }

    int count  = (int) entries.size();
    int height = shelfY + shelfHeight;

    std::vector<uint8_t> pixels((size_t) width * (size_t) height * 4, 0);

    for (int i = 0; i < count; i++)
    {
        BuildEntry &entry = entries[(size_t) i];

        for (int y = 0; y < entry.image.GetPixelHeight(); y++)
        {
            memcpy(pixels.data() + ((size_t) (entry.atlasY + y) * (size_t) width + (size_t) entry.atlasX) * 4,
                   entry.image.GetPixels().data() + (size_t) y * (size_t) entry.image.GetPixelWidth() * 4, (size_t) entry.image.GetPixelWidth() * 4);
        }

        Region region;
        region.u0             = (float) entry.atlasX / (float) width;
        region.v0             = (float) entry.atlasY / (float) height;
        region.u1             = (float) (entry.atlasX + entry.image.GetPixelWidth()) / (float) width;
        region.v1             = (float) (entry.atlasY + entry.image.GetPixelHeight()) / (float) height;
        region.pixelWidth     = entry.image.GetPixelWidth();
        region.pixelHeight    = entry.image.GetPixelHeight();
        s_regions[entry.path] = region;
    }

    s_texture = std::make_shared<Texture>(BufferedImage(width, height, pixels.data()));
}

#endif
