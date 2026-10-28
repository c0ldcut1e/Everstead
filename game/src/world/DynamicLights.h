#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

class DynamicLights
{
public:
    static constexpr size_t kMaxLights     = 32;
    static constexpr int kMaxSuns          = 3;
    static constexpr size_t kMaxFogVolumes = 8;

    enum eLIGHT_TYPE : int
    {
        eLightType_Point = 0,
        eLightType_Spot,
        eLightType_Directional
    };

    struct Light
    {
        uint32_t id;
        eLIGHT_TYPE type;
        glm::dvec3 position;
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
        float radius;
        float coneAngle;
        float size;
        bool volumetric;
        bool occlusion;
    };

    enum eFOG_SHAPE : int
    {
        eFogShape_Box = 0,
        eFogShape_Sphere
    };

    struct FogVolume
    {
        uint32_t id;
        eFOG_SHAPE shape;
        glm::dvec3 position;
        glm::vec3 extent;
        glm::vec3 color;
        float density;
        float edge;
        float noise;
        float drift;
        float ambient;
    };

    DynamicLights();

    const std::vector<Light> &GetLights() const;
    const Light *GetLight(uint32_t id) const;
    bool SetLight(const Light &light);
    void RemoveLight(uint32_t id);

    const std::vector<FogVolume> &GetFogVolumes() const;
    const FogVolume *GetFogVolume(uint32_t id) const;
    bool SetFogVolume(const FogVolume &fog);
    void RemoveFogVolume(uint32_t id);

    float GetAirDensity() const;
    float GetAnisotropy() const;
    void SetAtmosphere(float airDensity, float anisotropy);

    void Clear();

private:
    std::vector<Light> m_lights;
    std::vector<FogVolume> m_fogVolumes;
    float m_airDensity;
    float m_anisotropy;
};
