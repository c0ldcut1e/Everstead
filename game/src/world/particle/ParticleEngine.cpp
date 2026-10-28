#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ParticleEngine.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../../core/Everstead.h"
#include "../../rendering/ParticleAtlas.h"
#include "../../threading/ThreadStorage.h"
#include "../../utils/AABB.h"
#include "../../utils/Random.h"
#include "../../utils/math/Mth.h"
#include "../Level.h"
#include "ParticleRegistry.h"

double ParticleEngine::RandomCentered(Random *random, double spread) const
{
    if (spread == 0.0)
    {
        return 0.0;
    }

    return (random->NextDouble() * 2.0 - 1.0) * spread;
}

double ParticleEngine::RandomGaussian(Random *random) const
{
    double u = std::max(random->NextDouble(), 0.000001);
    double v = random->NextDouble();
    return sqrt(-2.0 * log(u)) * cos(2.0 * std::numbers::pi * v);
}

glm::dvec3 ParticleEngine::RandomSpread(Random *random, const glm::dvec3 &spread) const
{
    return glm::dvec3(RandomCentered(random, spread.x), RandomCentered(random, spread.y), RandomCentered(random, spread.z));
}

glm::dvec3 ParticleEngine::BuildTerrainVelocity(Random *random, const glm::dvec3 &velocity) const
{
    double x      = velocity.x + RandomCentered(random, 0.4);
    double y      = velocity.y + RandomCentered(random, 0.4);
    double z      = velocity.z + RandomCentered(random, 0.4);
    double speed  = (random->NextDouble() + random->NextDouble() + 1.0) * 0.15;
    double length = sqrt(x * x + y * y + z * z);
    if (length > 0.000001)
    {
        double scale = 0.4 * speed / length;
        x *= scale;
        y *= scale;
        z *= scale;
    }

    return glm::dvec3(x, y + 0.1, z);
}

float ParticleEngine::BuildTerrainSize(Random *random) const
{
    return (random->NextFloat() * 0.5f + 0.5f) * 0.1f;
}

int ParticleEngine::BuildTerrainLifetime(Random *random) const
{
    return std::max(1, (int) (4.0f / (random->NextFloat() * 0.9f + 0.1f)));
}

TextureAtlas::Region ParticleEngine::BuildTerrainRegion(Random *random, const TextureAtlas::Region &region) const
{
    float sampleU = random->NextFloat() * 3.0f;
    float sampleV = random->NextFloat() * 3.0f;
    float width   = (region.u1 - region.u0) * 0.25f;
    float height  = (region.v1 - region.v0) * 0.25f;
    float u0      = region.u0 + width * sampleU;
    float v0      = region.v0 + height * sampleV;
    float u1      = u0 + width * 0.999f;
    float v1      = v0 + height * 0.999f;
    return {u0, v0, u1, v1};
}

ParticleEngine::ParticleEngine(Level *level)
{
    m_level              = level;
    m_tickAccumulator    = 0.0;
    m_renderPartialTicks = 0.0f;
}

ParticleEngine::~ParticleEngine()
{}

void ParticleEngine::Tick(float delta)
{
    std::vector<Landing> landings;
    TickLocked(delta, &landings);
    PlayLandingSounds(landings);
}

void ParticleEngine::TickLocked(float delta, std::vector<Landing> *landings)
{
    static const double tickDelta = 1.0 / 20.0;
    static const int maxSteps     = 8;

    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (delta > 0.0f)
    {
        m_tickAccumulator += (double) delta;
    }

    if (m_particles.empty() && m_pendingSpawns.empty())
    {
        m_tickAccumulator    = 0.0;
        m_renderPartialTicks = 0.0f;
        return;
    }

    int steps = (int) floor(m_tickAccumulator / tickDelta);
    if (steps > maxSteps)
    {
        steps = maxSteps;
    }

    if (steps <= 0 && m_pendingSpawns.empty())
    {
        m_renderPartialTicks = (float) std::clamp(m_tickAccumulator / tickDelta, 0.0, 1.0);
        return;
    }

    std::vector<ParticleSpawnParams> spawns;
    spawns.swap(m_pendingSpawns);
    SpawnParticles(spawns, &m_particles);
    size_t limit = (size_t) Everstead::GetInstance()->options.particleLimit;
    if (m_particles.size() > limit)
    {
        m_particles.erase(m_particles.begin(), m_particles.begin() + (m_particles.size() - limit));
    }

    if (steps > 0)
    {
        m_tickAccumulator -= (double) steps * tickDelta;
        UpdateParticles(steps, &m_particles, landings);
    }

    m_renderPartialTicks = (float) std::clamp(m_tickAccumulator / tickDelta, 0.0, 1.0);
    BuildRenderParticles(m_particles, &m_renderParticles);
}

void ParticleEngine::Clear()
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_particles.clear();
    m_renderParticles.clear();
    m_pendingSpawns.clear();
    m_tickAccumulator    = 0.0;
    m_renderPartialTicks = 0.0f;
}

bool ParticleEngine::ShouldSkipSpawn()
{
    int setting = Everstead::GetInstance()->options.particles;
    return setting == eParticleSetting_Minimal || (setting == eParticleSetting_Decreased && m_spawnRandom.NextInt(0, 2) == 0);
}

void ParticleEngine::Spawn(const ParticleSpawnParams &params)
{
    if (ShouldSkipSpawn())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_pendingSpawns.push_back(params);
}

void ParticleEngine::Spawn(const std::vector<ParticleSpawnParams> &spawns)
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    for (size_t i = 0; i < spawns.size(); i++)
    {
        if (!ShouldSkipSpawn())
        {
            m_pendingSpawns.push_back(spawns[i]);
        }
    }
}

void ParticleEngine::CopyRenderParticles(std::vector<RenderParticle> *out) const
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    out->assign(m_renderParticles.begin(), m_renderParticles.end());
}

float ParticleEngine::GetRenderPartialTicks() const
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_renderPartialTicks;
}

size_t ParticleEngine::GetParticleCount() const
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_particles.size() + m_pendingSpawns.size();
}

void ParticleEngine::SpawnParticles(const std::vector<ParticleSpawnParams> &spawns, std::vector<Particle> *particles) const
{
    static thread_local Random random;

    for (const ParticleSpawnParams &params : spawns)
    {
        const ParticleDefinition *definition = ParticleRegistry::GetDefinition(params.type);
        if (!definition)
        {
            continue;
        }

        int amount                  = std::max(params.amount, 1);
        bool terrainStyle           = definition->terrain;
        bool depthSuspend           = params.type == eParticleType_DepthSuspend;
        bool happyVillager          = params.type == eParticleType_HappyVillager;
        bool hasRegion              = terrainStyle ? params.hasRegion : ParticleAtlas::HasRegion(params.type);
        TextureAtlas::Region region = terrainStyle ? params.region : ParticleAtlas::GetRegion(params.type);
        for (int i = 0; i < amount; i++)
        {
            glm::dvec3 spawnOffset = RandomSpread(&random, params.spread);
            glm::dvec3 velocity    = params.hasVelocity ? params.velocity : definition->velocity;
            if (!params.hasVelocity)
            {
                velocity += RandomSpread(&random, definition->velocitySpread);
            }

            Particle particle;
            particle.type        = params.type;
            particle.position    = params.position + spawnOffset;
            particle.oldPosition = particle.position;
            particle.velocity    = terrainStyle ? BuildTerrainVelocity(&random, velocity) : velocity;
            if (params.dustSpeed > 0.0)
            {
                particle.velocity = glm::dvec3(RandomGaussian(&random) * params.dustSpeed, RandomGaussian(&random) * params.dustSpeed,
                                               RandomGaussian(&random) * params.dustSpeed);
            }
            particle.color     = params.hasColor ? params.color : (terrainStyle ? 0xFF999999 : definition->color);
            particle.size      = (terrainStyle ? BuildTerrainSize(&random) : definition->size) * params.scale;
            particle.age       = 0.0f;
            particle.lifetime  = terrainStyle ? BuildTerrainLifetime(&random) : definition->lifetime + random.NextInt(0, definition->lifetimeSpread);
            particle.gravity   = definition->gravity;
            particle.drag      = definition->drag;
            particle.lit       = definition->lit;
            particle.collides  = definition->collides;
            particle.onGround  = false;
            particle.terrain   = terrainStyle;
            particle.region    = terrainStyle && hasRegion ? BuildTerrainRegion(&random, region) : region;
            particle.hasRegion = hasRegion;
            particle.velocity  = glm::dvec3(particle.velocity.x * params.power, (particle.velocity.y - 0.1) * params.power + 0.1,
                                            particle.velocity.z * params.power);
            if (depthSuspend)
            {
                uint32_t gray     = (uint32_t) ((random.NextFloat() * 0.1f + 0.2f) * 255.0f);
                particle.color    = 0xFF000000 | (gray << 16) | (gray << 8) | gray;
                particle.size     = definition->size * (random.NextFloat() * 0.5f + 0.5f) * (random.NextFloat() * 0.6f + 0.5f);
                particle.lifetime = (int) (20.0 / (random.NextDouble() * 0.8 + 0.2));
                particle.velocity *= 0.02;
            }

            if (happyVillager)
            {
                glm::dvec3 motion =
                        velocity +
                        glm::dvec3(random.NextDouble() * 2.0 - 1.0, random.NextDouble() * 2.0 - 1.0, random.NextDouble() * 2.0 - 1.0) * 0.4;
                double speed  = (random.NextDouble() + random.NextDouble() + 1.0) * 0.15;
                double length = glm::length(motion);
                if (length > 0.0)
                {
                    motion *= (speed * 0.4 / length);
                }

                motion.y += 0.1;
                particle.velocity = motion * 0.02;
                particle.size     = definition->size * (random.NextFloat() * 0.5f + 0.5f) * (random.NextFloat() * 0.6f + 0.5f);
                particle.lifetime = (int) (20.0 / (random.NextDouble() * 0.8 + 0.2));
            }

            if (params.type == eParticleType_ItemCrack)
            {
                particle.velocity = BuildTerrainVelocity(&random, glm::dvec3()) * 0.1 + velocity;
                particle.color    = 0xFFFFFFFF;
            }

            if (params.type == eParticleType_Crit)
            {
                uint32_t gray     = (uint32_t) ((random.NextFloat() * 0.3f + 0.6f) * 255.0f);
                particle.color    = 0xFF000000 | (gray << 16) | (gray << 8) | gray;
                particle.velocity = velocity * 0.4;
                particle.size     = definition->size * (random.NextFloat() * 0.5f + 0.5f);
                particle.lifetime = (int) (6.0 / (random.NextDouble() * 0.8 + 0.6));
            }

            if (params.type == eParticleType_Explode)
            {
                uint32_t gray  = (uint32_t) ((random.NextFloat() * 0.3f + 0.7f) * 255.0f);
                particle.color = 0xFF000000 | (gray << 16) | (gray << 8) | gray;
                particle.velocity =
                        velocity +
                        glm::dvec3(random.NextFloat() * 2.0f - 1.0f, random.NextFloat() * 2.0f - 1.0f, random.NextFloat() * 2.0f - 1.0f) * 0.05;
                particle.size     = definition->size * (random.NextFloat() * random.NextFloat() * 6.0f + 1.0f);
                particle.lifetime = (int) (16.0 / (random.NextDouble() * 0.8 + 0.2)) + 2;
            }

            if (params.type == eParticleType_Smoke)
            {
                glm::dvec3 motion =
                        velocity +
                        glm::dvec3(random.NextDouble() * 2.0 - 1.0, random.NextDouble() * 2.0 - 1.0, random.NextDouble() * 2.0 - 1.0) * 0.4;
                double speed  = (random.NextDouble() + random.NextDouble() + 1.0) * 0.15;
                double length = glm::length(motion);
                if (length > 0.0)
                {
                    motion *= (speed * 0.4 / length);
                }

                motion.y += 0.1;
                uint32_t gray     = (uint32_t) (random.NextFloat() * 0.3f * 255.0f);
                particle.color    = 0xFF000000 | (gray << 16) | (gray << 8) | gray;
                particle.velocity = motion * 0.1 + velocity;
                particle.size     = definition->size * (random.NextFloat() * 0.5f + 0.5f) * 2.0f;
                particle.lifetime = (int) (8.0 / (random.NextDouble() * 0.8 + 0.2));
            }

            if (params.type == eParticleType_LargeExplode)
            {
                uint32_t gray     = (uint32_t) ((random.NextFloat() * 0.6f + 0.4f) * 255.0f);
                particle.color    = 0xFF000000 | (gray << 16) | (gray << 8) | gray;
                particle.size     = definition->size * (1.0f - (float) velocity.x * 0.5f);
                particle.velocity = glm::dvec3(0.0, 0.0, 0.0);
                particle.lifetime = 6 + random.NextInt(0, 3);
            }

            // The note particle carries its pitch in the x velocity so the server can send it like any other particle
            if (params.type == eParticleType_Note)
            {
                double note       = velocity.x * std::numbers::pi * 2.0;
                uint32_t red      = (uint32_t) (std::max(0.0, sin(note) * 0.65 + 0.35) * 255.0);
                uint32_t green    = (uint32_t) (std::max(0.0, sin(note + std::numbers::pi * 2.0 / 3.0) * 0.65 + 0.35) * 255.0);
                uint32_t blue     = (uint32_t) (std::max(0.0, sin(note + std::numbers::pi * 4.0 / 3.0) * 0.65 + 0.35) * 255.0);
                particle.color    = 0xFF000000 | (red << 16) | (green << 8) | blue;
                particle.velocity = definition->velocity;
            }

            if (params.type == eParticleType_HugeExplosion)
            {
                particle.color = 0x00FFFFFF;
            }

            if (particle.lifetime <= 0 || particle.size <= 0.0f)
            {
                continue;
            }

            particles->push_back(particle);
        }
    }
}

void ParticleEngine::TransformParticle(Particle *particle, ePARTICLE_TYPE type) const
{
    const ParticleDefinition *definition = ParticleRegistry::GetDefinition(type);
    particle->type                       = type;
    particle->velocity                   = glm::dvec3(0.0, 0.0, 0.0);
    particle->age                        = 0.0f;
    particle->lifetime                   = definition->lifetime;
    particle->size                       = definition->size;
    particle->gravity                    = definition->gravity;
    particle->drag                       = definition->drag;
    particle->lit                        = definition->lit;
    particle->collides                   = definition->collides;
    particle->region                     = ParticleAtlas::GetRegion(type);
    particle->hasRegion                  = ParticleAtlas::HasRegion(type);
}

void ParticleEngine::TickParticles(std::vector<Particle> *particles, std::vector<Landing> *landings) const
{
    static thread_local Random random;

    std::vector<ParticleSpawnParams> emitted;
    size_t alive = 0;
    for (size_t i = 0; i < particles->size(); i++)
    {
        Particle &particle                   = (*particles)[i];
        const ParticleDefinition *definition = ParticleRegistry::GetDefinition(particle.type);
        particle.oldPosition                 = particle.position;
        particle.age += 1.0f;
        if (particle.age >= (float) particle.lifetime)
        {
            if (!definition || definition->expireType == eParticleType_NotSet)
            {
                continue;
            }

            TransformParticle(&particle, definition->expireType);
            definition = ParticleRegistry::GetDefinition(particle.type);
        }

        if (particle.type == eParticleType_HugeExplosion)
        {
            for (int j = 0; j < 6; j++)
            {
                ParticleSpawnParams params;
                params.type = eParticleType_LargeExplode;
                params.position =
                        particle.position + (glm::dvec3(random.NextDouble() - random.NextDouble(), random.NextDouble() - random.NextDouble(),
                                                        random.NextDouble() - random.NextDouble()) *
                                             4.0);
                params.velocity    = glm::dvec3((particle.age - 1.0f) / 8.0f, 0.0, 0.0);
                params.hasVelocity = true;
                emitted.push_back(params);
            }
        }

        if (particle.type == eParticleType_Crit)
        {
            uint32_t green = (uint32_t) ((float) ((particle.color >> 8) & 0xFF) * 0.96f * 0.96f);
            uint32_t blue  = (uint32_t) ((float) (particle.color & 0xFF) * 0.9f * 0.9f);
            particle.color = (particle.color & 0xFFFF0000) | (green << 8) | blue;
        }

        particle.velocity.y -= (double) particle.gravity * 0.04;
        if (particle.collides && m_level)
        {
            MoveParticle(&particle, 1.0);
        }
        else
        {
            particle.position += particle.velocity;
            particle.onGround = false;
        }

        if (particle.onGround && definition && definition->landType != eParticleType_NotSet)
        {
            landings->push_back({particle.type, particle.position});
            TransformParticle(&particle, definition->landType);
        }

        particle.velocity *= (double) particle.drag;
        if (particle.onGround)
        {
            particle.velocity.x *= 0.7;
            particle.velocity.z *= 0.7;
        }

        if (alive != i)
        {
            (*particles)[alive] = particle;
        }

        alive++;
    }

    particles->resize(alive);
    SpawnParticles(emitted, particles);
}

void ParticleEngine::UpdateParticles(int steps, std::vector<Particle> *particles, std::vector<Landing> *landings) const
{
    for (int i = 0; i < steps; i++)
    {
        if (particles->empty())
        {
            break;
        }

        TickParticles(particles, landings);
    }
}

// Sounds play outside the state lock since the sound system may call back into the level
void ParticleEngine::PlayLandingSounds(const std::vector<Landing> &landings)
{
    if (!m_level)
    {
        return;
    }

    for (size_t i = 0; i < landings.size(); i++)
    {
        const ParticleDefinition *definition = ParticleRegistry::GetDefinition(landings[i].type);
        if (!definition || definition->landSound.empty() || definition->landSoundVariants <= 0)
        {
            continue;
        }

        std::string sound = definition->landSound + std::to_string(m_spawnRandom.NextInt(1, definition->landSoundVariants));
        m_level->PlaySound(sound, landings[i].position, 0.3f + m_spawnRandom.NextFloat() * 0.3f, 0.8f + m_spawnRandom.NextFloat() * 0.4f);
    }
}

void ParticleEngine::BuildRenderParticles(const std::vector<Particle> &particles, std::vector<RenderParticle> *renderParticles) const
{
    renderParticles->clear();
    renderParticles->reserve(particles.size());

    for (const Particle &particle : particles)
    {
        uint8_t a = (uint8_t) ((particle.color >> 24) & 0xFF);
        if (a == 0)
        {
            continue;
        }

        RenderParticle renderParticle;
        renderParticle.type        = particle.type;
        renderParticle.position    = particle.position;
        renderParticle.oldPosition = particle.oldPosition;
        renderParticle.color       = (particle.color & 0x00FFFFFF) | ((uint32_t) a << 24);
        renderParticle.size        = particle.size;
        if (particle.type == eParticleType_Smoke)
        {
            renderParticle.size *= std::clamp(particle.age / (float) particle.lifetime * 32.0f, 0.0f, 1.0f);
        }

        renderParticle.lit       = particle.lit;
        renderParticle.terrain   = particle.terrain;
        renderParticle.region    = particle.region;
        renderParticle.hasRegion = particle.hasRegion;
        int frames               = ParticleRegistry::GetDefinition(particle.type)->frames;
        if (frames > 1)
        {
            int frame                = frames - 1 - (int) (particle.age * (float) frames / (float) particle.lifetime);
            float frameWidth         = (particle.region.u1 - particle.region.u0) / (float) frames;
            renderParticle.region.u0 = particle.region.u0 + frameWidth * (float) std::max(frame, 0);
            renderParticle.region.u1 = renderParticle.region.u0 + frameWidth;
        }

        renderParticles->push_back(renderParticle);
    }
}

void ParticleEngine::MoveParticle(Particle *particle, double deltaTime) const
{
    if (!particle || !m_level)
    {
        return;
    }

    particle->onGround = false;

    double movementX = particle->velocity.x * deltaTime;
    double movementY = particle->velocity.y * deltaTime;
    double movementZ = particle->velocity.z * deltaTime;

    double radius = std::max((double) particle->size * 0.5, 0.01);
    AABB aabb(glm::dvec3(particle->position.x - radius, particle->position.y - radius, particle->position.z - radius),
              glm::dvec3(particle->position.x + radius, particle->position.y + radius, particle->position.z + radius));

    const double maxStep = 1.0 / 16.0;

    double remainingY = movementY;
    while (remainingY != 0.0)
    {
        double step = remainingY;
        if (fabs(step) > maxStep)
        {
            step = step > 0.0 ? maxStep : -maxStep;
        }

        AABB test = aabb.Translated(glm::dvec3(0.0, step, 0.0));
        if (m_level->IntersectsBlock(test))
        {
            if (step < 0.0)
            {
                particle->onGround = true;
            }

            particle->velocity.y = 0.0;
            remainingY           = 0.0;
            break;
        }

        particle->position.y += step;
        aabb = test;
        remainingY -= step;
    }

    if (particle->onGround && particle->velocity.y < 0.0)
    {
        particle->velocity.y = 0.0;
    }

    double remainingX = movementX;
    while (remainingX != 0.0)
    {
        double step = remainingX;
        if (fabs(step) > maxStep)
        {
            step = step > 0.0 ? maxStep : -maxStep;
        }

        AABB test = aabb.Translated(glm::dvec3(step, 0.0, 0.0));
        if (m_level->IntersectsBlock(test))
        {
            particle->velocity.x = 0.0;
            remainingX           = 0.0;
            break;
        }

        particle->position.x += step;
        aabb = test;
        remainingX -= step;
    }

    double remainingZ = movementZ;
    while (remainingZ != 0.0)
    {
        double step = remainingZ;
        if (fabs(step) > maxStep)
        {
            step = step > 0.0 ? maxStep : -maxStep;
        }

        AABB test = aabb.Translated(glm::dvec3(0.0, 0.0, step));
        if (m_level->IntersectsBlock(test))
        {
            particle->velocity.z = 0.0;
            remainingZ           = 0.0;
            break;
        }

        particle->position.z += step;
        aabb = test;
        remainingZ -= step;
    }
}

#endif
