#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../core/ResourceLocation.h"
#include "ParticleType.h"

#include <glm/vec3.hpp>

struct ParticleDefinition
{
    ePARTICLE_TYPE type;
    ResourceLocation location;
    std::string texturePath;
    glm::dvec3 velocity;
    glm::dvec3 velocitySpread;
    float size;
    int lifetime;
    int lifetimeSpread = 0;
    float gravity;
    float drag;
    bool lit;
    bool collides;
    bool terrain;
    ePARTICLE_TYPE expireType = eParticleType_NotSet;
    ePARTICLE_TYPE landType   = eParticleType_NotSet;
    std::string landSound;
    int landSoundVariants = 0;
    int frames            = 1;
    uint32_t color        = 0xFFFFFFFF;
};

class ParticleRegistry
{
public:
    static void Init();
    static void Clear();

    static const ParticleDefinition *GetDefinition(ePARTICLE_TYPE type);
    static const std::vector<ParticleDefinition> &GetDefinitions();
    static bool IsTerrain(ePARTICLE_TYPE type);

private:
    static void Register(const ParticleDefinition &definition);
    static void RegisterDrip(ePARTICLE_TYPE hangType, ePARTICLE_TYPE fallType, ePARTICLE_TYPE landType, const std::string &name,
                             const std::string &landSound, int landSoundVariants, bool glowing);

    static std::vector<ParticleDefinition> s_definitions;
};
