#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ChunkRenderRegion.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "../../utils/math/Mth.h"
#include "Chunk.h"

ChunkRenderRegion::ChunkRenderRegion(const SectionPos &pos, bool translucent)
{
    m_pos         = pos;
    m_translucent = translucent;
    m_generation  = ++s_generation;
    m_drawId      = UINT32_MAX;
    m_sortDirty   = false;
    m_sortedX     = 0;
    m_sortedY     = 0;
    m_sortedZ     = 0;
}

SectionPos ChunkRenderRegion::GetRegionPos(const SectionPos &pos)
{
    return SectionPos(Mth::FloorDiv(pos.x, s_size), pos.y, Mth::FloorDiv(pos.z, s_size));
}

void ChunkRenderRegion::ToRegionSpace(const SectionPos &pos, std::vector<ChunkMesh::Vertex> *vertices) const
{
    uint32_t chunkX = (uint32_t) (pos.x - m_pos.x * s_size);
    uint32_t chunkZ = (uint32_t) (pos.z - m_pos.z * s_size);
    for (size_t i = 0; i < vertices->size(); i++)
    {
        ChunkMesh::Vertex &vertex = (*vertices)[i];
        vertex.position[0]        = (uint16_t) (vertex.position[0] + chunkX * Chunk::s_sizeX * (uint32_t) ChunkMesh::kPositionStepsXZ);
        vertex.position[2]        = (uint16_t) (vertex.position[2] + chunkZ * Chunk::s_sizeZ * (uint32_t) ChunkMesh::kPositionStepsXZ);
    }
}

size_t ChunkRenderRegion::FindOrCreateMesh(Texture *texture, bool sortQuads, int minimumCapacity)
{
    for (size_t m = 0; m < m_meshes.size(); m++)
    {
        if (m_meshes[m]->GetTexture() == texture && m_meshes[m]->SortsQuads() == sortQuads)
        {
            return m;
        }
    }

    std::unique_ptr<ChunkMesh> mesh = std::make_unique<ChunkMesh>(texture, m_translucent, sortQuads);
    mesh->Allocate((uint32_t) GetMeshCapacity(minimumCapacity));
    m_meshes.push_back(std::move(mesh));
    m_sectionRanges.push_back({});
    m_meshUsed.push_back(0);
    return m_meshes.size() - 1;
}

int ChunkRenderRegion::FindRange(size_t mesh, const SectionPos &pos) const
{
    const std::vector<SectionRange> &ranges = m_sectionRanges[mesh];
    for (size_t r = 0; r < ranges.size(); r++)
    {
        if (ranges[r].pos == pos)
        {
            return (int) r;
        }
    }

    return -1;
}

int ChunkRenderRegion::GetLiveVertexCount(size_t mesh) const
{
    int live = 0;
    for (const SectionRange &range : m_sectionRanges[mesh])
    {
        live += range.count;
    }

    return live;
}

int ChunkRenderRegion::GetMeshCapacity(int needed)
{
    return needed + std::max(needed / 16, s_minimumSlack);
}

int ChunkRenderRegion::GetRangeCapacity(int count)
{
    // Headroom lets a remesh that grew a little stay in place, otherwise constant block updates keep reallocating region
    // buffers and the driver holds on to the old memory
    int capacity = count + std::max(count / 32, 4);
    return (capacity + 3) & ~3;
}

void ChunkRenderRegion::Compact(size_t mesh, int extraVertices)
{
    ChunkMesh *old = m_meshes[mesh].get();
    int needed     = extraVertices > 0 ? GetRangeCapacity(extraVertices) : 0;
    for (const SectionRange &range : m_sectionRanges[mesh])
    {
        if (range.count > 0)
        {
            needed += GetRangeCapacity(range.count);
        }
    }

    std::unique_ptr<ChunkMesh> compacted = std::make_unique<ChunkMesh>(old->GetTexture(), m_translucent, old->SortsQuads());
    compacted->Allocate((uint32_t) GetMeshCapacity(needed));
    compacted->AddAtlasRegions(old->GetAtlasRegions());

    std::vector<SectionRange> ranges;
    int used = 0;
    for (SectionRange &range : m_sectionRanges[mesh])
    {
        if (range.count == 0)
        {
            continue;
        }

        compacted->CopyVertices(*old, range.first, used, range.count);
        range.first    = used;
        range.capacity = GetRangeCapacity(range.count);
        range.sorted   = false;
        used += range.capacity;
        ranges.push_back(std::move(range));
    }

    compacted->SetVertexCount((uint32_t) used);
    m_meshes[mesh]        = std::move(compacted);
    m_sectionRanges[mesh] = std::move(ranges);
    m_meshUsed[mesh]      = used;
}

bool ChunkRenderRegion::UpdateSection(const SectionPos &pos, std::vector<ChunkMesh::PackedData> &sectionMeshes)
{
    m_generation = ++s_generation;
    std::vector<ChunkMesh::PackedData> meshes;
    for (size_t i = 0; i < sectionMeshes.size(); i++)
    {
        ChunkMesh::PackedData *target = nullptr;
        for (size_t j = 0; j < meshes.size(); j++)
        {
            if (meshes[j].texture == sectionMeshes[i].texture && meshes[j].sortQuads == sectionMeshes[i].sortQuads)
            {
                target = &meshes[j];
            }
        }

        if (!target)
        {
            meshes.push_back(std::move(sectionMeshes[i]));
            continue;
        }

        target->vertexCount += sectionMeshes[i].vertexCount;
        target->vertices.insert(target->vertices.end(), sectionMeshes[i].vertices.begin(), sectionMeshes[i].vertices.end());
        target->atlasRegions.insert(target->atlasRegions.end(), sectionMeshes[i].atlasRegions.begin(), sectionMeshes[i].atlasRegions.end());
    }

    sectionMeshes.clear();

    for (size_t m = 0; m < m_sectionRanges.size(); m++)
    {
        int r = FindRange(m, pos);
        if (r >= 0)
        {
            m_sectionRanges[m][(size_t) r].count = 0;
            m_sectionRanges[m][(size_t) r].sortQuads.clear();
        }
    }

    for (size_t i = 0; i < meshes.size(); i++)
    {
        int count = (int) meshes[i].vertexCount;
        if (count == 0)
        {
            continue;
        }

        int capacity = GetRangeCapacity(count);
        size_t m     = FindOrCreateMesh(meshes[i].texture, meshes[i].sortQuads, capacity);
        int r        = FindRange(m, pos);
        int first;
        if (r >= 0 && count <= m_sectionRanges[m][(size_t) r].capacity)
        {
            first = m_sectionRanges[m][(size_t) r].first;
        }
        else
        {
            if (m_meshUsed[m] + capacity > (int) m_meshes[m]->GetCapacity())
            {
                Compact(m, count);
                r = FindRange(m, pos);
            }

            first = m_meshUsed[m];
            m_meshUsed[m] += capacity;
            m_meshes[m]->SetVertexCount((uint32_t) m_meshUsed[m]);
            if (r >= 0)
            {
                m_sectionRanges[m][(size_t) r].first    = first;
                m_sectionRanges[m][(size_t) r].capacity = capacity;
            }
            else
            {
                m_sectionRanges[m].push_back({pos, first, 0, capacity, {}, false, glm::dvec3(), 0, {}});
                r = (int) m_sectionRanges[m].size() - 1;
            }
        }

        SectionRange &range = m_sectionRanges[m][(size_t) r];
        std::fill(range.faceCounts, range.faceCounts + s_faceGroupCount, 0);
        if (!m_translucent)
        {
            // These are the packed octahedral normals of the six axis directions in face group order
            const uint16_t axisNormals[6]            = {0x7FFE, 0x7F00, 0xFE7F, 0x007F, 0x7F7F, 0xFEFE};
            std::vector<ChunkMesh::Vertex> &vertices = meshes[i].vertices;
            std::vector<uint8_t> quadFaces(vertices.size() / 4);
            for (size_t q = 0; q < quadFaces.size(); q++)
            {
                quadFaces[q] = s_faceGroupCount - 1;
                for (int face = 0; face < 6; face++)
                {
                    if (vertices[q * 4].position[3] == axisNormals[face])
                    {
                        quadFaces[q] = (uint8_t) face;
                    }
                }

                range.faceCounts[quadFaces[q]] += 4;
            }

            int groupFirst[s_faceGroupCount];
            groupFirst[0] = 0;
            for (int face = 1; face < s_faceGroupCount; face++)
            {
                groupFirst[face] = groupFirst[face - 1] + range.faceCounts[face - 1];
            }

            std::vector<ChunkMesh::Vertex> grouped(vertices.size());
            for (size_t q = 0; q < quadFaces.size(); q++)
            {
                std::copy(vertices.begin() + (long) q * 4, vertices.begin() + (long) q * 4 + 4, grouped.begin() + groupFirst[quadFaces[q]]);
                groupFirst[quadFaces[q]] += 4;
            }

            vertices.swap(grouped);
        }

        ToRegionSpace(pos, &meshes[i].vertices);
        m_meshes[m]->UpdateVertices(first, meshes[i].vertices);
        m_meshes[m]->AddAtlasRegions(meshes[i].atlasRegions);
        range.count = count;
        if (meshes[i].sortQuads)
        {
            range.sortQuads = std::move(meshes[i].vertices);
            range.sorted    = false;
            range.sortId    = ++s_nextSortId;
            m_sortDirty     = true;
        }
    }

    m_sections.insert(pos);
    return true;
}

// Translucent faces draw without depth writes, so overlapping ones like ice seen through ice only blend right when drawn back to front
void ChunkRenderRegion::CollectSortJobs(const glm::dvec3 &cameraPos, int *budget, std::vector<QuadSortJob> *jobs)
{
    int cameraX = (int) std::floor(cameraPos.x);
    int cameraY = (int) std::floor(cameraPos.y);
    int cameraZ = (int) std::floor(cameraPos.z);
    if (!m_sortDirty && cameraX == m_sortedX && cameraY == m_sortedY && cameraZ == m_sortedZ)
    {
        return;
    }

    const double maxSortDistance = 96.0;
    // Far sections barely change order, so they wait until the camera moved a fraction of their distance
    const double resortDistanceScale = 0.125;
    double width                     = (double) (s_size * Chunk::s_sizeX);
    bool complete                    = true;
    for (size_t m = 0; m < m_meshes.size(); m++)
    {
        for (SectionRange &range : m_sectionRanges[m])
        {
            double sectionX = range.pos.x * Chunk::s_sizeX + Chunk::s_sizeX * 0.5 - cameraPos.x;
            double sectionY = range.pos.y * ChunkSection::s_size + ChunkSection::s_size * 0.5 - cameraPos.y;
            double sectionZ = range.pos.z * Chunk::s_sizeZ + Chunk::s_sizeZ * 0.5 - cameraPos.z;
            double distSq   = sectionX * sectionX + sectionY * sectionY + sectionZ * sectionZ;
            if (range.sortQuads.empty() || distSq > maxSortDistance * maxSortDistance)
            {
                continue;
            }

            double threshold = std::max(1.0, sqrt(distSq) * resortDistanceScale);
            glm::dvec3 moved = cameraPos - range.sortedCamera;
            if (range.sorted && moved.x * moved.x + moved.y * moved.y + moved.z * moved.z < threshold * threshold)
            {
                continue;
            }

            if (*budget <= 0)
            {
                complete = false;
                continue;
            }

            (*budget)--;
            range.sorted       = true;
            range.sortedCamera = cameraPos;
            jobs->push_back({m_pos, range.sortId, cameraPos - glm::dvec3(m_pos.x * width, 0.0, m_pos.z * width), range.sortQuads});
        }
    }

    // Leftover sections retry next frame
    m_sortDirty = !complete;
    m_sortedX   = cameraX;
    m_sortedY   = cameraY;
    m_sortedZ   = cameraZ;
}

void ChunkRenderRegion::QuadSortJob::Sort()
{
    std::vector<std::pair<double, int>> order;
    for (int q = 0; q < (int) vertices.size() / 4; q++)
    {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        for (int v = 0; v < 4; v++)
        {
            const ChunkMesh::Vertex &vertex = vertices[(size_t) (q * 4 + v)];
            x += vertex.position[0];
            y += vertex.position[1];
            z += vertex.position[2];
        }

        x = x / (4.0 * ChunkMesh::kPositionStepsXZ) - camera.x;
        y = y / (4.0 * ChunkMesh::kPositionStepsY) - camera.y;
        z = z / (4.0 * ChunkMesh::kPositionStepsXZ) - camera.z;
        order.push_back({x * x + y * y + z * z, q});
    }

    std::sort(order.begin(), order.end(),
              [](const std::pair<double, int> &a, const std::pair<double, int> &b)
              {
                  return a.first > b.first;
              });

    std::vector<ChunkMesh::Vertex> sorted;
    sorted.reserve(vertices.size());
    for (size_t i = 0; i < order.size(); i++)
    {
        sorted.insert(sorted.end(), vertices.begin() + order[i].second * 4, vertices.begin() + order[i].second * 4 + 4);
    }

    vertices.swap(sorted);
}

void ChunkRenderRegion::ApplySortedQuads(const QuadSortJob &job)
{
    for (size_t m = 0; m < m_meshes.size(); m++)
    {
        for (const SectionRange &range : m_sectionRanges[m])
        {
            if (range.sortId == job.sortId && !range.sortQuads.empty())
            {
                m_meshes[m]->UpdateVertices(range.first, job.vertices);
                return;
            }
        }
    }
}

void ChunkRenderRegion::SetSection(const SectionPos &pos, std::vector<ChunkMesh::PackedData> &&meshes)
{
    UpdateSection(pos, meshes);
}

void ChunkRenderRegion::ShrinkMeshes()
{
    for (size_t m = 0; m < m_meshes.size();)
    {
        int live = GetLiveVertexCount(m);
        if (live == 0)
        {
            m_meshes.erase(m_meshes.begin() + (long) m);
            m_sectionRanges.erase(m_sectionRanges.begin() + (long) m);
            m_meshUsed.erase(m_meshUsed.begin() + (long) m);
            continue;
        }

        if ((int) m_meshes[m]->GetCapacity() > s_minimumSlack * 2 && live * 4 < (int) m_meshes[m]->GetCapacity() * 3)
        {
            Compact(m, 0);
        }

        m++;
    }
}

void ChunkRenderRegion::RemoveSection(const SectionPos &pos)
{
    if (m_sections.erase(pos) == 0)
    {
        return;
    }

    m_generation = ++s_generation;

    for (size_t m = 0; m < m_sectionRanges.size(); m++)
    {
        int r = FindRange(m, pos);
        if (r >= 0)
        {
            m_sectionRanges[m].erase(m_sectionRanges[m].begin() + r);
        }
    }

    ShrinkMeshes();
}

bool ChunkRenderRegion::ClearSection(const SectionPos &pos)
{
    RemoveSection(pos);
    return !m_sections.empty();
}

void ChunkRenderRegion::Rebuild()
{
    m_generation = ++s_generation;
    ShrinkMeshes();
}

uint64_t ChunkRenderRegion::GetGeneration() const
{
    return m_generation;
}

uint32_t ChunkRenderRegion::GetDrawId() const
{
    return m_drawId;
}

void ChunkRenderRegion::SetDrawId(uint32_t drawId)
{
    m_drawId = drawId;
}

bool ChunkRenderRegion::HasSection(const SectionPos &pos) const
{
    return m_sections.find(pos) != m_sections.end();
}

bool ChunkRenderRegion::IsEmpty() const
{
    return m_sections.empty();
}

size_t ChunkRenderRegion::GetSectionCount() const
{
    return m_sections.size();
}

const std::vector<std::vector<ChunkRenderRegion::SectionRange>> &ChunkRenderRegion::GetSectionRanges() const
{
    return m_sectionRanges;
}

std::vector<std::unique_ptr<ChunkMesh>> &ChunkRenderRegion::GetMeshes()
{
    return m_meshes;
}

const std::vector<std::unique_ptr<ChunkMesh>> &ChunkRenderRegion::GetMeshes() const
{
    return m_meshes;
}

size_t ChunkRenderRegion::GetCpuBytes() const
{
    size_t bytes = m_sections.size() * sizeof(SectionPos);
    for (size_t m = 0; m < m_meshes.size(); m++)
    {
        bytes += m_sectionRanges[m].capacity() * sizeof(SectionRange);
        for (const SectionRange &range : m_sectionRanges[m])
        {
            bytes += range.sortQuads.capacity() * sizeof(ChunkMesh::Vertex);
        }

        bytes += m_meshes[m]->GetCpuBytes();
    }

    return bytes;
}

#endif
