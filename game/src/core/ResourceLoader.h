#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class ResourceLoader
{
public:
    ResourceLoader();

    bool Update();
    float GetProgress() const;
    std::wstring GetStatus() const;

private:
    enum eSTAGE : uint8_t
    {
        eStage_Blocks    = 0,
        eStage_Items     = 1,
        eStage_Particles = 2,
        eStage_Biomes    = 3,
        eStage_Models    = 4,
        eStage_Entities  = 5,
        eStage_Textures  = 6,
        eStage_Sounds    = 7,
        eStage_Done      = 8,
    };

    static constexpr double kFrameBudgetMs   = 12.0;
    static constexpr int kRegistryStageCount = 6;

    void RunStep();
    void CollectTextures(const std::string &directory);

    eSTAGE m_stage;
    std::vector<std::string> m_texturePaths;
    size_t m_textureIndex;
    uint32_t m_soundIndex;
    uint32_t m_soundCount;
    std::string m_currentName;
};
