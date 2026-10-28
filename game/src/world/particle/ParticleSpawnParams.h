#pragma once

#include <cstdint>

#include "../../rendering/TextureAtlas.h"
#include "ParticleType.h"

#include <glm/vec3.hpp>

struct ParticleSpawnParams
{
    ParticleSpawnParams()
    {
        type        = eParticleType_NotSet;
        position    = {};
        spread      = {};
        velocity    = {};
        region      = {};
        color       = 0xFFFFFFFF;
        amount      = 1;
        dustSpeed   = 0.0;
        power       = 1.0;
        scale       = 1.0f;
        hasVelocity = false;
        hasRegion   = false;
        hasColor    = false;
    }

    ePARTICLE_TYPE type;
    glm::dvec3 position;
    glm::dvec3 spread;
    glm::dvec3 velocity;
    TextureAtlas::Region region;
    uint32_t color;
    int amount;
    double dustSpeed;
    double power;
    float scale;
    bool hasVelocity;
    bool hasRegion;
    bool hasColor;
};
