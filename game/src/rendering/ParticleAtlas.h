#pragma once

#include <unordered_map>

#include "../world/particle/ParticleType.h"
#include "TextureAtlas.h"

class ParticleAtlas
{
public:
    static void Rebuild();
    static bool HasRegion(ePARTICLE_TYPE type);
    static TextureAtlas::Region GetRegion(ePARTICLE_TYPE type);
    static Texture *GetTexture();

private:
    static std::unordered_map<ePARTICLE_TYPE, TextureAtlas::Region, ParticleTypeHash> s_regions;
    static TextureAtlas s_atlas;
};
