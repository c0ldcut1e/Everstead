#pragma once

#include <mutex>
#include <vector>

#include "../../rendering/TextureAtlas.h"
#include "../../utils/Random.h"
#include "ParticleSpawnParams.h"

#include <glm/vec3.hpp>

class Level;

class ParticleEngine
{
public:
    explicit ParticleEngine(Level *level);

    struct RenderParticle
    {
        ePARTICLE_TYPE type;
        glm::dvec3 position;
        glm::dvec3 oldPosition;
        uint32_t color;
        float size;
        bool lit;
        bool terrain;
        TextureAtlas::Region region;
        bool hasRegion;
    };

    ~ParticleEngine();

    void Tick(float delta);
    void Clear();

    void Spawn(const ParticleSpawnParams &params);
    void Spawn(const std::vector<ParticleSpawnParams> &spawns);
    void CopyRenderParticles(std::vector<RenderParticle> *out) const;
    float GetRenderPartialTicks() const;
    size_t GetParticleCount() const;

private:
    double RandomCentered(Random *random, double spread) const;
    double RandomGaussian(Random *random) const;
    glm::dvec3 RandomSpread(Random *random, const glm::dvec3 &spread) const;
    glm::dvec3 BuildTerrainVelocity(Random *random, const glm::dvec3 &velocity) const;
    float BuildTerrainSize(Random *random) const;
    int BuildTerrainLifetime(Random *random) const;
    TextureAtlas::Region BuildTerrainRegion(Random *random, const TextureAtlas::Region &region) const;
    bool ShouldSkipSpawn();

    struct Landing
    {
        ePARTICLE_TYPE type;
        glm::dvec3 position;
    };

    void TickLocked(float delta, std::vector<Landing> *landings);
    void PlayLandingSounds(const std::vector<Landing> &landings);

    struct Particle
    {
        ePARTICLE_TYPE type;
        glm::dvec3 position;
        glm::dvec3 oldPosition;
        glm::dvec3 velocity;
        uint32_t color;
        float size;
        float age;
        int lifetime;
        float gravity;
        float drag;
        bool lit;
        bool collides;
        bool onGround;
        bool terrain;
        TextureAtlas::Region region;
        bool hasRegion;
    };

    void MoveParticle(Particle *particle, double deltaTime) const;
    void SpawnParticles(const std::vector<ParticleSpawnParams> &spawns, std::vector<Particle> *particles) const;
    void TransformParticle(Particle *particle, ePARTICLE_TYPE type) const;
    void TickParticles(std::vector<Particle> *particles, std::vector<Landing> *landings) const;
    void UpdateParticles(int steps, std::vector<Particle> *particles, std::vector<Landing> *landings) const;
    void BuildRenderParticles(const std::vector<Particle> &particles, std::vector<RenderParticle> *renderParticles) const;

    Level *m_level;
    Random m_spawnRandom;
    mutable std::mutex m_stateMutex;

    std::vector<Particle> m_particles;
    std::vector<RenderParticle> m_renderParticles;
    std::vector<ParticleSpawnParams> m_pendingSpawns;
    double m_tickAccumulator;
    float m_renderPartialTicks;
};
