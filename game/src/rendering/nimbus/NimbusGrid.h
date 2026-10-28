#pragma once

#include <cstdint>
#include <vector>

#include "../../world/chunk/SectionPos.h"
#include "../RenderTypes.h"

#include <glm/vec3.hpp>

class Level;
class Shader;

class NimbusGrid
{
public:
    static constexpr int kGridSections = 16;
    static constexpr int kGridSize     = kGridSections * 16;
    static constexpr int kSlotCount    = kGridSections * kGridSections * kGridSections;
    static constexpr int kSlotWords    = 259;

    struct Slot
    {
        SectionPos pos;
        bool loaded;
    };

    struct Emitter
    {
        glm::ivec3 pos;
        glm::vec3 color;
    };

    NimbusGrid();
    ~NimbusGrid();

    static int SlotIndex(const SectionPos &pos);

    void Update(Level *level, const glm::dvec3 &cameraPos, int sectionsPerFrame, bool trackEmitters);
    void MarkSectionDirty(const SectionPos &pos);
    bool HasSection(const SectionPos &pos) const;
    double Trace(const glm::dvec3 &from, const glm::dvec3 &to) const;
    bool IsSolid(const glm::dvec3 &pos) const;
    double SoftTrace(const glm::dvec3 &from, const glm::dvec3 &target, double radius) const;
    void Bind(Shader *shader, const glm::dvec3 &cameraPos) const;

    TextureHandle GetTexture() const;
    const SectionPos &GetMin() const;
    const std::vector<SectionPos> &GetSlotOrder() const;
    const Slot &GetSlot(int slot) const;
    const std::vector<Emitter> &GetEmitters(int slot) const;
    const std::vector<int> &GetRebuiltSlots() const;
    const std::vector<SectionPos> &GetShadowChanges() const;
    const std::vector<int> &GetEmitterSlots() const;
    void ClearEmitterSlots();

private:
    void BuildSection(Level *level, const SectionPos &pos, uint32_t *words, std::vector<Emitter> *emitters) const;

    Slot m_slots[kSlotCount];
    std::vector<uint32_t> m_words;
    std::vector<SectionPos> m_slotOrder;
    SectionPos m_min;
    TextureHandle m_texture;
    std::vector<Emitter> m_emitters[kSlotCount];
    std::vector<int> m_rebuiltSlots;
    std::vector<SectionPos> m_shadowChanges;
    std::vector<int> m_emitterSlots;
};
