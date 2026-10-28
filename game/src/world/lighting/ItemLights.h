#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

class Level;
class Shader;

class ItemLights
{
public:
    struct Light
    {
        glm::dvec3 position;
        glm::dvec3 color;
    };

    void Collect(Level *level, const glm::dvec3 &cameraPos, float partialTicks, int setting);
    void SampleBlockLight(const glm::dvec3 &pos, uint8_t *r, uint8_t *g, uint8_t *b) const;
    void Bind(Shader *shader, const glm::dvec3 &cameraPos) const;
    const std::vector<Light> &GetLights() const;

private:
    static constexpr size_t kMaxLights = 32;

    std::vector<Light> m_lights;
};
