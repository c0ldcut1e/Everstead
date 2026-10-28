#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../rendering/RenderTypes.h"
#include "../../rendering/Shader.h"
#include "../../rendering/TextureAtlas.h"
#include "../../scene/culling/FrustumCuller.h"
#include "../../threading/ThreadPool.h"
#include "../chunk/ChunkPos.h"
#include "../chunk/ChunkVertexArena.h"
#include "LodVolume.h"

#include <glm/glm.hpp>

class LevelRenderer;

class LodRenderer
{
public:
    explicit LodRenderer(LevelRenderer *levelRenderer);
    ~LodRenderer();

    void Clear();
    void Update(const glm::dvec3 &playerPos);
    void Render(const glm::dvec3 &cameraPos, const glm::dmat4 &viewMatrix, const glm::dmat4 &projection);
    void BindMask(Shader *shader, const glm::dvec3 &cameraPos) const;

    size_t GetTileCount() const;
    size_t GetDrawnTileCount() const;
    int GetInFlightCount() const;

    static const int s_maxMipmapLevels = TextureAtlas::s_mipLevelCount - 1;

private:
    enum eTILE_STATE : uint8_t
    {
        eTileState_None = 0,
        eTileState_Requested,
        eTileState_Meshing,
        eTileState_Ready
    };

    struct Vertex
    {
        uint16_t position[4];
        uint8_t color[4];
        uint16_t uv[4];
        uint8_t tint[4];
        uint8_t ao[4];
    };

    struct Tile
    {
        int level;
        int x;
        int z;
        eTILE_STATE state;
        bool split;
        float blend;
        uint32_t page;
        uint32_t first;
        uint32_t quadCount;
        int minY;
        int maxY;
        uint32_t readyMs;
        uint32_t checkedMs;
        uint64_t lastVisit;
        bool partial;
        bool refreshing;
    };

    struct MeshResult
    {
        uint64_t key;
        uint64_t generation;
        std::vector<Vertex> vertices;
        int minY;
        int maxY;
        bool partial;
    };

    struct BuiltTile
    {
        int level;
        int x;
        int z;
        LodVolume volume;
        LodVolume neighbors[4];
    };

    struct BuildQueue
    {
        std::mutex mutex;
        std::vector<BuiltTile> tiles;
        std::atomic<bool> cancelled;
    };

    struct DrawItem
    {
        Tile *tile;
        float fadeLow;
        float fadeHigh;
        bool drawn;
    };

    struct OcclusionItem
    {
        size_t drawItem;
        double minDistance;
        double maxDistance;
        double minAngle;
        double maxAngle;
    };

    struct BlockColors
    {
        uint32_t top;
        uint32_t side;
        glm::vec4 topRect;
        glm::vec4 sideRect;
    };

    struct RequestCandidate
    {
        Tile *tile;
        double priority;
    };

    static uint64_t MakeKey(int level, int tileX, int tileZ);
    static void SetupAttributes(BufferHandle vertexBuffer, BufferHandle offsetBuffer);

    Tile *GetTile(int level, int tileX, int tileZ);
    bool IsTileReady(const Tile *tile) const;
    float GetTileFade(const Tile *tile) const;
    bool IsTileCovered(int level, int tileX, int tileZ) const;
    void Visit(int level, int tileX, int tileZ, float fadeLow, float fadeHigh, bool fadeIn);
    void RequestTile(Tile *tile, double distance);
    void FreeTile(Tile *tile);
    void OnTile(BuiltTile &&built);
    void UploadResults();
    void SendRequests();
    void CullOccluded();
    void UpdateMask(const glm::dvec3 &cameraPos);
    bool IsColumnMeshed(const ChunkPos &pos, uint32_t meshedSections) const;
    bool IsTileMasked(int level, int tileX, int tileZ) const;
    std::vector<Vertex> BuildMesh(int level, const LodVolume &volume, const LodVolume *neighbors, int *minY, int *maxY);
    void AddFace(std::vector<Vertex> &vertices, int face, const glm::ivec3 &min, const glm::ivec3 &max, uint32_t color, uint32_t tint,
                 const glm::vec4 &rect);
    BlockColors GetBlockColors(uint32_t blockId);
    static uint32_t TintColor(uint32_t color, uint32_t tint);

    LevelRenderer *m_levelRenderer;
    Shader m_shader;
    FrustumCuller m_frustum;
    ThreadPool *m_meshPool;
    std::unordered_map<uint64_t, Tile> m_tiles;
    std::unordered_set<uint64_t> m_staleRequests;
    std::vector<DrawItem> m_drawItems;
    std::vector<OcclusionItem> m_occlusionItems;
    std::vector<float> m_horizon;
    ChunkVertexArena *m_arena;
    BufferHandle m_commandBuffer;
    BufferHandle m_cullCommandBuffer;
    BufferHandle m_boxBuffer;
    ProgramHandle m_cullProgram;
    std::vector<glm::vec4> m_boxes;
    std::vector<DrawIndirectCommand> m_commands;
    std::vector<float> m_instanceData;
    std::vector<RequestCandidate> m_requestCandidates;
    std::mutex m_resultMutex;
    std::vector<MeshResult> m_results;
    std::shared_ptr<BuildQueue> m_buildQueue;
    std::atomic<uint64_t> m_generation;
    std::unordered_map<uint32_t, BlockColors> m_blockColors;
    uint64_t m_frame;
    int m_inFlight;
    int m_dimension;
    glm::dvec3 m_cameraPos;
    int m_lodBlocks;
    int m_chunkBlocks;
    double m_splitFactor;
    float m_blendStep;

    TextureHandle m_maskTexture;
    std::vector<uint8_t> m_maskPixels;
    std::vector<uint8_t> m_maskComplete;
    int m_maskSize;
    int m_maskOriginX;
    int m_maskOriginZ;
    bool m_maskEnabled;
    uint64_t m_maskVersion;
    uint64_t m_maskFrame;
    uint32_t m_maskFadeMs;
    bool m_maskFading;
};
