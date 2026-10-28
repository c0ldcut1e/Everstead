#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../entity/EntityRenderer.h"
#include "../rendering/Framebuffer.h"
#include "../rendering/Shader.h"
#include "../rendering/Tesselator.h"
#include "../rendering/Texture.h"
#include "../rendering/TextureAtlas.h"
#include "../rendering/nimbus/Nimbus.h"
#include "../scene/culling/FrustumCuller.h"
#include "../threading/ThreadPool.h"
#include "../utils/Direction.h"
#include "MultiPlayerLevel.h"
#include "WorldEffects.h"
#include "chunk/ChunkMesh.h"
#include "chunk/ChunkPos.h"
#include "chunk/ChunkRenderRegion.h"
#include "chunk/SectionPos.h"
#include "environment/CloudMesh.h"
#include "lighting/ItemLights.h"
#include "lighting/LightCache.h"
#include "lighting/LightStorage.h"
#include "lod/LodRenderer.h"

#include <glm/vec2.hpp>

class Font;
class ItemInHandRenderer;
class LevelChunkMeshingService;
class LocalPlayer;
class Mob;

class LevelRenderer
{
public:
    explicit LevelRenderer(MultiPlayerLevel *level, int width, int height);
    ~LevelRenderer();

    void Render(float partialTicks, Framebuffer *outputFramebuffer = nullptr);
    void OnResize(int width, int height);
    void RenderNimbusImGui();
    void TickMeshing();
    void TickItemInHand(LocalPlayer *player);
    ItemInHandRenderer *GetItemInHandRenderer() const;
    LodRenderer *GetLodRenderer() const;

    void Rebuild();
    void RebuildChunk(const ChunkPos &pos);
    void RemeshAll();
    void RebuildChunkUrgent(const ChunkPos &pos);
    void RebuildChunkDeferred(const ChunkPos &pos, bool urgent);
    void RebuildSectionUrgent(const SectionPos &pos);
    void RebuildSectionDeferred(const SectionPos &pos, bool urgent);
    void RebuildSectionNow(const SectionPos &pos);
    void FlushMeshScheduling();
    void AddTear(const BlockPos &pos);
    void AddSettlingBlock(const BlockPos &pos, Block *block, uint16_t data);
    void AddAfterimage(const Entity *entity);
    WorldEffects &GetWorldEffects();
    void RebuildSectionsUrgentSynchronized(const std::vector<SectionPos> &positions);
    const ItemLights &GetItemLights() const;
    bool HasChunkMesh(const ChunkPos &pos) const;
    bool HasActiveRebuild(const ChunkPos &pos) const;
    void DropChunkMeshes(const ChunkPos &pos);
    size_t GetMeshedChunkCount() const;
    size_t GetMeshCpuBytes() const;
    size_t GetRegionCpuBytes() const;
    size_t GetVisibleChunkCount() const;
    size_t GetRenderedMeshCount() const;
    size_t GetPendingMeshCount() const;
    size_t GetQueuedRebuildCount() const;
    size_t GetUrgentRebuildCount() const;
    size_t GetReadyMeshCount();
    size_t GetMesherThreadCount() const;
    size_t GetActiveMesherTaskCount() const;

    bool ToggleChunkGrid();
    bool ToggleHitboxes();

    void SetBlockDestruction(uint8_t playerIndex, const BlockPos &pos, int stage);
    void AddRipple(const glm::dvec3 &position);

private:
    friend class LevelChunkMeshingService;
    friend class Lighting;
    friend class LodRenderer;

    struct BlockDestruction
    {
        BlockPos pos;
        int stage;
        uint32_t updateMs;
    };

    struct Ripple
    {
        glm::dvec3 position;
        int64_t tick;
        uint32_t light;
    };

    struct VisibleChunk
    {
        const SectionPos *pos;
        const std::vector<std::unique_ptr<ChunkMesh>> *meshes;
        double distanceSquared;
        int chunkWidth;
        const ChunkRenderRegion *region;
    };

    struct ChunkDraw
    {
        uint32_t page;
        Texture *texture;
        DrawIndirectCommand command;
    };

    struct ChunkBatch
    {
        uint32_t page;
        Texture *texture;
        uint32_t firstCommand;
        int commandCount;
        uint32_t maxQuads;
    };

    struct MeshSlot
    {
        uint32_t group;
        uint32_t first;
        uint32_t capacity;
        uint32_t count;
    };

    struct RegionDraw
    {
        ChunkRenderRegion *region;
        SectionPos pos;
        uint64_t generation;
        std::vector<MeshSlot> slots;
    };

    struct DrawGroup
    {
        uint32_t page;
        Texture *texture;
        uint32_t commandCount;
        uint32_t maxQuads;
    };

    struct OcclusionNode
    {
        int x;
        int y;
        int z;
    };

    struct VisibleParticle
    {
        TextureAtlas::Region region;
        glm::dvec3 pos;
        uint32_t color;
        float size;
    };

    struct CloudSection
    {
        int x;
        int z;
        int firstVertex;
        int vertexCount;
    };

    struct PendingMeshUpload
    {
        SectionPos pos;
        uint64_t generation;
        uint64_t syncGroup;
        uint64_t visibility;
        std::vector<ChunkMesh::PackedData> packedResults;
    };

    struct ReadyMeshSwap
    {
        uint64_t generation;
        bool hadMeshes;
        uint64_t syncGroup;
        uint64_t visibility;
        std::vector<ChunkMesh::PackedData> translucentMeshes;
        std::vector<ChunkMesh::PackedData> opaqueMeshes;
    };

    static std::vector<float> BuildStarVertices();

    static uint32_t MultiplyColorRgb(uint32_t color, uint32_t lightColor);
    static void PushCloudVertex(std::vector<float> *vertices, float x, float y, float z, float u, float v, float r, float g, float b, float a);
    static void PushCloudQuad(std::vector<float> *vertices, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3,
                              float x4, float y4, float z4, float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4, float r,
                              float g, float b, float a);
    static void PushSkyVertex(std::vector<float> *vertices, double x, double y, double z);
    static void PushSkyQuad(std::vector<float> *vertices, double x1, double y1, double z1, double x2, double y2, double z2, double x3, double y3,
                            double z3, double x4, double y4, double z4);
    static void EmitQuad(BufferBuilder *builder, float x1, float y1, float z1, float u1, float v1, float x2, float y2, float z2, float u2, float v2,
                         float x3, float y3, float z3, float u3, float v3, float x4, float y4, float z4, float u4, float v4);
    static void EmitSkyPlane(BufferBuilder *builder, float y, bool flip, float radius);
    static float GetSkyBrightnessFactor(float celestialAngle);
    static void SampleSkyColor(Level *level, float *r, float *g, float *b);
    static void SampleFogColor(Level *level, float *r, float *g, float *b);
    static bool GetSunriseColor(Level *level, float *color);
    static int GetMoonPhase(Level *level);
    static void ApplyClearColorScale(float sourceY, float *r, float *g, float *b);

    void SubmitMesh(const SectionPos &pos, uint64_t generation, uint64_t syncGroup, uint64_t visibility,
                    std::vector<ChunkMesh::PackedData> &&packedResults);
    void RemoveChunkFromPendingSyncGroups(const SectionPos &pos);
    void SetRegionSection(const SectionPos &pos, std::vector<ChunkMesh::PackedData> &&meshes, bool rebuild);
    void SetSectionVisibility(const SectionPos &pos, uint64_t visibility);
    void UpdateOcclusion(const glm::dvec3 &cameraPos);
    void RunOcclusionWorker();
    bool IsSectionVisible(const SectionPos &pos) const;
    bool IsRegionVisible(const SectionPos &regionPos) const;
    void RemoveRegionSection(const SectionPos &pos);
    void RebuildDirtyRegions();
    bool HasRegionSection(const SectionPos &pos) const;
    void SetTranslucentSection(const SectionPos &pos, std::vector<ChunkMesh::PackedData> &&meshes);
    void RemoveTranslucentSection(const SectionPos &pos);
    bool HasTranslucentSection(const SectionPos &pos) const;

    void SetupMatrices(const glm::dvec3 &cameraPos);
    void SetupFog(const glm::dvec3 &eyePos, float partialTicks);
    void RenderSky(const glm::dmat4 &viewMatrix, const glm::dmat4 &projection);
    void RenderClouds(const glm::dmat4 &viewMatrix, const glm::dmat4 &projection, float partialTicks, Framebuffer *outputFramebuffer);
    void RebuildCloudMesh(int radius, bool noCullMode);
    void RenderStars(const glm::dmat4 &viewMatrix, const glm::dmat4 &projection);
    void BindLevelShader(Shader *shader, const glm::dvec3 &cameraPos, const glm::dmat4 &viewMatrix, const glm::dmat4 &projection);
    void UploadPendingMeshes();
    void CollectVisibleChunks(const glm::dvec3 &playerPos, const glm::dvec3 &cameraPos, const glm::dmat4 &viewMatrix, const glm::dmat4 &projection,
                              std::vector<VisibleChunk> *opaqueChunks);
    void BuildChunkBatches(bool depthPrepass);
    void FreeMeshSlot(const MeshSlot &slot);
    void ClearRegionDraws();
    void UploadChunkBatches();
    void AppendChunkBatches(std::vector<ChunkBatch> *batches, std::vector<DrawIndirectCommand> *commands);
    void CullOpaqueChunks(const glm::dvec3 &playerPos);
    void BuildDepthPyramid(const glm::dmat4 &viewProjection);
    void DrawChunkBatches(const std::vector<ChunkBatch> &batches, BufferHandle commands, bool culled);
    void RenderChunks(Shader *shader);
    void RenderTranslucentChunks(bool depthPrepass);
    void RenderTranslucentFrontChunks();
    void RenderParticles(float partialTicks);
    void CopySceneDepth(Framebuffer *sceneFramebuffer, Framebuffer *outputFramebuffer);
    void RenderWeather(float partialTicks);
    void RenderWorldBorder(float partialTicks);
    void FindTears();
    void RenderTears(Framebuffer *outputFramebuffer, const glm::dmat4 &projection, float partialTicks);
    void RenderWorldEffects(Framebuffer *outputFramebuffer, const glm::dmat4 &projection, float partialTicks);
    void BeginHalfResPass(Framebuffer *outputFramebuffer);
    void CompositeHalfResPass(Framebuffer *outputFramebuffer, Framebuffer *source);
    void RenderWeatherPass(bool snow, float rain, const glm::vec2 &wind, const glm::dvec3 &cameraPos, int cameraX, int cameraY, int cameraZ,
                           BufferBuilder *builder);
    void RenderWeatherHaze(float radius, BufferBuilder *builder);
    void RenderRipples(float partialTicks, const glm::dvec3 &cameraPos, BufferBuilder *builder);
    void EmitParticles(const std::vector<VisibleParticle> &items, Texture *texture, const glm::dvec3 &right, const glm::dvec3 &up, bool depthOnly);
    void RenderChunkGrid() const;
    void RenderBlockOutline();
    void RenderBlockDestruction();
    void EmitDestroyStage(BufferBuilder *builder, const BlockPos &pos, int stage);
    void RenderEntities(float partialTicks);
    void RenderEntityShadows(float partialTicks);
    void AddEntityShadow(BufferBuilder *builder, Entity *entity, const glm::dvec3 &renderPos, const glm::dvec3 &cameraPos);
    void RenderSignText(const glm::dvec3 &cameraPos);
    void RenderEntityNameTags(float partialTicks);
    void RenderNameTagPlate(Font *font, float width, float scale, float pixelSize, uint32_t fillColor, uint32_t borderColor, bool throughWalls);
    static void AppendNameTagQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1);
    void SetFogUniforms(Shader *shader, bool applyFog);
    void RenderFogPass(const glm::dmat4 &projection, Framebuffer *sourceFramebuffer, bool applyFog);

    void ScheduleMesher();

    void UpdateLightState(const WorldProvider &worldProvider);
    uint32_t SampleWorldLightColor(const glm::dvec3 &pos) const;
    uint32_t SampleEntityLightColor(const Entity *entity, float partialTicks) const;
    void RenderLeash(const Mob *mob, float partialTicks) const;
    void RenderAfterimages(LocalPlayer *localPlayer, float partialTicks);

    Shader *m_levelShader;
    Shader *m_waterShader;
    Shader *m_fogShader;
    Shader *m_tearShader;
    Shader *m_worldEffectShader;
    Shader *m_compositeShader;
    Shader *m_worldEffectResolveShader;
    Framebuffer *m_halfResTarget;
    Framebuffer *m_worldEffectHistory[2];
    int m_worldEffectHistoryIndex;
    bool m_worldEffectHistoryValid;
    uint32_t m_worldEffectFrame;
    glm::dmat4 m_worldEffectViewProj;
    glm::dvec3 m_worldEffectCameraPos;
    VertexArrayHandle m_fullscreenVao;
    float m_fogDarken;
    float m_leafSnow;
    glm::vec3 m_fogColor;
    float m_skyFog;
    float m_voidFogSkyLight;
    bool m_biomeFogReady;
    std::vector<ParticleEngine::RenderParticle> m_particleScratch;
    std::vector<VisibleParticle> m_visibleParticles;
    std::vector<VisibleParticle> m_visibleTerrainParticles;
    std::vector<VisibleParticle> m_visibleItemParticles;
    std::unordered_map<BlockPos, uint32_t> m_particleLightCache;
    int64_t m_particleLightCacheTick;

    int m_starVertexCount;
    std::vector<float> m_starVertices;

    Framebuffer *m_sceneFramebuffer;
    Nimbus *m_nimbus;
    ItemInHandRenderer *m_itemInHandRenderer;
    LodRenderer *m_lodRenderer;

    MultiPlayerLevel *m_level;

    glm::dvec3 m_renderCameraPos;

    FrustumCuller m_frustum;
    bool m_frustumCulling;
    bool m_gpuCulling;
    double m_fogCullDistance;

    EntityRenderer m_entityRenderer;

    bool m_renderChunkGrid;
    bool m_renderHitboxes;

    Texture *m_cloudTexture;
    CloudMesh *m_cloudMesh;
    std::vector<float> m_cloudVertices;
    std::vector<CloudSection> m_cloudSections;
    float m_cloudOffset;
    int m_cloudRadius;
    bool m_cloudNoCullMode;
    Texture *m_grassSideOverlayMaskTexture;
    Texture *m_rainTexture;
    Texture *m_forcefieldTexture;

    struct Tear
    {
        BlockPos pos;
        float open;
    };

    std::vector<Tear> m_tears;

    struct SettlingBlock
    {
        BlockPos pos;
        Block *block;
        uint16_t data;
        uint64_t generation;
    };

    std::vector<SettlingBlock> m_settlingBlocks;

    struct Afterimage
    {
        int entityId;
        std::vector<glm::dvec3> trail;
        uint32_t startTime;
        uint32_t sampleTime;
        uint32_t stopTime;
    };

    std::vector<Afterimage> m_afterimages;
    uint32_t m_tearScanTime;
    uint32_t m_tearFrameTime;
    std::array<Texture *, 10> m_destroyStageTextures;
    std::unordered_map<uint8_t, BlockDestruction> m_blockDestructions;
    std::vector<int> m_rainHeights;
    std::vector<uint32_t> m_rainLight;
    std::vector<bool> m_rainSnow;
    std::vector<Ripple> m_ripples;
    Texture *m_snowTexture;
    Texture *m_rippleTexture;
    double m_weatherTime;
    float m_rainPhase;
    float m_snowPhase;
    float m_weatherFog;
    float m_snowFog;
    std::array<int, 2> m_weatherColumns;
    Texture *m_shadowTexture;
    int64_t m_rainCacheTick;
    int m_rainCacheX;
    int m_rainCacheZ;

    LightStorage m_lightStorage;
    LightCache m_lightCache;
    ItemLights m_itemLights;
    WorldEffects m_worldEffects;
    int m_lastLightCacheClampKey;

    std::unordered_map<SectionPos, std::unique_ptr<ChunkRenderRegion>, SectionPosHash> m_translucentRegions;
    std::unordered_map<SectionPos, std::unique_ptr<ChunkRenderRegion>, SectionPosHash> m_renderRegions;
    std::unordered_set<SectionPos, SectionPosHash> m_dirtyRegions;
    std::unordered_map<ChunkPos, std::array<uint64_t, Chunk::s_sectionCount>, ChunkPosHash> m_columnVisibility;
    std::unordered_map<ChunkPos, uint32_t, ChunkPosHash> m_meshedSections;
    uint64_t m_meshedSectionsVersion;
    std::vector<uint64_t> m_occlusionColumns;
    std::vector<uint8_t> m_visibleSections;
    std::vector<uint8_t> m_occlusionResult;
    std::vector<OcclusionNode> m_occlusionQueue;
    std::thread m_occlusionThread;
    std::mutex m_occlusionMutex;
    std::condition_variable m_occlusionCondition;
    int m_occlusionJobOriginX;
    int m_occlusionJobOriginY;
    int m_occlusionJobOriginZ;
    int m_occlusionJobCameraY;
    int m_occlusionJobRadius;
    bool m_occlusionRequested;
    bool m_occlusionFinished;
    bool m_occlusionStop;
    bool m_occlusionBusy;
    std::unique_ptr<ThreadPool> m_sortPool;
    std::vector<ChunkRenderRegion::QuadSortJob> m_sortJobs;
    std::atomic<bool> m_sortBusy;
    std::vector<std::pair<double, size_t>> m_translucentRangeOrder;
    std::vector<float> m_chunkOffsets;
    std::vector<ChunkDraw> m_chunkDraws;
    std::vector<DrawGroup> m_drawGroups;
    std::vector<uint32_t> m_groupWindows;
    std::vector<uint32_t> m_cpuCullCursors;
    std::vector<DrawIndirectCommand> m_cpuCulledCommands;
    std::vector<DrawIndirectCommand> m_chunkCommands;
    std::vector<ChunkBatch> m_opaqueBatches;
    std::vector<ChunkBatch> m_translucentBatches[3];
    BufferHandle m_chunkCommandBuffer;
    std::vector<RegionDraw> m_regionDraws;
    std::vector<uint32_t> m_freeRegionDraws;
    std::array<std::vector<uint32_t>, 32> m_freeSlots;
    std::vector<DrawIndirectCommand> m_opaqueCommands;
    std::vector<glm::ivec4> m_opaqueSections;
    std::vector<std::pair<uint32_t, uint32_t>> m_dirtyCommands;
    BufferHandle m_opaqueCommandBuffer;
    BufferHandle m_opaqueSectionBuffer;
    BufferHandle m_culledCommandBuffer;
    BufferHandle m_culledCountBuffer;
    BufferHandle m_groupWindowBuffer;
    BufferHandle m_visibilityBuffer;
    size_t m_opaqueCommandCapacity;
    ProgramHandle m_cullProgram;
    ProgramHandle m_hizProgram;
    BufferHandle m_hizBuffer;
    glm::dmat4 m_hizMatrix;
    glm::dvec3 m_hizCameraPos;
    int m_hizWidth;
    int m_hizHeight;
    int m_hizLevels;
    bool m_hizValid;
    std::vector<const ChunkRenderRegion *> m_batchRegions;
    std::vector<const ChunkRenderRegion *> m_visibleRegions;
    size_t m_batchOpaqueCount;
    std::vector<uint64_t> m_batchGenerations;
    glm::ivec3 m_batchCameraBlock;
    bool m_batchDepthPrepass;
    bool m_chunkBatchesDirty;
    int m_occlusionOriginX;
    int m_occlusionOriginY;
    int m_occlusionOriginZ;
    int m_occlusionRadius;
    bool m_occlusionValid;
    bool m_occlusionDirty;
    uint32_t m_lastOcclusionUpdateMs;
    static const uint32_t s_occlusionUpdateIntervalMs = 250;
    static constexpr double kSkyHorizonHeight         = 63.0;
    static const uint8_t s_occlusionStartMask         = 0x40;
    static const int s_occlusionAllFaces              = 0x3F;
    static const int s_translucentSortBudget          = 32;
    std::vector<VisibleChunk> m_translucentChunks;
    static constexpr int kDirtyRegionRebuildsPerFrame = 2;
    size_t m_lastVisibleChunkCount{0};
    size_t m_lastRenderedMeshCount{0};
    mutable std::mutex m_meshQueueMutex;
    std::deque<PendingMeshUpload> m_pendingMeshes;
    std::unordered_map<SectionPos, ReadyMeshSwap, SectionPosHash> m_readyMeshes;
    double m_lastUploadTime{0.0};

    std::unique_ptr<ThreadPool> m_mesherPool;
    std::atomic<bool> m_mesherRunning{false};
    std::atomic<int> m_activeMesherTasks{0};
    size_t m_maxMesherTasks{1};
    // This is the Chebyshev chunk radius that gets meshes while memory is tight, or -1 when it follows the render distance
    int m_meshRadius{-1};
    uint32_t m_meshRadiusChangeMs{0};
    uint32_t m_meshRadiusGrowMs{0};

    bool m_grassSideOverlayEnabled;

    mutable std::mutex m_rebuildQueueMutex;
    std::vector<SectionPos> m_rebuildQueue;
    std::vector<SectionPos> m_urgentQueue;
    std::unordered_set<SectionPos, SectionPosHash> m_rebuildQueued;
    std::unordered_set<SectionPos, SectionPosHash> m_urgentQueued;
    std::unordered_set<SectionPos, SectionPosHash> m_activeRebuilds;
    std::unordered_set<SectionPos, SectionPosHash> m_deferredRebuilds;
    std::unordered_set<SectionPos, SectionPosHash> m_deferredUrgentRebuilds;
    std::unordered_map<SectionPos, uint64_t, SectionPosHash> m_requestedMeshGenerations;
    std::unordered_map<SectionPos, uint64_t, SectionPosHash> m_requestedMeshSyncGroups;
    std::unordered_map<uint64_t, std::unordered_set<SectionPos, SectionPosHash>> m_pendingSyncGroups;
    uint64_t m_nextSyncGroupId{1};
    // This is global so a chunk that unloads and reloads can never match a stale in-flight mesh generation
    uint64_t m_nextMeshGeneration{0};
};
