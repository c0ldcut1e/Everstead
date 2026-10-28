#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ResourceLoader.h"

#include <algorithm>

#include <SDL2/SDL.h>

#include "../entity/EntityFactory.h"
#include "../entity/EntityRendererRegistry.h"
#include "../io/File.h"
#include "../rendering/BlockAtlas.h"
#include "../rendering/ItemAtlas.h"
#include "../rendering/ParticleAtlas.h"
#include "../rendering/TextureRepository.h"
#include "../sound/SoundEngine.h"
#include "../sound/SoundEvent.h"
#include "../sound/SoundRegistry.h"
#include "../utils/StringUtils.h"
#include "../world/biome/BiomeRegistry.h"
#include "../world/block/BlockRegistry.h"
#include "../world/blockentity/renderer/BlockEntityRendererRegistry.h"
#include "../world/crafting/FurnaceRecipes.h"
#include "../world/crafting/Recipes.h"
#include "../world/item/ItemRegistry.h"
#include "../world/loot/LootTables.h"
#include "../world/models/ModelRegistry.h"
#include "../world/particle/ParticleRegistry.h"
#include "Everstead.h"
#include "Logger.h"

ResourceLoader::ResourceLoader()
{
    m_stage        = eStage_Blocks;
    m_texturePaths = {};
    m_textureIndex = 0;
    m_soundIndex   = 0;
    m_soundCount   = SoundRegistry::Size();
    m_currentName  = "";
    CollectTextures("assets/texture");
}

bool ResourceLoader::Update()
{
    uint64_t start     = SDL_GetPerformanceCounter();
    uint64_t frequency = SDL_GetPerformanceFrequency();
    while (m_stage != eStage_Done)
    {
        RunStep();
        if ((double) (SDL_GetPerformanceCounter() - start) * 1000.0 / (double) frequency >= kFrameBudgetMs)
        {
            break;
        }
    }

    return m_stage == eStage_Done;
}

float ResourceLoader::GetProgress() const
{
    float registries = (float) std::min((int) m_stage, kRegistryStageCount);
    float textures   = m_texturePaths.empty() ? 1.0f : (float) m_textureIndex / (float) m_texturePaths.size();
    float sounds     = m_soundCount == 0 ? 1.0f : (float) m_soundIndex / (float) m_soundCount;
    return (registries + textures + sounds) / (float) (kRegistryStageCount + 2);
}

std::wstring ResourceLoader::GetStatus() const
{
    static const char *stageKeys[] = {"loading.game.blocks",   "loading.game.items",  "loading.game.particles",
                                      "loading.game.biomes",   "loading.game.models", "loading.game.entities",
                                      "loading.game.textures", "loading.game.sounds", "loading.game.done"};

    std::wstring status = Everstead::GetInstance()->GetString(stageKeys[m_stage]);
    if (m_stage == eStage_Textures)
    {
        status += L" (" + std::to_wstring(m_textureIndex) + L"/" + std::to_wstring(m_texturePaths.size()) + L")";
    }
    else if (m_stage == eStage_Sounds)
    {
        status += L" (" + std::to_wstring(m_soundIndex) + L"/" + std::to_wstring(m_soundCount) + L")";
    }

    if (!m_currentName.empty())
    {
        status += L": " + StringUtils::ToUtf16(m_currentName);
    }

    return status;
}

void ResourceLoader::RunStep()
{
    m_currentName.clear();
    switch (m_stage)
    {
        case eStage_Blocks:
            BlockRegistry::Init();
            BlockAtlas::GetInstance()->Build();
            break;
        case eStage_Items:
            ItemRegistry::Init();
            Recipes::Init();
            FurnaceRecipes::Init();
            LootTables::Init();
            ItemAtlas::GetInstance()->Build();
            break;
        case eStage_Particles:
            ParticleRegistry::Init();
            ParticleAtlas::Rebuild();
            break;
        case eStage_Biomes:
            BiomeRegistry::Init();
            break;
        case eStage_Models:
            ModelRegistry::Init();
            break;
        case eStage_Entities:
            EntityFactory::Init();
            EntityRendererRegistry::Init();
            BlockEntityRendererRegistry::Init();
            break;
        case eStage_Textures:
            if (m_textureIndex < m_texturePaths.size())
            {
                m_currentName = m_texturePaths[m_textureIndex];
                TextureRepository::GetInstance()->GetRaw(m_currentName);
                Logger::LogInfo("Texture loaded path=%s", m_currentName.c_str());
                m_textureIndex++;
                return;
            }

            break;
        case eStage_Sounds:
            if (m_soundIndex < m_soundCount)
            {
                SoundEvent *sound = SoundRegistry::ById(m_soundIndex);
                m_currentName     = sound->GetFile().GetPath();
                Everstead::GetInstance()->GetSoundEngine()->LoadBuffer(sound);
                m_soundIndex++;
                return;
            }

            break;
        case eStage_Done:
            return;
    }

    static const char *stageNames[] = {"blocks", "items", "particles", "biomes", "models", "entities", "textures", "sounds"};
    Logger::LogInfo("Finished loading %s", stageNames[m_stage]);
    m_stage = (eSTAGE) (m_stage + 1);
}

void ResourceLoader::CollectTextures(const std::string &directory)
{
    std::vector<File> files = File(directory).ListFiles();
    for (size_t i = 0; i < files.size(); i++)
    {
        std::string path = files[i].GetPath();
        std::replace(path.begin(), path.end(), '\\', '/');
        // Atlas sources are read straight into their atlas, so preloading them would only make unused GL textures
        if (path == "assets/texture/ui")
        {
            CollectTextures("assets/texture/ui/panorama");
        }
        else if (path == "assets/texture/block" || path == "assets/texture/item" || path == "assets/texture/particle")
        {
            continue;
        }
        else if (files[i].IsDirectory())
        {
            CollectTextures(path);
        }
        else if (files[i].GetExtension() == ".png")
        {
            m_texturePaths.push_back(path);
        }
    }
}

#endif
