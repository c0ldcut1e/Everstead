#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "Texture.h"

class TextureRepository
{
public:
    static TextureRepository *GetInstance();

    TextureRepository();

    static BufferedImage CreateMissingImage();

    std::shared_ptr<Texture> Get(const std::string &path);
    std::shared_ptr<Texture> GetSized(const std::string &path, int pixelWidth, int pixelHeight);
    std::shared_ptr<Texture> GetPlayerSkin(const std::string &path);
    std::shared_ptr<Texture> GetDynamic(const std::string &path);
    Texture *GetRaw(const std::string &path);
    void Clear();
    void Invalidate(const std::string &path);

private:
    int64_t GetModifiedTime(const std::string &path) const;
    uint64_t GetFileSize(const std::string &path) const;
    uint64_t GetFileHash(const std::string &path) const;
    bool HasFileChanged(const std::string &path, int64_t modifiedTime, uint64_t fileSize,
                        uint64_t fileHash) const;
    std::shared_ptr<Texture> GetMissingTexture();
    std::shared_ptr<Texture> LoadTextureOrMissing(const std::string &path);
    std::shared_ptr<Texture> Reload(const std::string &path, int64_t modifiedTime,
                                    uint64_t fileSize, uint64_t fileHash);
    void Forget(const std::string &path);

    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
    std::unordered_map<std::string, int64_t> m_modifiedTimes;
    std::unordered_map<std::string, uint64_t> m_fileSizes;
    std::unordered_map<std::string, uint64_t> m_fileHashes;
    std::shared_ptr<Texture> m_missingTexture;
};
