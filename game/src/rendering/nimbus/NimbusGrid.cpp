#ifndef EVERSTEAD_DEDICATED_SERVER

#include "NimbusGrid.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "../../core/FrameProfiler.h"
#include "../../world/Level.h"
#include "../../world/block/Block.h"
#include "../../world/chunk/Chunk.h"
#include "../RenderDevice.h"
#include "../Shader.h"
#include "../Textures.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

NimbusGrid::NimbusGrid()
{
    for (int i = 0; i < kSlotCount; i++)
    {
        m_slots[i].loaded = false;
    }

    m_words.assign((size_t) kSlotCount * kSlotWords, 0);
    m_texture = Textures::Create(kSlotWords, kSlotCount, eTextureFormat_R32UI, eTextureWrap_ClampToEdge, m_words.data());
    for (int y = 0; y < kGridSections; y++)
    {
        for (int z = 0; z < kGridSections; z++)
        {
            for (int x = 0; x < kGridSections; x++)
            {
                m_slotOrder.push_back(SectionPos(x, y, z));
            }
        }
    }

    // The camera sits in the middle section of the grid, so the closest sections get rebuilt first
    std::sort(m_slotOrder.begin(), m_slotOrder.end(),
              [](const SectionPos &a, const SectionPos &b)
              {
                  return (a.x - kGridSections / 2) * (a.x - kGridSections / 2) + (a.y - kGridSections / 2) * (a.y - kGridSections / 2) +
                                 (a.z - kGridSections / 2) * (a.z - kGridSections / 2) <
                         (b.x - kGridSections / 2) * (b.x - kGridSections / 2) + (b.y - kGridSections / 2) * (b.y - kGridSections / 2) +
                                 (b.z - kGridSections / 2) * (b.z - kGridSections / 2);
              });
}

NimbusGrid::~NimbusGrid()
{
    RenderDevice::GetInstance()->DeleteTexture(m_texture);
}

int NimbusGrid::SlotIndex(const SectionPos &pos)
{
    return (pos.x & (kGridSections - 1)) + (pos.y & (kGridSections - 1)) * kGridSections +
           (pos.z & (kGridSections - 1)) * kGridSections * kGridSections;
}

void NimbusGrid::Update(Level *level, const glm::dvec3 &cameraPos, int sectionsPerFrame, bool trackEmitters)
{
    m_rebuiltSlots.clear();
    m_shadowChanges.clear();
    SectionPos gridMin((int) floor(cameraPos.x / 16.0) - kGridSections / 2, (int) floor(cameraPos.y / 16.0) - kGridSections / 2,
                       (int) floor(cameraPos.z / 16.0) - kGridSections / 2);
    int firstSlot = kSlotCount;
    int lastSlot  = -1;
    if (!(gridMin == m_min))
    {
        m_min = gridMin;
        // Slots that wrapped to a new section still hold the old one's blocks, so we empty them until they get rebuilt
        for (int slot = 0; slot < kSlotCount; slot++)
        {
            SectionPos pos = m_slots[slot].pos;
            bool inGrid    = pos.x >= m_min.x && pos.y >= m_min.y && pos.z >= m_min.z && pos.x < m_min.x + kGridSections &&
                             pos.y < m_min.y + kGridSections && pos.z < m_min.z + kGridSections;
            if (!inGrid && m_slots[slot].loaded)
            {
                m_slots[slot].loaded                      = false;
                m_words[(size_t) slot * kSlotWords + 258] = 0;
                firstSlot                                 = std::min(firstSlot, slot);
                lastSlot                                  = std::max(lastSlot, slot);
            }
        }
    }

    for (size_t i = 0; i < m_slotOrder.size() && (int) m_rebuiltSlots.size() < sectionsPerFrame; i++)
    {
        SectionPos pos(m_min.x + m_slotOrder[i].x, m_min.y + m_slotOrder[i].y, m_min.z + m_slotOrder[i].z);
        int slot = SlotIndex(pos);
        if (HasSection(pos))
        {
            continue;
        }

        uint32_t *words = &m_words[(size_t) slot * kSlotWords];
        if (m_slots[slot].loaded && words[258] != 0)
        {
            m_shadowChanges.push_back(m_slots[slot].pos);
        }

        bool hadEmitters = !m_emitters[slot].empty();
        {
            FrameProfilerScope buildScope("Nimbus.BuildSection");
            BuildSection(level, pos, words, &m_emitters[slot]);
        }

        if (trackEmitters && (hadEmitters || !m_emitters[slot].empty()))
        {
            m_emitterSlots.push_back(slot);
        }

        if (words[258] != 0)
        {
            m_shadowChanges.push_back(pos);
        }

        m_slots[slot].pos    = pos;
        m_slots[slot].loaded = true;
        m_rebuiltSlots.push_back(slot);
        firstSlot = std::min(firstSlot, slot);
        lastSlot  = std::max(lastSlot, slot);
    }

    if (lastSlot >= firstSlot)
    {
        RenderDevice::GetInstance()->UploadTextureRegion(m_texture, 0, firstSlot, kSlotWords, lastSlot - firstSlot + 1, eTextureFormat_R32UI,
                                                         &m_words[(size_t) firstSlot * kSlotWords]);
    }
}

void NimbusGrid::MarkSectionDirty(const SectionPos &pos)
{
    Slot &slot = m_slots[SlotIndex(pos)];
    if (slot.pos == pos)
    {
        slot.loaded = false;
    }
}

bool NimbusGrid::HasSection(const SectionPos &pos) const
{
    const Slot &slot = m_slots[SlotIndex(pos)];
    return slot.loaded && slot.pos == pos;
}

void NimbusGrid::BuildSection(Level *level, const SectionPos &pos, uint32_t *words, std::vector<Emitter> *emitters) const
{
    memset(words, 0, kSlotWords * sizeof(uint32_t));
    emitters->clear();
    if (pos.y < 0 || pos.y >= Chunk::s_sectionCount)
    {
        return;
    }

    const Chunk *chunk = level->GetChunk(ChunkPos(pos.x, pos.z));
    if (!chunk)
    {
        return;
    }

    const ChunkSection &section = chunk->GetSection(pos.y);
    if (section.IsEmpty())
    {
        return;
    }

    for (int y = 0; y < 16; y++)
    {
        for (int z = 0; z < 16; z++)
        {
            for (int x = 0; x < 16; x++)
            {
                uint32_t id = section.GetBlockId(x, y, z);
                if (id == 0)
                {
                    continue;
                }

                Block *block = Block::ById(id);
                if (block->GetLightEmission() > 0)
                {
                    uint8_t r;
                    uint8_t g;
                    uint8_t b;
                    block->GetLightColor(&r, &g, &b);
                    emitters->push_back({glm::ivec3(pos.x * 16 + x, pos.y * 16 + y, pos.z * 16 + z),
                                         glm::vec3(r, g, b) * (block->GetLightEmission() / (255.0f * 15.0f))});
                }

                // Lava stays open so the flood keeps its own light in the cell, a solid cell would read black on every face touching it
                if (block->IsLava())
                {
                    continue;
                }

                bool solid = block->IsSolid() && !block->IsTransparent();

                if (!solid && !block->IsLiquid())
                {
                    continue;
                }

                int brick = (x >> 2) + (z >> 2) * 4 + (y >> 2) * 16;
                int local = (x & 3) + (z & 3) * 4 + (y & 3) * 16;
                words[(solid ? 0 : 128) + brick * 2 + (local >> 5)] |= (uint32_t) 1 << (local & 31);
                words[256 + (brick >> 5)] |= (uint32_t) 1 << (brick & 31);
                words[258] = 1;
            }
        }
    }
}

double NimbusGrid::Trace(const glm::dvec3 &from, const glm::dvec3 &to) const
{
    glm::dvec3 delta = to - from;
    double length    = glm::length(delta);
    if (length < 0.001)
    {
        return 1.0;
    }

    glm::dvec3 direction = delta / length;
    int voxel[3]         = {(int) floor(from.x), (int) floor(from.y), (int) floor(from.z)};
    int end[3]           = {(int) floor(to.x), (int) floor(to.y), (int) floor(to.z)};
    int step[3];
    double next[3];
    double span[3];
    for (int axis = 0; axis < 3; axis++)
    {
        if (direction[axis] == 0.0)
        {
            step[axis] = 0;
            span[axis] = 0.0;
            next[axis] = std::numeric_limits<double>::infinity();
            continue;
        }

        step[axis] = direction[axis] > 0.0 ? 1 : -1;
        span[axis] = std::abs(1.0 / direction[axis]);
        next[axis] = (direction[axis] > 0.0 ? voxel[axis] + 1.0 - from[axis] : from[axis] - voxel[axis]) * span[axis];
    }

    double transmittance = 1.0;
    while (true)
    {
        int axis = next[0] < next[1] ? (next[0] < next[2] ? 0 : 2) : (next[1] < next[2] ? 1 : 2);
        if (next[axis] >= length)
        {
            return transmittance;
        }

        voxel[axis] += step[axis];
        next[axis] += span[axis];
        if (voxel[0] == end[0] && voxel[1] == end[1] && voxel[2] == end[2])
        {
            return transmittance;
        }

        SectionPos pos(voxel[0] >> 4, voxel[1] >> 4, voxel[2] >> 4);
        if (pos.x < m_min.x || pos.y < m_min.y || pos.z < m_min.z || pos.x >= m_min.x + kGridSections || pos.y >= m_min.y + kGridSections ||
            pos.z >= m_min.z + kGridSections)
        {
            return transmittance;
        }

        if (!HasSection(pos))
        {
            continue;
        }

        int x                 = voxel[0] & 15;
        int y                 = voxel[1] & 15;
        int z                 = voxel[2] & 15;
        int local             = (x & 3) + (z & 3) * 4 + (y & 3) * 16;
        int word              = ((x >> 2) + (z >> 2) * 4 + (y >> 2) * 16) * 2 + (local >> 5);
        uint32_t bit          = (uint32_t) 1 << (local & 31);
        const uint32_t *words = &m_words[(size_t) SlotIndex(pos) * kSlotWords];
        if (words[word] & bit)
        {
            return 0.0;
        }

        if (words[128 + word] & bit)
        {
            transmittance *= 0.8;
            if (transmittance < 0.02)
            {
                return 0.0;
            }
        }
    }
}

bool NimbusGrid::IsSolid(const glm::dvec3 &pos) const
{
    int voxel[3] = {(int) floor(pos.x), (int) floor(pos.y), (int) floor(pos.z)};
    SectionPos section(voxel[0] >> 4, voxel[1] >> 4, voxel[2] >> 4);
    if (!HasSection(section))
    {
        return false;
    }

    int x     = voxel[0] & 15;
    int y     = voxel[1] & 15;
    int z     = voxel[2] & 15;
    int local = (x & 3) + (z & 3) * 4 + (y & 3) * 16;
    int word  = ((x >> 2) + (z >> 2) * 4 + (y >> 2) * 16) * 2 + (local >> 5);
    return (m_words[(size_t) SlotIndex(section) * kSlotWords + word] >> (local & 31)) & 1;
}

double NimbusGrid::SoftTrace(const glm::dvec3 &from, const glm::dvec3 &target, double radius) const
{
    if (radius <= 0.0)
    {
        return Trace(from, target);
    }

    // We take fewer samples than the GPU caches because this runs for every entity and particle each frame
    constexpr int kSamples = 4;

    glm::dvec3 toLight   = glm::normalize(target - from + glm::dvec3(0.0, 0.0001, 0.0));
    glm::dvec3 tangent   = glm::normalize(glm::cross(toLight, std::abs(toLight.y) < 0.99 ? glm::dvec3(0.0, 1.0, 0.0) : glm::dvec3(1.0, 0.0, 0.0)));
    glm::dvec3 bitangent = glm::cross(toLight, tangent);
    double visibility    = 0.0;
    double weight        = 0.0;
    for (int i = 0; i < kSamples; i++)
    {
        double angle     = i * 2.3999632;
        double offset    = sqrt((i + 0.5) / kSamples) * radius;
        glm::dvec3 point = target + (tangent * cos(angle) + bitangent * sin(angle)) * offset;
        if (!IsSolid(point) && Trace(target, point) > 0.0)
        {
            visibility += Trace(from, point);
            weight += 1.0;
        }
    }

    return weight > 0.0 ? visibility / weight : Trace(from, target);
}

void NimbusGrid::Bind(Shader *shader, const glm::dvec3 &cameraPos) const
{
    glm::dvec3 origin = glm::dvec3(m_min.x, m_min.y, m_min.z) * 16.0 - cameraPos;
    shader->SetVec3("u_nimbusGridOrigin", (float) origin.x, (float) origin.y, (float) origin.z);
    shader->SetVec3("u_nimbusGridWrap", (float) ((m_min.x * 16) & (kGridSize - 1)), (float) ((m_min.y * 16) & (kGridSize - 1)),
                    (float) ((m_min.z * 16) & (kGridSize - 1)));
}

TextureHandle NimbusGrid::GetTexture() const
{
    return m_texture;
}

const SectionPos &NimbusGrid::GetMin() const
{
    return m_min;
}

const std::vector<SectionPos> &NimbusGrid::GetSlotOrder() const
{
    return m_slotOrder;
}

const NimbusGrid::Slot &NimbusGrid::GetSlot(int slot) const
{
    return m_slots[slot];
}

const std::vector<NimbusGrid::Emitter> &NimbusGrid::GetEmitters(int slot) const
{
    return m_emitters[slot];
}

const std::vector<int> &NimbusGrid::GetRebuiltSlots() const
{
    return m_rebuiltSlots;
}

const std::vector<SectionPos> &NimbusGrid::GetShadowChanges() const
{
    return m_shadowChanges;
}

const std::vector<int> &NimbusGrid::GetEmitterSlots() const
{
    return m_emitterSlots;
}

void NimbusGrid::ClearEmitterSlots()
{
    m_emitterSlots.clear();
}

#endif
