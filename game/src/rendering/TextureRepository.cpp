#ifndef EVERSTEAD_DEDICATED_SERVER

#include "TextureRepository.h"

#include <filesystem>
#include <fstream>
#include <system_error>
#include <vector>

#include "../core/Logger.h"

TextureRepository *TextureRepository::GetInstance()
{
    static TextureRepository *s_instance = new TextureRepository();
    return s_instance;
}

TextureRepository::TextureRepository()
{
    m_missingTexture = nullptr;
}

BufferedImage TextureRepository::CreateMissingImage()
{
    const uint8_t pixels[] = {
            0, 0, 0, 255, 255, 0, 255, 255, 255, 0, 255, 255, 0, 0, 0, 255,
    };

    return BufferedImage(2, 2, pixels);
}

std::shared_ptr<Texture> TextureRepository::Get(const std::string &path)
{
    if (path.empty())
    {
        return nullptr;
    }

    auto it = m_textures.find(path);
    if (it != m_textures.end())
    {
        return it->second;
    }

    std::shared_ptr<Texture> texture = LoadTextureOrMissing(path);
    m_textures[path]                 = texture;
    return texture;
}

std::shared_ptr<Texture> TextureRepository::GetSized(const std::string &path, int pixelWidth, int pixelHeight)
{
    if (path.empty())
    {
        return nullptr;
    }

    std::string cachePath = "sized:" + std::to_string(pixelWidth) + "x" + std::to_string(pixelHeight) + ":" + path;
    auto it               = m_textures.find(cachePath);
    if (it != m_textures.end())
    {
        return it->second;
    }

    try
    {
        BufferedImage image(path.c_str());
        if (image.GetPixelWidth() != pixelWidth || image.GetPixelHeight() != pixelHeight)
        {
            Logger::LogError("Texture size invalid path=%s width=%d height=%d expected=%dx%d", path.c_str(), image.GetPixelWidth(),
                             image.GetPixelHeight(), pixelWidth, pixelHeight);
            return GetMissingTexture();
        }

        std::shared_ptr<Texture> texture = std::make_shared<Texture>(image);
        m_textures[cachePath]            = texture;
        return texture;
    }
    catch (const std::exception &exception)
    {
        Logger::LogError("Texture load failed path=%s error=%s", path.c_str(), exception.what());
    }
    catch (...)
    {
        Logger::LogError("Texture load failed path=%s", path.c_str());
    }

    return GetMissingTexture();
}

std::shared_ptr<Texture> TextureRepository::GetPlayerSkin(const std::string &path)
{
    if (path.empty())
    {
        return nullptr;
    }

    std::string cachePath = "player_skin:" + path;
    auto it               = m_textures.find(cachePath);
    if (it != m_textures.end())
    {
        return it->second;
    }

    try
    {
        BufferedImage image(path.c_str());
        if (image.GetPixelWidth() != 64 || image.GetPixelHeight() != 64)
        {
            Logger::LogError("Player skin size invalid path=%s width=%d height=%d expected=64x64", path.c_str(), image.GetPixelWidth(),
                             image.GetPixelHeight());
            return GetMissingTexture();
        }

        std::vector<uint8_t> pixels = image.GetPixels();
        for (int y = 0; y < 64; y++)
        {
            for (int x = 0; x < 64; x++)
            {
                bool basePixel = (y < 16 && x < 32) || (y >= 16 && y < 32 && x < 56) || (y >= 48 && x >= 16 && x < 48);
                if (basePixel)
                {
                    size_t offset = ((size_t) y * 64 + (size_t) x) * 4;
                    if (pixels[offset + 3] == 0)
                    {
                        pixels[offset + 0] = 0;
                        pixels[offset + 1] = 0;
                        pixels[offset + 2] = 0;
                    }

                    pixels[offset + 3] = 255;
                }
            }
        }

        BufferedImage skinImage(64, 64, pixels.data());
        std::shared_ptr<Texture> texture = std::make_shared<Texture>(skinImage);
        m_textures[cachePath]            = texture;
        return texture;
    }
    catch (const std::exception &exception)
    {
        Logger::LogError("Player skin load failed path=%s error=%s", path.c_str(), exception.what());
    }
    catch (...)
    {
        Logger::LogError("Player skin load failed path=%s", path.c_str());
    }

    return GetMissingTexture();
}

std::shared_ptr<Texture> TextureRepository::GetDynamic(const std::string &path)
{
    if (path.empty())
    {
        return nullptr;
    }

    std::error_code errorCode;
    if (!std::filesystem::is_regular_file(path, errorCode) || errorCode)
    {
        Forget(path);
        Logger::LogError("Texture file missing path=%s", path.c_str());
        return GetMissingTexture();
    }

    int64_t modifiedTime = GetModifiedTime(path);
    uint64_t fileSize    = GetFileSize(path);
    uint64_t fileHash    = GetFileHash(path);
    if (fileSize == 0 || fileHash == 0)
    {
        Forget(path);
        Logger::LogError("Texture file unreadable path=%s", path.c_str());
        return GetMissingTexture();
    }

    auto it = m_textures.find(path);
    if (it != m_textures.end() && !HasFileChanged(path, modifiedTime, fileSize, fileHash))
    {
        return it->second;
    }

    return Reload(path, modifiedTime, fileSize, fileHash);
}

Texture *TextureRepository::GetRaw(const std::string &path)
{
    std::shared_ptr<Texture> texture = Get(path);
    return texture ? texture.get() : nullptr;
}

void TextureRepository::Clear()
{
    m_textures.clear();
    m_modifiedTimes.clear();
    m_fileSizes.clear();
    m_fileHashes.clear();
    m_missingTexture = nullptr;
}

void TextureRepository::Invalidate(const std::string &path)
{
    if (path.empty())
    {
        return;
    }

    Forget(path);
    Forget("player_skin:" + path);
    Forget("sized:64x32:" + path);
    Forget("sized:64x64:" + path);
}

int64_t TextureRepository::GetModifiedTime(const std::string &path) const
{
    std::error_code errorCode;
    std::filesystem::file_time_type modifiedTime = std::filesystem::last_write_time(path, errorCode);
    if (errorCode)
    {
        return 0;
    }

    return (int64_t) modifiedTime.time_since_epoch().count();
}

uint64_t TextureRepository::GetFileSize(const std::string &path) const
{
    std::error_code errorCode;
    uintmax_t fileSize = std::filesystem::file_size(path, errorCode);
    if (errorCode)
    {
        return 0;
    }

    return (uint64_t) fileSize;
}

uint64_t TextureRepository::GetFileHash(const std::string &path) const
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return 0;
    }

    uint64_t hash = 1469598103934665603;
    char buffer[4096];
    while (stream)
    {
        stream.read(buffer, sizeof(buffer));
        std::streamsize readSize = stream.gcount();
        for (std::streamsize i = 0; i < readSize; i++)
        {
            hash ^= (uint8_t) buffer[i];
            hash *= 1099511628211;
        }
    }

    return hash;
}

bool TextureRepository::HasFileChanged(const std::string &path, int64_t modifiedTime, uint64_t fileSize, uint64_t fileHash) const
{
    auto modifiedTimeIt = m_modifiedTimes.find(path);
    if (modifiedTimeIt == m_modifiedTimes.end() || modifiedTimeIt->second != modifiedTime)
    {
        return true;
    }

    auto fileSizeIt = m_fileSizes.find(path);
    if (fileSizeIt == m_fileSizes.end() || fileSizeIt->second != fileSize)
    {
        return true;
    }

    auto fileHashIt = m_fileHashes.find(path);
    return fileHashIt == m_fileHashes.end() || fileHashIt->second != fileHash;
}

std::shared_ptr<Texture> TextureRepository::GetMissingTexture()
{
    if (!m_missingTexture)
    {
        m_missingTexture = std::make_shared<Texture>(CreateMissingImage());
    }

    return m_missingTexture;
}

std::shared_ptr<Texture> TextureRepository::LoadTextureOrMissing(const std::string &path)
{
    try
    {
        return std::make_shared<Texture>(BufferedImage(path.c_str()));
    }
    catch (const std::exception &exception)
    {
        Logger::LogError("Texture load failed path=%s error=%s", path.c_str(), exception.what());
    }
    catch (...)
    {
        Logger::LogError("Texture load failed path=%s", path.c_str());
    }

    return GetMissingTexture();
}

std::shared_ptr<Texture> TextureRepository::Reload(const std::string &path, int64_t modifiedTime, uint64_t fileSize, uint64_t fileHash)
{
    try
    {
        auto it = m_textures.find(path);
        if (it != m_textures.end())
        {
            it->second            = LoadTextureOrMissing(path);
            m_modifiedTimes[path] = modifiedTime;
            m_fileSizes[path]     = fileSize;
            m_fileHashes[path]    = fileHash;
            return it->second;
        }

        std::shared_ptr<Texture> texture = LoadTextureOrMissing(path);
        m_textures[path]                 = texture;
        m_modifiedTimes[path]            = modifiedTime;
        m_fileSizes[path]                = fileSize;
        m_fileHashes[path]               = fileHash;
        return texture;
    }
    catch (...)
    {
        Forget(path);
        return GetMissingTexture();
    }
}

void TextureRepository::Forget(const std::string &path)
{
    m_textures.erase(path);
    m_modifiedTimes.erase(path);
    m_fileSizes.erase(path);
    m_fileHashes.erase(path);
}

#endif
