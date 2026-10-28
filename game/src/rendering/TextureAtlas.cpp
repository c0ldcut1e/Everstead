#ifndef EVERSTEAD_DEDICATED_SERVER

#include "TextureAtlas.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "../core/Logger.h"
#include "../io/File.h"
#include "BufferedImage.h"
#include "RenderDevice.h"
#include "TextureRepository.h"
#include "Textures.h"

std::string TextureAtlas::BuildSidecarJsonPath(const std::string &texturePath) const
{
    size_t dot = texturePath.find_last_of('.');
    if (dot == std::string::npos)
    {
        return texturePath + ".json";
    }

    return texturePath.substr(0, dot) + ".json";
}

bool TextureAtlas::ReadTextFile(const std::string &path, std::string *outText) const
{
    File file(path);
    if (!file.Exists())
    {
        return false;
    }

    *outText = file.ReadAllText();
    return true;
}

bool TextureAtlas::LoadAnimationMetadata(const std::string &texturePath, int *outFrameTime, bool *outInterpolate,
                                         std::vector<AnimationFrame> *outFrames) const
{
    std::string jsonPath = BuildSidecarJsonPath(texturePath);
    std::string jsonText;
    if (!ReadTextFile(jsonPath, &jsonText))
    {
        return false;
    }

    nlohmann::json document = nlohmann::json::parse(jsonText, nullptr, false);
    if (document.is_discarded() || !document.contains("animation") || !document["animation"].is_object())
    {
        return false;
    }

    int frameTime              = 1;
    const nlohmann::json &anim = document["animation"];
    if (anim.contains("frametime") && anim["frametime"].is_number_integer())
    {
        int parsedValue = anim["frametime"].get<int>();
        if (parsedValue > 0)
        {
            frameTime = parsedValue;
        }
    }

    outFrames->clear();
    if (anim.contains("frames") && anim["frames"].is_array())
    {
        const nlohmann::json &frames = anim["frames"];
        for (size_t i = 0; i < frames.size(); i++)
        {
            const nlohmann::json &frame = frames[i];
            if (frame.is_number_integer())
            {
                outFrames->push_back({frame.get<int>(), frameTime});
            }
            else if (frame.is_object() && frame.contains("index") && frame["index"].is_number_integer())
            {
                int time = frame.contains("time") && frame["time"].is_number_integer() ? frame["time"].get<int>() : frameTime;
                outFrames->push_back({frame["index"].get<int>(), time > 0 ? time : frameTime});
            }
        }
    }

    *outInterpolate = anim.contains("interpolate") && anim["interpolate"].is_boolean() && anim["interpolate"].get<bool>();
    *outFrameTime   = frameTime;
    return true;
}

TextureAtlas::TextureAtlas()
{
    m_cellWidth  = 0;
    m_cellHeight = 0;
    m_columns    = 0;
    m_rows       = 0;
}

void TextureAtlas::Build(const std::vector<std::string> &paths, const std::unordered_map<std::string, BufferedImage> &images)
{
    m_regions.clear();
    m_animations.clear();
    m_animatedRegions.clear();
    m_texture.reset();

    if (paths.empty())
    {
        return;
    }

    std::vector<AtlasBuildEntry> entries;
    entries.reserve(paths.size());

    int cellWidth  = 0;
    int cellHeight = 0;
    std::unordered_set<std::string> loadedPaths;

    for (auto pathIt = paths.begin(); pathIt != paths.end(); ++pathIt)
    {
        const std::string &path   = *pathIt;
        std::string rotatedSuffix = "#rotated";
        bool rotated =
                path.size() > rotatedSuffix.size() && path.compare(path.size() - rotatedSuffix.size(), rotatedSuffix.size(), rotatedSuffix) == 0;
        if (!loadedPaths.insert(path).second)
        {
            continue;
        }

        std::string imagePath = rotated ? path.substr(0, path.size() - rotatedSuffix.size()) : path;

        AtlasBuildEntry entry;
        auto generated = images.find(path);
        try
        {
            if (generated != images.end())
            {
                entry.image = generated->second;
            }
            else
            {
                entry.image.Load(imagePath.c_str());
            }
        }
        catch (const std::exception &exception)
        {
            Logger::LogError("Atlas texture load failed path=%s error=%s", imagePath.c_str(), exception.what());
            entry.image = TextureRepository::CreateMissingImage();
        }

        entry.path        = path;
        entry.animated    = false;
        entry.rotated     = rotated && entry.image.GetPixelWidth() == entry.image.GetPixelHeight();
        entry.interpolate = false;
        entry.frameTime   = 1;
        entry.frameWidth  = entry.image.GetPixelWidth();
        entry.frameHeight = entry.image.GetPixelHeight();
        entry.frameCount  = 1;
        entry.atlasX      = 0;
        entry.atlasY      = 0;

        int parsedFrameTime = 1;
        std::vector<AnimationFrame> parsedFrames;
        if (!rotated && LoadAnimationMetadata(path, &parsedFrameTime, &entry.interpolate, &parsedFrames))
        {
            int frameWidth  = entry.image.GetPixelWidth();
            int frameHeight = frameWidth;
            int frameCount  = 0;
            if (frameWidth > 0 && frameHeight > 0)
            {
                frameCount = entry.image.GetPixelHeight() / frameHeight;
            }

            if (frameCount >= 1)
            {
                entry.frameTime   = parsedFrameTime;
                entry.frameWidth  = frameWidth;
                entry.frameHeight = frameHeight;
                entry.frameCount  = frameCount;
                entry.animated    = frameCount > 1;
                for (size_t i = 0; i < parsedFrames.size(); i++)
                {
                    if (parsedFrames[i].index >= 0 && parsedFrames[i].index < frameCount)
                    {
                        entry.frames.push_back(parsedFrames[i]);
                    }
                }

                if (entry.frames.empty())
                {
                    for (int i = 0; i < frameCount; i++)
                    {
                        entry.frames.push_back({i, parsedFrameTime});
                    }
                }
            }
        }

        entries.push_back(entry);

        if (entry.frameWidth > cellWidth)
        {
            cellWidth = entry.frameWidth;
        }

        if (entry.frameHeight > cellHeight)
        {
            cellHeight = entry.frameHeight;
        }
    }

    if (entries.empty() || cellWidth <= 0 || cellHeight <= 0)
    {
        return;
    }

    int count   = (int) entries.size();
    int columns = (int) ceil(sqrt((double) count));
    int rows    = (count + columns - 1) / columns;
    int width   = columns * cellWidth;
    int height  = rows * cellHeight;

    std::vector<uint8_t> atlasPixels((size_t) width * (size_t) height * 4, 0);

    for (int i = 0; i < count; i++)
    {
        AtlasBuildEntry &entry = entries[(size_t) i];

        int col      = i % columns;
        int row      = i / columns;
        int dstX     = col * cellWidth;
        int dstY     = row * cellHeight;
        entry.atlasX = dstX;
        entry.atlasY = dstY;

        int srcWidth  = entry.frameWidth;
        int srcHeight = entry.frameHeight;

        const std::vector<uint8_t> &srcPixels = entry.image.GetPixels();
        for (int y = 0; y < srcHeight; y++)
        {
            size_t dstOffset = ((size_t) (dstY + y) * (size_t) width + (size_t) dstX) * 4;
            if (!entry.rotated)
            {
                memcpy(atlasPixels.data() + dstOffset, srcPixels.data() + (size_t) y * (size_t) srcWidth * 4, (size_t) srcWidth * 4);
                continue;
            }

            for (int x = 0; x < srcWidth; x++)
            {
                size_t srcOffset = ((size_t) (srcHeight - 1 - x) * (size_t) srcWidth + (size_t) y) * 4;
                memcpy(atlasPixels.data() + dstOffset + (size_t) x * 4, srcPixels.data() + srcOffset, 4);
            }
        }

        Region region;
        region.u0             = (float) dstX / (float) width;
        region.v0             = (float) dstY / (float) height;
        region.u1             = (float) (dstX + srcWidth) / (float) width;
        region.v1             = (float) (dstY + srcHeight) / (float) height;
        m_regions[entry.path] = region;

        if (entry.animated)
        {
            AnimationState state;
            state.frameTime          = entry.frameTime;
            state.frameCount         = entry.frameCount;
            state.frameWidth         = entry.frameWidth;
            state.frameHeight        = entry.frameHeight;
            state.atlasX             = entry.atlasX;
            state.atlasY             = entry.atlasY;
            state.frameIndex         = 0;
            state.accumulator        = 0.0;
            state.interpolate        = entry.interpolate;
            state.lastWeight         = -1;
            state.frames             = entry.frames;
            state.region             = region;
            state.sourcePixels       = srcPixels;
            m_animations[entry.path] = std::move(state);
            m_animatedRegions.push_back(region);
        }
    }

    m_cellWidth  = cellWidth;
    m_cellHeight = cellHeight;
    m_columns    = columns;
    m_rows       = rows;
    BufferedImage image(width, height, atlasPixels.data());
    m_texture = std::make_shared<Texture>(image);
    Textures::UploadAlphaSafeMipmaps(m_texture->GetId(), atlasPixels, cellWidth, cellHeight, columns, rows, s_mipLevelCount);
    Logger::LogInfo("Built texture atlas (%dx%d, %d entries, %d animated)", width, height, count, (int) m_animations.size());
}

void TextureAtlas::UpdateAnimations(double deltaTime, const std::vector<Region> &visibleRegions)
{
    if (!m_texture || m_animations.empty() || (visibleRegions.empty() && m_markedRegions.empty()) || deltaTime <= 0.0)
    {
        return;
    }

    for (auto it = m_animations.begin(); it != m_animations.end(); ++it)
    {
        AnimationState &state = it->second;
        bool visible          = false;
        for (size_t i = 0; i < visibleRegions.size() + m_markedRegions.size() && !visible; i++)
        {
            const Region &region = i < visibleRegions.size() ? visibleRegions[i] : m_markedRegions[i - visibleRegions.size()];
            visible = state.region.u0 == region.u0 && state.region.v0 == region.v0 && state.region.u1 == region.u1 && state.region.v1 == region.v1;
        }

        if (!visible)
        {
            continue;
        }

        if (state.frameCount <= 1 || state.frameWidth <= 0 || state.frameHeight <= 0)
        {
            continue;
        }

        state.accumulator += deltaTime;
        bool changedFrame = false;
        while (state.accumulator >= (double) state.frames[(size_t) state.frameIndex].time * 0.05)
        {
            state.accumulator -= (double) state.frames[(size_t) state.frameIndex].time * 0.05;
            state.frameIndex = (state.frameIndex + 1) % (int) state.frames.size();
            changedFrame     = true;
        }

        if (!changedFrame && !state.interpolate)
        {
            continue;
        }

        size_t frameStride = (size_t) state.frameWidth * (size_t) state.frameHeight * (size_t) 4;
        size_t frameOffset = frameStride * (size_t) state.frames[(size_t) state.frameIndex].index;
        if (frameOffset + frameStride > state.sourcePixels.size())
        {
            continue;
        }

        const uint8_t *framePixels = state.sourcePixels.data() + frameOffset;
        if (state.interpolate)
        {
            int nextIndex     = (state.frameIndex + 1) % (int) state.frames.size();
            size_t nextOffset = frameStride * (size_t) state.frames[(size_t) nextIndex].index;
            if (nextOffset + frameStride > state.sourcePixels.size())
            {
                continue;
            }

            // The blend weight steps in 1/256, so unchanged weights skip the upload
            double duration = (double) state.frames[(size_t) state.frameIndex].time * 0.05;
            int weight      = (int) (state.accumulator / duration * 256.0);
            weight          = weight < 0 ? 0 : (weight > 256 ? 256 : weight);
            if (!changedFrame && weight == state.lastWeight)
            {
                continue;
            }

            state.lastWeight = weight;
            state.blendPixels.resize(frameStride);
            const uint8_t *nextPixels = state.sourcePixels.data() + nextOffset;
            for (size_t i = 0; i < frameStride; i++)
            {
                state.blendPixels[i] = (uint8_t) ((framePixels[i] * (256 - weight) + nextPixels[i] * weight) >> 8);
            }

            framePixels = state.blendPixels.data();
        }

        RenderDevice::GetInstance()->UploadTextureRegion(m_texture->GetId(), state.atlasX, state.atlasY, state.frameWidth, state.frameHeight,
                                                         eTextureFormat_RGBA8, framePixels);
    }

    m_markedRegions.clear();
}

// Item and block item draws mark their tiles here because they never show up in a chunk mesh
void TextureAtlas::MarkVisible(const Region &region)
{
    if (!IsAnimated(region))
    {
        return;
    }

    for (size_t i = 0; i < m_markedRegions.size(); i++)
    {
        const Region &marked = m_markedRegions[i];
        if (marked.u0 == region.u0 && marked.v0 == region.v0 && marked.u1 == region.u1 && marked.v1 == region.v1)
        {
            return;
        }
    }

    m_markedRegions.push_back(region);
}

bool TextureAtlas::Has(const std::string &path) const
{
    return m_regions.find(path) != m_regions.end();
}

bool TextureAtlas::IsAnimated(const Region &region) const
{
    for (size_t i = 0; i < m_animatedRegions.size(); i++)
    {
        const Region &animated = m_animatedRegions[i];
        if (animated.u0 == region.u0 && animated.v0 == region.v0 && animated.u1 == region.u1 && animated.v1 == region.v1)
        {
            return true;
        }
    }

    return false;
}

TextureAtlas::Region TextureAtlas::Get(const std::string &path) const
{
    auto it = m_regions.find(path);
    if (it == m_regions.end())
    {
        throw std::runtime_error("missing atlas region");
    }

    return it->second;
}

Texture *TextureAtlas::GetTexture() const
{
    return m_texture.get();
}

int TextureAtlas::GetCellWidth() const
{
    return m_cellWidth;
}

int TextureAtlas::GetCellHeight() const
{
    return m_cellHeight;
}

int TextureAtlas::GetColumns() const
{
    return m_columns;
}

int TextureAtlas::GetRows() const
{
    return m_rows;
}

#endif
