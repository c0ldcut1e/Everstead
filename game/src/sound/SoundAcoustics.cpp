#ifndef EVERSTEAD_DEDICATED_SERVER

#include "SoundAcoustics.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

#include "../utils/AABB.h"
#include "../world/Level.h"
#include "../world/block/Block.h"
#include "../world/block/BlockPos.h"
#include "../world/block/LeavesBlock.h"

#include <glm/geometric.hpp>

SoundAcoustics::SoundAcoustics()
{
    const int directionCount = 32;
    double goldenAngle       = std::numbers::pi * (3.0 - sqrt(5.0));
    for (int i = 0; i < directionCount; i++)
    {
        double y      = 1.0 - ((double) i + 0.5) * 2.0 / directionCount;
        double radius = sqrt(1.0 - y * y);
        double theta  = goldenAngle * i;
        m_directions.emplace_back(cos(theta) * radius, y, sin(theta) * radius);
    }

    m_traceCount = 0;
}

AcousticEnvironment SoundAcoustics::TraceEnvironment(const Level *level, const glm::dvec3 &listener)
{
    const int maxBounces      = 3;
    const double maxSegment   = 32.0;
    const double farDistance  = 16.0;
    const double speedOfSound = 343.0;

    // We spin the ray set a little every trace so thin gaps between rays get covered over time
    double angle = std::numbers::pi * (3.0 - sqrt(5.0)) * m_traceCount;
    double cosA  = cos(angle);
    double sinA  = sin(angle);
    m_traceCount++;

    double hits          = 0.0;
    double firstHitSum   = 0.0;
    double returned      = 0.0;
    double returnedPaths = 0.0;
    double farHits       = 0.0;
    double farHitSum     = 0.0;
    for (size_t i = 0; i < m_directions.size(); i++)
    {
        const glm::dvec3 &base = m_directions[i];
        glm::dvec3 direction(base.x * cosA - base.z * sinA, base.y, base.x * sinA + base.z * cosA);
        glm::dvec3 origin = listener;
        double traveled   = 0.0;
        double energy     = 1.0;
        for (int bounce = 0; bounce < maxBounces; bounce++)
        {
            glm::dvec3 hit;
            glm::dvec3 normal;
            if (!Trace(level, origin, direction, maxSegment, &hit, &normal))
            {
                break;
            }

            double reflectivity = GetReflectivity(level, hit, normal);
            energy *= reflectivity;
            if (energy <= 0.0)
            {
                break;
            }

            double segment = glm::distance(origin, hit);
            traveled += segment;
            if (bounce == 0)
            {
                hits += reflectivity;
                firstHitSum += segment * reflectivity;
                // Shallow rays reach the ground far away on flat land, so only rays aimed level or up count as echo walls
                if (segment >= farDistance && direction.y >= 0.0)
                {
                    farHits += reflectivity;
                    farHitSum += segment * reflectivity;
                }
            }

            glm::dvec3 bounceOrigin = hit + normal * 0.01;
            double toListener       = glm::distance(bounceOrigin, listener);
            glm::dvec3 blocked;
            if (!Trace(level, bounceOrigin, (listener - bounceOrigin) / toListener, toListener, &blocked, &blocked))
            {
                returned += energy;
                returnedPaths += (traveled + toListener) * energy;
            }

            direction = direction - 2.0 * glm::dot(direction, normal) * normal;
            origin    = bounceOrigin;
        }
    }

    double rayCount       = (double) m_directions.size();
    double roomSize       = hits > 0.0 ? firstHitSum / hits : 0.0;
    double averagePath    = returned > 0.0 ? returnedPaths / returned : 0.0;
    double returnFraction = returned / (rayCount * maxBounces);

    AcousticEnvironment environment;
    environment.enclosure = (float) (hits / rayCount);

    // Open ground already blocks the lower half of the rays, so only enclosure past that counts as a room
    double roomWeight            = std::clamp((environment.enclosure - 0.55) / 0.45, 0.0, 1.0);
    environment.reverbGain       = (float) (std::min(returnFraction * 1.5, 1.0) * roomWeight);
    environment.decayTime        = (float) std::clamp(0.4 + averagePath * 0.08, 0.4, 6.0);
    environment.reflectionsDelay = (float) std::clamp(roomSize * 2.0 / speedOfSound, 0.0, 0.3);
    environment.lateDelay        = (float) std::clamp(averagePath / speedOfSound, 0.0, 0.1);
    environment.echoGain         = (float) std::clamp(farHits * 2.0 / rayCount, 0.0, 1.0) * 0.5f;
    environment.echoDelay        = (float) std::clamp((farHits > 0.0 ? farHitSum / farHits : 0.0) * 2.0 / speedOfSound, 0.0, 0.207);
    return environment;
}

float SoundAcoustics::TraceOcclusion(const Level *level, const glm::dvec3 &source, const glm::dvec3 &listener)
{
    const int maxOcclusion = 8;
    const double offset    = 0.45;

    int occlusion = CountSolid(level, source, listener);

    // A few shifted paths let sound slip through doorways and gaps next to the source
    const glm::dvec3 offsets[] = {{offset, 0.0, 0.0}, {-offset, 0.0, 0.0}, {0.0, 0.0, offset}, {0.0, 0.0, -offset}, {0.0, offset, 0.0}};
    for (size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]) && occlusion > 0; i++)
    {
        occlusion = std::min(occlusion, CountSolid(level, source + offsets[i], listener));
    }

    return (float) std::min(occlusion, maxOcclusion);
}

bool SoundAcoustics::IsSolid(const Level *level, const glm::ivec3 &cell)
{
    BlockPos pos(cell.x, cell.y, cell.z);
    uint32_t id   = level->GetBlockId(pos);
    uint16_t data = level->GetBlockData(pos);
    uint64_t key  = (uint64_t) id << 16 | data;
    auto cached   = m_blocksSound.find(key);
    if (cached != m_blocksSound.end())
    {
        return cached->second;
    }

    // Sound passes anything whose collision boxes don't cover most of a cell face, and leaves since they scatter it instead of reflecting
    Block *block = Block::ById(id);
    int open     = block->FindProperty("open");
    bool blocks  = false;
    if (block->IsCollidable() && !LeavesBlock::IsLeaves(block) && (open < 0 || block->GetPropertyValue(data, open) == 0))
    {
        std::vector<AABB> boxes;
        block->GetCollisionBoxes(data, &boxes);
        for (size_t i = 0; i < boxes.size() && !blocks; i++)
        {
            glm::dvec3 size = boxes[i].GetMax() - boxes[i].GetMin();
            blocks          = std::max({size.x * size.y, size.y * size.z, size.x * size.z}) >= 0.75;
        }
    }

    m_blocksSound[key] = blocks;
    return blocks;
}

// Thin things like tree trunks scatter sound instead of reflecting it, so reflectivity scales with how much of the 3x3 patch around the hit is solid
double SoundAcoustics::GetReflectivity(const Level *level, const glm::dvec3 &hit, const glm::dvec3 &normal)
{
    glm::ivec3 axisNormal((int) normal.x, (int) normal.y, (int) normal.z);
    glm::ivec3 cell((int) floor(hit.x - normal.x * 0.5), (int) floor(hit.y - normal.y * 0.5), (int) floor(hit.z - normal.z * 0.5));
    glm::ivec3 tangentA = axisNormal.x != 0 ? glm::ivec3(0, 1, 0) : glm::ivec3(1, 0, 0);
    glm::ivec3 tangentB = axisNormal.z != 0 ? glm::ivec3(0, 1, 0) : glm::ivec3(0, 0, 1);
    int solid           = 0;
    for (int a = -1; a <= 1; a++)
    {
        for (int b = -1; b <= 1; b++)
        {
            if (IsSolid(level, cell + tangentA * a + tangentB * b))
            {
                solid++;
            }
        }
    }

    return std::clamp((solid - 3) / 5.0, 0.0, 1.0);
}

// This walks the voxel grid from the cell after the start, so a trace never hits the block it starts in
bool SoundAcoustics::Trace(const Level *level, const glm::dvec3 &from, const glm::dvec3 &direction, double maxDistance, glm::dvec3 *hit,
                           glm::dvec3 *normal)
{
    const double infinity = std::numeric_limits<double>::infinity();

    glm::ivec3 cell((int) floor(from.x), (int) floor(from.y), (int) floor(from.z));
    glm::ivec3 step;
    glm::dvec3 delta;
    glm::dvec3 next;
    for (int axis = 0; axis < 3; axis++)
    {
        step[axis] = direction[axis] > 0.0 ? 1 : -1;
        if (direction[axis] == 0.0)
        {
            delta[axis] = infinity;
            next[axis]  = infinity;
            continue;
        }

        delta[axis] = fabs(1.0 / direction[axis]);
        next[axis]  = direction[axis] > 0.0 ? (cell[axis] + 1 - from[axis]) * delta[axis] : (from[axis] - cell[axis]) * delta[axis];
    }

    while (true)
    {
        int axis        = next.x < next.y ? (next.x < next.z ? 0 : 2) : (next.y < next.z ? 1 : 2);
        double distance = next[axis];
        if (distance > maxDistance)
        {
            return false;
        }

        cell[axis] += step[axis];
        next[axis] += delta[axis];
        if (IsSolid(level, cell))
        {
            *hit            = from + direction * distance;
            *normal         = glm::dvec3(0.0);
            (*normal)[axis] = (double) -step[axis];
            return true;
        }
    }
}

int SoundAcoustics::CountSolid(const Level *level, const glm::dvec3 &from, const glm::dvec3 &to)
{
    glm::ivec3 target((int) floor(to.x), (int) floor(to.y), (int) floor(to.z));
    double length = glm::distance(from, to);
    if (length <= 0.0)
    {
        return 0;
    }

    glm::dvec3 direction = (to - from) / length;
    glm::dvec3 origin    = from;
    int count            = 0;
    glm::dvec3 hit;
    glm::dvec3 normal;
    while (Trace(level, origin, direction, length - glm::distance(from, origin), &hit, &normal))
    {
        glm::ivec3 cell((int) floor(hit.x - normal.x * 0.5), (int) floor(hit.y - normal.y * 0.5), (int) floor(hit.z - normal.z * 0.5));
        if (cell == target)
        {
            break;
        }

        count++;
        origin = hit - normal * 0.001;
    }

    return count;
}

#endif
