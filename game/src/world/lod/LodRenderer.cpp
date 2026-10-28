#ifndef EVERSTEAD_DEDICATED_SERVER

#include "LodRenderer.h"

#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>
#include <numbers>
#include <queue>

#include "../../core/Everstead.h"
#include "../../core/EversteadServer.h"
#include "../../rendering/BlockAtlas.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/RenderDevice.h"
#include "../../rendering/Texture.h"
#include "../../rendering/TextureAtlas.h"
#include "../../rendering/Textures.h"
#include "../../utils/AABB.h"
#include "../../utils/Direction.h"
#include "../../utils/Time.h"
#include "../../utils/math/Mth.h"
#include "../Level.h"
#include "../LevelRenderer.h"
#include "../ServerLevel.h"
#include "../biome/Biome.h"
#include "../block/Block.h"
#include "../block/BlockRegistry.h"
#include "../chunk/Chunk.h"
#include "../chunk/ChunkMesh.h"
#include "../chunk/ChunkRenderRegion.h"
#include "../generation/TerrainGenerator.h"
#include "../lighting/LightSource.h"
#include "LodGenerator.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const int LodRenderer::s_maxMipmapLevels;

void LodRenderer::SetupAttributes(BufferHandle vertexBuffer, BufferHandle offsetBuffer)
{
    RenderDevice *device = RenderDevice::GetInstance();
    device->BindArrayBuffer(vertexBuffer);
    device->SetVertexAttrib(0, 4, eVertexAttribType_UnsignedShort, false, sizeof(Vertex), offsetof(Vertex, position));
    device->SetVertexAttrib(1, 4, eVertexAttribType_UnsignedByte, true, sizeof(Vertex), offsetof(Vertex, color));
    device->SetVertexAttrib(2, 4, eVertexAttribType_UnsignedShort, true, sizeof(Vertex), offsetof(Vertex, uv));
    device->SetVertexAttrib(3, 4, eVertexAttribType_UnsignedByte, true, sizeof(Vertex), offsetof(Vertex, tint));
    device->SetVertexAttrib(4, 4, eVertexAttribType_UnsignedByte, true, sizeof(Vertex), offsetof(Vertex, ao));
    device->BindArrayBuffer(offsetBuffer);
    device->SetVertexAttrib(5, 4, eVertexAttribType_Float, false, 8 * sizeof(float), 0);
    device->SetVertexAttrib(6, 4, eVertexAttribType_Float, false, 8 * sizeof(float), 4 * sizeof(float));
    device->SetVertexAttribDivisor(5, 1);
    device->SetVertexAttribDivisor(6, 1);
}

LodRenderer::LodRenderer(LevelRenderer *levelRenderer)
{
    m_levelRenderer = levelRenderer;
    m_shader.Load("assets/shader/lod.vert", "assets/shader/lod.frag");
    m_meshPool              = new ThreadPool(1, true);
    m_buildQueue            = std::make_shared<BuildQueue>();
    m_buildQueue->cancelled = false;
    m_generation            = 0;
    m_frame                 = 0;
    m_inFlight              = 0;
    m_dimension             = -1;
    m_cameraPos             = glm::dvec3(0.0);
    m_lodBlocks             = 0;
    m_chunkBlocks           = 0;
    m_splitFactor           = 2.0;
    m_blendStep             = 1.0f;
    uint8_t blank[4]        = {255, 0, 0, 0};
    m_maskPixels.assign(blank, blank + 4);
    m_maskTexture        = Textures::Create(1, 1, eTextureFormat_RGBA8, eTextureWrap_ClampToEdge, blank);
    m_maskSize           = 1;
    m_maskOriginX        = 0;
    m_maskOriginZ        = 0;
    m_maskEnabled        = false;
    m_maskVersion        = 0;
    m_maskFrame          = 0;
    m_maskFadeMs         = 0;
    m_maskFading         = false;
    m_arena              = new ChunkVertexArena(sizeof(Vertex), SetupAttributes);
    RenderDevice *device = RenderDevice::GetInstance();
    m_commandBuffer      = device->CreateBuffer();
    m_cullCommandBuffer  = device->CreateBuffer();
    m_boxBuffer          = device->CreateBuffer();
    m_cullProgram        = device->CreateComputeProgram("assets/shader/lod_cull.comp", {"LodCullParams", 80}, 3, {});
}

LodRenderer::~LodRenderer()
{
    m_buildQueue->cancelled = true;
    delete m_meshPool;
    for (auto tileIt = m_tiles.begin(); tileIt != m_tiles.end(); ++tileIt)
    {
        FreeTile(&tileIt->second);
    }

    RenderDevice *device = RenderDevice::GetInstance();
    device->DeleteBuffer(m_arena->GetOffsetBuffer());
    device->DeleteBuffer(m_commandBuffer);
    device->DeleteBuffer(m_cullCommandBuffer);
    device->DeleteBuffer(m_boxBuffer);
    device->DeleteProgram(m_cullProgram);
    device->DeleteTexture(m_maskTexture);
    delete m_arena;
}

void LodRenderer::Clear()
{
    for (auto tileIt = m_tiles.begin(); tileIt != m_tiles.end(); ++tileIt)
    {
        if (tileIt->second.state == eTileState_Requested || tileIt->second.refreshing)
        {
            m_staleRequests.insert(tileIt->first);
        }

        FreeTile(&tileIt->second);
    }

    m_tiles.clear();
    m_drawItems.clear();
    m_maskEnabled = false;
    m_generation++;
    std::lock_guard<std::mutex> lock(m_resultMutex);
    m_results.clear();
}

void LodRenderer::OnTile(BuiltTile &&built)
{
    uint64_t key = MakeKey(built.level, built.x, built.z);
    if (m_staleRequests.erase(key) != 0)
    {
        m_inFlight--;
        return;
    }

    auto tileIt = m_tiles.find(key);
    if (tileIt == m_tiles.end() || (tileIt->second.state != eTileState_Requested && !tileIt->second.refreshing))
    {
        return;
    }

    m_inFlight--;
    Tile *tile = &tileIt->second;
    // A refreshing tile stays ready so its old mesh keeps drawing until the new one uploads
    if (!tile->refreshing)
    {
        tile->state = eTileState_Meshing;
    }

    uint64_t generation = m_generation;
    m_meshPool->DetachTask(
            [this, key, generation, built = std::move(built)]
            {
                MeshResult result;
                result.key        = key;
                result.generation = generation;
                result.partial    = built.volume.HasColumnFlag(LodVolume::s_columnMissing);
                result.vertices   = BuildMesh(built.level, built.volume, built.neighbors, &result.minY, &result.maxY);
                std::lock_guard<std::mutex> lock(m_resultMutex);
                m_results.push_back(std::move(result));
            });
}

void LodRenderer::Update(const glm::dvec3 &playerPos)
{
    Everstead *everstead = Everstead::GetInstance();
    Level *level         = m_levelRenderer->m_level;
    if ((int) level->GetDimensionId() != m_dimension)
    {
        Clear();
        m_dimension = (int) level->GetDimensionId();
    }

    const double splitFactors[4] = {1.5, 2.0, 3.0, 4.0};
    m_frame++;
    m_lodBlocks   = everstead->options.lodDistance * Chunk::s_sizeX;
    m_chunkBlocks = level->GetRenderDistance() * Chunk::s_sizeX;
    m_splitFactor = splitFactors[std::clamp(everstead->options.lodQuality, 0, 3)];

    std::vector<BuiltTile> builtTiles;
    {
        std::lock_guard<std::mutex> lock(m_buildQueue->mutex);
        builtTiles.swap(m_buildQueue->tiles);
    }

    for (size_t i = 0; i < builtTiles.size(); i++)
    {
        OnTile(std::move(builtTiles[i]));
    }

    UploadResults();

    if (m_frame % 60 == 0)
    {
        for (auto tileIt = m_tiles.begin(); tileIt != m_tiles.end();)
        {
            Tile *tile = &tileIt->second;
            if (tile->lastVisit + 120 < m_frame && tile->state != eTileState_Requested && tile->state != eTileState_Meshing && !tile->refreshing)
            {
                FreeTile(tile);
                tileIt = m_tiles.erase(tileIt);
            }
            else
            {
                ++tileIt;
            }
        }
    }

    UpdateMask(playerPos);
}

void LodRenderer::Render(const glm::dvec3 &cameraPos, const glm::dmat4 &viewMatrix, const glm::dmat4 &projection)
{
    Everstead *everstead = Everstead::GetInstance();
    m_cameraPos          = cameraPos;

    double nearPlane         = 4.0;
    double farPlane          = m_lodBlocks * 1.5 + 512.0;
    glm::dmat4 lodProjection = projection;
    lodProjection[2][2]      = -(farPlane + nearPlane) / (farPlane - nearPlane);
    lodProjection[3][2]      = -2.0 * farPlane * nearPlane / (farPlane - nearPlane);
    m_frustum.ExtractPlanes(lodProjection * viewMatrix * GlStateManager::GetMatrix());

    int fadeTime = everstead->options.lodFadeTime;
    m_blendStep  = fadeTime > 0 ? (float) (Time::GetDelta() * 1000.0 / fadeTime) : 1.0f;
    m_drawItems.clear();
    m_requestCandidates.clear();
    int rootSize = LodGenerator::GetTileSize(LodGenerator::s_levelCount - 1);
    int minRootX = Mth::FloorDiv((int) floor(cameraPos.x) - m_lodBlocks, rootSize);
    int maxRootX = Mth::FloorDiv((int) floor(cameraPos.x) + m_lodBlocks, rootSize);
    int minRootZ = Mth::FloorDiv((int) floor(cameraPos.z) - m_lodBlocks, rootSize);
    int maxRootZ = Mth::FloorDiv((int) floor(cameraPos.z) + m_lodBlocks, rootSize);
    for (int rootZ = minRootZ; rootZ <= maxRootZ; rootZ++)
    {
        for (int rootX = minRootX; rootX <= maxRootX; rootX++)
        {
            double dx = std::max({(double) rootX * rootSize - cameraPos.x, cameraPos.x - (double) (rootX + 1) * rootSize, 0.0});
            double dz = std::max({(double) rootZ * rootSize - cameraPos.z, cameraPos.z - (double) (rootZ + 1) * rootSize, 0.0});
            if (dx * dx + dz * dz <= (double) m_lodBlocks * m_lodBlocks)
            {
                Visit(LodGenerator::s_levelCount - 1, rootX, rootZ, 0.0f, 1.0f, true);
            }
        }
    }

    SendRequests();
    CullOccluded();
    if (m_drawItems.empty())
    {
        return;
    }

    RenderDevice *device = RenderDevice::GetInstance();
    Level *level         = m_levelRenderer->m_level;
    float fogStart       = 1.0e8f;
    float fogEnd         = 1.0e8f + 1.0f;
    float fogColor[3]    = {};
    if (GlStateManager::IsFogEnabled())
    {
        GlStateManager::GetFogRange(&fogStart, &fogEnd);
        GlStateManager::GetFogColor(&fogColor[0], &fogColor[1], &fogColor[2]);
    }

    m_shader.Bind();
    glm::dmat4 cameraRelativeModel = glm::translate(GlStateManager::GetMatrix(), cameraPos);
    m_shader.SetMat4("u_model", glm::value_ptr(cameraRelativeModel));
    m_shader.SetMat4("u_view", glm::value_ptr(viewMatrix));
    m_shader.SetMat4("u_projection", glm::value_ptr(lodProjection));
    m_shader.SetFloat("u_skyDarken",
                      LightSource::SampleSkyDarken(level->GetWorldProvider(), level->GetRainLevel(1.0f), level->GetThunderLevel(1.0f)));
    m_shader.SetFloat("u_darkAdaptation", LightSource::s_darkAdaptation);
    m_shader.SetVec2("u_fogRange", fogStart, fogEnd);
    m_shader.SetVec3("u_fogColor", fogColor[0], fogColor[1], fogColor[2]);
    glm::vec3 hazeColor = m_levelRenderer->m_fogColor;
    m_shader.SetVec3("u_hazeColor", hazeColor.x, hazeColor.y, hazeColor.z);
    m_shader.SetVec2("u_hazeRange", (float) m_chunkBlocks, (float) std::max(m_lodBlocks, m_chunkBlocks + 1));
    BindMask(&m_shader, cameraPos);
    Textures::Bind(BlockAtlas::GetInstance()->GetBlockAtlas()->GetTexture()->GetId(), 0);
    m_shader.SetInt("u_lodTextures", everstead->options.lodTextures ? 1 : 0);
    m_shader.SetInt("u_lodMipLevels", std::clamp(everstead->options.lodMipmapLevels, 0, s_maxMipmapLevels));

    GlStateManager::EnableCull();
    GlStateManager::SetFrontFace(eFrontFace_Ccw);
    GlStateManager::SetCullFace(eCullFace_Back);
    GlStateManager::EnableDepthTest();
    GlStateManager::DisableBlend();

    std::sort(m_drawItems.begin(), m_drawItems.end(),
              [](const DrawItem &a, const DrawItem &b)
              {
                  return a.tile->page < b.tile->page;
              });
    const float levelColors[LodGenerator::s_levelCount][3] = {{1.0f, 0.4f, 0.4f}, {1.0f, 0.8f, 0.3f}, {0.5f, 1.0f, 0.4f}, {0.3f, 0.9f, 1.0f},
                                                              {0.5f, 0.5f, 1.0f}, {1.0f, 0.4f, 1.0f}, {1.0f, 1.0f, 1.0f}};
    const float white[3]                                   = {1.0f, 1.0f, 1.0f};
    bool debug                                             = everstead->options.lodDebug;
    m_commands.clear();
    m_instanceData.clear();
    m_boxes.clear();
    for (size_t i = 0; i < m_drawItems.size(); i++)
    {
        const DrawItem &item = m_drawItems[i];
        const Tile *tile     = item.tile;
        double tileSize      = LodGenerator::GetTileSize(tile->level);
        const float *color   = debug ? levelColors[tile->level] : white;
        m_instanceData.insert(m_instanceData.end(),
                              {(float) (tile->x * tileSize - cameraPos.x), (float) -cameraPos.y, (float) (tile->z * tileSize - cameraPos.z),
                               item.fadeLow, item.fadeHigh, color[0], color[1], color[2]});
        m_commands.push_back({tile->quadCount * 6, 1, 0, (int) tile->first, (uint32_t) i});
        glm::vec3 boxMin((float) (tile->x * tileSize - cameraPos.x), (float) (tile->minY - cameraPos.y), (float) (tile->z * tileSize - cameraPos.z));
        m_boxes.push_back(glm::vec4(boxMin, 0.0f));
        m_boxes.push_back(glm::vec4(boxMin.x + (float) tileSize, (float) (tile->maxY + 1 - cameraPos.y), boxMin.z + (float) tileSize, 0.0f));
    }

    bool occlusion             = !m_commands.empty() && m_levelRenderer->m_hizValid && m_levelRenderer->m_frustumCulling &&
                                 glm::distance(cameraPos, m_levelRenderer->m_hizCameraPos) < 16.0;
    BufferHandle commandBuffer = occlusion ? m_cullCommandBuffer : m_commandBuffer;
    device->BindArrayBuffer(commandBuffer);
    device->UploadArrayBuffer(m_commands.data(), (uint32_t) (m_commands.size() * sizeof(DrawIndirectCommand)),
                              occlusion ? eBufferUsage_Static : eBufferUsage_Stream);
    if (occlusion)
    {
        device->BindArrayBuffer(m_boxBuffer);
        device->UploadArrayBuffer(m_boxes.data(), (uint32_t) (m_boxes.size() * sizeof(glm::vec4)), eBufferUsage_Static);

        float uniforms[20];
        glm::mat4 occlusionMatrix(glm::translate(m_levelRenderer->m_hizMatrix, cameraPos - m_levelRenderer->m_hizCameraPos));
        glm::ivec4 occlusionSize(m_levelRenderer->m_hizWidth, m_levelRenderer->m_hizHeight, m_levelRenderer->m_hizLevels, (int) m_commands.size());
        memcpy(uniforms, glm::value_ptr(occlusionMatrix), sizeof(occlusionMatrix));
        memcpy(uniforms + 16, glm::value_ptr(occlusionSize), sizeof(occlusionSize));
        BufferHandle buffers[3] = {m_cullCommandBuffer, m_boxBuffer, m_levelRenderer->m_hizBuffer};
        device->DispatchCompute(m_cullProgram, uniforms, buffers, 3, nullptr, (uint32_t) (m_commands.size() + 63) / 64);
    }

    device->BindArrayBuffer(m_arena->GetOffsetBuffer());
    device->UploadArrayBuffer(m_instanceData.data(), (uint32_t) (m_instanceData.size() * sizeof(float)), eBufferUsage_Stream);
    for (size_t first = 0; first < m_drawItems.size();)
    {
        uint32_t page     = m_drawItems[first].tile->page;
        uint32_t maxQuads = 0;
        size_t end        = first;
        while (end < m_drawItems.size() && m_drawItems[end].tile->page == page)
        {
            maxQuads = std::max(maxQuads, m_drawItems[end].tile->quadCount);
            end++;
        }

        device->BindVertexArray(m_arena->GetVertexArray(page));
        ChunkMesh::BindQuadIndices(maxQuads);
        device->DrawMultiElementsIndirect(ePrimitiveType_Triangles, commandBuffer, (uint32_t) (first * sizeof(DrawIndirectCommand)),
                                          (int) (end - first));
        first = end;
    }

    device->BindVertexArray(VertexArrayHandle());
}

void LodRenderer::BindMask(Shader *shader, const glm::dvec3 &cameraPos) const
{
    Textures::Bind(m_maskTexture, 2);
    shader->SetInt("u_lodMaskEnabled", m_maskEnabled ? 1 : 0);
    shader->SetVec2("u_lodMaskOrigin", (float) (m_maskOriginX * Chunk::s_sizeX - cameraPos.x),
                    (float) (m_maskOriginZ * Chunk::s_sizeZ - cameraPos.z));
    shader->SetInt("u_lodMaskSize", m_maskSize);
}

size_t LodRenderer::GetTileCount() const
{
    return m_tiles.size();
}

size_t LodRenderer::GetDrawnTileCount() const
{
    return m_drawItems.size();
}

int LodRenderer::GetInFlightCount() const
{
    return m_inFlight;
}

uint64_t LodRenderer::MakeKey(int level, int tileX, int tileZ)
{
    return (uint64_t) level << 56 | ((uint64_t) (uint32_t) tileX & 0xfffffff) << 28 | ((uint64_t) (uint32_t) tileZ & 0xfffffff);
}

LodRenderer::Tile *LodRenderer::GetTile(int level, int tileX, int tileZ)
{
    auto inserted = m_tiles.try_emplace(MakeKey(level, tileX, tileZ));
    Tile *tile    = &inserted.first->second;
    if (inserted.second)
    {
        tile->level      = level;
        tile->x          = tileX;
        tile->z          = tileZ;
        tile->state      = eTileState_None;
        tile->split      = false;
        tile->blend      = 0.0f;
        tile->page       = 0;
        tile->first      = 0;
        tile->quadCount  = 0;
        tile->minY       = 0;
        tile->maxY       = 0;
        tile->readyMs    = 0;
        tile->checkedMs  = 0;
        tile->partial    = false;
        tile->refreshing = false;
    }

    tile->lastVisit = m_frame;
    return tile;
}

bool LodRenderer::IsTileReady(const Tile *tile) const
{
    return tile->state == eTileState_Ready;
}

float LodRenderer::GetTileFade(const Tile *tile) const
{
    int fadeTime = Everstead::GetInstance()->options.lodFadeTime;
    if (fadeTime <= 0)
    {
        return 1.0f;
    }

    return std::clamp((float) (Time::GetMilliseconds() - tile->readyMs) / (float) fadeTime, 0.0f, 1.0f);
}

bool LodRenderer::IsTileCovered(int level, int tileX, int tileZ) const
{
    double tileSize = LodGenerator::GetTileSize(level);
    double minX     = tileX * tileSize;
    double minZ     = tileZ * tileSize;
    double farX     = std::max(fabs(minX - m_cameraPos.x), fabs(minX + tileSize - m_cameraPos.x));
    double farZ     = std::max(fabs(minZ - m_cameraPos.z), fabs(minZ + tileSize - m_cameraPos.z));
    // Real chunks inside render distance can still be missing or unmeshed, so a tile only counts as covered once the mask hides all of it
    return farX * farX + farZ * farZ < (m_chunkBlocks - 32.0) * (m_chunkBlocks - 32.0) && IsTileMasked(level, tileX, tileZ);
}

void LodRenderer::Visit(int level, int tileX, int tileZ, float fadeLow, float fadeHigh, bool fadeIn)
{
    if (IsTileCovered(level, tileX, tileZ))
    {
        return;
    }

    Tile *tile      = GetTile(level, tileX, tileZ);
    double tileSize = LodGenerator::GetTileSize(level);
    double minX     = tileX * tileSize;
    double minZ     = tileZ * tileSize;
    double dx       = std::max({minX - m_cameraPos.x, m_cameraPos.x - (minX + tileSize), 0.0});
    double dz       = std::max({minZ - m_cameraPos.z, m_cameraPos.z - (minZ + tileSize), 0.0});
    double distance = sqrt(dx * dx + dz * dz);
    // Tiles with missing chunks ask again every few seconds since those chunks can be generated or saved after the first request
    if (tile->state == eTileState_None ||
        (tile->state == eTileState_Ready && tile->partial && !tile->refreshing && Time::GetMilliseconds() - tile->checkedMs > 5000))
    {
        RequestTile(tile, distance);
    }

    double splitDistance = tileSize * m_splitFactor * (tile->split ? 1.2 : 1.0);
    bool wantSplit       = level > 0 && (distance < splitDistance || distance < m_chunkBlocks + 32.0);
    bool ready           = IsTileReady(tile);
    tile->split          = wantSplit;

    // We request children a bit before the split so they are usually meshed by the time the tile has to split
    bool prefetch      = level > 0 && !wantSplit && ready && distance < splitDistance * 1.5;
    Tile *children[4]  = {};
    bool needChildren  = level > 0 && (wantSplit || prefetch || !ready || tile->blend > 0.0f);
    bool childrenReady = needChildren;
    for (int i = 0; i < 4 && needChildren; i++)
    {
        int childX = tileX * 2 + (i & 1);
        int childZ = tileZ * 2 + (i >> 1);
        if (IsTileCovered(level - 1, childX, childZ))
        {
            continue;
        }

        if (wantSplit || prefetch)
        {
            children[i] = GetTile(level - 1, childX, childZ);
        }
        else
        {
            auto childIt = m_tiles.find(MakeKey(level - 1, childX, childZ));
            children[i]  = childIt != m_tiles.end() ? &childIt->second : nullptr;
        }

        if (!children[i] || !IsTileReady(children[i]))
        {
            childrenReady = false;
            if (children[i] && (wantSplit || prefetch) && children[i]->state == eTileState_None)
            {
                RequestTile(children[i], wantSplit ? distance : distance + m_lodBlocks * 8.0);
            }
        }
    }

    if (!childrenReady)
    {
        tile->blend = 0.0f;
    }
    else if (!ready)
    {
        tile->blend = 1.0f;
    }
    else if (wantSplit)
    {
        tile->blend = std::min(tile->blend + m_blendStep, 1.0f);
    }
    else
    {
        tile->blend = std::max(tile->blend - m_blendStep, 0.0f);
    }

    float fadeSplit = fadeLow + (fadeHigh - fadeLow) * tile->blend;
    if (tile->blend > 0.0f)
    {
        for (int i = 0; i < 4; i++)
        {
            Visit(level - 1, tileX * 2 + (i & 1), tileZ * 2 + (i >> 1), fadeLow, fadeSplit, !ready);
        }
    }

    if (!ready || tile->quadCount == 0 || tile->blend >= 1.0f)
    {
        return;
    }

    float high = fadeIn ? fadeSplit + (fadeHigh - fadeSplit) * GetTileFade(tile) : fadeHigh;
    AABB bounds(glm::dvec3(minX, tile->minY, minZ), glm::dvec3(minX + tileSize, tile->maxY, minZ + tileSize));
    if (high > fadeSplit && m_frustum.TestAABB(bounds))
    {
        m_drawItems.push_back({tile, fadeSplit, high, !IsTileMasked(level, tileX, tileZ)});
    }
}

void LodRenderer::RequestTile(Tile *tile, double distance)
{
    if (m_staleRequests.count(MakeKey(tile->level, tile->x, tile->z)) != 0)
    {
        return;
    }

    double tileSize = LodGenerator::GetTileSize(tile->level);
    AABB bounds(glm::dvec3(tile->x * tileSize, 0.0, tile->z * tileSize),
                glm::dvec3((tile->x + 1) * tileSize, Chunk::s_sizeY, (tile->z + 1) * tileSize));
    m_requestCandidates.push_back({tile, m_frustum.TestAABB(bounds) ? distance : distance * 4.0 + tileSize});
}

void LodRenderer::FreeTile(Tile *tile)
{
    if (tile->quadCount > 0)
    {
        m_arena->Free(tile->page, tile->first, tile->quadCount * 4);
    }

    tile->quadCount = 0;
}

void LodRenderer::UploadResults()
{
    std::vector<MeshResult> results;
    {
        std::lock_guard<std::mutex> lock(m_resultMutex);
        // We cap uploads by tile and vertex count, so a frame full of dense tiles can't hitch
        size_t count    = 0;
        size_t vertices = 0;
        while (count < m_results.size() && count < 16 && (count == 0 || vertices + m_results[count].vertices.size() <= 262144))
        {
            vertices += m_results[count].vertices.size();
            count++;
        }

        results.assign(std::make_move_iterator(m_results.begin()), std::make_move_iterator(m_results.begin() + (long) count));
        m_results.erase(m_results.begin(), m_results.begin() + (long) count);
    }

    for (size_t i = 0; i < results.size(); i++)
    {
        MeshResult &result = results[i];
        auto tileIt        = m_tiles.find(result.key);
        if (result.generation != m_generation || tileIt == m_tiles.end() ||
            (tileIt->second.state != eTileState_Meshing && !tileIt->second.refreshing))
        {
            continue;
        }

        Tile *tile = &tileIt->second;
        FreeTile(tile);
        if (tile->state == eTileState_Meshing)
        {
            tile->readyMs = Time::GetMilliseconds();
        }

        tile->state      = eTileState_Ready;
        tile->refreshing = false;
        tile->partial    = result.partial;
        tile->checkedMs  = Time::GetMilliseconds();
        tile->minY       = result.minY;
        tile->maxY       = result.maxY;
        tile->quadCount  = (uint32_t) result.vertices.size() / 4;
        if (tile->quadCount == 0)
        {
            continue;
        }

        tile->first = m_arena->Allocate((uint32_t) result.vertices.size(), &tile->page);
        m_arena->Upload(tile->page, tile->first, result.vertices.data(), (uint32_t) result.vertices.size());
    }

    RenderDevice::GetInstance()->BindVertexArray(VertexArrayHandle());
}

void LodRenderer::SendRequests()
{
    Everstead *everstead = Everstead::GetInstance();
    int budget           = everstead->options.lodTilesInFlight - m_inFlight;
    if (budget <= 0 || m_requestCandidates.empty() || m_dimension != eDimensionId_Overworld)
    {
        return;
    }

    // Only the host has the seed, so everyone else builds from the chunks they were sent
    bool generate = everstead->GetMultiplayerSession()->IsHostingSession() && everstead->options.lodGenerateTerrain;
    uint32_t seed = 0;
    WorldGenSettings settings;
    if (generate)
    {
        ServerLevel *overworld = EversteadServer::GetInstance()->GetLevel();
        seed                   = overworld->GetSeed();
        settings               = overworld->GetGenSettings();
    }

    std::sort(m_requestCandidates.begin(), m_requestCandidates.end(),
              [](const RequestCandidate &a, const RequestCandidate &b)
              {
                  return a.priority < b.priority;
              });
    for (size_t i = 0; i < m_requestCandidates.size() && budget > 0; i++)
    {
        Tile *tile = m_requestCandidates[i].tile;
        if (tile->state == eTileState_Ready && tile->partial && !tile->refreshing)
        {
            tile->refreshing = true;
        }
        else if (tile->state == eTileState_None)
        {
            tile->state = eTileState_Requested;
        }
        else
        {
            continue;
        }

        budget--;
        m_inFlight++;
        LodGenerator::GetInstance()->Submit(
                [queue = m_buildQueue, seed, settings, generate, level = tile->level, x = tile->x, z = tile->z]
                {
                    if (queue->cancelled)
                    {
                        return;
                    }

                    LodGenerator *generator = LodGenerator::GetInstance();
                    BuiltTile built;
                    built.level        = level;
                    built.x            = x;
                    built.z            = z;
                    built.volume       = generator->BuildTile(seed, settings, level, x, z, generate);
                    built.neighbors[0] = generator->BuildVolume(level, x + 1, z, generate);
                    built.neighbors[1] = generator->BuildVolume(level, x - 1, z, generate);
                    built.neighbors[2] = generator->BuildVolume(level, x, z + 1, generate);
                    built.neighbors[3] = generator->BuildVolume(level, x, z - 1, generate);
                    std::lock_guard<std::mutex> lock(queue->mutex);
                    queue->tiles.push_back(std::move(built));
                });
    }
}

void LodRenderer::CullOccluded()
{
    const int bucketCount    = 1024;
    const double bucketScale = bucketCount / (2.0 * std::numbers::pi);
    m_occlusionItems.clear();
    for (size_t i = 0; i < m_drawItems.size(); i++)
    {
        const Tile *tile = m_drawItems[i].tile;
        double tileSize  = LodGenerator::GetTileSize(tile->level);
        double minX      = tile->x * tileSize - m_cameraPos.x;
        double minZ      = tile->z * tileSize - m_cameraPos.z;
        double maxX      = minX + tileSize;
        double maxZ      = minZ + tileSize;
        double nearX     = std::max({minX, -maxX, 0.0});
        double nearZ     = std::max({minZ, -maxZ, 0.0});
        double farX      = std::max(-minX, maxX);
        double farZ      = std::max(-minZ, maxZ);
        double center    = atan2(minZ + tileSize * 0.5, minX + tileSize * 0.5);
        double minAngle  = 0.0;
        double maxAngle  = 0.0;
        for (int corner = 0; corner < 4; corner++)
        {
            double angle = remainder(atan2(corner >> 1 ? maxZ : minZ, corner & 1 ? maxX : minX) - center, 2.0 * std::numbers::pi);
            minAngle     = std::min(minAngle, angle);
            maxAngle     = std::max(maxAngle, angle);
        }

        m_occlusionItems.push_back({i, sqrt(nearX * nearX + nearZ * nearZ), sqrt(farX * farX + farZ * farZ), (center + minAngle) * bucketScale,
                                    (center + maxAngle) * bucketScale});
    }

    std::sort(m_occlusionItems.begin(), m_occlusionItems.end(),
              [](const OcclusionItem &a, const OcclusionItem &b)
              {
                  return a.minDistance < b.minDistance;
              });
    // We keep the highest terrain slope per direction, and a tile only joins it once every tile still to be tested lies behind it
    m_horizon.assign(bucketCount, -FLT_MAX);
    std::priority_queue<std::pair<double, size_t>, std::vector<std::pair<double, size_t>>, std::greater<std::pair<double, size_t>>> pending;
    for (size_t i = 0; i < m_occlusionItems.size(); i++)
    {
        const OcclusionItem &item = m_occlusionItems[i];
        while (!pending.empty() && pending.top().first <= item.minDistance)
        {
            const OcclusionItem &occluder = m_occlusionItems[pending.top().second];
            pending.pop();
            double height = m_drawItems[occluder.drawItem].tile->minY - m_cameraPos.y;
            float slope   = (float) (height / (height >= 0.0 ? occluder.maxDistance : occluder.minDistance));
            for (int bucket = (int) ceil(occluder.minAngle); bucket < (int) floor(occluder.maxAngle); bucket++)
            {
                float &horizon = m_horizon[Mth::FloorMod(bucket, bucketCount)];
                horizon        = std::max(horizon, slope);
            }
        }

        if (item.minDistance <= 0.0)
        {
            continue;
        }

        DrawItem &drawItem = m_drawItems[item.drawItem];
        const Tile *tile   = drawItem.tile;
        // Dithered tiles let the terrain behind them show through, so only fully faded in or chunk covered tiles can hide others
        bool solid = !tile->partial && (!drawItem.drawn || (drawItem.fadeLow <= 0.0f && drawItem.fadeHigh >= 1.0f));
        if (drawItem.drawn)
        {
            double top  = tile->maxY - m_cameraPos.y;
            float slope = (float) (top / (top > 0.0 ? item.minDistance : item.maxDistance));
            bool hidden = true;
            for (int bucket = (int) floor(item.minAngle); bucket <= (int) floor(item.maxAngle) && hidden; bucket++)
            {
                hidden = slope < m_horizon[Mth::FloorMod(bucket, bucketCount)];
            }

            drawItem.drawn = !hidden;
        }

        if (solid)
        {
            pending.push({item.maxDistance, i});
        }
    }

    std::erase_if(m_drawItems,
                  [](const DrawItem &item)
                  {
                      return !item.drawn;
                  });
}

void LodRenderer::UpdateMask(const glm::dvec3 &playerPos)
{
    int renderDistance = m_levelRenderer->m_level->GetRenderDistance();
    int size           = 2 * (renderDistance + ChunkRenderRegion::s_size) + 1;
    int playerChunkX   = Mth::FloorDiv((int) floor(playerPos.x), Chunk::s_sizeX);
    int playerChunkZ   = Mth::FloorDiv((int) floor(playerPos.z), Chunk::s_sizeZ);
    int originX        = playerChunkX - size / 2;
    int originZ        = playerChunkZ - size / 2;
    // This looks up every column in range, so new meshes only rebuild it every few frames while moving rebuilds it right away
    bool moved     = size != m_maskSize || originX != m_maskOriginX || originZ != m_maskOriginZ;
    bool recompute = !m_maskEnabled || moved || (m_maskVersion != m_levelRenderer->m_meshedSectionsVersion && m_frame >= m_maskFrame + 10);
    if (!recompute && !m_maskFading)
    {
        return;
    }

    uint32_t now  = Time::GetMilliseconds();
    int fadeTime  = Everstead::GetInstance()->options.lodFadeTime;
    int step      = fadeTime > 0 && m_maskEnabled ? std::max((int) ((now - m_maskFadeMs) * 255 / (uint32_t) fadeTime), 1) : 255;
    m_maskEnabled = true;
    m_maskFadeMs  = now;

    bool resized = size != m_maskSize;
    if (moved)
    {
        // Each texel is a fade value, so we carry them over to the shifted origin instead of restarting every fade
        std::vector<uint8_t> pixels((size_t) size * size * 4, 0);
        for (int z = 0; z < size; z++)
        {
            int oldZ = z + originZ - m_maskOriginZ;
            for (int x = 0; x < size; x++)
            {
                int oldX = x + originX - m_maskOriginX;
                if (oldX >= 0 && oldZ >= 0 && oldX < m_maskSize && oldZ < m_maskSize)
                {
                    pixels[(size_t) (z * size + x) * 4] = m_maskPixels[(size_t) (oldZ * m_maskSize + oldX) * 4];
                }
            }
        }

        m_maskPixels.swap(pixels);
        m_maskSize    = size;
        m_maskOriginX = originX;
        m_maskOriginZ = originZ;
    }

    if (resized)
    {
        RenderDevice::GetInstance()->DeleteTexture(m_maskTexture);
        m_maskTexture = Textures::Create(size, size, eTextureFormat_RGBA8, eTextureWrap_ClampToEdge, m_maskPixels.data());
        m_maskComplete.assign((size_t) size * size, 0);
    }

    if (recompute)
    {
        m_maskVersion = m_levelRenderer->m_meshedSectionsVersion;
        m_maskFrame   = m_frame;
        for (int z = 0; z < size; z++)
        {
            int chunkZ  = originZ + z;
            int regionZ = Mth::FloorDiv(chunkZ, ChunkRenderRegion::s_size) * ChunkRenderRegion::s_size;
            int dz      = std::max({regionZ - playerChunkZ, playerChunkZ - (regionZ + ChunkRenderRegion::s_size - 1), 0});
            for (int x = 0; x < size; x++)
            {
                int chunkX    = originX + x;
                int regionX   = Mth::FloorDiv(chunkX, ChunkRenderRegion::s_size) * ChunkRenderRegion::s_size;
                int dx        = std::max({regionX - playerChunkX, playerChunkX - (regionX + ChunkRenderRegion::s_size - 1), 0});
                auto meshedIt = m_levelRenderer->m_meshedSections.find(ChunkPos(chunkX, chunkZ));
                bool nearby   = dx * dx + dz * dz <= renderDistance * renderDistance;
                // Fully empty chunks never mesh a section, so they get no entry and still count as complete
                bool complete = nearby &&
                                IsColumnMeshed(ChunkPos(chunkX, chunkZ), meshedIt != m_levelRenderer->m_meshedSections.end() ? meshedIt->second : 0);
                m_maskComplete[(size_t) (z * size + x)] = complete;
            }
        }
    }

    // Chunks treat missing neighbors as solid and drop those side faces, so we only swap a column in once its neighbors are in too
    m_maskFading = false;
    for (int z = 1; z < size - 1; z++)
    {
        for (int x = 1; x < size - 1; x++)
        {
            size_t index = (size_t) (z * size + x);
            bool shown   = m_maskComplete[index] && m_maskComplete[index - 1] && m_maskComplete[index + 1] && m_maskComplete[index - size] &&
                           m_maskComplete[index + size];
            int value    = m_maskPixels[index * 4];
            int target   = shown ? 255 : 0;
            value        = value < target ? std::min(value + step, target) : std::max(value - step, target);
            m_maskPixels[index * 4] = (uint8_t) value;
            m_maskFading            = m_maskFading || value != target;
        }
    }

    RenderDevice::GetInstance()->UploadTextureRegion(m_maskTexture, 0, 0, size, size, eTextureFormat_RGBA8, m_maskPixels.data());
}

bool LodRenderer::IsColumnMeshed(const ChunkPos &pos, uint32_t meshedSections) const
{
    const Chunk *chunk = m_levelRenderer->m_level->GetChunk(pos);
    if (!chunk)
    {
        return false;
    }

    for (int section = 0; section < Chunk::s_sectionCount; section++)
    {
        if (!chunk->IsSectionEmpty(section) && !(meshedSections >> section & 1))
        {
            return false;
        }
    }

    return true;
}

bool LodRenderer::IsTileMasked(int level, int tileX, int tileZ) const
{
    int tileChunks = LodGenerator::GetTileSize(level) / Chunk::s_sizeX;
    int minX       = tileX * tileChunks - m_maskOriginX;
    int minZ       = tileZ * tileChunks - m_maskOriginZ;
    if (!m_maskEnabled || minX < 0 || minZ < 0 || minX + tileChunks > m_maskSize || minZ + tileChunks > m_maskSize)
    {
        return false;
    }

    for (int z = minZ; z < minZ + tileChunks; z++)
    {
        for (int x = minX; x < minX + tileChunks; x++)
        {
            if (m_maskPixels[(size_t) (z * m_maskSize + x) * 4] < 255)
            {
                return false;
            }
        }
    }

    return true;
}

std::vector<LodRenderer::Vertex> LodRenderer::BuildMesh(int level, const LodVolume &volume, const LodVolume *neighbors, int *minY, int *maxY)
{
    const int size    = LodGenerator::s_tileCells;
    int sizeY         = volume.GetSizeY();
    int spacing       = LodGenerator::s_minSpacing << level;
    int cellHeight    = LodGenerator::GetCellHeight(level);
    int skirtCells    = std::max(1, Chunk::s_sizeX / cellHeight);
    int chunkCells    = std::max(1, Chunk::s_sizeX / spacing);
    uint16_t waterId  = (uint16_t) Block::ByName("water_still")->GetId();
    Block *grassBlock = Block::ByName("grass_block");
    Block *dirt       = Block::ByName("dirt");
    int tops[size * size];
    for (int z = 0; z < size; z++)
    {
        for (int x = 0; x < size; x++)
        {
            int top = -1;
            if (!(volume.GetFlags(x, z) & LodVolume::s_columnMissing))
            {
                for (top = sizeY - 1; top >= 0 && volume.GetBlock(x, top, z) == 0; top--)
                {
                }
            }

            tops[z * size + x] = top;
        }
    }

    // A missing neighbor column counts as -1 so the whole side shows instead of leaving a hole
    int borderTops[4][size];
    for (int side = 0; side < 4; side++)
    {
        for (int i = 0; i < size; i++)
        {
            int x   = side == 0 ? 0 : (side == 1 ? size - 1 : i);
            int z   = side == 2 ? 0 : (side == 3 ? size - 1 : i);
            int top = -1;
            if (!(neighbors[side].GetFlags(x, z) & LodVolume::s_columnMissing))
            {
                for (top = sizeY - 1; top >= 0 && neighbors[side].GetBlock(x, top, z) == 0; top--)
                {
                }
            }

            borderTops[side][i] = top;
        }
    }

    // Air only counts as open when it connects to the sky inside this tile, so caves and holes under overhangs stay closed
    std::vector<uint8_t> outside((size_t) size * sizeY * size, 0);
    std::vector<int> queue;
    for (int z = 0; z < size; z++)
    {
        for (int x = 0; x < size; x++)
        {
            if (volume.GetFlags(x, z) & LodVolume::s_columnMissing)
            {
                continue;
            }

            for (int y = tops[z * size + x] + 1; y < sizeY; y++)
            {
                outside[((size_t) y * size + z) * size + x] = 1;
                queue.push_back((y * size + z) * size + x);
            }
        }
    }

    const int offsets[6][3] = {{0, 1, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}};
    for (size_t i = 0; i < queue.size(); i++)
    {
        int x = queue[i] % size;
        int z = queue[i] / size % size;
        int y = queue[i] / (size * size);
        for (int direction = 0; direction < 6; direction++)
        {
            int nx = x + offsets[direction][0];
            int ny = y + offsets[direction][1];
            int nz = z + offsets[direction][2];
            if (nx < 0 || nx >= size || ny < 0 || ny >= sizeY || nz < 0 || nz >= size || (volume.GetFlags(nx, nz) & LodVolume::s_columnMissing))
            {
                continue;
            }

            int neighbor = (ny * size + nz) * size + nx;
            if (!outside[neighbor] && volume.GetBlock(nx, ny, nz) == 0)
            {
                outside[neighbor] = 1;
                queue.push_back(neighbor);
            }
        }
    }

    struct FaceStyle
    {
        uint32_t color;
        uint32_t tint;
        glm::vec4 rect;
    };

    std::unordered_map<uint64_t, FaceStyle> styles;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> mask;
    const int faces[6] = {0, 1, 2, 3, 4, 6};
    int lowest         = INT_MAX;
    int highest        = INT_MIN;
    for (int direction = 0; direction < 6; direction++)
    {
        int axis    = offsets[direction][0] != 0 ? 0 : (offsets[direction][1] != 0 ? 1 : 2);
        int axisU   = (axis + 1) % 3;
        int axisV   = (axis + 2) % 3;
        int dims[3] = {size, sizeY, size};
        mask.assign((size_t) dims[axisU] * dims[axisV], 0);
        for (int slice = 0; slice < dims[axis]; slice++)
        {
            for (int v = 0; v < dims[axisV]; v++)
            {
                for (int u = 0; u < dims[axisU]; u++)
                {
                    int position[3];
                    position[axis]   = slice;
                    position[axisU]  = u;
                    position[axisV]  = v;
                    int x            = position[0];
                    int y            = position[1];
                    int z            = position[2];
                    uint16_t blockId = volume.GetBlock(x, y, z);
                    uint32_t &key    = mask[(size_t) v * dims[axisU] + u];
                    int nx           = x + offsets[direction][0];
                    int ny           = y + offsets[direction][1];
                    int nz           = z + offsets[direction][2];
                    bool water       = blockId == waterId;
                    key              = 0;
                    if (blockId == 0 || ny < 0 || (water && direction != 0))
                    {
                        continue;
                    }

                    bool exposed = false;
                    if (ny >= sizeY)
                    {
                        exposed = true;
                    }
                    else if (nx < 0 || nx >= size || nz < 0 || nz >= size)
                    {
                        // The skirt covers cracks against neighbors drawn at a coarser level
                        exposed = y > borderTops[direction - 1][nx < 0 || nx >= size ? z : x] || y >= tops[z * size + x] - skirtCells;
                    }
                    else if (volume.GetFlags(nx, nz) & LodVolume::s_columnMissing)
                    {
                        exposed = true;
                    }
                    else if (outside[((size_t) ny * size + nz) * size + nx])
                    {
                        exposed = true;
                    }
                    // The mask hides whole chunk columns, so sides on a chunk border can end up next to a lower real chunk
                    else if (!water && volume.GetBlock(nx, ny, nz) != waterId &&
                             (nx / chunkCells != x / chunkCells || nz / chunkCells != z / chunkCells))
                    {
                        exposed = y >= tops[z * size + x] - skirtCells;
                    }

                    if (exposed)
                    {
                        key = blockId | (uint32_t) volume.GetBiome(x, z) << 16;
                    }
                }
            }

            for (int v = 0; v < dims[axisV]; v++)
            {
                for (int u = 0; u < dims[axisU];)
                {
                    uint32_t key = mask[(size_t) v * dims[axisU] + u];
                    if (key == 0)
                    {
                        u++;
                        continue;
                    }

                    int width = 1;
                    while (u + width < dims[axisU] && mask[(size_t) v * dims[axisU] + u + width] == key)
                    {
                        width++;
                    }

                    int height = 1;
                    for (bool fits = true; v + height < dims[axisV] && fits; height += fits)
                    {
                        for (int i = 0; i < width && fits; i++)
                        {
                            fits = mask[(size_t) (v + height) * dims[axisU] + u + i] == key;
                        }
                    }

                    for (int j = 0; j < height; j++)
                    {
                        for (int i = 0; i < width; i++)
                        {
                            mask[(size_t) (v + j) * dims[axisU] + u + i] = 0;
                        }
                    }

                    uint16_t blockId  = (uint16_t) (key & 0xffff);
                    bool water        = blockId == waterId;
                    int face          = water ? 5 : faces[direction];
                    bool top          = direction == 0;
                    uint64_t styleKey = (uint64_t) key | (uint64_t) top << 32;
                    auto styleIt      = styles.find(styleKey);
                    if (styleIt == styles.end())
                    {
                        Block *block       = Block::ById(blockId);
                        Block *styled      = top || block != grassBlock ? block : dirt;
                        Biome *biome       = Biome::ById(key >> 16);
                        BlockColors colors = GetBlockColors(styled->GetId());
                        uint32_t tint =
                                biome ? styled->ResolveTint(top ? Direction::up : Direction::north, biome->GetGrassColor(), biome->GetFoliageColor())
                                      : 0xFFFFFF;
                        FaceStyle style;
                        style.color = TintColor(top ? colors.top : colors.side, tint);
                        style.tint  = tint;
                        style.rect  = top ? colors.topRect : colors.sideRect;
                        styleIt     = styles.emplace(styleKey, style).first;
                    }

                    int boxMin[3];
                    int boxMax[3];
                    boxMin[axis]  = slice;
                    boxMax[axis]  = slice + 1;
                    boxMin[axisU] = u;
                    boxMax[axisU] = u + width;
                    boxMin[axisV] = v;
                    boxMax[axisV] = v + height;
                    glm::ivec3 min(boxMin[0] * spacing, boxMin[1] * cellHeight, boxMin[2] * spacing);
                    glm::ivec3 max(boxMax[0] * spacing, boxMax[1] * cellHeight, boxMax[2] * spacing);
                    AddFace(vertices, face, min, max, styleIt->second.color, styleIt->second.tint, styleIt->second.rect);
                    lowest  = std::min(lowest, min.y);
                    highest = std::max(highest, max.y);
                    u += width;
                }
            }
        }
    }

    *minY = lowest == INT_MAX ? 0 : lowest;
    *maxY = highest == INT_MIN ? 0 : highest;
    return vertices;
}

void LodRenderer::AddFace(std::vector<Vertex> &vertices, int face, const glm::ivec3 &min, const glm::ivec3 &max, uint32_t color, uint32_t tint,
                          const glm::vec4 &rect)
{
    const float faceShades[7]   = {1.0f, 0.6f, 0.6f, 0.8f, 0.8f, 1.0f, 0.5f};
    float shade                 = faceShades[face];
    uint8_t r                   = (uint8_t) ((float) (color >> 16 & 0xff) * shade);
    uint8_t g                   = (uint8_t) ((float) (color >> 8 & 0xff) * shade);
    uint8_t b                   = (uint8_t) ((float) (color & 0xff) * shade);
    uint8_t tr                  = (uint8_t) ((float) (tint >> 16 & 0xff) * shade);
    uint8_t tg                  = (uint8_t) ((float) (tint >> 8 & 0xff) * shade);
    uint8_t tb                  = (uint8_t) ((float) (tint & 0xff) * shade);
    uint16_t u0                 = (uint16_t) (rect.x * 65535.0f);
    uint16_t v0                 = (uint16_t) (rect.y * 65535.0f);
    uint16_t u1                 = (uint16_t) (rect.z * 65535.0f);
    uint16_t v1                 = (uint16_t) (rect.w * 65535.0f);
    uint16_t x0                 = (uint16_t) min.x;
    uint16_t y0                 = (uint16_t) min.y;
    uint16_t z0                 = (uint16_t) min.z;
    uint16_t x1                 = (uint16_t) max.x;
    uint16_t y1                 = (uint16_t) max.y;
    uint16_t z1                 = (uint16_t) max.z;
    uint16_t positions[7][4][3] = {{{x0, y1, z0}, {x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0}}, {{x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1}, {x1, y0, z1}},
                                   {{x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0}, {x0, y0, z0}}, {{x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}},
                                   {{x0, y1, z0}, {x1, y1, z0}, {x1, y0, z0}, {x0, y0, z0}}, {{x0, y1, z0}, {x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0}},
                                   {{x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1}}};
    for (int corner = 0; corner < 4; corner++)
    {
        const uint16_t *position = positions[face][corner];
        vertices.push_back(Vertex{{position[0], position[1], position[2], (uint16_t) face},
                                  {r, g, b, 255},
                                  {u0, v0, u1, v1},
                                  {tr, tg, tb, 255},
                                  {255, 255, 255, 255}});
    }
}

LodRenderer::BlockColors LodRenderer::GetBlockColors(uint32_t blockId)
{
    auto colorIt = m_blockColors.find(blockId);
    if (colorIt != m_blockColors.end())
    {
        return colorIt->second;
    }

    BlockColors colors = {0x7f7f7f, 0x7f7f7f, glm::vec4(0.0f), glm::vec4(0.0f)};
    Block *block       = Block::ById(blockId);
    if (block)
    {
        Texture *atlas                     = BlockAtlas::GetInstance()->GetBlockAtlas()->GetTexture();
        const std::vector<uint8_t> &pixels = atlas->GetPixels();
        int width                          = atlas->GetPixelWidth();
        int height                         = atlas->GetPixelHeight();
        Direction *directions[2]           = {Direction::up, Direction::north};
        uint32_t *targets[2]               = {&colors.top, &colors.side};
        glm::vec4 *rects[2]                = {&colors.topRect, &colors.sideRect};
        for (int face = 0; face < 2; face++)
        {
            Block::UVRect rect = block->GetAtlasUVRect(directions[face]);
            *rects[face] = glm::vec4(std::min(rect.u0, rect.u1), std::min(rect.v0, rect.v1), std::max(rect.u0, rect.u1), std::max(rect.v0, rect.v1));
            int minX     = std::clamp((int) (std::min(rect.u0, rect.u1) * width), 0, width);
            int maxX     = std::clamp((int) (std::max(rect.u0, rect.u1) * width + 0.5f), 0, width);
            int minY     = std::clamp((int) (std::min(rect.v0, rect.v1) * height), 0, height);
            int maxY     = std::clamp((int) (std::max(rect.v0, rect.v1) * height + 0.5f), 0, height);
            double sums[4] = {};
            for (int y = minY; y < maxY; y++)
            {
                for (int x = minX; x < maxX; x++)
                {
                    const uint8_t *pixel = &pixels[((size_t) y * width + x) * 4];
                    double alpha         = pixel[3] / 255.0;
                    sums[0] += pixel[0] * alpha;
                    sums[1] += pixel[1] * alpha;
                    sums[2] += pixel[2] * alpha;
                    sums[3] += alpha;
                }
            }

            if (sums[3] > 0.0)
            {
                *targets[face] = (uint32_t) (sums[0] / sums[3]) << 16 | (uint32_t) (sums[1] / sums[3]) << 8 | (uint32_t) (sums[2] / sums[3]);
            }
        }
    }

    m_blockColors[blockId] = colors;
    return colors;
}

uint32_t LodRenderer::TintColor(uint32_t color, uint32_t tint)
{
    uint32_t result = 0;
    for (int shift = 0; shift <= 16; shift += 8)
    {
        result |= (((color >> shift) & 0xff) * ((tint >> shift) & 0xff) / 255) << shift;
    }

    return result;
}

#endif
