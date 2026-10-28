#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ParticleAtlas.h"

#include <algorithm>
#include <string>
#include <vector>

#include "../world/particle/ParticleRegistry.h"

std::unordered_map<ePARTICLE_TYPE, TextureAtlas::Region, ParticleTypeHash> ParticleAtlas::s_regions;
TextureAtlas ParticleAtlas::s_atlas;

void ParticleAtlas::Rebuild()
{
    s_regions.clear();

    const std::vector<ParticleDefinition> &definitions = ParticleRegistry::GetDefinitions();
    std::vector<std::string> paths;
    paths.reserve(definitions.size());

    for (size_t i = 0; i < definitions.size(); i++)
    {
        const ParticleDefinition &definition = definitions[i];
        if (definition.texturePath.empty())
        {
            continue;
        }

        if (std::find(paths.begin(), paths.end(), definition.texturePath) == paths.end())
        {
            paths.push_back(definition.texturePath);
        }
    }

    s_atlas.Build(paths);

    for (size_t i = 0; i < definitions.size(); i++)
    {
        const ParticleDefinition &definition = definitions[i];
        if (!definition.texturePath.empty() && s_atlas.Has(definition.texturePath))
        {
            s_regions[definition.type] = s_atlas.Get(definition.texturePath);
        }
    }
}

bool ParticleAtlas::HasRegion(ePARTICLE_TYPE type)
{
    return s_regions.find(type) != s_regions.end();
}

TextureAtlas::Region ParticleAtlas::GetRegion(ePARTICLE_TYPE type)
{
    auto it = s_regions.find(type);
    if (it == s_regions.end())
    {
        return TextureAtlas::Region{0.0f, 0.0f, 1.0f, 1.0f};
    }

    return it->second;
}

Texture *ParticleAtlas::GetTexture()
{
    return s_atlas.GetTexture();
}

#endif
