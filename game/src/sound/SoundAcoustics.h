#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <glm/vec3.hpp>

class Level;

struct AcousticEnvironment
{
    float enclosure;
    float reverbGain;
    float decayTime;
    float reflectionsDelay;
    float lateDelay;
    float echoGain;
    float echoDelay;
};

class SoundAcoustics
{
public:
    SoundAcoustics();

    AcousticEnvironment TraceEnvironment(const Level *level, const glm::dvec3 &listener);
    float TraceOcclusion(const Level *level, const glm::dvec3 &source, const glm::dvec3 &listener);

private:
    bool IsSolid(const Level *level, const glm::ivec3 &cell);
    double GetReflectivity(const Level *level, const glm::dvec3 &hit, const glm::dvec3 &normal);
    bool Trace(const Level *level, const glm::dvec3 &from, const glm::dvec3 &direction, double maxDistance, glm::dvec3 *hit, glm::dvec3 *normal);
    int CountSolid(const Level *level, const glm::dvec3 &from, const glm::dvec3 &to);

    std::vector<glm::dvec3> m_directions;
    std::unordered_map<uint64_t, bool> m_blocksSound;
    int m_traceCount;
};
