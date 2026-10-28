#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

#include "ChunkMesh.h"
#include "SectionPos.h"

#include <glm/vec3.hpp>

class ChunkRenderRegion
{
public:
    // Opaque sections store their quads grouped as +X, -X, +Y, -Y, +Z, -Z and then every other facing
    static const int s_faceGroupCount = 7;

    struct SectionRange
    {
        SectionPos pos;
        int first;
        int count;
        int capacity;
        std::vector<ChunkMesh::Vertex> sortQuads;
        bool sorted = false;
        glm::dvec3 sortedCamera;
        uint64_t sortId;
        int faceCounts[s_faceGroupCount];
    };

    struct QuadSortJob
    {
        SectionPos region;
        uint64_t sortId;
        glm::dvec3 camera;
        std::vector<ChunkMesh::Vertex> vertices;

        void Sort();
    };

    static const int s_size = 4;

    ChunkRenderRegion(const SectionPos &pos, bool translucent);

    static SectionPos GetRegionPos(const SectionPos &pos);

    void SetSection(const SectionPos &pos, std::vector<ChunkMesh::PackedData> &&meshes);
    void RemoveSection(const SectionPos &pos);
    void Rebuild();
    bool UpdateSection(const SectionPos &pos, std::vector<ChunkMesh::PackedData> &sectionMeshes);
    bool ClearSection(const SectionPos &pos);
    void CollectSortJobs(const glm::dvec3 &cameraPos, int *budget, std::vector<QuadSortJob> *jobs);
    void ApplySortedQuads(const QuadSortJob &job);

    uint64_t GetGeneration() const;
    uint32_t GetDrawId() const;
    void SetDrawId(uint32_t drawId);
    bool HasSection(const SectionPos &pos) const;
    bool IsEmpty() const;
    size_t GetSectionCount() const;
    size_t GetCpuBytes() const;
    std::vector<std::unique_ptr<ChunkMesh>> &GetMeshes();
    const std::vector<std::unique_ptr<ChunkMesh>> &GetMeshes() const;
    const std::vector<std::vector<SectionRange>> &GetSectionRanges() const;

private:
    // This is kept small since the NVIDIA Windows driver mirrors every VBO byte in system memory several times over
    static inline const int s_minimumSlack = 256;
    static inline uint64_t s_generation    = 0;
    static inline uint64_t s_nextSortId    = 0;

    void ToRegionSpace(const SectionPos &pos, std::vector<ChunkMesh::Vertex> *vertices) const;
    size_t FindOrCreateMesh(Texture *texture, bool sortQuads, int minimumCapacity);
    int FindRange(size_t mesh, const SectionPos &pos) const;
    int GetLiveVertexCount(size_t mesh) const;
    static int GetMeshCapacity(int needed);
    static int GetRangeCapacity(int count);
    void Compact(size_t mesh, int extraVertices);
    void ShrinkMeshes();

    SectionPos m_pos;
    bool m_translucent;
    uint64_t m_generation;
    uint32_t m_drawId;
    std::unordered_set<SectionPos, SectionPosHash> m_sections;
    std::vector<std::unique_ptr<ChunkMesh>> m_meshes;
    std::vector<std::vector<SectionRange>> m_sectionRanges;
    std::vector<int> m_meshUsed;
    bool m_sortDirty;
    int m_sortedX;
    int m_sortedY;
    int m_sortedZ;
};
