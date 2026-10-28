#ifndef EVERSTEAD_DEDICATED_SERVER

#include "LevelRenderer.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>
#include <limits>
#include <numbers>
#include <vector>

#include "../core/Everstead.h"
#include "../core/FrameProfiler.h"
#include "../core/Logger.h"
#include "../entity/EntityRendererRegistry.h"
#include "../entity/Mob.h"
#include "../entity/item/Boat.h"
#include "../entity/item/BoatRenderer.h"
#include "../entity/item/ItemEntity.h"
#include "../entity/player/RemotePlayer.h"
#include "../memory/MemoryTracker.h"
#include "../rendering/BlockAtlas.h"
#include "../rendering/GlStateManager.h"
#include "../rendering/ItemAtlas.h"
#include "../rendering/ParticleAtlas.h"
#include "../rendering/RenderDevice.h"
#include "../rendering/Tesselator.h"
#include "../rendering/TextureRepository.h"
#include "../rendering/Textures.h"
#include "../scene/Camera.h"
#include "../threading/ThreadStorage.h"
#include "../ui/ChatMessageParser.h"
#include "../utils/Random.h"
#include "../utils/StringUtils.h"
#include "../utils/Time.h"
#include "../utils/math/Mth.h"
#include "ItemInHandRenderer.h"
#include "LevelChunkMeshingService.h"
#include "biome/Biome.h"
#include "block/Block.h"
#include "block/BlockRegistry.h"
#include "block/BlockRenderer.h"
#include "block/FractureCoreBlock.h"
#include "block/SignBlock.h"
#include "blockentity/SignBlockEntity.h"
#include "blockentity/renderer/BlockEntityRendererRegistry.h"
#include "chunk/ChunkMesh.h"
#include "chunk/ChunkVertexArena.h"
#include "chunk/SectionVisibility.h"
#include "item/ItemRegistry.h"
#include "lighting/LightEngine.h"
#include "lighting/LightSource.h"
#include "lighting/Lighting.h"
#include "models/Model.h"
#include "models/ModelDefinition.h"
#include "models/ModelPartsSkinned.h"
#include "particle/ParticleEngine.h"
#include "particle/ParticleRegistry.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

static const float s_underwaterFogDensity     = 0.1f;
static const float s_underwaterFogMinimum     = 0.25f;
static const float s_underwaterFogCullOpacity = 0.999f;
static const float s_underwaterFogRed         = 0.4f;
static const float s_underwaterFogGreen       = 0.4f;
static const float s_underwaterFogBlue        = 0.9f;

static void EmitBorderWall(BufferBuilder *builder, bool alongZ, float fixed, float from, float to, float u0, float u1, float bottom, float top,
                           float vBottom, float vTop)
{
    if (alongZ)
    {
        builder->VertexUV(fixed, bottom, from, u0, vBottom);
        builder->VertexUV(fixed, bottom, to, u1, vBottom);
        builder->VertexUV(fixed, top, to, u1, vTop);
        builder->VertexUV(fixed, bottom, from, u0, vBottom);
        builder->VertexUV(fixed, top, to, u1, vTop);
        builder->VertexUV(fixed, top, from, u0, vTop);
        return;
    }

    builder->VertexUV(from, bottom, fixed, u0, vBottom);
    builder->VertexUV(to, bottom, fixed, u1, vBottom);
    builder->VertexUV(to, top, fixed, u1, vTop);
    builder->VertexUV(from, bottom, fixed, u0, vBottom);
    builder->VertexUV(to, top, fixed, u1, vTop);
    builder->VertexUV(from, top, fixed, u0, vTop);
}

uint32_t LevelRenderer::MultiplyColorRgb(uint32_t color, uint32_t lightColor)
{
    uint32_t a  = (color >> 24) & 0xFF;
    uint32_t r  = (color >> 16) & 0xFF;
    uint32_t g  = (color >> 8) & 0xFF;
    uint32_t b  = color & 0xFF;
    uint32_t lr = (lightColor >> 16) & 0xFF;
    uint32_t lg = (lightColor >> 8) & 0xFF;
    uint32_t lb = lightColor & 0xFF;

    r = (r * lr + 127) / 255;
    g = (g * lg + 127) / 255;
    b = (b * lb + 127) / 255;

    return (a << 24) | (r << 16) | (g << 8) | b;
}

void LevelRenderer::PushCloudVertex(std::vector<float> *vertices, float x, float y, float z, float u, float v, float r, float g, float b, float a)
{
    vertices->push_back(x);
    vertices->push_back(y);
    vertices->push_back(z);
    vertices->push_back(u);
    vertices->push_back(v);
    vertices->push_back(r);
    vertices->push_back(g);
    vertices->push_back(b);
    vertices->push_back(a);
}

void LevelRenderer::PushCloudQuad(std::vector<float> *vertices, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3,
                                  float z3, float x4, float y4, float z4, float u1, float v1, float u2, float v2, float u3, float v3, float u4,
                                  float v4, float r, float g, float b, float a)
{
    PushCloudVertex(vertices, x1, y1, z1, u1, v1, r, g, b, a);
    PushCloudVertex(vertices, x2, y2, z2, u2, v2, r, g, b, a);
    PushCloudVertex(vertices, x3, y3, z3, u3, v3, r, g, b, a);
    PushCloudVertex(vertices, x1, y1, z1, u1, v1, r, g, b, a);
    PushCloudVertex(vertices, x3, y3, z3, u3, v3, r, g, b, a);
    PushCloudVertex(vertices, x4, y4, z4, u4, v4, r, g, b, a);
}

void LevelRenderer::PushSkyVertex(std::vector<float> *vertices, double x, double y, double z)
{
    vertices->push_back((float) x);
    vertices->push_back((float) y);
    vertices->push_back((float) z);
}

void LevelRenderer::PushSkyQuad(std::vector<float> *vertices, double x1, double y1, double z1, double x2, double y2, double z2, double x3, double y3,
                                double z3, double x4, double y4, double z4)
{
    PushSkyVertex(vertices, x1, y1, z1);
    PushSkyVertex(vertices, x2, y2, z2);
    PushSkyVertex(vertices, x3, y3, z3);
    PushSkyVertex(vertices, x1, y1, z1);
    PushSkyVertex(vertices, x3, y3, z3);
    PushSkyVertex(vertices, x4, y4, z4);
}

void LevelRenderer::EmitQuad(BufferBuilder *builder, float x1, float y1, float z1, float u1, float v1, float x2, float y2, float z2, float u2,
                             float v2, float x3, float y3, float z3, float u3, float v3, float x4, float y4, float z4, float u4, float v4)
{
    builder->VertexUV(x1, y1, z1, u1, v1);
    builder->VertexUV(x2, y2, z2, u2, v2);
    builder->VertexUV(x3, y3, z3, u3, v3);
    builder->VertexUV(x1, y1, z1, u1, v1);
    builder->VertexUV(x3, y3, z3, u3, v3);
    builder->VertexUV(x4, y4, z4, u4, v4);
}

void LevelRenderer::EmitSkyPlane(BufferBuilder *builder, float y, bool flip, float radius)
{
    const int s = 64;
    int d       = (int) (radius / s) + 1;

    for (int xx = -s * d; xx <= s * d; xx += s)
    {
        for (int zz = -s * d; zz <= s * d; zz += s)
        {
            if (flip)
            {
                EmitQuad(builder, (float) (xx + s), y, (float) (zz + 0), 0.0f, 0.0f, (float) (xx + 0), y, (float) (zz + 0), 0.0f, 0.0f,
                         (float) (xx + 0), y, (float) (zz + s), 0.0f, 0.0f, (float) (xx + s), y, (float) (zz + s), 0.0f, 0.0f);
            }
            else
            {
                EmitQuad(builder, (float) (xx + 0), y, (float) (zz + 0), 0.0f, 0.0f, (float) (xx + s), y, (float) (zz + 0), 0.0f, 0.0f,
                         (float) (xx + s), y, (float) (zz + s), 0.0f, 0.0f, (float) (xx + 0), y, (float) (zz + s), 0.0f, 0.0f);
            }
        }
    }
}

float LevelRenderer::GetSkyBrightnessFactor(float celestialAngle)
{
    float factor = cosf(celestialAngle * 6.28318530718f) * 2.0f + 0.5f;
    return std::clamp(factor, 0.0f, 1.0f);
}

void LevelRenderer::SampleSkyColor(Level *level, float *r, float *g, float *b)
{
    const Fog &fog = level->GetWorldProvider().GetFog();

    *r = (float) fog.GetSkyColor().x;
    *g = (float) fog.GetSkyColor().y;
    *b = (float) fog.GetSkyColor().z;

    float factor = GetSkyBrightnessFactor(level->GetCelestialAngle());
    *r *= factor;
    *g *= factor;
    *b *= factor;

    float rain = level->GetRainLevel(1.0f);
    if (rain > 0.0f)
    {
        float grey = (*r * 0.3f + *g * 0.59f + *b * 0.11f) * 0.6f;
        float keep = 1.0f - rain * 0.75f;
        *r         = *r * keep + grey * (1.0f - keep);
        *g         = *g * keep + grey * (1.0f - keep);
        *b         = *b * keep + grey * (1.0f - keep);
    }

    float thunder = level->GetThunderLevel(1.0f);
    if (thunder > 0.0f)
    {
        float grey = (*r * 0.3f + *g * 0.59f + *b * 0.11f) * 0.2f;
        float keep = 1.0f - thunder * 0.75f;
        *r         = *r * keep + grey * (1.0f - keep);
        *g         = *g * keep + grey * (1.0f - keep);
        *b         = *b * keep + grey * (1.0f - keep);
    }

    float flash = level->GetLightningFlash(0.0f) * 0.45f;
    if (flash > 0.0f)
    {
        *r = *r * (1.0f - flash) + 0.8f * flash;
        *g = *g * (1.0f - flash) + 0.8f * flash;
        *b = *b * (1.0f - flash) + 1.0f * flash;
    }
}

void LevelRenderer::SampleFogColor(Level *level, float *r, float *g, float *b)
{
    const Fog &fog = level->GetWorldProvider().GetFog();

    *r = (float) fog.GetColor().x;
    *g = (float) fog.GetColor().y;
    *b = (float) fog.GetColor().z;

    float factor = GetSkyBrightnessFactor(level->GetCelestialAngle());
    *r *= factor * 0.94f + 0.06f;
    *g *= factor * 0.94f + 0.06f;
    *b *= factor * 0.91f + 0.09f;

    float skyR;
    float skyG;
    float skyB;
    SampleSkyColor(level, &skyR, &skyG, &skyB);
    float skyBlend = 1.0f - powf(0.25f + 0.75f * (float) level->GetRenderDistance() / 32.0f, 0.25f);
    *r += (skyR - *r) * skyBlend;
    *g += (skyG - *g) * skyBlend;
    *b += (skyB - *b) * skyBlend;

    float rain    = level->GetRainLevel(1.0f);
    float thunder = level->GetThunderLevel(1.0f);
    *r *= (1.0f - rain * 0.5f) * (1.0f - thunder * 0.5f);
    *g *= (1.0f - rain * 0.5f) * (1.0f - thunder * 0.5f);
    *b *= (1.0f - rain * 0.4f) * (1.0f - thunder * 0.5f);
}

bool LevelRenderer::GetSunriseColor(Level *level, float *color)
{
    float span   = 0.4f;
    float offset = cosf(level->GetCelestialAngle() * 6.28318530718f);

    if (offset < -span || offset > span)
    {
        return false;
    }

    float blend = (offset / span) * 0.5f + 0.5f;
    float alpha = 1.0f - (1.0f - sinf(blend * 3.14159265359f)) * 0.99f;
    alpha *= alpha;

    color[0] = blend * 0.3f + 0.7f;
    color[1] = blend * blend * 0.7f + 0.2f;
    color[2] = blend * blend * 0.0f + 0.2f;
    color[3] = alpha;

    return true;
}

int LevelRenderer::GetMoonPhase(Level *level)
{
    int64_t day = level->GetWorldProvider().GetWorldTime() / (int64_t) WorldProvider::s_dayTicks;
    return (int) (((day % 8) + 8) % 8);
}

void LevelRenderer::ApplyClearColorScale(float sourceY, float *r, float *g, float *b)
{
    double scaled = (double) sourceY / 32.0;
    if (scaled < 1.0)
    {
        if (scaled < 0.0)
        {
            scaled = 0.0;
        }

        scaled *= scaled;
        *r = (float) ((double) *r * scaled);
        *g = (float) ((double) *g * scaled);
        *b = (float) ((double) *b * scaled);
    }
}

LevelRenderer::LevelRenderer(MultiPlayerLevel *level, int width, int height)
{
    m_levelShader                 = nullptr;
    m_waterShader                 = nullptr;
    m_fogShader                   = nullptr;
    m_tearScanTime                = 0;
    m_tearFrameTime               = 0;
    m_tearShader                  = nullptr;
    m_worldEffectShader           = nullptr;
    m_compositeShader             = nullptr;
    m_halfResTarget               = nullptr;
    m_worldEffectResolveShader    = nullptr;
    m_worldEffectHistory[0]       = nullptr;
    m_worldEffectHistory[1]       = nullptr;
    m_worldEffectHistoryIndex     = 0;
    m_worldEffectHistoryValid     = false;
    m_worldEffectFrame            = 0;
    m_worldEffectViewProj         = glm::dmat4(1.0);
    m_worldEffectCameraPos        = glm::dvec3(0.0);
    m_fullscreenVao               = {};
    m_fogDarken                   = 1.0f;
    m_fogColor                    = glm::vec3(0.0f, 0.0f, 0.0f);
    m_skyFog                      = 0.0f;
    m_voidFogSkyLight             = -1.0f;
    m_biomeFogReady               = false;
    m_starVertexCount             = 0;
    m_sceneFramebuffer            = nullptr;
    m_nimbus                      = nullptr;
    m_itemInHandRenderer          = new ItemInHandRenderer();
    m_level                       = level;
    m_renderCameraPos             = glm::dvec3();
    m_renderChunkGrid             = false;
    m_renderHitboxes              = false;
    m_cloudTexture                = nullptr;
    m_rainTexture                 = nullptr;
    m_rainHeights                 = {};
    m_rainLight                   = {};
    m_rainSnow                    = {};
    m_ripples                     = {};
    m_snowTexture                 = nullptr;
    m_rippleTexture               = nullptr;
    m_weatherTime                 = 0.0;
    m_rainPhase                   = 0.0f;
    m_snowPhase                   = 0.0f;
    m_weatherFog                  = 0.0f;
    m_snowFog                     = 0.0f;
    m_weatherColumns              = {};
    m_rainCacheTick               = -1;
    m_particleLightCacheTick      = -1;
    m_rainCacheX                  = 0;
    m_rainCacheZ                  = 0;
    m_cloudMesh                   = nullptr;
    m_cloudOffset                 = 0.0f;
    m_leafSnow                    = 0.0f;
    m_cloudRadius                 = -1;
    m_cloudNoCullMode             = false;
    m_grassSideOverlayMaskTexture = nullptr;
    m_lastLightCacheClampKey      = -1;
    m_grassSideOverlayEnabled     = true;
    m_occlusionOriginX            = 0;
    m_occlusionOriginY            = 0;
    m_occlusionOriginZ            = 0;
    m_occlusionRadius             = 0;
    m_occlusionValid              = false;
    m_occlusionDirty              = true;
    m_meshedSectionsVersion       = 0;
    m_lastOcclusionUpdateMs       = 0;
    m_occlusionJobOriginX         = 0;
    m_occlusionJobOriginY         = 0;
    m_occlusionJobOriginZ         = 0;
    m_occlusionJobCameraY         = 0;
    m_occlusionJobRadius          = 0;
    m_occlusionRequested          = false;
    m_occlusionFinished           = false;
    m_occlusionStop               = false;
    m_occlusionBusy               = false;
    m_occlusionThread             = std::thread(&LevelRenderer::RunOcclusionWorker, this);
    m_sortPool                    = std::make_unique<ThreadPool>(1, true);
    m_sortBusy                    = false;
    m_batchOpaqueCount            = 0;
    m_opaqueCommandCapacity       = 0;
    m_batchDepthPrepass           = false;
    m_chunkBatchesDirty           = true;
    // Entry 0 stays empty so partial updates never write at offset 0, where threaded Mesa stalls on large subdata
    m_opaqueCommands.assign(1, {});
    m_opaqueSections.assign(1, glm::ivec4(0));

    m_frustumCulling  = true;
    m_gpuCulling      = true;
    m_fogCullDistance = 0.0;
    m_hizMatrix       = glm::dmat4(1.0);
    m_hizCameraPos    = glm::dvec3();
    m_hizWidth        = 0;
    m_hizHeight       = 0;
    m_hizLevels       = 0;
    m_hizValid        = false;

    RenderDevice *device  = RenderDevice::GetInstance();
    m_chunkCommandBuffer  = device->CreateBuffer();
    m_opaqueCommandBuffer = device->CreateBuffer();
    m_opaqueSectionBuffer = device->CreateBuffer();
    m_culledCommandBuffer = device->CreateBuffer();
    m_culledCountBuffer   = device->CreateBuffer();
    m_groupWindowBuffer   = device->CreateBuffer();
    m_visibilityBuffer    = device->CreateBuffer();
    m_cullProgram         = device->CreateComputeProgram("assets/shader/chunk_cull.comp", {"CullParams", 256}, 7, {});
    m_hizProgram          = device->CreateComputeProgram("assets/shader/depth_pyramid.comp", {"PyramidParams", 32}, 1, {{"u_depth", 2}});
    m_hizBuffer           = device->CreateBuffer();
    // Intel drivers crash dispatching a compute shader with a storage buffer that has no data store yet
    uint32_t emptyBuffer        = 0;
    BufferHandle cullBuffers[3] = {m_visibilityBuffer, m_groupWindowBuffer, m_hizBuffer};
    for (int i = 0; i < 3; i++)
    {
        device->BindArrayBuffer(cullBuffers[i]);
        device->UploadArrayBuffer(&emptyBuffer, sizeof(emptyBuffer), eBufferUsage_Static);
    }

    m_levelShader              = new Shader("assets/shader/level.vert", "assets/shader/level.frag");
    m_waterShader              = new Shader("assets/shader/level.vert", "assets/shader/water.frag");
    m_fogShader                = new Shader("assets/shader/fog.vert", "assets/shader/fog.frag");
    m_tearShader               = new Shader("assets/shader/fog.vert", "assets/shader/tear.frag");
    m_worldEffectShader        = new Shader("assets/shader/fog.vert", "assets/shader/world_effect.frag");
    m_compositeShader          = new Shader("assets/shader/gamma.vert", "assets/shader/gamma.frag");
    m_halfResTarget            = new Framebuffer(1, 1);
    m_worldEffectResolveShader = new Shader("assets/shader/fog.vert", "assets/shader/world_effect_resolve.frag");
    m_worldEffectHistory[0]    = new Framebuffer(1, 1, eTextureFormat_RGBA16F);
    m_worldEffectHistory[1]    = new Framebuffer(1, 1, eTextureFormat_RGBA16F);
    m_fullscreenVao            = RenderDevice::GetInstance()->CreateVertexArray();
    m_lodRenderer              = new LodRenderer(this);

    m_cloudTexture = TextureRepository::GetInstance()->GetRaw("assets/texture/terrain/clouds.png");
    m_cloudTexture->SetWrapMode(eTextureWrap_Repeat, eTextureWrap_Repeat);
    m_cloudTexture->SetFilterMode(eTextureFilter_Nearest, eTextureFilter_Nearest);
    m_cloudMesh = new CloudMesh();

    m_rainTexture = TextureRepository::GetInstance()->GetRaw("assets/texture/terrain/rain.png");
    m_rainTexture->SetWrapMode(eTextureWrap_Repeat, eTextureWrap_Repeat);
    m_rainTexture->SetFilterMode(eTextureFilter_Nearest, eTextureFilter_Nearest);

    m_forcefieldTexture = TextureRepository::GetInstance()->GetRaw("assets/texture/misc/forcefield.png");
    m_forcefieldTexture->SetWrapMode(eTextureWrap_Repeat, eTextureWrap_Repeat);
    m_forcefieldTexture->SetFilterMode(eTextureFilter_Nearest, eTextureFilter_Nearest);

    for (size_t i = 0; i < m_destroyStageTextures.size(); i++)
    {
        m_destroyStageTextures[i] = TextureRepository::GetInstance()->GetRaw("assets/texture/misc/destroy_stage_" + std::to_string(i) + ".png");
        m_destroyStageTextures[i]->SetFilterMode(eTextureFilter_Nearest, eTextureFilter_Nearest);
    }

    m_snowTexture = TextureRepository::GetInstance()->GetRaw("assets/texture/terrain/snow.png");
    m_snowTexture->SetWrapMode(eTextureWrap_Repeat, eTextureWrap_Repeat);
    m_snowTexture->SetFilterMode(eTextureFilter_Nearest, eTextureFilter_Nearest);

    m_rippleTexture = TextureRepository::GetInstance()->GetRaw("assets/texture/terrain/ripple.png");
    m_rippleTexture->SetFilterMode(eTextureFilter_Nearest, eTextureFilter_Nearest);

    m_shadowTexture = TextureRepository::GetInstance()->GetRaw("assets/texture/misc/shadow.png");
    if (m_shadowTexture)
    {
        m_shadowTexture->SetWrapMode(eTextureWrap_ClampToEdge, eTextureWrap_ClampToEdge);
        m_shadowTexture->SetFilterMode(eTextureFilter_Nearest, eTextureFilter_Nearest);
    }

    m_grassSideOverlayMaskTexture = TextureRepository::GetInstance()->Get("assets/texture/block/grass_side_overlay.png").get();
    if (m_grassSideOverlayMaskTexture)
    {
        Textures::UploadAlphaSafeMipmaps(m_grassSideOverlayMaskTexture->GetId(), m_grassSideOverlayMaskTexture->GetPixels(),
                                         m_grassSideOverlayMaskTexture->GetPixelWidth(), m_grassSideOverlayMaskTexture->GetPixelHeight(), 1, 1, 4);
    }

    m_starVertices    = BuildStarVertices();
    m_starVertexCount = (int) (m_starVertices.size() / 3);

    m_sceneFramebuffer = new Framebuffer(width, height);
    m_nimbus           = new Nimbus(width, height, &m_level->GetDynamicLights(), &m_itemLights);

    m_mesherRunning      = true;
    size_t mesherThreads = (size_t) Everstead::GetInstance()->GetChunkWorkerThreadsPerPool();
    m_mesherPool         = std::make_unique<ThreadPool>(mesherThreads);
    m_maxMesherTasks     = std::max((size_t) 1, mesherThreads);

    float skyLightClamp = LightSource::SampleSkyLightClamp(m_level->GetWorldProvider());
    m_lightStorage.Reset(skyLightClamp);
    m_lightCache.Rebuild(skyLightClamp);
    m_lastLightCacheClampKey = (int) floorf(skyLightClamp * 4096.0f + 0.5f);
}

LevelRenderer::~LevelRenderer()
{
    {
        std::lock_guard<std::mutex> lock(m_occlusionMutex);
        m_occlusionStop = true;
    }

    m_occlusionCondition.notify_one();
    m_occlusionThread.join();
    m_sortPool.reset();

    m_mesherRunning = false;
    if (m_mesherPool)
    {
        m_mesherPool->Wait();
        m_mesherPool.reset();
    }

    if (m_sceneFramebuffer)
    {
        delete m_sceneFramebuffer;
        m_sceneFramebuffer = nullptr;
    }

    delete m_nimbus;

    if (m_itemInHandRenderer)
    {
        delete m_itemInHandRenderer;
        m_itemInHandRenderer = nullptr;
    }

    if (m_cloudMesh)
    {
        delete m_cloudMesh;
        m_cloudMesh = nullptr;
    }

    if (m_fogShader)
    {
        delete m_fogShader;
        m_fogShader = nullptr;
    }

    delete m_lodRenderer;

    if (m_levelShader)
    {
        delete m_levelShader;
        m_levelShader = nullptr;
    }

    delete m_waterShader;
    m_waterShader = nullptr;
    delete m_tearShader;
    m_tearShader = nullptr;
    delete m_worldEffectShader;
    m_worldEffectShader = nullptr;
    delete m_compositeShader;
    m_compositeShader = nullptr;
    delete m_halfResTarget;
    m_halfResTarget = nullptr;
    delete m_worldEffectResolveShader;
    m_worldEffectResolveShader = nullptr;
    delete m_worldEffectHistory[0];
    m_worldEffectHistory[0] = nullptr;
    delete m_worldEffectHistory[1];
    m_worldEffectHistory[1] = nullptr;
    RenderDevice *device    = RenderDevice::GetInstance();
    device->DeleteVertexArray(m_fullscreenVao);
    device->DeleteBuffer(m_chunkCommandBuffer);
    device->DeleteBuffer(m_opaqueCommandBuffer);
    device->DeleteBuffer(m_opaqueSectionBuffer);
    device->DeleteBuffer(m_culledCommandBuffer);
    device->DeleteBuffer(m_culledCountBuffer);
    device->DeleteBuffer(m_groupWindowBuffer);
    device->DeleteBuffer(m_visibilityBuffer);
    device->DeleteProgram(m_cullProgram);
    device->DeleteBuffer(m_hizBuffer);
    device->DeleteProgram(m_hizProgram);
}

void LevelRenderer::OnResize(int width, int height)
{
    m_sceneFramebuffer->Resize(width, height);
    m_nimbus->Resize(width, height);
}

void LevelRenderer::Render(float partialTicks, Framebuffer *outputFramebuffer)
{
    Everstead *everstead = Everstead::GetInstance();
    const Camera *camera = everstead->GetCamera();

    const glm::dvec3 cameraPos = camera->GetPosition();
    m_renderCameraPos          = cameraPos;
    const glm::dvec3 playerPos = everstead->GetLocalPlayer()->GetPosition();
    glm::dmat4 viewMatrix      = everstead->GetGameRenderer()->GetViewMatrix(camera);
    viewMatrix                 = everstead->GetGameRenderer()->GetViewBobbing(everstead->GetLocalPlayer(), partialTicks) * viewMatrix;
    float fovModifier          = everstead->GetLocalPlayer()->GetFovModifier(partialTicks);
    if (everstead->GetLocalPlayer()->GetHealth() <= 0)
    {
        float deathTime = everstead->GetLocalPlayer()->GetRenderPlayer()->GetDeathTime() + partialTicks;
        fovModifier /= (1.0f - 500.0f / (deathTime + 500.0f)) * 2.0f + 1.0f;
    }

    glm::dmat4 projection = everstead->GetGameRenderer()->BuildProjection(fovModifier);

    const WorldProvider &worldProvider = m_level->GetWorldProvider();
    const float leafSnowCoverSeconds   = 40.0f;
    const float leafSnowMeltSeconds    = 25.0f;
    float rain                         = m_level->GetRainLevel(partialTicks);
    if (rain > 0.2f)
    {
        m_leafSnow = std::min(1.0f, m_leafSnow + (float) Time::GetDelta() * rain / leafSnowCoverSeconds);
    }
    else
    {
        m_leafSnow = std::max(0.0f, m_leafSnow - (float) Time::GetDelta() / leafSnowMeltSeconds);
    }

    {
        FrameProfilerScope scope("Render.LightState");
        UpdateLightState(worldProvider);
        m_itemLights.Collect(m_level, cameraPos, partialTicks, everstead->options.dynamicLights);
        if (kNimbusEnabled)
        {
            m_nimbus->Update(m_level, cameraPos, projection * viewMatrix);
        }
    }

    SetupMatrices(cameraPos);
    SetupFog(everstead->GetLocalPlayer()->GetEyePosition(partialTicks), partialTicks);

    everstead->GetDefaultFont()->SetNearest(true);

    GlStateManager::MatrixMode(0);
    GlStateManager::LoadIdentity();
    GlStateManager::Translated(-cameraPos.x, -cameraPos.y, -cameraPos.z);

    {
        FrameProfilerScope scope("Render.UploadMeshes");
        UploadPendingMeshes();
    }

    {
        FrameProfilerScope scope("Render.ScheduleMesher");
        ScheduleMesher();
    }

    std::vector<VisibleChunk> opaqueChunks;
    {
        FrameProfilerScope scope("Render.Occlusion");
        UpdateOcclusion(cameraPos);
    }

    {
        FrameProfilerScope scope("Render.CollectChunks");
        CollectVisibleChunks(playerPos, cameraPos, viewMatrix, projection, &opaqueChunks);
    }

    if (everstead->options.lodEnabled)
    {
        m_lodRenderer->Update(playerPos);
    }
    else
    {
        m_lodRenderer->Clear();
    }

    {
        FrameProfilerScope scope("Render.TextureAnimation");
        Texture *blockTexture = BlockAtlas::GetInstance()->GetBlockAtlas()->GetTexture();
        Texture *flowTexture  = BlockAtlas::GetInstance()->GetFlowAtlas()->GetTexture();
        std::vector<TextureAtlas::Region> visibleBlockRegions;
        std::vector<TextureAtlas::Region> visibleFlowRegions;
        for (size_t i = 0; i < opaqueChunks.size() + m_translucentChunks.size(); i++)
        {
            const VisibleChunk &chunk = i < opaqueChunks.size() ? opaqueChunks[i] : m_translucentChunks[i - opaqueChunks.size()];
            for (const std::unique_ptr<ChunkMesh> &mesh : *chunk.meshes)
            {
                if (mesh->GetTexture() == blockTexture)
                {
                    mesh->AppendAtlasRegions(&visibleBlockRegions);
                }
                else if (mesh->GetTexture() == flowTexture)
                {
                    mesh->AppendAtlasRegions(&visibleFlowRegions);
                }
            }
        }

        const std::vector<std::unique_ptr<Entity>> &entities = m_level->GetEntities();
        for (size_t i = 0; i < entities.size(); i++)
        {
            if (entities[i]->GetSharedFlag(eEntityFlag_OnFire))
            {
                Block *fire          = Block::ByName("fire");
                Block::UVRect layer0 = fire->GetAtlasUVRect(Direction::up);
                Block::UVRect layer1 = fire->GetSecondaryAtlasUVRect();
                visibleBlockRegions.push_back({layer0.u0, layer0.v0, layer0.u1, layer0.v1});
                visibleBlockRegions.push_back({layer1.u0, layer1.v0, layer1.u1, layer1.v1});
                break;
            }
        }

        if (Everstead::GetInstance()->options.animatedTextures)
        {
            BlockAtlas::GetInstance()->TickAnimations(Time::GetDelta(), visibleBlockRegions, visibleFlowRegions);
        }
    }

    {
        FrameProfilerScope scope("Render.BuildChunkBatches");
        // The worker only touches m_sortJobs while m_sortBusy is set, and results land a frame late
        if (!m_sortBusy)
        {
            for (size_t i = 0; i < m_sortJobs.size(); i++)
            {
                auto regionIt = m_translucentRegions.find(m_sortJobs[i].region);
                if (regionIt != m_translucentRegions.end())
                {
                    regionIt->second->ApplySortedQuads(m_sortJobs[i]);
                }
            }

            m_sortJobs.clear();
            // Every resort re-uploads its quads, so only a few sections resort per frame
            int sortBudget = s_translucentSortBudget;
            for (auto regionIt = m_translucentRegions.begin(); regionIt != m_translucentRegions.end(); ++regionIt)
            {
                regionIt->second->CollectSortJobs(m_renderCameraPos, &sortBudget, &m_sortJobs);
            }

            if (!m_sortJobs.empty())
            {
                m_sortBusy = true;
                m_sortPool->DetachTask(
                        [this]()
                        {
                            for (size_t i = 0; i < m_sortJobs.size(); i++)
                            {
                                m_sortJobs[i].Sort();
                            }

                            m_sortBusy = false;
                        });
            }
        }

        BuildChunkBatches(outputFramebuffer != nullptr);
        CullOpaqueChunks(playerPos);
    }

    GlStateManager::SetClearColor(m_fogColor.x, m_fogColor.y, m_fogColor.z, 1.0f);

    m_sceneFramebuffer->Bind();
    RenderDevice::GetInstance()->ClearAll();

    {
        FrameProfilerScope scope("Render.Sky");
        RenderSky(viewMatrix, projection);
    }

    GlStateManager::MatrixMode(0);
    GlStateManager::LoadIdentity();
    GlStateManager::Translated(-cameraPos.x, -cameraPos.y, -cameraPos.z);

    if (everstead->options.lodEnabled)
    {
        FrameProfilerScope scope("Render.Lod");
        m_lodRenderer->Render(cameraPos, viewMatrix, projection);
        RenderDevice::GetInstance()->ClearDepth();
    }

    bool hasVisibleChunkGeometry = !opaqueChunks.empty();

    if (hasVisibleChunkGeometry)
    {
        BindLevelShader(m_levelShader, cameraPos, viewMatrix, projection);
        {
            FrameProfilerScope scope("Render.OpaqueChunks");
            RenderDevice::GetInstance()->BeginGpuTimer("Render.OpaqueChunks");
            RenderChunks(m_levelShader);
            RenderDevice::GetInstance()->EndGpuTimer();
            if (m_gpuCulling)
            {
                BuildDepthPyramid(projection * viewMatrix);
            }
        }
    }
    else
    {
        m_hizValid = false;
    }

    GlStateManager::MatrixMode(0);
    GlStateManager::LoadIdentity();
    GlStateManager::Translated(-cameraPos.x, -cameraPos.y, -cameraPos.z);

    GlStateManager::MatrixMode(0);
    GlStateManager::PushMatrix();
    GlStateManager::LoadIdentity();
    GlStateManager::MultMatrix(viewMatrix);
    GlStateManager::Translated(-cameraPos.x, -cameraPos.y, -cameraPos.z);

    {
        FrameProfilerScope scope("Render.BlockOutline");
        RenderBlockOutline();
    }

    GlStateManager::PopMatrix();

    // Without an output buffer clouds have to go before the fog pass, so water behind them stays hidden
    if (!outputFramebuffer)
    {
        FrameProfilerScope scope("Render.Clouds");
        RenderClouds(viewMatrix, projection, partialTicks, outputFramebuffer);
    }

    {
        FrameProfilerScope scope("Render.Entities");
        RenderEntities(partialTicks);
    }

    {
        FrameProfilerScope scope("Render.Particles");
        RenderParticles(partialTicks);
    }

    if (outputFramebuffer)
    {
        outputFramebuffer->Bind();
    }
    else
    {
        m_sceneFramebuffer->Unbind();
    }

    {
        FrameProfilerScope scope("Render.Present");
        RenderFogPass(projection, m_sceneFramebuffer, true);
    }

    // The output needs the opaque depth before translucents so they are tested against this frame instead of the last one
    if (outputFramebuffer)
    {
        CopySceneDepth(m_sceneFramebuffer, outputFramebuffer);
    }

    if (!m_translucentChunks.empty())
    {
        GlStateManager::MatrixMode(0);
        GlStateManager::LoadIdentity();
        GlStateManager::Translated(-cameraPos.x, -cameraPos.y, -cameraPos.z);
        BindLevelShader(m_waterShader, cameraPos, viewMatrix, projection);
        SetFogUniforms(m_waterShader, true);
        Textures::Bind(m_sceneFramebuffer->GetDepthTexture(), 3);
        {
            FrameProfilerScope scope("Render.TranslucentChunks");
            RenderDevice::GetInstance()->BeginGpuTimer("Render.TranslucentChunks");
            RenderTranslucentChunks(outputFramebuffer != nullptr);
            RenderDevice::GetInstance()->EndGpuTimer();
        }
        Textures::Bind({}, 3);

        // Clouds raymarch up to the output depth, so glass in front of the water has to be in it before they draw
        if (outputFramebuffer)
        {
            GlStateManager::SetColorMask(false, false, false, false);
            DrawChunkBatches(m_translucentBatches[2], m_chunkCommandBuffer, false);
            GlStateManager::SetColorMask(true, true, true, true);
        }
    }

    if (outputFramebuffer)
    {
        FrameProfilerScope scope("Render.Clouds");
        RenderClouds(viewMatrix, projection, partialTicks, outputFramebuffer);
    }

    // We draw after water and the world fog so neither of them covers our fog
    if (kNimbusEnabled && outputFramebuffer)
    {
        FrameProfilerScope scope("Render.Nimbus");
        SetFogUniforms(m_nimbus->GetVolumeShader(), true);
        SetFogUniforms(m_nimbus->GetApplyShader(), true);
        m_nimbus->RenderVolume(outputFramebuffer, glm::inverse(projection * viewMatrix), cameraPos);
    }

    if (outputFramebuffer)
    {
        FrameProfilerScope scope("Render.Weather");
        RenderWeather(partialTicks);
    }

    if (outputFramebuffer)
    {
        FrameProfilerScope scope("Render.WorldBorder");
        RenderWorldBorder(partialTicks);
    }

    if (outputFramebuffer)
    {
        FrameProfilerScope scope("Render.Tears");
        RenderTears(outputFramebuffer, projection, partialTicks);
    }

    if (outputFramebuffer)
    {
        FrameProfilerScope scope("Render.WorldEffects");
        RenderWorldEffects(outputFramebuffer, projection, partialTicks);
    }

    if (outputFramebuffer && !m_translucentChunks.empty())
    {
        GlStateManager::MatrixMode(0);
        GlStateManager::LoadIdentity();
        GlStateManager::Translated(-cameraPos.x, -cameraPos.y, -cameraPos.z);
        BindLevelShader(m_waterShader, cameraPos, viewMatrix, projection);
        SetFogUniforms(m_waterShader, true);
        Textures::Bind(m_sceneFramebuffer->GetDepthTexture(), 3);
        {
            FrameProfilerScope scope("Render.TranslucentFrontChunks");
            RenderTranslucentFrontChunks();
        }
        Textures::Bind({}, 3);
    }

    {
        FrameProfilerScope scope("Render.NameTags");
        RenderEntityNameTags(partialTicks);
    }

    if (m_renderChunkGrid)
    {
        RenderChunkGrid();
    }

    LocalPlayer *localPlayer = everstead->GetLocalPlayer();
    if (localPlayer->IsFirstPerson() && everstead->options.displayHand && everstead->IsHudVisible() &&
        !everstead->GetGameRenderer()->IsPanoramaCapture() && !GameType::IsSpectator(localPlayer->GetGameType()))
    {
        m_itemInHandRenderer->Render(everstead->GetGameRenderer()->BuildHandProjection(), localPlayer, partialTicks,
                                     SampleEntityLightColor(localPlayer, partialTicks));
    }
}

void LevelRenderer::TickItemInHand(LocalPlayer *player)
{
    if (m_itemInHandRenderer && player)
    {
        m_itemInHandRenderer->Tick(player);
    }
}

ItemInHandRenderer *LevelRenderer::GetItemInHandRenderer() const
{
    return m_itemInHandRenderer;
}

LodRenderer *LevelRenderer::GetLodRenderer() const
{
    return m_lodRenderer;
}

void LevelRenderer::TickMeshing()
{
    LevelChunkMeshingService::TickMeshing(this);
    RebuildDirtyRegions();
}

void LevelRenderer::Rebuild()
{
    LevelChunkMeshingService::Rebuild(this);
}

void LevelRenderer::RebuildChunk(const ChunkPos &pos)
{
    LevelChunkMeshingService::RebuildChunk(this, pos);
}

void LevelRenderer::RemeshAll()
{
    std::vector<std::pair<ChunkPos, Chunk *>> chunks = m_level->GetLoadedChunks();
    for (size_t i = 0; i < chunks.size(); i++)
    {
        RebuildChunk(chunks[i].first);
    }
}

void LevelRenderer::RebuildChunkUrgent(const ChunkPos &pos)
{
    LevelChunkMeshingService::RebuildChunkUrgent(this, pos);
}

void LevelRenderer::RebuildChunkDeferred(const ChunkPos &pos, bool urgent)
{
    LevelChunkMeshingService::RebuildChunkDeferred(this, pos, urgent);
}

void LevelRenderer::RebuildSectionUrgent(const SectionPos &pos)
{
    LevelChunkMeshingService::RebuildSectionUrgent(this, pos);
}

void LevelRenderer::RebuildSectionNow(const SectionPos &pos)
{
    LevelChunkMeshingService::RebuildSectionNow(this, pos);
}

void LevelRenderer::RebuildSectionDeferred(const SectionPos &pos, bool urgent)
{
    LevelChunkMeshingService::RebuildSectionDeferred(this, pos, urgent);
}

void LevelRenderer::FlushMeshScheduling()
{
    LevelChunkMeshingService::FlushScheduling(this);
}

void LevelRenderer::RebuildSectionsUrgentSynchronized(const std::vector<SectionPos> &positions)
{
    LevelChunkMeshingService::RebuildSectionsUrgentSynchronized(this, positions);
}

bool LevelRenderer::HasChunkMesh(const ChunkPos &pos) const
{
    return LevelChunkMeshingService::HasChunkMesh(this, pos);
}

bool LevelRenderer::HasActiveRebuild(const ChunkPos &pos) const
{
    std::lock_guard<std::mutex> lock(m_rebuildQueueMutex);
    for (int section = 0; section < Chunk::s_sectionCount; section++)
    {
        if (m_activeRebuilds.find(SectionPos(pos.x, section, pos.z)) != m_activeRebuilds.end())
        {
            return true;
        }
    }

    return false;
}

void LevelRenderer::DropChunkMeshes(const ChunkPos &pos)
{
    m_occlusionDirty = m_columnVisibility.erase(pos) != 0 || m_occlusionDirty;
    m_meshedSections.erase(pos);
    m_meshedSectionsVersion++;
    for (int section = 0; section < Chunk::s_sectionCount; section++)
    {
        SectionPos sectionPos(pos.x, section, pos.z);
        RemoveTranslucentSection(sectionPos);
        RemoveRegionSection(sectionPos);
        m_nimbus->MarkSectionDirty(sectionPos);
    }

    {
        std::lock_guard<std::mutex> lock(m_meshQueueMutex);
        for (int section = 0; section < Chunk::s_sectionCount; section++)
        {
            m_readyMeshes.erase(SectionPos(pos.x, section, pos.z));
        }
    }

    // Sync groups touching this chunk stay pending until these are gone, which pins their other ready meshes in memory
    std::lock_guard<std::mutex> lock(m_rebuildQueueMutex);
    for (int section = 0; section < Chunk::s_sectionCount; section++)
    {
        LevelChunkMeshingService::ForgetSection(this, SectionPos(pos.x, section, pos.z));
    }
}

size_t LevelRenderer::GetMeshedChunkCount() const
{
    size_t count = 0;
    for (const auto &region : m_renderRegions)
    {
        count += region.second->GetSectionCount();
    }

    for (const auto &region : m_translucentRegions)
    {
        count += region.second->GetSectionCount();
    }

    return count;
}

size_t LevelRenderer::GetMeshCpuBytes() const
{
    size_t bytes = 0;
    for (auto it = m_translucentRegions.begin(); it != m_translucentRegions.end(); ++it)
    {
        bytes += it->second->GetCpuBytes();
    }

    return bytes;
}

size_t LevelRenderer::GetRegionCpuBytes() const
{
    size_t bytes = 0;
    for (auto it = m_renderRegions.begin(); it != m_renderRegions.end(); ++it)
    {
        bytes += it->second->GetCpuBytes();
    }

    return bytes;
}

size_t LevelRenderer::GetVisibleChunkCount() const
{
    return m_lastVisibleChunkCount;
}

size_t LevelRenderer::GetRenderedMeshCount() const
{
    return m_lastRenderedMeshCount;
}

size_t LevelRenderer::GetReadyMeshCount()
{
    std::lock_guard<std::mutex> lock(m_meshQueueMutex);
    return m_readyMeshes.size();
}

size_t LevelRenderer::GetMesherThreadCount() const
{
    if (!m_mesherPool)
    {
        return 0;
    }

    return m_mesherPool->GetThreadCount();
}

size_t LevelRenderer::GetActiveMesherTaskCount() const
{
    return (size_t) m_activeMesherTasks.load();
}

size_t LevelRenderer::GetPendingMeshCount() const
{
    return LevelChunkMeshingService::GetPendingMeshCount(this);
}

size_t LevelRenderer::GetQueuedRebuildCount() const
{
    return LevelChunkMeshingService::GetQueuedRebuildCount(this);
}

size_t LevelRenderer::GetUrgentRebuildCount() const
{
    return LevelChunkMeshingService::GetUrgentRebuildCount(this);
}

void LevelRenderer::UpdateLightState(const WorldProvider &worldProvider)
{
    float skyLightClamp = LightSource::SampleSkyLightClamp(worldProvider);
    m_lightStorage.Update(skyLightClamp, Time::GetDelta(), true);

    float effectiveClamp = m_lightStorage.GetSkyLightClamp();
    int clampKey         = (int) floorf(effectiveClamp * 4096.0f + 0.5f);
    if (clampKey != m_lastLightCacheClampKey)
    {
        m_lightCache.Rebuild(effectiveClamp);
        m_lastLightCacheClampKey = clampKey;
    }
}

void LevelRenderer::BindLevelShader(Shader *shader, const glm::dvec3 &cameraPos, const glm::dmat4 &viewMatrix, const glm::dmat4 &projection)
{
    shader->Bind();
    shader->SetInt("u_grassSideOverlayEnabled", m_grassSideOverlayEnabled ? 1 : 0);
    shader->SetFloat("u_skyDarken",
                     LightSource::SampleSkyDarken(m_level->GetWorldProvider(), m_level->GetRainLevel(1.0f), m_level->GetThunderLevel(1.0f)));
    shader->SetFloat("u_blockLightScale", LightSource::SampleBlockLightScale(LightSource::s_blockFlicker));
    shader->SetInt("u_nimbusLighting", kNimbusEnabled && m_nimbus->IsBlockLighting() ? 1 : 0);
    shader->SetFloat("u_darkAdaptation", LightSource::s_darkAdaptation);
    shader->SetFloat("u_leafSnow", m_leafSnow);
    m_itemLights.Bind(shader, cameraPos);
    if (kNimbusEnabled)
    {
        m_nimbus->BindSurface(shader, cameraPos);
    }

    if (!m_grassSideOverlayMaskTexture)
    {
        m_grassSideOverlayMaskTexture = TextureRepository::GetInstance()->Get("assets/texture/block/grass_side_overlay.png").get();
    }

    Textures::Bind(m_grassSideOverlayMaskTexture ? m_grassSideOverlayMaskTexture->GetId() : TextureHandle(), 1);

    m_renderCameraPos              = cameraPos;
    glm::dmat4 cameraRelativeModel = glm::translate(GlStateManager::GetMatrix(), cameraPos);
    shader->SetMat4("u_model", glm::value_ptr(cameraRelativeModel));
    shader->SetMat4("u_view", glm::value_ptr(viewMatrix));
    shader->SetMat4("u_projection", glm::value_ptr(projection));
    shader->SetVec3("u_cameraPos", cameraPos.x, cameraPos.y, cameraPos.z);
    shader->SetVec3("u_chunkScale", 1.0f / ChunkMesh::kPositionStepsXZ, 1.0f / ChunkMesh::kPositionStepsY, 1.0f / ChunkMesh::kPositionStepsXZ);
    m_lodRenderer->BindMask(shader, cameraPos);

    GlStateManager::EnableCull();
    GlStateManager::SetFrontFace(eFrontFace_Ccw);
    GlStateManager::SetCullFace(eCullFace_Back);
    GlStateManager::EnableDepthTest();
}

void LevelRenderer::CollectVisibleChunks(const glm::dvec3 &playerPos, const glm::dvec3 &cameraPos, const glm::dmat4 &viewMatrix,
                                         const glm::dmat4 &projection, std::vector<VisibleChunk> *opaqueChunks)
{
    int playerChunkX       = Mth::FloorDiv((int) floor(playerPos.x), Chunk::s_sizeX);
    int playerChunkZ       = Mth::FloorDiv((int) floor(playerPos.z), Chunk::s_sizeZ);
    int renderDistance     = m_level->GetRenderDistance();
    glm::dvec3 cameraFront = glm::normalize(Everstead::GetInstance()->GetCamera()->GetFront());
    m_frustumCulling       = Everstead::GetInstance()->options.frustumCulling && !Everstead::GetInstance()->GetGameRenderer()->IsPanoramaCapture();
    m_gpuCulling           = Everstead::GetInstance()->options.gpuChunkCulling;
    double chunkHalfY      = (double) ChunkSection::s_size * 0.5;
    m_fogCullDistance      = 0.0;
    if (m_level->IsLavaAt(cameraPos))
    {
        m_fogCullDistance = 4.0;
    }
    else if (m_level->IsLiquidAt(cameraPos))
    {
        m_fogCullDistance = -log(1.0 - s_underwaterFogCullOpacity) / s_underwaterFogDensity;
    }
    else if (Everstead::GetInstance()->options.fogCulling && GlStateManager::IsFogEnabled())
    {
        float fogStart;
        float fogEnd;
        GlStateManager::GetFogRange(&fogStart, &fogEnd);
        if (fogEnd > fogStart)
        {
            m_fogCullDistance = fogEnd;
        }
    }

    glm::dmat4 viewProjection = projection * viewMatrix * GlStateManager::GetMatrix();
    m_frustum.ExtractPlanes(viewProjection);

    opaqueChunks->clear();
    opaqueChunks->reserve(m_renderRegions.size());
    m_translucentChunks.clear();

    size_t visibleChunkCount = 0;
    size_t renderedMeshCount = 0;

    auto collectRegions =
            [&](const std::unordered_map<SectionPos, std::unique_ptr<ChunkRenderRegion>, SectionPosHash> &regions, std::vector<VisibleChunk> *out)
    {
        for (auto regionIt = regions.begin(); regionIt != regions.end(); ++regionIt)
        {
            const SectionPos &pos                                 = regionIt->first;
            const std::vector<std::unique_ptr<ChunkMesh>> &meshes = regionIt->second->GetMeshes();
            int minChunkX                                         = pos.x * ChunkRenderRegion::s_size;
            int minChunkZ                                         = pos.z * ChunkRenderRegion::s_size;
            int maxChunkX                                         = minChunkX + ChunkRenderRegion::s_size - 1;
            int maxChunkZ                                         = minChunkZ + ChunkRenderRegion::s_size - 1;
            int dx = playerChunkX < minChunkX ? minChunkX - playerChunkX : playerChunkX > maxChunkX ? playerChunkX - maxChunkX : 0;
            int dz = playerChunkZ < minChunkZ ? minChunkZ - playerChunkZ : playerChunkZ > maxChunkZ ? playerChunkZ - maxChunkZ : 0;
            if (dx * dx + dz * dz > renderDistance * renderDistance || !IsRegionVisible(pos))
            {
                continue;
            }

            double width = Chunk::s_sizeX * ChunkRenderRegion::s_size;
            glm::dvec3 chunkMin(pos.x * width, pos.y * ChunkSection::s_size, pos.z * width);
            glm::dvec3 chunkMax(chunkMin.x + width, chunkMin.y + ChunkSection::s_size, chunkMin.z + width);
            double centerX = chunkMin.x + width * 0.5;
            double centerY = chunkMin.y + ChunkSection::s_size * 0.5;
            double centerZ = chunkMin.z + width * 0.5;
            double radius  = sqrt(width * width * 0.5 + chunkHalfY * chunkHalfY);
            glm::dvec3 toChunk(centerX - cameraPos.x, centerY - cameraPos.y, centerZ - cameraPos.z);
            if (m_frustumCulling && glm::dot(toChunk, cameraFront) < -radius)
            {
                continue;
            }

            if (m_fogCullDistance > 0.0)
            {
                double fogCullRadius = m_fogCullDistance + radius;
                if (glm::length2(toChunk) >= fogCullRadius * fogCullRadius)
                {
                    continue;
                }
            }

            if (m_frustumCulling && !m_frustum.TestSphere(glm::dvec3(centerX, centerY, centerZ), radius))
            {
                continue;
            }

            AABB chunkAABB(chunkMin, chunkMax);
            if (m_frustumCulling && !m_frustum.TestAABB(chunkAABB))
            {
                continue;
            }

            visibleChunkCount++;
            renderedMeshCount += meshes.size();
            out->push_back({&pos, &meshes, glm::length2(glm::dvec3(centerX, centerY, centerZ) - cameraPos), ChunkRenderRegion::s_size,
                            regionIt->second.get()});
        }
    };

    collectRegions(m_renderRegions, opaqueChunks);
    collectRegions(m_translucentRegions, &m_translucentChunks);
    std::sort(m_translucentChunks.begin(), m_translucentChunks.end(),
              [](const VisibleChunk &a, const VisibleChunk &b)
              {
                  return a.distanceSquared > b.distanceSquared;
              });
    m_lastVisibleChunkCount = visibleChunkCount;
    m_lastRenderedMeshCount = renderedMeshCount;
}

// Opaque commands live in persistent per region slots, so a remesh only rewrites that region's commands
void LevelRenderer::BuildChunkBatches(bool depthPrepass)
{
    RenderDevice *device = RenderDevice::GetInstance();
    for (size_t r = 0; r < m_regionDraws.size(); r++)
    {
        RegionDraw &draw = m_regionDraws[r];
        if (draw.region == nullptr || draw.region->GetGeneration() == draw.generation)
        {
            continue;
        }

        draw.generation                                                         = draw.region->GetGeneration();
        const std::vector<std::unique_ptr<ChunkMesh>> &meshes                   = draw.region->GetMeshes();
        const std::vector<std::vector<ChunkRenderRegion::SectionRange>> &ranges = draw.region->GetSectionRanges();
        for (size_t m = meshes.size(); m < draw.slots.size(); m++)
        {
            FreeMeshSlot(draw.slots[m]);
        }

        draw.slots.resize(meshes.size());
        for (size_t m = 0; m < meshes.size(); m++)
        {
            uint32_t group = 0;
            while (group < m_drawGroups.size() &&
                   (m_drawGroups[group].page != meshes[m]->GetPage() || m_drawGroups[group].texture != meshes[m]->GetTexture()))
            {
                group++;
            }

            if (group == m_drawGroups.size())
            {
                m_drawGroups.push_back({meshes[m]->GetPage(), meshes[m]->GetTexture(), 0, 0});
            }

            uint32_t count = 0;
            for (size_t j = 0; j < ranges[m].size(); j++)
            {
                for (int face = 0; face < ChunkRenderRegion::s_faceGroupCount; face++)
                {
                    if (ranges[m][j].count != 0 && ranges[m][j].faceCounts[face] != 0)
                    {
                        count++;
                    }
                }
            }

            MeshSlot &slot = draw.slots[m];
            if (slot.capacity != 0 && (slot.group != group || count > slot.capacity))
            {
                FreeMeshSlot(slot);
                slot = {group, 0, 0, 0};
            }

            uint32_t dirty = std::max(count, slot.count);
            if (slot.capacity == 0 && count != 0)
            {
                slot.capacity                    = std::bit_ceil(std::max<uint32_t>(count, 8));
                std::vector<uint32_t> &freeSlots = m_freeSlots[std::countr_zero(slot.capacity)];
                if (!freeSlots.empty())
                {
                    slot.first = freeSlots.back();
                    freeSlots.pop_back();
                }
                else
                {
                    slot.first = (uint32_t) m_opaqueCommands.size();
                    m_opaqueCommands.resize(slot.first + slot.capacity, {});
                    m_opaqueSections.resize(slot.first + slot.capacity, glm::ivec4(0));
                    dirty = slot.capacity;
                }
            }

            for (uint32_t i = slot.first + count; i < slot.first + slot.count; i++)
            {
                m_opaqueCommands[i] = {};
                m_opaqueSections[i] = glm::ivec4(0);
            }

            m_drawGroups[group].commandCount += count - slot.count;
            slot.group      = group;
            slot.count      = count;
            uint32_t next   = slot.first;
            DrawGroup &info = m_drawGroups[group];
            for (size_t j = 0; j < ranges[m].size(); j++)
            {
                const ChunkRenderRegion::SectionRange &range = ranges[m][j];
                if (range.count == 0)
                {
                    continue;
                }

                int baseVertex = (int) meshes[m]->GetFirstVertex() + range.first;
                for (int face = 0; face < ChunkRenderRegion::s_faceGroupCount; face++)
                {
                    int faceCount = range.faceCounts[face];
                    if (faceCount != 0)
                    {
                        m_opaqueCommands[next] = {(uint32_t) (faceCount / 4 * 6), 1, 0, baseVertex, (uint32_t) r};
                        m_opaqueSections[next] = glm::ivec4(range.pos.x, range.pos.y, range.pos.z, face | (int) (group << 3));
                        info.maxQuads          = std::max(info.maxQuads, (uint32_t) (faceCount / 4));
                        next++;
                    }

                    baseVertex += faceCount;
                }
            }

            if (dirty != 0)
            {
                m_dirtyCommands.push_back({slot.first, slot.first + dirty});
            }
        }
    }

    // Resizing reallocates GPU memory, which stalls on Vulkan, so the buffers only grow and otherwise get partial updates
    if (m_opaqueCommands.size() > m_opaqueCommandCapacity)
    {
        m_opaqueCommandCapacity = m_opaqueCommands.size() * 3 / 2;
        device->BindArrayBuffer(m_opaqueCommandBuffer);
        device->UploadArrayBuffer(nullptr, (uint32_t) (m_opaqueCommandCapacity * sizeof(DrawIndirectCommand)), eBufferUsage_Static);
        device->UpdateArrayBuffer(0, (uint32_t) (m_opaqueCommands.size() * sizeof(DrawIndirectCommand)), m_opaqueCommands.data());
        device->BindArrayBuffer(m_opaqueSectionBuffer);
        device->UploadArrayBuffer(nullptr, (uint32_t) (m_opaqueCommandCapacity * sizeof(glm::ivec4)), eBufferUsage_Static);
        device->UpdateArrayBuffer(0, (uint32_t) (m_opaqueSections.size() * sizeof(glm::ivec4)), m_opaqueSections.data());
        device->BindArrayBuffer(m_culledCommandBuffer);
        device->UploadArrayBuffer(nullptr, (uint32_t) (m_opaqueCommandCapacity * sizeof(DrawIndirectCommand)), eBufferUsage_Static);
        device->BindArrayBuffer(m_culledCountBuffer);
        device->UploadArrayBuffer(nullptr, (uint32_t) (m_opaqueCommandCapacity * sizeof(uint32_t)), eBufferUsage_Static);
        m_dirtyCommands.clear();
    }

    std::sort(m_dirtyCommands.begin(), m_dirtyCommands.end());
    size_t dirtyIndex = 0;
    while (dirtyIndex < m_dirtyCommands.size())
    {
        uint32_t first = m_dirtyCommands[dirtyIndex].first;
        uint32_t end   = m_dirtyCommands[dirtyIndex].second;
        for (dirtyIndex++; dirtyIndex < m_dirtyCommands.size() && m_dirtyCommands[dirtyIndex].first <= end + 64; dirtyIndex++)
        {
            end = std::max(end, m_dirtyCommands[dirtyIndex].second);
        }

        if (end > first)
        {
            device->BindArrayBuffer(m_opaqueCommandBuffer);
            device->UpdateArrayBuffer((uint32_t) (first * sizeof(DrawIndirectCommand)), (uint32_t) ((end - first) * sizeof(DrawIndirectCommand)),
                                      &m_opaqueCommands[first]);
            device->BindArrayBuffer(m_opaqueSectionBuffer);
            device->UpdateArrayBuffer((uint32_t) (first * sizeof(glm::ivec4)), (uint32_t) ((end - first) * sizeof(glm::ivec4)),
                                      &m_opaqueSections[first]);
        }
    }

    m_dirtyCommands.clear();

    // Every group gets a window in the culled buffer as big as its live commands, which the cull shader compacts into
    bool windowsChanged = m_groupWindows.size() != m_drawGroups.size();
    uint32_t window     = 0;
    m_groupWindows.resize(m_drawGroups.size());
    m_opaqueBatches.clear();
    for (size_t g = 0; g < m_drawGroups.size(); g++)
    {
        const DrawGroup &group = m_drawGroups[g];
        windowsChanged |= m_groupWindows[g] != window;
        m_groupWindows[g] = window;
        if (group.commandCount != 0)
        {
            m_opaqueBatches.push_back({group.page, group.texture, window, (int) group.commandCount, group.maxQuads});
        }

        window += group.commandCount;
    }

    if (windowsChanged && !m_groupWindows.empty())
    {
        device->BindArrayBuffer(m_groupWindowBuffer);
        device->UploadArrayBuffer(m_groupWindows.data(), (uint32_t) (m_groupWindows.size() * sizeof(uint32_t)), eBufferUsage_Static);
    }

    m_visibleRegions.clear();
    m_chunkOffsets.clear();
    for (size_t i = 0; i < m_regionDraws.size() + m_translucentChunks.size(); i++)
    {
        const SectionPos &pos = i < m_regionDraws.size() ? m_regionDraws[i].pos : *m_translucentChunks[i - m_regionDraws.size()].pos;
        m_chunkOffsets.push_back((float) ((double) pos.x * ChunkRenderRegion::s_size * Chunk::s_sizeX - m_renderCameraPos.x));
        m_chunkOffsets.push_back((float) -m_renderCameraPos.y);
        m_chunkOffsets.push_back((float) ((double) pos.z * ChunkRenderRegion::s_size * Chunk::s_sizeZ - m_renderCameraPos.z));
        if (i >= m_regionDraws.size())
        {
            m_visibleRegions.push_back(m_translucentChunks[i - m_regionDraws.size()].region);
        }
    }

    bool regionsChanged     = m_chunkBatchesDirty || m_batchOpaqueCount != m_regionDraws.size() || m_visibleRegions != m_batchRegions;
    glm::ivec3 cameraBlock  = glm::ivec3(glm::floor(m_renderCameraPos));
    bool translucentChanged = regionsChanged || m_batchDepthPrepass != depthPrepass || m_batchCameraBlock != cameraBlock;
    for (size_t i = 0; i < m_visibleRegions.size() && !translucentChanged; i++)
    {
        translucentChanged = m_visibleRegions[i]->GetGeneration() != m_batchGenerations[i];
    }

    if (regionsChanged)
    {
        m_batchRegions.swap(m_visibleRegions);
        m_batchGenerations.resize(m_batchRegions.size());
    }

    m_batchOpaqueCount  = m_regionDraws.size();
    m_chunkBatchesDirty = false;

    // Translucent draws only change order when the camera crosses a block, so most frames reuse them
    if (translucentChanged)
    {
        m_batchCameraBlock  = cameraBlock;
        m_batchDepthPrepass = depthPrepass;
        m_chunkCommands.clear();
        for (int i = 0; i < 3; i++)
        {
            m_translucentBatches[i].clear();
        }

        for (size_t i = 0; i < m_translucentChunks.size(); i++)
        {
            m_batchGenerations[i] = m_translucentChunks[i].region->GetGeneration();
        }

        double sectionHalf = (double) Chunk::s_sizeX * 0.5;
        for (int filter = depthPrepass ? 1 : 0; filter <= (depthPrepass ? 2 : 0); filter++)
        {
            for (size_t i = 0; i < m_translucentChunks.size(); i++)
            {
                const std::vector<std::unique_ptr<ChunkMesh>> &meshes                   = m_translucentChunks[i].region->GetMeshes();
                const std::vector<std::vector<ChunkRenderRegion::SectionRange>> &ranges = m_translucentChunks[i].region->GetSectionRanges();
                for (size_t m = 0; m < meshes.size(); m++)
                {
                    // Water and the other translucent faces live in separate meshes, so a pass skips whole meshes instead of discarding pixels
                    if (filter != 0 && meshes[m]->SortsQuads() != (filter == 2))
                    {
                        continue;
                    }

                    m_translucentRangeOrder.clear();
                    for (size_t j = 0; j < ranges[m].size(); j++)
                    {
                        const ChunkRenderRegion::SectionRange &range = ranges[m][j];
                        if (range.count == 0 || !IsSectionVisible(range.pos))
                        {
                            continue;
                        }

                        double dx = (double) range.pos.x * Chunk::s_sizeX + sectionHalf - m_renderCameraPos.x;
                        double dy = (double) range.pos.y * ChunkSection::s_size + sectionHalf - m_renderCameraPos.y;
                        double dz = (double) range.pos.z * Chunk::s_sizeZ + sectionHalf - m_renderCameraPos.z;
                        m_translucentRangeOrder.push_back({dx * dx + dy * dy + dz * dz, j});
                    }

                    std::sort(m_translucentRangeOrder.begin(), m_translucentRangeOrder.end(),
                              [](const std::pair<double, size_t> &a, const std::pair<double, size_t> &b)
                              {
                                  return a.first > b.first;
                              });
                    for (size_t j = 0; j < m_translucentRangeOrder.size(); j++)
                    {
                        const ChunkRenderRegion::SectionRange &range = ranges[m][m_translucentRangeOrder[j].second];
                        m_chunkDraws.push_back({meshes[m]->GetPage(),
                                                meshes[m]->GetTexture(),
                                                {(uint32_t) (range.count / 4 * 6), 1, 0, (int) meshes[m]->GetFirstVertex() + range.first,
                                                 (uint32_t) (m_regionDraws.size() + i)}});
                    }
                }
            }

            AppendChunkBatches(&m_translucentBatches[filter], &m_chunkCommands);
        }
    }

    UploadChunkBatches();
}

void LevelRenderer::FreeMeshSlot(const MeshSlot &slot)
{
    if (slot.capacity == 0)
    {
        return;
    }

    std::fill(m_opaqueCommands.begin() + slot.first, m_opaqueCommands.begin() + slot.first + slot.count, DrawIndirectCommand{});
    std::fill(m_opaqueSections.begin() + slot.first, m_opaqueSections.begin() + slot.first + slot.count, glm::ivec4(0));
    m_dirtyCommands.push_back({slot.first, slot.first + slot.count});
    m_drawGroups[slot.group].commandCount -= slot.count;
    m_freeSlots[std::countr_zero(slot.capacity)].push_back(slot.first);
}

void LevelRenderer::ClearRegionDraws()
{
    m_regionDraws.clear();
    m_freeRegionDraws.clear();
    for (size_t i = 0; i < m_freeSlots.size(); i++)
    {
        m_freeSlots[i].clear();
    }

    m_opaqueCommands.assign(1, {});
    m_opaqueSections.assign(1, glm::ivec4(0));
    m_dirtyCommands.clear();
    m_drawGroups.clear();
    m_groupWindows.clear();
    m_opaqueBatches.clear();
}

void LevelRenderer::UploadChunkBatches()
{
    if (m_chunkOffsets.empty())
    {
        return;
    }

    RenderDevice *device = RenderDevice::GetInstance();
    if (!m_chunkCommands.empty())
    {
        device->BindArrayBuffer(m_chunkCommandBuffer);
        device->UploadArrayBuffer(m_chunkCommands.data(), (uint32_t) (m_chunkCommands.size() * sizeof(DrawIndirectCommand)), eBufferUsage_Stream);
    }

    device->BindArrayBuffer(ChunkVertexArena::GetInstance()->GetOffsetBuffer());
    device->UploadArrayBuffer(m_chunkOffsets.data(), (uint32_t) (m_chunkOffsets.size() * sizeof(float)), eBufferUsage_Stream);
}

void LevelRenderer::AppendChunkBatches(std::vector<ChunkBatch> *batches, std::vector<DrawIndirectCommand> *commands)
{
    for (size_t i = 0; i < m_chunkDraws.size(); i++)
    {
        const ChunkDraw &draw = m_chunkDraws[i];
        if (batches->empty() || batches->back().page != draw.page || batches->back().texture != draw.texture)
        {
            batches->push_back({draw.page, draw.texture, (uint32_t) commands->size(), 0, 0});
        }

        ChunkBatch &batch = batches->back();
        batch.commandCount++;
        batch.maxQuads = std::max(batch.maxQuads, draw.command.count / 6);
        commands->push_back(draw.command);
    }

    m_chunkDraws.clear();
}

void LevelRenderer::CullOpaqueChunks(const glm::dvec3 &playerPos)
{
    if (m_opaqueBatches.empty())
    {
        return;
    }

    RenderDevice *device = RenderDevice::GetInstance();
    int areaX            = Mth::FloorDiv((int) floor(playerPos.x), Chunk::s_sizeX);
    int areaZ            = Mth::FloorDiv((int) floor(playerPos.z), Chunk::s_sizeZ);
    int renderDistance   = m_level->GetRenderDistance();
    if (!m_gpuCulling)
    {
        int regionSize = ChunkRenderRegion::s_size;
        int gridWidth  = m_occlusionRadius * 2 + 1;
        m_cpuCullCursors.assign(m_groupWindows.begin(), m_groupWindows.end());
        m_cpuCulledCommands.assign(m_opaqueCommands.size(), {});
        for (size_t i = 0; i < m_opaqueCommands.size(); i++)
        {
            if (m_opaqueCommands[i].count == 0)
            {
                continue;
            }

            const glm::ivec4 &section = m_opaqueSections[i];
            glm::dvec3 sectionOrigin((double) section.x * Chunk::s_sizeX, (double) section.y * ChunkSection::s_size,
                                     (double) section.z * Chunk::s_sizeZ);
            glm::dvec3 boxMin = sectionOrigin - 1.0;
            glm::dvec3 boxMax = boxMin + 18.0;
            if (m_fogCullDistance > 0.0 && glm::distance(glm::clamp(m_renderCameraPos, boxMin, boxMax), m_renderCameraPos) >= m_fogCullDistance)
            {
                continue;
            }

            int regionX  = Mth::FloorDiv(section.x, regionSize) * regionSize;
            int regionZ  = Mth::FloorDiv(section.z, regionSize) * regionSize;
            int outsideX = std::max({regionX - areaX, areaX - (regionX + regionSize - 1), 0});
            int outsideZ = std::max({regionZ - areaZ, areaZ - (regionZ + regionSize - 1), 0});
            if (outsideX * outsideX + outsideZ * outsideZ > renderDistance * renderDistance)
            {
                continue;
            }

            int cellX = section.x - m_occlusionOriginX;
            int cellZ = section.z - m_occlusionOriginZ;
            if (m_occlusionValid && abs(cellX) <= m_occlusionRadius && abs(cellZ) <= m_occlusionRadius && section.y >= 0 &&
                section.y < Chunk::s_sectionCount &&
                m_visibleSections[(size_t) (((cellX + m_occlusionRadius) + (cellZ + m_occlusionRadius) * gridWidth) * Chunk::s_sectionCount +
                                            section.y)] == 0)
            {
                continue;
            }

            if (m_frustumCulling && !m_frustum.TestAABB(AABB(boxMin, boxMax)))
            {
                continue;
            }

            int face = section.w & 7;
            if (face < 6)
            {
                double coordinate = m_renderCameraPos[face >> 1] - sectionOrigin[face >> 1];
                if ((face & 1) == 0 ? coordinate <= -1.0 : coordinate >= 17.0)
                {
                    continue;
                }
            }

            m_cpuCulledCommands[m_cpuCullCursors[section.w >> 3]++] = m_opaqueCommands[i];
        }

        device->BindArrayBuffer(m_culledCommandBuffer);
        device->UpdateArrayBuffer(0, (uint32_t) (m_cpuCulledCommands.size() * sizeof(DrawIndirectCommand)), m_cpuCulledCommands.data());
        return;
    }

    glm::ivec4 cameraSection((int) floor(m_renderCameraPos.x / Chunk::s_sizeX), (int) floor(m_renderCameraPos.y / ChunkSection::s_size),
                             (int) floor(m_renderCameraPos.z / Chunk::s_sizeZ), (int) m_opaqueCommands.size());
    float uniforms[64];
    for (int i = 0; i < FrustumData::s_planeCount; i++)
    {
        const FrustumPlane &plane = m_frustum.GetData().planes[i];
        uniforms[i * 4]           = m_frustumCulling ? (float) plane.normal.x : 0.0f;
        uniforms[i * 4 + 1]       = m_frustumCulling ? (float) plane.normal.y : 0.0f;
        uniforms[i * 4 + 2]       = m_frustumCulling ? (float) plane.normal.z : 0.0f;
        uniforms[i * 4 + 3]       = m_frustumCulling ? (float) (plane.distance + glm::dot(plane.normal, m_renderCameraPos)) : 1.0f;
    }

    memcpy(uniforms + 24, glm::value_ptr(cameraSection), sizeof(cameraSection));
    uniforms[28] = (float) (m_renderCameraPos.x - (double) cameraSection.x * Chunk::s_sizeX);
    uniforms[29] = (float) (m_renderCameraPos.y - (double) cameraSection.y * ChunkSection::s_size);
    uniforms[30] = (float) (m_renderCameraPos.z - (double) cameraSection.z * Chunk::s_sizeZ);
    uniforms[31] = 0.0f;
    glm::mat4 occlusionMatrix(glm::translate(m_hizMatrix, m_renderCameraPos - m_hizCameraPos));
    glm::ivec3 occlusionSize(m_hizWidth, m_hizHeight, m_hizLevels);
    memcpy(uniforms + 32, glm::value_ptr(occlusionMatrix), sizeof(occlusionMatrix));
    memcpy(uniforms + 48, glm::value_ptr(occlusionSize), sizeof(occlusionSize));
    uniforms[51] = m_hizValid && m_frustumCulling && glm::distance(m_renderCameraPos, m_hizCameraPos) < 16.0 ? 1.0f : 0.0f;
    uniforms[52] = (float) m_fogCullDistance;
    uniforms[55] = 0.0f;
    glm::ivec4 occlusionGrid(m_occlusionOriginX, m_occlusionOriginZ, m_occlusionRadius, m_occlusionValid ? 1 : 0);
    glm::ivec4 renderArea(areaX, areaZ, renderDistance, ChunkRenderRegion::s_size);
    memcpy(uniforms + 56, glm::value_ptr(occlusionGrid), sizeof(occlusionGrid));
    memcpy(uniforms + 60, glm::value_ptr(renderArea), sizeof(renderArea));
    device->ClearBuffer(m_culledCountBuffer);
    if (!device->HasDrawIndirectCount())
    {
        device->ClearBuffer(m_culledCommandBuffer);
    }

    BufferHandle buffers[7] = {m_opaqueCommandBuffer, m_opaqueSectionBuffer, m_culledCommandBuffer, m_culledCountBuffer,
                               m_hizBuffer,           m_visibilityBuffer,    m_groupWindowBuffer};
    // We cull in distance rings so the compacted draws come out roughly front to back for early depth rejection
    float rings[5] = {0.0f, 32.0f, 96.0f, 256.0f, std::numeric_limits<float>::infinity()};
    for (int i = 0; i < 4; i++)
    {
        uniforms[53] = rings[i];
        uniforms[54] = rings[i + 1];
        device->DispatchCompute(m_cullProgram, uniforms, buffers, 7, nullptr, (uint32_t) (m_opaqueCommands.size() + 63) / 64);
    }
}

void LevelRenderer::BuildDepthPyramid(const glm::dmat4 &viewProjection)
{
    RenderDevice *device = RenderDevice::GetInstance();
    int width            = m_sceneFramebuffer->GetWidth();
    int height           = m_sceneFramebuffer->GetHeight();
    if (width != m_hizWidth || height != m_hizHeight)
    {
        m_hizWidth      = width;
        m_hizHeight     = height;
        m_hizLevels     = 1;
        int levelWidth  = (width + 3) / 4;
        int levelHeight = (height + 3) / 4;
        int totalSize   = levelWidth * levelHeight;
        while (levelWidth > 1 || levelHeight > 1)
        {
            levelWidth  = (levelWidth + 1) / 2;
            levelHeight = (levelHeight + 1) / 2;
            totalSize += levelWidth * levelHeight;
            m_hizLevels++;
        }

        device->BindArrayBuffer(m_hizBuffer);
        device->UploadArrayBuffer(nullptr, (uint32_t) totalSize * sizeof(float), eBufferUsage_Static);
    }

    TextureHandle depthTexture = m_sceneFramebuffer->GetDepthTexture();
    int sourceOffset           = 0;
    int sourceWidth            = width;
    int sourceHeight           = height;
    int destinationOffset      = 0;
    for (int level = 0; level < m_hizLevels; level++)
    {
        int scale             = level == 0 ? 4 : 2;
        int destinationWidth  = (sourceWidth + scale - 1) / scale;
        int destinationHeight = (sourceHeight + scale - 1) / scale;
        int params[8] = {sourceOffset, sourceWidth, sourceHeight, scale, destinationOffset, destinationWidth, destinationHeight, level == 0 ? 1 : 0};
        device->DispatchCompute(m_hizProgram, (const float *) params, &m_hizBuffer, 1, &depthTexture,
                                (uint32_t) (destinationWidth * destinationHeight + 63) / 64);
        sourceOffset = destinationOffset;
        sourceWidth  = destinationWidth;
        sourceHeight = destinationHeight;
        destinationOffset += destinationWidth * destinationHeight;
    }

    m_hizMatrix    = viewProjection;
    m_hizCameraPos = m_renderCameraPos;
    m_hizValid     = true;
}

void LevelRenderer::DrawChunkBatches(const std::vector<ChunkBatch> &batches, BufferHandle commands, bool culled)
{
    RenderDevice *device    = RenderDevice::GetInstance();
    ChunkVertexArena *arena = ChunkVertexArena::GetInstance();
    for (size_t i = 0; i < batches.size(); i++)
    {
        const ChunkBatch &batch = batches[i];
        Textures::Bind(batch.texture->GetId(), 0);
        device->BindVertexArray(arena->GetVertexArray(batch.page));
        ChunkMesh::BindQuadIndices(batch.maxQuads);
        uint32_t offset = batch.firstCommand * (uint32_t) sizeof(DrawIndirectCommand);
        if (culled && m_gpuCulling && device->HasDrawIndirectCount())
        {
            device->DrawMultiElementsIndirectCount(ePrimitiveType_Triangles, commands, offset, m_culledCountBuffer,
                                                   batch.firstCommand * (uint32_t) sizeof(uint32_t), batch.commandCount);
        }
        else
        {
            device->DrawMultiElementsIndirect(ePrimitiveType_Triangles, commands, offset, batch.commandCount);
        }
    }
}

void LevelRenderer::RenderChunks(Shader *shader)
{
    if (!shader)
    {
        return;
    }

    GlStateManager::SetDepthMask(true);
    GlStateManager::DisableBlend();

    DrawChunkBatches(m_opaqueBatches, m_culledCommandBuffer, true);

    GlStateManager::SetDepthMask(true);
}

void LevelRenderer::RenderTranslucentChunks(bool depthPrepass)
{
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    if (!depthPrepass)
    {
        GlStateManager::SetDepthMask(false);
        DrawChunkBatches(m_translucentBatches[0], m_chunkCommandBuffer, false);
    }
    else
    {
        // Water depth goes down first, then everything else is split into the part behind the water and the part in front of it
        GlStateManager::SetColorMask(false, false, false, false);
        GlStateManager::SetDepthMask(true);
        DrawChunkBatches(m_translucentBatches[1], m_chunkCommandBuffer, false);
        GlStateManager::SetColorMask(true, true, true, true);
        GlStateManager::SetDepthMask(false);

        GlStateManager::SetDepthFunc(eCompareFunc_Greater);
        DrawChunkBatches(m_translucentBatches[2], m_chunkCommandBuffer, false);
        GlStateManager::SetDepthFunc(eCompareFunc_LessEqual);
        // The water shader tests against the opaque depth itself, so water behind other water still draws
        GlStateManager::DisableDepthTest();
        DrawChunkBatches(m_translucentBatches[1], m_chunkCommandBuffer, false);
        GlStateManager::EnableDepthTest();
    }

    GlStateManager::DisableBlend();
    GlStateManager::SetDepthMask(true);
}

void LevelRenderer::RenderTranslucentFrontChunks()
{
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::SetDepthMask(false);
    DrawChunkBatches(m_translucentBatches[2], m_chunkCommandBuffer, false);
    GlStateManager::DisableBlend();
    GlStateManager::SetDepthMask(true);
}

bool LevelRenderer::ToggleChunkGrid()
{
    m_renderChunkGrid = !m_renderChunkGrid;
    return m_renderChunkGrid;
}

bool LevelRenderer::ToggleHitboxes()
{
    m_renderHitboxes = !m_renderHitboxes;
    return m_renderHitboxes;
}

void LevelRenderer::RenderChunkGrid() const
{
    const uint32_t neighbourColor = 0xFFFF0000;
    const uint32_t cornerColor    = 0xFF3F3FFF;
    const uint32_t majorColor     = 0xFFFFFF00;
    const uint32_t minorColor     = 0xFF00BFBF;
    const uint32_t sectionColor   = 0xFF3F3FFF;

    GlStateManager::EnableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();

    Tesselator *renderer   = Tesselator::GetInstance();
    BufferBuilder *builder = renderer->GetBuilder();

    const glm::dvec3 origin = m_renderCameraPos;
    int chunkX              = Mth::FloorDiv((int) floor(origin.x), Chunk::s_sizeX);
    int chunkZ              = Mth::FloorDiv((int) floor(origin.z), Chunk::s_sizeZ);
    float minX              = (float) ((double) chunkX * Chunk::s_sizeX - origin.x);
    float minZ              = (float) ((double) chunkZ * Chunk::s_sizeZ - origin.z);
    float minY              = (float) (-origin.y);
    float maxY              = minY + Chunk::s_sizeY;
    float sizeX             = (float) Chunk::s_sizeX;
    float sizeZ             = (float) Chunk::s_sizeZ;

    GlStateManager::PushMatrix();
    GlStateManager::Translated(origin.x, origin.y, origin.z);

    builder->Begin(ePrimitiveType_Lines);

    builder->Color(neighbourColor);
    for (int i = -1; i <= 2; i++)
    {
        for (int j = -1; j <= 2; j++)
        {
            if (i >= 0 && i <= 1 && j >= 0 && j <= 1)
            {
                continue;
            }

            builder->Vertex(minX + i * sizeX, minY, minZ + j * sizeZ);
            builder->Vertex(minX + i * sizeX, maxY, minZ + j * sizeZ);
        }
    }

    for (int i = 0; i <= Chunk::s_sizeX; i += 2)
    {
        bool corner = i == 0 || i == Chunk::s_sizeX;
        builder->Color(corner ? cornerColor : (i % 4 == 0 ? majorColor : minorColor));
        builder->Vertex(minX + i, minY, minZ);
        builder->Vertex(minX + i, maxY, minZ);
        builder->Vertex(minX + i, minY, minZ + sizeZ);
        builder->Vertex(minX + i, maxY, minZ + sizeZ);
    }

    for (int i = 2; i < Chunk::s_sizeZ; i += 2)
    {
        builder->Color(i % 4 == 0 ? majorColor : minorColor);
        builder->Vertex(minX, minY, minZ + i);
        builder->Vertex(minX, maxY, minZ + i);
        builder->Vertex(minX + sizeX, minY, minZ + i);
        builder->Vertex(minX + sizeX, maxY, minZ + i);
    }

    for (int y = 0; y <= Chunk::s_sizeY; y += 2)
    {
        builder->Color(y % 16 == 0 ? sectionColor : (y % 8 == 0 ? majorColor : minorColor));
        float lineY = minY + y;
        builder->Vertex(minX, lineY, minZ);
        builder->Vertex(minX + sizeX, lineY, minZ);
        builder->Vertex(minX + sizeX, lineY, minZ);
        builder->Vertex(minX + sizeX, lineY, minZ + sizeZ);
        builder->Vertex(minX + sizeX, lineY, minZ + sizeZ);
        builder->Vertex(minX, lineY, minZ + sizeZ);
        builder->Vertex(minX, lineY, minZ + sizeZ);
        builder->Vertex(minX, lineY, minZ);
    }

    builder->End();

    GlStateManager::PopMatrix();
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableCull();
}

void LevelRenderer::RenderParticles(float partialTicks)
{
    const double particleRenderDistance     = 32.0;
    const double particleRenderDistanceSq   = particleRenderDistance * particleRenderDistance;
    const double particleFrustumRadiusScale = 1.41421356237;

    ParticleEngine *particleEngine = m_level->GetParticleEngine();
    if (!particleEngine)
    {
        return;
    }

    std::vector<ParticleEngine::RenderParticle> &particles = m_particleScratch;
    particleEngine->CopyRenderParticles(&particles);
    float particlePartialTicks = particleEngine->GetRenderPartialTicks();
    if (particles.empty())
    {
        return;
    }

    Everstead *everstead     = Everstead::GetInstance();
    const Camera *camera     = everstead->GetCamera();
    Texture *particleTexture = ParticleAtlas::GetTexture();
    TextureAtlas *blockAtlas = BlockAtlas::GetInstance()->GetBlockAtlas();
    Texture *terrainTexture  = blockAtlas ? blockAtlas->GetTexture() : nullptr;
    Texture *itemTexture     = ItemAtlas::GetInstance()->GetTexture();
    glm::dvec3 cameraPos     = camera->GetPosition();
    glm::dvec3 front         = glm::normalize(camera->GetFront());
    glm::dvec3 worldUp       = fabs(front.y) > 0.99 ? glm::dvec3(0.0, 0.0, 1.0) : glm::dvec3(0.0, 1.0, 0.0);
    glm::dvec3 right         = glm::normalize(glm::cross(worldUp, front));
    glm::dvec3 up            = glm::normalize(glm::cross(front, right));

    std::vector<VisibleParticle> &visibleParticles        = m_visibleParticles;
    std::vector<VisibleParticle> &visibleTerrainParticles = m_visibleTerrainParticles;
    std::vector<VisibleParticle> &visibleItemParticles    = m_visibleItemParticles;
    visibleParticles.clear();
    visibleTerrainParticles.clear();
    visibleItemParticles.clear();
    if (m_particleLightCacheTick != m_level->GetGameTime())
    {
        m_particleLightCacheTick = m_level->GetGameTime();
        m_particleLightCache.clear();
    }

    for (const ParticleEngine::RenderParticle &particle : particles)
    {
        glm::dvec3 pos = particle.oldPosition + (particle.position - particle.oldPosition) * (double) particlePartialTicks;
        if (glm::distance2(pos, camera->GetPosition()) > particleRenderDistanceSq)
        {
            continue;
        }

        if (!m_frustum.TestSphere(pos, std::max((double) particle.size * particleFrustumRadiusScale, 0.05)))
        {
            continue;
        }

        uint32_t color = particle.color;
        if (!particle.lit)
        {
            BlockPos lightPos((int) floor(pos.x), (int) floor(pos.y), (int) floor(pos.z));
            auto cached = m_particleLightCache.find(lightPos);
            if (cached == m_particleLightCache.end())
            {
                // Every particle in this block shares the sample, so we take it at the block center instead of wherever the first one is
                cached = m_particleLightCache.emplace(lightPos, SampleWorldLightColor(glm::dvec3(lightPos.x, lightPos.y, lightPos.z) + 0.5)).first;
            }

            color = MultiplyColorRgb(color, cached->second);
        }

        if (particle.type == eParticleType_ItemCrack)
        {
            visibleItemParticles.push_back({particle.region, pos, color, particle.size});
            continue;
        }

        if (particle.terrain)
        {
            if (!terrainTexture || !particle.hasRegion)
            {
                continue;
            }

            visibleTerrainParticles.push_back({particle.region, pos, color, particle.size});
            continue;
        }

        if (!particleTexture || !particle.hasRegion)
        {
            continue;
        }

        visibleParticles.push_back({particle.region, pos, color, particle.size});
    }

    if (visibleParticles.empty() && visibleTerrainParticles.empty() && visibleItemParticles.empty())
    {
        return;
    }

    GlStateManager::MatrixMode(0);
    GlStateManager::LoadIdentity();
    glm::dmat4 viewMatrix = everstead->GetGameRenderer()->GetViewMatrix(camera);
    viewMatrix            = everstead->GetGameRenderer()->GetViewBobbing(everstead->GetLocalPlayer(), partialTicks) * viewMatrix;
    GlStateManager::MultMatrix(viewMatrix);
    GlStateManager::Translated(-cameraPos.x, -cameraPos.y, -cameraPos.z);

    GlStateManager::EnableDepthTest();
    GlStateManager::DisableCull();

    GlStateManager::DisableBlend();
    GlStateManager::SetDepthMask(true);
    GlStateManager::SetColorMask(false, false, false, false);
    EmitParticles(visibleParticles, particleTexture, right, up, true);
    EmitParticles(visibleTerrainParticles, terrainTexture, right, up, true);
    EmitParticles(visibleItemParticles, itemTexture, right, up, true);

    GlStateManager::SetColorMask(true, true, true, true);
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::SetDepthMask(false);
    GlStateManager::SetDepthFunc(eCompareFunc_Equal);
    EmitParticles(visibleParticles, particleTexture, right, up, false);
    EmitParticles(visibleTerrainParticles, terrainTexture, right, up, false);
    EmitParticles(visibleItemParticles, itemTexture, right, up, false);

    GlStateManager::SetDepthFunc(eCompareFunc_LessEqual);
    GlStateManager::SetDepthMask(true);
    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
}

void LevelRenderer::CopySceneDepth(Framebuffer *sceneFramebuffer, Framebuffer *outputFramebuffer)
{
    RenderDevice::GetInstance()->BlitFramebufferDepth(sceneFramebuffer->GetId(), sceneFramebuffer->GetWidth(), sceneFramebuffer->GetHeight(),
                                                      outputFramebuffer->GetId(), outputFramebuffer->GetWidth(), outputFramebuffer->GetHeight());
}

void LevelRenderer::RenderWeather(float partialTicks)
{
    const int radius   = 9;
    const int diameter = radius * 2 + 1;

    float rain = m_level->GetRainLevel(partialTicks);
    if (rain <= 0.0f)
    {
        return;
    }

    Everstead *everstead = Everstead::GetInstance();
    const Camera *camera = everstead->GetCamera();
    glm::dvec3 cameraPos = camera->GetPosition();
    int cameraX          = (int) floor(cameraPos.x);
    int cameraY          = (int) floor(cameraPos.y);
    int cameraZ          = (int) floor(cameraPos.z);
    int64_t tick         = m_level->GetGameTime();

    if (tick != m_rainCacheTick || cameraX != m_rainCacheX || cameraZ != m_rainCacheZ)
    {
        m_rainCacheTick = tick;
        m_rainCacheX    = cameraX;
        m_rainCacheZ    = cameraZ;
        m_rainHeights.resize(diameter * diameter);
        m_rainLight.resize(diameter * diameter);
        m_rainSnow.resize(diameter * diameter);
        m_weatherColumns = {};
        for (int dz = 0; dz < diameter; dz++)
        {
            for (int dx = 0; dx < diameter; dx++)
            {
                int x      = cameraX + dx - radius;
                int z      = cameraZ + dz - radius;
                int height = m_level->GetRainHeight(x, z);
                int index  = dz * diameter + dx;
                // This skips the level's rain threshold so weather fades in from zero instead of popping
                const Chunk *chunk = m_level->GetChunk(ChunkPos(x >> 4, z >> 4));
                Biome *biome       = chunk ? chunk->GetSurfaceBiome(x & 15, z & 15) : nullptr;
                m_rainSnow[index]  = biome && biome->IsSnowyAt(std::max(height, cameraY));
                if (!m_rainSnow[index] && (!biome || !biome->HasRain()))
                {
                    m_rainHeights[index] = INT32_MAX;
                    continue;
                }

                m_weatherColumns[m_rainSnow[index]]++;
                m_rainHeights[index] = height;
                m_rainLight[index]   = SampleWorldLightColor(glm::dvec3(x + 0.5, std::max(height, cameraY) + 0.5, z + 0.5)) & 0x00FFFFFF;
            }
        }
    }

    // We derive the wind from game time so every player sees the same gusts
    double seconds  = ((double) tick + partialTicks) / 20.0;
    float windAngle = (float) (sin(seconds * 0.011) * 1.7 + sin(seconds * 0.0043 + 2.1) * 1.1);
    float windStrength =
            std::clamp((float) (0.45 + sin(seconds * 0.07) * 0.25 + sin(seconds * 0.19 + 1.3) * 0.12) + m_level->GetThunderLevel(partialTicks) * 0.3f,
                       0.0f, 1.0f);
    glm::vec2 wind = glm::vec2(cosf(windAngle), sinf(windAngle)) * windStrength;

    // The phases wrap where every column speed scrolls a whole number of texture repeats, so the wrap never shows
    double weatherTime = (double) tick + partialTicks;
    float elapsed      = (float) std::clamp(weatherTime - m_weatherTime, 0.0, 20.0);
    m_weatherTime      = weatherTime;
    m_rainPhase        = fmodf(m_rainPhase + elapsed / 32.0f, 4.0f);
    m_snowPhase        = fmodf(m_snowPhase + elapsed / 512.0f * (0.8f + windStrength * 0.6f), 8.0f);

    GlStateManager::MatrixMode(0);
    GlStateManager::LoadIdentity();
    glm::dmat4 viewMatrix = everstead->GetGameRenderer()->GetViewMatrix(camera);
    viewMatrix            = everstead->GetGameRenderer()->GetViewBobbing(everstead->GetLocalPlayer(), partialTicks) * viewMatrix;
    GlStateManager::MultMatrix(viewMatrix);

    GlStateManager::EnableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::SetDepthMask(false);
    GlStateManager::SetAlphaCutoff(-1.0f);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    GlStateManager::EnablePolygonOffsetFill();
    GlStateManager::SetPolygonOffset(-3.0f, -10.0f);
    RenderRipples(partialTicks, cameraPos, builder);
    GlStateManager::SetPolygonOffset(0.0f, 0.0f);
    GlStateManager::DisablePolygonOffsetFill();

    GlStateManager::SetDepthMask(true);
    GlStateManager::SetAlphaCutoff(0.1f);
    for (int pass = 0; pass < 2; pass++)
    {
        if (m_weatherColumns[pass] == 0)
        {
            continue;
        }

        RenderWeatherPass(pass == 1, rain, wind, cameraPos, cameraX, cameraY, cameraZ, builder);
    }

    GlStateManager::SetAlphaCutoff(-1.0f);
    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
}

void LevelRenderer::RenderWeatherHaze(float radius, BufferBuilder *builder)
{
    const int segments      = 48;
    const int bands         = 4;
    const float hazeAlpha   = 0.55f;
    const float cloudHeight = (float) Everstead::GetInstance()->options.cloudHeight + 0.33f;

    float strength = hazeAlpha * m_level->GetRainLevel(1.0f) * m_weatherFog * (1.0f + m_snowFog * 0.3f) * m_voidFogSkyLight / 15.0f;
    float top      = (float) (cloudHeight - Everstead::GetInstance()->GetCamera()->GetPosition().y);
    if (strength <= 0.0f || top <= 0.0f)
    {
        return;
    }

    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::DisableCull();
    builder->Begin(ePrimitiveType_Triangles);
    for (int segment = 0; segment < segments; segment++)
    {
        double angle0 = segment * std::numbers::pi * 2.0 / segments;
        double angle1 = (segment + 1) * std::numbers::pi * 2.0 / segments;
        float x0      = (float) cos(angle0) * radius;
        float z0      = (float) sin(angle0) * radius;
        float x1      = (float) cos(angle1) * radius;
        float z1      = (float) sin(angle1) * radius;
        for (int band = -1; band < bands; band++)
        {
            float t0     = std::max((float) band / bands, 0.0f);
            float t1     = (float) (band + 1) / bands;
            float y0     = band < 0 ? -radius : top * t0;
            float y1     = top * t1;
            float alpha0 = strength * (1.0f - t0) * (1.0f - t0);
            float alpha1 = strength * (1.0f - t1) * (1.0f - t1);
            builder->Color(m_fogColor.x, m_fogColor.y, m_fogColor.z, alpha0);
            builder->Vertex(x0, y0, z0);
            builder->Vertex(x1, y0, z1);
            builder->Color(m_fogColor.x, m_fogColor.y, m_fogColor.z, alpha1);
            builder->Vertex(x1, y1, z1);
            builder->Color(m_fogColor.x, m_fogColor.y, m_fogColor.z, alpha0);
            builder->Vertex(x0, y0, z0);
            builder->Color(m_fogColor.x, m_fogColor.y, m_fogColor.z, alpha1);
            builder->Vertex(x1, y1, z1);
            builder->Vertex(x0, y1, z0);
        }
    }

    builder->End();
    GlStateManager::EnableCull();
    GlStateManager::DisableBlend();
}

void LevelRenderer::AddRipple(const glm::dvec3 &position)
{
    const size_t maxRipples = 256;

    if (m_ripples.size() < maxRipples)
    {
        m_ripples.push_back({position, m_level->GetGameTime(), SampleWorldLightColor(position) & 0x00FFFFFF});
    }
}

void LevelRenderer::RenderRipples(float partialTicks, const glm::dvec3 &cameraPos, BufferBuilder *builder)
{
    const int rippleTicks = 8;
    const int frames      = 4;
    const float halfSize  = 0.25f;

    int64_t tick = m_level->GetGameTime();
    std::erase_if(m_ripples,
                  [tick](const Ripple &ripple)
                  {
                      return ripple.tick > tick || tick - ripple.tick >= rippleTicks;
                  });
    if (m_ripples.empty())
    {
        return;
    }

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(m_rippleTexture->GetId());
    for (size_t i = 0; i < m_ripples.size(); i++)
    {
        const Ripple &ripple = m_ripples[i];
        float age            = (float) (tick - ripple.tick) + partialTicks;
        float alpha          = std::clamp(1.0f - age / rippleTicks, 0.0f, 1.0f);
        int frame            = std::min((int) (age * frames / rippleTicks), frames - 1);
        float v0             = (float) frame / frames;
        float v1             = (float) (frame + 1) / frames;
        float x              = (float) (ripple.position.x - cameraPos.x);
        float y              = (float) (ripple.position.y - cameraPos.y);
        float z              = (float) (ripple.position.z - cameraPos.z);

        builder->Color(((uint32_t) (alpha * 255.0f) << 24) | ripple.light);
        builder->VertexUV(x - halfSize, y, z - halfSize, 0.0f, v0);
        builder->VertexUV(x + halfSize, y, z - halfSize, 1.0f, v0);
        builder->VertexUV(x + halfSize, y, z + halfSize, 1.0f, v1);
        builder->VertexUV(x - halfSize, y, z - halfSize, 0.0f, v0);
        builder->VertexUV(x + halfSize, y, z + halfSize, 1.0f, v1);
        builder->VertexUV(x - halfSize, y, z + halfSize, 0.0f, v1);
    }

    builder->End();
    Textures::Unbind();
}

void LevelRenderer::RenderWorldBorder(float partialTicks)
{
    const uint32_t staticColor = 0x20A0FF;
    const uint32_t growColor   = 0x40FF80;
    const uint32_t shrinkColor = 0xFF3030;

    const WorldBorder &border = m_level->GetWorldBorder();
    Everstead *everstead      = Everstead::GetInstance();
    const Camera *camera      = everstead->GetCamera();
    glm::dvec3 cameraPos      = camera->GetPosition();
    double viewDistance       = (double) (m_level->GetRenderDistance() * Chunk::s_sizeX);
    double halfSize           = border.GetSize(partialTicks) * 0.5;
    double minX               = border.GetCenterX() - halfSize;
    double maxX               = border.GetCenterX() + halfSize;
    double minZ               = border.GetCenterZ() - halfSize;
    double maxZ               = border.GetCenterZ() + halfSize;
    double distance           = std::min(std::min(cameraPos.x - minX, maxX - cameraPos.x), std::min(cameraPos.z - minZ, maxZ - cameraPos.z));
    if (distance >= viewDistance)
    {
        return;
    }

    double fade         = std::clamp(1.0 - distance / viewDistance, 0.0, 1.0);
    uint32_t alpha      = (uint32_t) (pow(fade, 4.0) * 255.0);
    uint32_t color      = border.GetStatus() == eWorldBorderStatus_Growing
                                  ? growColor
                                  : (border.GetStatus() == eWorldBorderStatus_Shrinking ? shrinkColor : staticColor);
    float scroll        = (float) (Time::GetMilliseconds() % 3000) / 3000.0f;
    float height        = (float) viewDistance;
    float vTop          = scroll - (float) (cameraPos.y * 0.5 - floor(cameraPos.y * 0.5));
    float vBottom       = vTop + height;
    double walls[4]     = {maxX, minX, maxZ, minZ};
    float directions[4] = {-1.0f, 1.0f, 1.0f, -1.0f};

    GlStateManager::MatrixMode(0);
    GlStateManager::LoadIdentity();
    glm::dmat4 viewMatrix = everstead->GetGameRenderer()->GetViewMatrix(camera);
    viewMatrix            = everstead->GetGameRenderer()->GetViewBobbing(everstead->GetLocalPlayer(), partialTicks) * viewMatrix;
    GlStateManager::MultMatrix(viewMatrix);

    GlStateManager::EnableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_One);
    GlStateManager::SetDepthMask(false);
    GlStateManager::SetAlphaCutoff(-1.0f);
    GlStateManager::EnablePolygonOffsetFill();
    GlStateManager::SetPolygonOffset(-3.0f, -10.0f);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(m_forcefieldTexture->GetId());
    builder->Color((alpha << 24) | color);
    for (int i = 0; i < 4; i++)
    {
        bool alongZ   = i < 2;
        double across = alongZ ? cameraPos.x : cameraPos.z;
        double along  = alongZ ? cameraPos.z : cameraPos.x;
        double from   = std::max(floor(along - viewDistance), alongZ ? minZ : minX);
        double to     = std::min(ceil(along + viewDistance), alongZ ? maxZ : maxX);
        if (fabs(across - walls[i]) >= viewDistance || from >= to)
        {
            continue;
        }

        // The wall shares its plane with block faces, so it is nudged toward the camera by an amount that grows with distance
        double offset = walls[i] - across;
        double nudge  = std::min(0.002 + fabs(offset) * 0.0005, 0.25);
        double plane  = offset > 0.0 ? offset - nudge : offset + nudge;
        float start   = (float) (from * 0.5 - floor(from * 0.5));
        float u0      = scroll + directions[i] * start;
        float u1      = scroll + directions[i] * (start + (float) ((to - from) * 0.5));
        EmitBorderWall(builder, alongZ, (float) plane, (float) (from - along), (float) (to - along), u0, u1, -height, height, vBottom, vTop);
    }

    builder->End();
    Textures::Unbind();

    GlStateManager::SetPolygonOffset(0.0f, 0.0f);
    GlStateManager::DisablePolygonOffsetFill();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::SetDepthMask(true);
    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
}

void LevelRenderer::AddTear(const BlockPos &pos)
{
    for (size_t i = 0; i < m_tears.size(); i++)
    {
        if (m_tears[i].pos == pos)
        {
            return;
        }
    }

    m_tears.push_back({pos, 0.0f});
}

// The section mesh with this block is built on a worker, so we keep drawing it until a mesh requested after the change is applied
void LevelRenderer::AddSettlingBlock(const BlockPos &pos, Block *block, uint16_t data)
{
    m_settlingBlocks.push_back({pos, block, data, m_nextMeshGeneration + 1});
}

void LevelRenderer::AddAfterimage(const Entity *entity)
{
    uint32_t now = Time::GetMilliseconds();
    m_afterimages.push_back({entity->GetId(), {}, now, now, 0});
}

void LevelRenderer::RenderAfterimages(LocalPlayer *localPlayer, float partialTicks)
{
    const size_t maxGhosts        = 8;
    const uint32_t sampleInterval = 40;
    const uint32_t fadeTime       = 300;
    uint32_t now                  = Time::GetMilliseconds();
    Tesselator *tesselator        = Tesselator::GetInstance();
    for (size_t i = 0; i < m_afterimages.size();)
    {
        Afterimage &afterimage = m_afterimages[i];
        Entity *entity         = m_level->GetEntityById(afterimage.entityId);
        if (entity == localPlayer)
        {
            entity = localPlayer->IsFirstPerson() ? nullptr : localPlayer->GetRenderPlayer();
        }

        float fade = afterimage.stopTime == 0 ? 1.0f : 1.0f - (float) (now - afterimage.stopTime) / fadeTime;
        if (!entity || fade <= 0.0f)
        {
            m_afterimages.erase(m_afterimages.begin() + (ptrdiff_t) i);
            continue;
        }

        glm::dvec3 renderPos = entity->GetRenderPosition(partialTicks);
        if (afterimage.stopTime == 0 && now - afterimage.sampleTime >= sampleInterval)
        {
            // We stop once the entity barely moves between samples, which is when it lands or hits a wall
            bool still = !afterimage.trail.empty() && glm::distance2(renderPos, afterimage.trail.back()) < 0.01 && now - afterimage.startTime > 200;
            if (still || now - afterimage.startTime > 2000)
            {
                afterimage.stopTime = now;
            }
            else
            {
                afterimage.trail.push_back(renderPos);
                if (afterimage.trail.size() > maxGhosts)
                {
                    afterimage.trail.erase(afterimage.trail.begin());
                }
            }

            afterimage.sampleTime = now;
        }

        uint32_t lightColor = SampleEntityLightColor(entity, partialTicks);
        Lighting::SetEntityLight(lightColor);
        EntityRenderer *renderer = EntityRendererRegistry::GetInstance()->GetValue(entity->GetType());
        renderer->SetRenderBoundingBox(false);
        for (size_t j = 0; j < afterimage.trail.size(); j++)
        {
            glm::dvec3 offset = afterimage.trail[j] - renderPos;
            tesselator->SetGhostAlpha(0.45f * (float) (j + 1) / (float) afterimage.trail.size() * fade);
            GlStateManager::PushMatrix();
            GlStateManager::Translated(offset.x, offset.y, offset.z);
            renderer->Render(entity, partialTicks, lightColor);
            GlStateManager::PopMatrix();
        }

        i++;
    }

    tesselator->SetGhostAlpha(0.0f);
}

void LevelRenderer::FindTears()
{
    uint32_t fractureCore = Block::ByName("fracture_core")->GetId();
    glm::dvec3 cameraPos  = Everstead::GetInstance()->GetCamera()->GetPosition();
    int cameraChunkX      = (int) floor(cameraPos.x / Chunk::s_sizeX);
    int cameraChunkZ      = (int) floor(cameraPos.z / Chunk::s_sizeZ);
    int radius            = m_level->GetRenderDistance();

    // Tears still fading out keep their entry, so a broken core finishes its close animation
    std::vector<Tear> previous = std::move(m_tears);
    m_tears.clear();
    for (size_t i = 0; i < previous.size(); i++)
    {
        if (previous[i].open > 0.0f)
        {
            m_tears.push_back(previous[i]);
        }
    }

    std::vector<std::pair<ChunkPos, Chunk *>> chunks = m_level->GetLoadedChunks();
    for (size_t i = 0; i < chunks.size(); i++)
    {
        const ChunkPos &pos = chunks[i].first;
        if (abs(pos.x - cameraChunkX) > radius || abs(pos.z - cameraChunkZ) > radius)
        {
            continue;
        }

        Chunk *chunk = chunks[i].second;
        for (int section = 0; section < Chunk::s_sectionCount; section++)
        {
            if (chunk->IsSectionEmpty(section) || !chunk->GetSection(section).ContainsBlock(fractureCore))
            {
                continue;
            }

            const ChunkSection &blocks = chunk->GetSection(section);
            for (int y = 0; y < ChunkSection::s_size; y++)
            {
                for (int z = 0; z < ChunkSection::s_size; z++)
                {
                    for (int x = 0; x < ChunkSection::s_size; x++)
                    {
                        if (blocks.GetBlockId(x, y, z) == fractureCore)
                        {
                            AddTear(BlockPos(pos.x * Chunk::s_sizeX + x, section * ChunkSection::s_size + y, pos.z * Chunk::s_sizeZ + z));
                        }
                    }
                }
            }
        }
    }
}

void LevelRenderer::RenderTears(Framebuffer *outputFramebuffer, const glm::dmat4 &projection, float partialTicks)
{
    const int maxTears = 16;

    uint32_t now    = Time::GetMilliseconds();
    float delta     = std::min((float) (now - m_tearFrameTime) / 1000.0f, 0.1f);
    m_tearFrameTime = now;
    if (now - m_tearScanTime >= 1000)
    {
        m_tearScanTime = now;
        FindTears();
    }

    Everstead *everstead    = Everstead::GetInstance();
    glm::dvec3 cameraPos    = everstead->GetCamera()->GetPosition();
    FractureCoreBlock *core = (FractureCoreBlock *) Block::ByName("fracture_core");
    float tears[maxTears * 4];
    float states[maxTears * 4];
    int count = 0;
    for (size_t i = 0; i < m_tears.size(); i++)
    {
        Tear &tear    = m_tears[i];
        int stability = core->GetStability(m_level, tear.pos);
        bool open     = m_level->GetBlockId(tear.pos) == core->GetId() && core->IsOpen(m_level->GetBlockData(tear.pos)) && stability >= 0;
        tear.open     = std::clamp(tear.open + (open ? delta / 1.9f : -delta / 1.0f), 0.0f, 1.0f);
        if (tear.open <= 0.0f || count == maxTears)
        {
            continue;
        }

        glm::dvec3 center     = core->GetTearCenter(tear.pos) - cameraPos;
        tears[count * 4]      = (float) center.x;
        tears[count * 4 + 1]  = (float) center.y;
        tears[count * 4 + 2]  = (float) center.z;
        tears[count * 4 + 3]  = core->GetTearSeed(tear.pos);
        states[count * 4]     = tear.open;
        states[count * 4 + 1] = 1.0f - (float) std::max(stability, 0) / 3.0f;
        states[count * 4 + 2] = open ? 1.0f : 0.0f;
        states[count * 4 + 3] = 0.0f;
        count++;
    }

    if (count == 0)
    {
        return;
    }

    GameRenderer *gameRenderer = everstead->GetGameRenderer();
    glm::dmat4 viewMatrix =
            gameRenderer->GetViewBobbing(everstead->GetLocalPlayer(), partialTicks) * gameRenderer->GetViewMatrix(everstead->GetCamera());
    glm::dmat4 invViewProj = glm::inverse(projection * viewMatrix);
    BeginHalfResPass(outputFramebuffer);
    m_tearShader->Bind();
    Textures::Bind(m_sceneFramebuffer->GetDepthTexture(), 0);
    m_tearShader->SetMat4("u_invViewProj", glm::value_ptr(invViewProj));
    m_tearShader->SetVec4Array("u_tears", count, tears);
    m_tearShader->SetVec4Array("u_tearStates", count, states);
    m_tearShader->SetFloat("u_tearCount", (float) count);
    m_tearShader->SetFloat("u_time", (float) (now % 600000) / 1000.0f);
    RenderDevice::GetInstance()->DrawArrays(ePrimitiveType_Triangles, 0, 3);
    CompositeHalfResPass(outputFramebuffer, m_halfResTarget);
}

void LevelRenderer::RenderWorldEffects(Framebuffer *outputFramebuffer, const glm::dmat4 &projection, float partialTicks)
{
    m_worldEffects.Update(&m_level->GetDynamicLights());
    const std::vector<WorldEffects::Effect> &effects = m_worldEffects.GetEffects();
    if (effects.empty())
    {
        m_worldEffectHistoryValid = false;
        return;
    }

    Everstead *everstead = Everstead::GetInstance();
    glm::dvec3 cameraPos = everstead->GetCamera()->GetPosition();
    uint32_t now         = Time::GetMilliseconds();
    float centers[WorldEffects::kMaxEffects * 4];
    float states[WorldEffects::kMaxEffects * 4];
    double focusDistance = INFINITY;
    for (size_t i = 0; i < effects.size(); i++)
    {
        glm::dvec3 center  = effects[i].center - cameraPos;
        focusDistance      = std::min(focusDistance, glm::length(center));
        centers[i * 4]     = (float) center.x;
        centers[i * 4 + 1] = (float) center.y;
        centers[i * 4 + 2] = (float) center.z;
        centers[i * 4 + 3] = effects[i].radius;
        states[i * 4]      = (float) effects[i].type;
        states[i * 4 + 1]  = m_worldEffects.GetProgress(effects[i], now);
        states[i * 4 + 2]  = effects[i].seed;
        states[i * 4 + 3]  = 0.0f;
    }

    GameRenderer *gameRenderer = everstead->GetGameRenderer();
    glm::dmat4 viewMatrix =
            gameRenderer->GetViewBobbing(everstead->GetLocalPlayer(), partialTicks) * gameRenderer->GetViewMatrix(everstead->GetCamera());
    glm::dmat4 invViewProj = glm::inverse(projection * viewMatrix);
    glm::vec4 flash        = m_worldEffects.GetFlash(cameraPos);
    BeginHalfResPass(outputFramebuffer);
    m_worldEffectShader->Bind();
    Textures::Bind(m_sceneFramebuffer->GetDepthTexture(), 0);
    m_worldEffectShader->SetMat4("u_invViewProj", glm::value_ptr(invViewProj));
    m_worldEffectShader->SetVec4Array("u_effects", (int) effects.size(), centers);
    m_worldEffectShader->SetVec4Array("u_effectStates", (int) effects.size(), states);
    m_worldEffectShader->SetVec4("u_flash", flash.r, flash.g, flash.b, flash.a);
    m_worldEffectShader->SetFloat("u_effectCount", (float) effects.size());
    m_worldEffectShader->SetFloat("u_time", (float) (now % 600000) / 1000.0f);
    m_worldEffectShader->SetFloat("u_frame", (float) (m_worldEffectFrame % 64));
    RenderDevice *device = RenderDevice::GetInstance();
    device->DrawArrays(ePrimitiveType_Triangles, 0, 3);

    Framebuffer *history  = m_worldEffectHistory[m_worldEffectHistoryIndex];
    Framebuffer *resolved = m_worldEffectHistory[m_worldEffectHistoryIndex ^ 1];
    bool sizeChanged      = resolved->GetWidth() != m_halfResTarget->GetWidth() || resolved->GetHeight() != m_halfResTarget->GetHeight();
    history->Resize(m_halfResTarget->GetWidth(), m_halfResTarget->GetHeight());
    resolved->Resize(m_halfResTarget->GetWidth(), m_halfResTarget->GetHeight());
    resolved->Bind();
    glm::dmat4 viewProj  = projection * viewMatrix;
    glm::dmat4 reproject = glm::translate(m_worldEffectViewProj, cameraPos - m_worldEffectCameraPos);
    m_worldEffectResolveShader->Bind();
    Textures::Bind(m_halfResTarget->GetColorTexture(), 0);
    Textures::Bind(history->GetColorTexture(), 1);
    Textures::Bind(m_sceneFramebuffer->GetDepthTexture(), 2);
    m_worldEffectResolveShader->SetMat4("u_invViewProj", glm::value_ptr(invViewProj));
    m_worldEffectResolveShader->SetMat4("u_reproject", glm::value_ptr(reproject));
    m_worldEffectResolveShader->SetFloat("u_focusDistance", (float) focusDistance);
    m_worldEffectResolveShader->SetFloat("u_historyWeight", m_worldEffectHistoryValid && !sizeChanged ? 0.85f : 0.0f);
    device->DrawArrays(ePrimitiveType_Triangles, 0, 3);
    CompositeHalfResPass(outputFramebuffer, resolved);

    m_worldEffectHistoryIndex ^= 1;
    m_worldEffectHistoryValid = true;
    m_worldEffectFrame++;
    m_worldEffectViewProj  = viewProj;
    m_worldEffectCameraPos = cameraPos;
}

// The marches are too heavy at full resolution once the camera is inside one, so we trace at half size and upscale
void LevelRenderer::BeginHalfResPass(Framebuffer *outputFramebuffer)
{
    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();
    GlStateManager::DisableBlend();
    m_halfResTarget->Resize(std::max(outputFramebuffer->GetWidth() / 2, 1), std::max(outputFramebuffer->GetHeight() / 2, 1));
    m_halfResTarget->Bind();
    RenderDevice *device = RenderDevice::GetInstance();
    device->SetViewport(0, 0, m_halfResTarget->GetWidth(), m_halfResTarget->GetHeight());
    device->BindVertexArray(m_fullscreenVao);
}

void LevelRenderer::CompositeHalfResPass(Framebuffer *outputFramebuffer, Framebuffer *source)
{
    RenderDevice *device = RenderDevice::GetInstance();
    outputFramebuffer->Bind();
    device->SetViewport(0, 0, outputFramebuffer->GetWidth(), outputFramebuffer->GetHeight());
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_One, eBlendFactor_OneMinusSrcAlpha);
    device->SetTextureFilter(source->GetColorTexture(), eTextureFilter_Linear, eTextureFilter_Linear);
    m_compositeShader->Bind();
    Textures::Bind(source->GetColorTexture(), 0);
    m_compositeShader->SetFloat("u_gammaExponent", 1.0f);
    device->DrawArrays(ePrimitiveType_Triangles, 0, 3);
    device->BindVertexArray({});
    Textures::Unbind();

    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();
}

void LevelRenderer::RenderWeatherPass(bool snow, float rain, const glm::vec2 &wind, const glm::dvec3 &cameraPos, int cameraX, int cameraY,
                                      int cameraZ, BufferBuilder *builder)
{
    const int radius        = 9;
    const int diameter      = radius * 2 + 1;
    const float halfWidth   = 0.5f;
    const float cloudHeight = (float) Everstead::GetInstance()->options.cloudHeight + 0.33f;

    float phase    = snow ? m_snowPhase : m_rainPhase;
    glm::vec2 lean = wind * (snow ? 0.5f : 0.2f);

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(snow ? m_snowTexture->GetId() : m_rainTexture->GetId());
    for (int dz = 0; dz < diameter; dz++)
    {
        for (int dx = 0; dx < diameter; dx++)
        {
            int index  = dz * diameter + dx;
            int height = m_rainHeights[index];
            if (height == INT32_MAX || m_rainSnow[index] != snow)
            {
                continue;
            }

            int y0 = std::max(cameraY - radius, height);
            int y1 = std::max(cameraY + radius, height);
            if (cameraPos.y > cloudHeight)
            {
                y0 = std::max(y0, (int) ceilf(cloudHeight));
            }
            else
            {
                y1 = std::min(y1, (int) cloudHeight);
            }

            if (y0 >= y1)
            {
                continue;
            }

            int x           = cameraX + dx - radius;
            int z           = cameraZ + dz - radius;
            double centerX  = x + 0.5 - cameraPos.x;
            double centerZ  = z + 0.5 - cameraPos.z;
            double distance = sqrt(centerX * centerX + centerZ * centerZ);
            if (distance < 0.001)
            {
                continue;
            }

            double normalX = centerZ / distance * halfWidth;
            double normalZ = -centerX / distance * halfWidth;
            uint32_t hash  = (uint32_t) (x * x * 3121 + x * 45238971) ^ (uint32_t) (z * z * 418711 + z * 13761);
            float speed    = snow ? 1.0f + (float) ((hash >> 24) & 3) * 0.125f : 3.0f + (float) ((hash >> 24) & 3) * 0.25f;
            float scroll   = phase * speed + (float) (hash & 255) / 256.0f;
            float u0       = 0.0f;
            if (snow)
            {
                u0 = (float) ((hash >> 16) & 255) / 255.0f + phase * (float) ((int) ((hash >> 8) & 7) - 4);
            }

            float u1    = u0 + 1.0f;
            float fade  = (float) (distance / radius);
            float alpha = std::clamp(((1.0f - fade * fade) * 0.5f + 0.5f) * rain * (0.7f + (float) ((hash >> 12) & 15) * 0.02f), 0.0f, 1.0f);
            float shade = (0.85f + (float) ((hash >> 28) & 15) * 0.01f) / 255.0f;
            float x0    = (float) (centerX - normalX);
            float z0    = (float) (centerZ - normalZ);
            float x1    = (float) (centerX + normalX);
            float z1    = (float) (centerZ + normalZ);

            uint32_t light = m_rainLight[index];
            builder->Color((float) ((light >> 16) & 255) * shade, (float) ((light >> 8) & 255) * shade, (float) (light & 255) * shade, alpha);
            // The streak leans over neighbouring columns, so each block of it is clipped against the column it is above
            for (int y = y0; y < y1; y++)
            {
                float leanBottom = (float) (y - y0);
                float leanTop    = leanBottom + 1.0f;
                int columnX      = dx + (int) floorf(0.5f + lean.x * (leanBottom + 0.5f));
                int columnZ      = dz + (int) floorf(0.5f + lean.y * (leanBottom + 0.5f));
                if (columnX >= 0 && columnX < diameter && columnZ >= 0 && columnZ < diameter && m_rainHeights[columnZ * diameter + columnX] > y)
                {
                    continue;
                }

                float bottom = (float) (y - cameraPos.y);
                float top    = bottom + 1.0f;
                float v0     = y * 0.25f + scroll;
                float v1     = v0 + 0.25f;
                builder->VertexUV(x0 + lean.x * leanBottom, bottom, z0 + lean.y * leanBottom, u0, v0);
                builder->VertexUV(x1 + lean.x * leanBottom, bottom, z1 + lean.y * leanBottom, u1, v0);
                builder->VertexUV(x1 + lean.x * leanTop, top, z1 + lean.y * leanTop, u1, v1);
                builder->VertexUV(x0 + lean.x * leanBottom, bottom, z0 + lean.y * leanBottom, u0, v0);
                builder->VertexUV(x1 + lean.x * leanTop, top, z1 + lean.y * leanTop, u1, v1);
                builder->VertexUV(x0 + lean.x * leanTop, top, z0 + lean.y * leanTop, u0, v1);
            }
        }
    }

    builder->End();
    Textures::Unbind();
}

void LevelRenderer::EmitParticles(const std::vector<VisibleParticle> &items, Texture *texture, const glm::dvec3 &right, const glm::dvec3 &up,
                                  bool depthOnly)
{
    const float particleDepthAlphaCutoff = 0.1f;

    if (items.empty() || !texture)
    {
        return;
    }

    Tesselator *renderer   = Tesselator::GetInstance();
    BufferBuilder *builder = renderer->GetBuilder();

    const glm::dvec3 origin = m_renderCameraPos;
    GlStateManager::PushMatrix();
    GlStateManager::Translated(origin.x, origin.y, origin.z);

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(texture->GetId());
    GlStateManager::SetAlphaCutoff(depthOnly ? particleDepthAlphaCutoff : -1.0f);

    for (const VisibleParticle &particle : items)
    {
        const glm::dvec3 pos = particle.pos - origin;
        uint32_t color       = depthOnly ? 0xFFFFFFFF : particle.color;
        glm::dvec3 dx        = right * (double) particle.size;
        glm::dvec3 dy        = up * (double) particle.size;

        glm::dvec3 p1 = pos - dx - dy;
        glm::dvec3 p2 = pos - dx + dy;
        glm::dvec3 p3 = pos + dx + dy;
        glm::dvec3 p4 = pos + dx - dy;

        builder->Color(color);
        builder->VertexUV((float) p1.x, (float) p1.y, (float) p1.z, particle.region.u0, particle.region.v1);
        builder->VertexUV((float) p2.x, (float) p2.y, (float) p2.z, particle.region.u0, particle.region.v0);
        builder->VertexUV((float) p3.x, (float) p3.y, (float) p3.z, particle.region.u1, particle.region.v0);
        builder->VertexUV((float) p1.x, (float) p1.y, (float) p1.z, particle.region.u0, particle.region.v1);
        builder->VertexUV((float) p3.x, (float) p3.y, (float) p3.z, particle.region.u1, particle.region.v0);
        builder->VertexUV((float) p4.x, (float) p4.y, (float) p4.z, particle.region.u1, particle.region.v1);
    }

    builder->End();
    GlStateManager::PopMatrix();
    Textures::Unbind();
    GlStateManager::SetAlphaCutoff(-1.0f);
}

void LevelRenderer::RenderBlockOutline()
{
    LocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    if (GameType::IsSpectator(localPlayer->GetGameType()))
    {
        return;
    }

    HitResult *hit = localPlayer->Pick();
    if (!hit)
    {
        return;
    }

    if (!hit->IsBlock())
    {
        delete hit;
        return;
    }

    const BlockPos blockPos = hit->GetBlockPos();
    delete hit;

    const float outlineEpsilon = 0.002f;

    Block *block = Block::ById(m_level->GetBlockId(blockPos));
    if (!block)
    {
        return;
    }

    ChunkPos chunkPos(Mth::FloorDiv(blockPos.x, Chunk::s_sizeX), Mth::FloorDiv(blockPos.z, Chunk::s_sizeZ));
    const Chunk *chunk = m_level->GetChunk(chunkPos);
    if (!chunk)
    {
        return;
    }

    int lx = Mth::FloorMod(blockPos.x, Chunk::s_sizeX);
    int ly = Mth::FloorMod(blockPos.y, Chunk::s_sizeY);
    int lz = Mth::FloorMod(blockPos.z, Chunk::s_sizeZ);
    std::vector<AABB> boxes;
    block->GetSelectionBoxes(m_level, blockPos, chunk->GetBlockData(lx, ly, lz), &boxes);

    const glm::dvec3 origin((double) blockPos.x, (double) blockPos.y, (double) blockPos.z);

    GlStateManager::EnableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::SetDepthFunc(eCompareFunc_LessEqual);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();

    RenderDevice::GetInstance()->SetLineWidth(2.0f);

    GlStateManager::PushMatrix();
    GlStateManager::Translated(origin.x, origin.y, origin.z);

    builder->Begin(ePrimitiveType_Lines);
    builder->Color(0x66000000);
    for (size_t i = 0; i < boxes.size(); i++)
    {
        float minX = (float) (boxes[i].GetMin().x - origin.x) - outlineEpsilon;
        float minY = (float) (boxes[i].GetMin().y - origin.y) - outlineEpsilon;
        float minZ = (float) (boxes[i].GetMin().z - origin.z) - outlineEpsilon;
        float maxX = (float) (boxes[i].GetMax().x - origin.x) + outlineEpsilon;
        float maxY = (float) (boxes[i].GetMax().y - origin.y) + outlineEpsilon;
        float maxZ = (float) (boxes[i].GetMax().z - origin.z) + outlineEpsilon;

        builder->Vertex(minX, minY, minZ);
        builder->Vertex(maxX, minY, minZ);
        builder->Vertex(maxX, minY, minZ);
        builder->Vertex(maxX, minY, maxZ);
        builder->Vertex(maxX, minY, maxZ);
        builder->Vertex(minX, minY, maxZ);
        builder->Vertex(minX, minY, maxZ);
        builder->Vertex(minX, minY, minZ);

        builder->Vertex(minX, maxY, minZ);
        builder->Vertex(maxX, maxY, minZ);
        builder->Vertex(maxX, maxY, minZ);
        builder->Vertex(maxX, maxY, maxZ);
        builder->Vertex(maxX, maxY, maxZ);
        builder->Vertex(minX, maxY, maxZ);
        builder->Vertex(minX, maxY, maxZ);
        builder->Vertex(minX, maxY, minZ);

        builder->Vertex(minX, minY, minZ);
        builder->Vertex(minX, maxY, minZ);
        builder->Vertex(maxX, minY, minZ);
        builder->Vertex(maxX, maxY, minZ);
        builder->Vertex(maxX, minY, maxZ);
        builder->Vertex(maxX, maxY, maxZ);
        builder->Vertex(minX, minY, maxZ);
        builder->Vertex(minX, maxY, maxZ);
    }

    builder->End();

    GlStateManager::PopMatrix();

    GlStateManager::DisableBlend();
    GlStateManager::SetDepthFunc(eCompareFunc_LessEqual);
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableCull();
}

void LevelRenderer::SetBlockDestruction(uint8_t playerIndex, const BlockPos &pos, int stage)
{
    if (stage < 0 || stage >= (int) m_destroyStageTextures.size())
    {
        m_blockDestructions.erase(playerIndex);
        return;
    }

    m_blockDestructions[playerIndex] = {pos, stage, Time::GetMilliseconds()};
}

void LevelRenderer::RenderBlockDestruction()
{
    LocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    uint32_t now             = Time::GetMilliseconds();
    for (auto it = m_blockDestructions.begin(); it != m_blockDestructions.end();)
    {
        if (now - it->second.updateMs > 20000)
        {
            it = m_blockDestructions.erase(it);
        }
        else
        {
            it++;
        }
    }

    if (m_blockDestructions.empty() && localPlayer->GetDestroyStage() < 0)
    {
        return;
    }

    GlStateManager::EnableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_DstColor, eBlendFactor_SrcColor);
    GlStateManager::SetAlphaCutoff(0.1f);
    GlStateManager::EnablePolygonOffsetFill();
    GlStateManager::SetPolygonOffset(-3.0f, -3.0f);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    if (localPlayer->GetDestroyStage() >= 0)
    {
        EmitDestroyStage(builder, localPlayer->GetDestroyPos(), localPlayer->GetDestroyStage());
    }

    const glm::dvec3 &playerPos = localPlayer->GetPosition();
    for (const std::pair<const uint8_t, BlockDestruction> &entry : m_blockDestructions)
    {
        const BlockPos &pos = entry.second.pos;
        double dx           = pos.x + 0.5 - playerPos.x;
        double dy           = pos.y + 0.5 - playerPos.y;
        double dz           = pos.z + 0.5 - playerPos.z;
        if (dx * dx + dy * dy + dz * dz <= 1024.0)
        {
            EmitDestroyStage(builder, pos, entry.second.stage);
        }
    }

    Textures::Unbind();
    GlStateManager::SetPolygonOffset(0.0f, 0.0f);
    GlStateManager::DisablePolygonOffsetFill();
    GlStateManager::SetAlphaCutoff(-1.0f);
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::DisableBlend();
    GlStateManager::SetDepthMask(true);
}

void LevelRenderer::EmitDestroyStage(BufferBuilder *builder, const BlockPos &pos, int stage)
{
    uint32_t blockId = m_level->GetBlockId(pos);
    if (blockId == 0)
    {
        return;
    }

    std::vector<AABB> boxes;
    Block::ById(blockId)->GetSelectionBoxes(m_level, pos, m_level->GetBlockData(pos), &boxes);

    GlStateManager::PushMatrix();
    GlStateManager::Translated((double) pos.x, (double) pos.y, (double) pos.z);
    Textures::Bind(m_destroyStageTextures[stage]->GetId());
    builder->Begin(ePrimitiveType_Triangles);
    builder->Color(0x80FFFFFF);
    for (size_t i = 0; i < boxes.size(); i++)
    {
        const glm::dvec3 &min = boxes[i].GetMin();
        const glm::dvec3 &max = boxes[i].GetMax();
        float x0              = (float) (min.x - pos.x);
        float y0              = (float) (min.y - pos.y);
        float z0              = (float) (min.z - pos.z);
        float x1              = (float) (max.x - pos.x);
        float y1              = (float) (max.y - pos.y);
        float z1              = (float) (max.z - pos.z);
        EmitQuad(builder, x0, y0, z0, x0, z0, x1, y0, z0, x1, z0, x1, y0, z1, x1, z1, x0, y0, z1, x0, z1);
        EmitQuad(builder, x0, y1, z1, x0, z1, x1, y1, z1, x1, z1, x1, y1, z0, x1, z0, x0, y1, z0, x0, z0);
        EmitQuad(builder, x1, y1, z0, 1.0f - x1, 1.0f - y1, x1, y0, z0, 1.0f - x1, 1.0f - y0, x0, y0, z0, 1.0f - x0, 1.0f - y0, x0, y1, z0, 1.0f - x0,
                 1.0f - y1);
        EmitQuad(builder, x0, y1, z1, x0, 1.0f - y1, x0, y0, z1, x0, 1.0f - y0, x1, y0, z1, x1, 1.0f - y0, x1, y1, z1, x1, 1.0f - y1);
        EmitQuad(builder, x0, y1, z0, z0, 1.0f - y1, x0, y0, z0, z0, 1.0f - y0, x0, y0, z1, z1, 1.0f - y0, x0, y1, z1, z1, 1.0f - y1);
        EmitQuad(builder, x1, y1, z1, 1.0f - z1, 1.0f - y1, x1, y0, z1, 1.0f - z1, 1.0f - y0, x1, y0, z0, 1.0f - z0, 1.0f - y0, x1, y1, z0, 1.0f - z0,
                 1.0f - y1);
    }

    builder->End();
    GlStateManager::PopMatrix();
}

void LevelRenderer::RenderEntities(float partialTicks)
{
    RenderDevice::GetInstance()->SetColorMask(true, true, true, true);
    GlStateManager::EnableDepthTest();
    GlStateManager::SetDepthMask(true);
    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();

    Everstead *everstead       = Everstead::GetInstance();
    const glm::dvec3 cameraPos = everstead->GetCamera()->GetPosition();
    LocalPlayer *localPlayer   = everstead->GetLocalPlayer();
    glm::dmat4 viewMatrix      = everstead->GetGameRenderer()->GetViewMatrix(everstead->GetCamera());
    viewMatrix                 = everstead->GetGameRenderer()->GetViewBobbing(localPlayer, partialTicks) * viewMatrix;

    GlStateManager::MatrixMode(0);
    GlStateManager::LoadIdentity();
    GlStateManager::MultMatrix(viewMatrix);
    GlStateManager::Translated(-cameraPos.x, -cameraPos.y, -cameraPos.z);

    Lighting::TurnOn();

    // Entity draws are grouped by state and texture, so everything up to the block entities goes out as a few draws
    Tesselator::GetInstance()->BeginBatch();
    std::vector<Entity *> items;
    std::vector<Boat *> boats;
    TickRateManager *tickRateManager = everstead->GetTickRateManager();
    for (const std::unique_ptr<Entity> &entity : m_level->GetEntities())
    {
        if (entity.get() == localPlayer || (entity->GetSharedFlag(eEntityFlag_Spectator) && !GameType::IsSpectator(localPlayer->GetGameType())))
        {
            continue;
        }

        const AABB &bounds = entity->GetBoundingBox();
        glm::dvec3 extent  = bounds.GetMax() - bounds.GetMin();
        double maxDistance = (extent.x + extent.y + extent.z) / 3.0 * 64.0 * entity->GetRenderDistanceScale();
        if (entity->GetType() == eEntityType_Player)
        {
            maxDistance *= 10.0;
        }

        maxDistance *= everstead->options.entityRenderDistance / 25.0;

        float entityPartialTicks = tickRateManager->IsEntityFrozen(entity.get()) ? 1.0f : partialTicks;
        glm::dvec3 renderPos     = entity->GetRenderPosition(entityPartialTicks);
        if (glm::distance2(renderPos, cameraPos) >= maxDistance * maxDistance)
        {
            continue;
        }

        if (!m_frustum.TestAABB(entity->GetBoundingBox().Translated(renderPos)))
        {
            continue;
        }

        if (entity->GetType() == eEntityType_Item)
        {
            items.push_back(entity.get());
            continue;
        }

        if (entity->GetType() == eEntityType_Boat)
        {
            boats.push_back((Boat *) entity.get());
        }

        uint32_t lightColor = SampleEntityLightColor(entity.get(), entityPartialTicks);
        Lighting::SetEntityLight(lightColor);
        EntityRenderer *renderer = EntityRendererRegistry::GetInstance()->GetValue(entity->GetType());
        renderer->SetRenderBoundingBox(m_renderHitboxes);
        renderer->Render(entity.get(), entityPartialTicks, lightColor);
        const Mob *mob = dynamic_cast<const Mob *>(entity.get());
        if (mob && mob->GetLeashHolderId() >= 0)
        {
            RenderLeash(mob, entityPartialTicks);
        }
    }

    EntityRenderer *itemRenderer = EntityRendererRegistry::GetInstance()->GetValue(eEntityType_Item);
    itemRenderer->SetRenderBoundingBox(m_renderHitboxes);
    Lighting::SetEntityLight(0xFFFFFFFF);
    for (Entity *item : items)
    {
        float entityPartialTicks = tickRateManager->IsEntityFrozen(item) ? 1.0f : partialTicks;
        itemRenderer->Render(item, entityPartialTicks, SampleEntityLightColor(item, entityPartialTicks));
    }

    for (const std::pair<const BlockPos, std::unique_ptr<BlockEntity>> &blockEntity : m_level->GetBlockEntities())
    {
        const BlockPos &pos           = blockEntity.first;
        BlockEntityRenderer *renderer = BlockEntityRendererRegistry::GetInstance()->GetValue(blockEntity.second->GetId());
        glm::dvec3 center((double) pos.x + 0.5, (double) pos.y + 0.5, (double) pos.z + 0.5);
        // The cull box reaches one block past the origin so double chests stay visible
        if (!renderer || glm::distance2(center, cameraPos) >= 64.0 * 64.0 ||
            !m_frustum.TestAABB(AABB(glm::dvec3(pos.x, pos.y, pos.z), glm::dvec3(pos.x + 2.0, pos.y + 1.0, pos.z + 2.0))))
        {
            continue;
        }

        Lighting::SetEntityLight(SampleWorldLightColor(center));
        renderer->Render(blockEntity.second.get(), partialTicks);
    }

    for (size_t i = 0; i < m_settlingBlocks.size();)
    {
        const SettlingBlock &settling = m_settlingBlocks[i];
        if (m_level->GetBlockId(settling.pos) != settling.block->GetId())
        {
            m_settlingBlocks.erase(m_settlingBlocks.begin() + i);
            continue;
        }

        glm::dvec3 center((double) settling.pos.x + 0.5, (double) settling.pos.y + 0.5, (double) settling.pos.z + 0.5);
        Lighting::SetEntityLight(SampleWorldLightColor(center));
        GlStateManager::PushMatrix();
        GlStateManager::Translated(center.x, center.y, center.z);
        BlockRenderer::RenderBlockAsEntity(settling.block, settling.data, Lighting::GetEntityLight());
        GlStateManager::PopMatrix();
        i++;
    }

    RenderAfterimages(localPlayer, partialTicks);
    Tesselator::GetInstance()->EndBatch();
    Textures::Unbind();
    RenderSignText(cameraPos);
    RenderBlockDestruction();

    if (!localPlayer->IsFirstPerson())
    {
        RemotePlayer *renderPlayer = localPlayer->GetRenderPlayer();
        if (m_frustum.TestAABB(renderPlayer->GetBoundingBox().Translated(renderPlayer->GetRenderPosition(partialTicks))))
        {
            uint32_t lightColor = SampleEntityLightColor(renderPlayer, partialTicks);
            Lighting::SetEntityLight(lightColor);
            EntityRenderer *renderer = EntityRendererRegistry::GetInstance()->GetValue(renderPlayer->GetType());
            renderer->SetRenderBoundingBox(m_renderHitboxes);
            renderer->Render(renderPlayer, partialTicks, lightColor);
        }
    }

    // Boat water patches only write depth, so they go last or they would cut away riders drawn after them
    BoatRenderer *boatRenderer = (BoatRenderer *) EntityRendererRegistry::GetInstance()->GetValue(eEntityType_Boat);
    RenderDevice::GetInstance()->SetColorMask(false, false, false, false);
    for (size_t i = 0; i < boats.size(); i++)
    {
        boatRenderer->RenderWaterPatch(boats[i], partialTicks);
    }

    RenderDevice::GetInstance()->SetColorMask(true, true, true, true);
    Textures::Unbind();
    Lighting::TurnOff();
    Lighting::SetEntityLight(0xFFFFFFFF);
    RenderEntityShadows(partialTicks);
}

void LevelRenderer::RenderEntityShadows(float partialTicks)
{
    const double shadowDistance = 16.0;

    Everstead *everstead = Everstead::GetInstance();
    if (!m_shadowTexture || !everstead->options.entityShadows)
    {
        return;
    }

    const glm::dvec3 cameraPos = everstead->GetCamera()->GetPosition();
    LocalPlayer *localPlayer   = everstead->GetLocalPlayer();

    GlStateManager::PushMatrix();
    GlStateManager::Translated(cameraPos.x, cameraPos.y, cameraPos.z);
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();
    Textures::Bind(m_shadowTexture->GetId());

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    for (const std::unique_ptr<Entity> &entity : m_level->GetEntities())
    {
        if (entity.get() == localPlayer || entity->GetSharedFlag(eEntityFlag_Spectator) ||
            (entity->GetType() == RemotePlayer::s_type && ((RemotePlayer *) entity.get())->GetDeathTime() >= 20))
        {
            continue;
        }

        glm::dvec3 renderPos = entity->GetRenderPosition(partialTicks);
        if (glm::distance2(renderPos, cameraPos) <= shadowDistance * shadowDistance)
        {
            AddEntityShadow(builder, entity.get(), renderPos, cameraPos);
        }
    }

    if (!localPlayer->IsFirstPerson())
    {
        RemotePlayer *renderPlayer = localPlayer->GetRenderPlayer();
        if (!GameType::IsSpectator(localPlayer->GetGameType()))
        {
            AddEntityShadow(builder, renderPlayer, renderPlayer->GetRenderPosition(partialTicks), cameraPos);
        }
    }

    builder->End();
    Textures::Unbind();
    GlStateManager::EnableCull();
    GlStateManager::SetDepthMask(true);
    GlStateManager::DisableBlend();
    GlStateManager::PopMatrix();
}

// This projects the shadow texture onto the top of every block under the entity, fading with height and light
void LevelRenderer::AddEntityShadow(BufferBuilder *builder, Entity *entity, const glm::dvec3 &renderPos, const glm::dvec3 &cameraPos)
{
    const double shadowDistanceSq = 256.0;
    const double surfaceLift      = 1.0 / 64.0;
    const int minLight            = 3;

    double radius   = 0.0;
    double strength = 1.0;
    switch (entity->GetType())
    {
        case eEntityType_RemotePlayer:
        case eEntityType_FallingBlock:
        case eEntityType_PrimedTnt:
        case eEntityType_Boat:
            radius = 0.5;
            break;
        case eEntityType_Item:
            if (((const ItemEntity *) entity)->IsCollecting())
            {
                return;
            }

            radius   = 0.15;
            strength = 0.75;
            break;
        default:
            radius = EntityRendererRegistry::GetInstance()->GetValue(entity->GetType())->GetShadowRadius();
            if (radius <= 0.0)
            {
                return;
            }

            if (((const Mob *) entity)->IsBaby())
            {
                radius *= 0.5;
            }

            break;
    }

    double baseAlpha = (1.0 - glm::distance2(renderPos, cameraPos) / shadowDistanceSq) * strength;
    if (baseAlpha <= 0.0)
    {
        return;
    }

    std::vector<AABB> boxes;
    int minX = (int) floor(renderPos.x - radius);
    int maxX = (int) floor(renderPos.x + radius);
    int minY = (int) floor(renderPos.y - radius);
    int maxY = (int) floor(renderPos.y);
    int minZ = (int) floor(renderPos.z - radius);
    int maxZ = (int) floor(renderPos.z + radius);
    for (int x = minX; x <= maxX; x++)
    {
        for (int y = minY; y <= maxY; y++)
        {
            for (int z = minZ; z <= maxZ; z++)
            {
                BlockPos below(x, y - 1, z);
                Block *block = Block::ById(m_level->GetBlockId(below));
                if (!block->IsCollidable())
                {
                    continue;
                }

                int light = (int) m_level->GetLightLevel(BlockPos(x, y, z));
                if (light <= minLight)
                {
                    continue;
                }

                // Only full collision cubes catch a shadow, partial shapes would leave it floating
                boxes.clear();
                block->GetCollisionBoxes(m_level->GetBlockData(below), &boxes);
                if (boxes.size() != 1 || boxes[0].GetMin() != glm::dvec3(0.0) || boxes[0].GetMax() != glm::dvec3(1.0))
                {
                    continue;
                }

                double top   = (double) y;
                double alpha = (baseAlpha - (renderPos.y - top) / 2.0) * 0.5 * LightSource::SampleBrightnessRamp((uint8_t) light);
                if (alpha < 0.0)
                {
                    continue;
                }

                double x0 = (double) x;
                double x1 = (double) x + 1.0;
                double z0 = (double) z;
                double z1 = (double) z + 1.0;
                float u0  = (float) ((x0 - renderPos.x) / 2.0 / radius + 0.5);
                float u1  = (float) ((x1 - renderPos.x) / 2.0 / radius + 0.5);
                float v0  = (float) ((z0 - renderPos.z) / 2.0 / radius + 0.5);
                float v1  = (float) ((z1 - renderPos.z) / 2.0 / radius + 0.5);
                float px0 = (float) (x0 - cameraPos.x);
                float px1 = (float) (x1 - cameraPos.x);
                float py  = (float) (top + surfaceLift - cameraPos.y);
                float pz0 = (float) (z0 - cameraPos.z);
                float pz1 = (float) (z1 - cameraPos.z);

                builder->Color(1.0f, 1.0f, 1.0f, (float) std::min(alpha, 1.0));
                builder->VertexUV(px0, py, pz0, u0, v0);
                builder->VertexUV(px0, py, pz1, u0, v1);
                builder->VertexUV(px1, py, pz1, u1, v1);
                builder->VertexUV(px0, py, pz0, u0, v0);
                builder->VertexUV(px1, py, pz1, u1, v1);
                builder->VertexUV(px1, py, pz0, u1, v0);
            }
        }
    }
}

void LevelRenderer::RenderSignText(const glm::dvec3 &cameraPos)
{
    const float modelScale = 2.0f / 3.0f;
    const float pixelSize  = modelScale / 64.0f;

    Font *font      = Everstead::GetInstance()->GetDefaultFont();
    float fontScale = 8.0f * pixelSize / font->GetAscent(1.0f);
    for (const std::pair<const BlockPos, std::unique_ptr<BlockEntity>> &blockEntity : m_level->GetBlockEntities())
    {
        const BlockPos &pos = blockEntity.first;
        glm::dvec3 center((double) pos.x + 0.5, (double) pos.y + 0.5, (double) pos.z + 0.5);
        double distSq = glm::distance2(center, cameraPos);
        if (strcmp(blockEntity.second->GetId(), "sign") != 0 || distSq >= 64.0 * 64.0 ||
            !m_frustum.TestAABB(AABB(glm::dvec3(pos.x, pos.y, pos.z), glm::dvec3(pos.x + 1.0, pos.y + 1.0, pos.z + 1.0))))
        {
            continue;
        }

        SignBlockEntity *sign = (SignBlockEntity *) blockEntity.second.get();
        SignBlock *block      = (SignBlock *) Block::ById(m_level->GetBlockId(pos));
        uint32_t light        = SampleWorldLightColor(center);

        GlStateManager::PushMatrix();
        GlStateManager::Translated(pos.x + 0.5, pos.y + 0.75 * modelScale, pos.z + 0.5);
        GlStateManager::Rotatef(-block->GetYRot(m_level->GetBlockData(pos)), 0.0f, 1.0f, 0.0f);
        if (block->IsWall())
        {
            GlStateManager::Translatef(0.0f, -0.3125f, -0.4375f);
        }

        for (int side = 0; side < 2; side++)
        {
            SignBlockEntity::SignText *text = sign->GetText(side == 0);
            uint32_t color                  = SignBlockEntity::GetTextColor(text->color);
            uint32_t outlineColor           = color == 0 ? 0xF0EBCC : color;
            float shade                     = text->glowing ? 1.0f : 0.4f;
            uint32_t r                      = (uint32_t) (((color >> 16) & 0xFF) * shade);
            uint32_t g                      = (uint32_t) (((color >> 8) & 0xFF) * shade);
            uint32_t b                      = (uint32_t) ((color & 0xFF) * shade);
            if (!text->glowing)
            {
                r = r * ((light >> 16) & 0xFF) / 255;
                g = g * ((light >> 8) & 0xFF) / 255;
                b = b * (light & 0xFF) / 255;
            }

            uint32_t argb = 0xFF000000 | (r << 16) | (g << 8) | b;
            outlineColor  = 0xFF000000 | ((uint32_t) (((outlineColor >> 16) & 0xFF) * 0.4f) << 16) |
                            ((uint32_t) (((outlineColor >> 8) & 0xFF) * 0.4f) << 8) | (uint32_t) ((outlineColor & 0xFF) * 0.4f);

            GlStateManager::PushMatrix();
            GlStateManager::Rotatef(side * 180.0f, 0.0f, 1.0f, 0.0f);
            GlStateManager::Translatef(0.0f, 0.5f * modelScale, 0.07f * modelScale);
            for (int line = 0; line < 4; line++)
            {
                std::wstring message = StringUtils::ToUtf16(text->messages[line]);
                if (message.empty())
                {
                    continue;
                }

                float x = -font->GetWidth(message, fontScale) * 0.5f;
                float y = (12 - line * 10) * pixelSize;
                if (text->glowing && distSq < 16.0 * 16.0)
                {
                    for (int ox = -1; ox <= 1; ox++)
                    {
                        for (int oy = -1; oy <= 1; oy++)
                        {
                            if (ox != 0 || oy != 0)
                            {
                                font->Draw(message, x + ox * pixelSize, y + oy * pixelSize, -0.001f, fontScale, outlineColor);
                            }
                        }
                    }
                }

                font->Draw(message, x, y, 0.0f, fontScale, argb);
            }

            GlStateManager::PopMatrix();
        }

        GlStateManager::PopMatrix();
    }

    GlStateManager::EnableCull();
    GlStateManager::DisableBlend();
}

void LevelRenderer::RenderEntityNameTags(float partialTicks)
{
    const double nameTagRenderDistance      = 64.0;
    const double nameTagSneakRenderDistance = 32.0;
    const float nameTagSneakDrop            = 0.25f;
    const uint32_t nameTagFaintTextColor    = 0x20FFFFFF;
    const float nameTagPixelSize            = 1.6f / 60.0f;
    const float nameTagAscentPixels         = 8.0f;
    const float nameTagHeadroom             = 0.5f;
    const uint32_t nameTagTextColor         = 0xFFFFFFFF;
    const uint32_t nameTagFillColor         = 0x40000000;
    const uint32_t nameTagBorderColor       = 0xFF000000;

    Everstead *everstead       = Everstead::GetInstance();
    Font *font                 = everstead->GetDefaultFont();
    const glm::dvec3 cameraPos = everstead->GetCamera()->GetPosition();
    LocalPlayer *localPlayer   = everstead->GetLocalPlayer();
    glm::dmat4 viewMatrix      = everstead->GetGameRenderer()->GetViewMatrix(everstead->GetCamera());
    viewMatrix                 = everstead->GetGameRenderer()->GetViewBobbing(localPlayer, partialTicks) * viewMatrix;

    float fontAscent   = font->GetAscent(1.0f);
    float nameTagScale = fontAscent > 0.0f ? nameTagAscentPixels * nameTagPixelSize / fontAscent : nameTagPixelSize;

    glm::dmat4 billboard = glm::dmat4(1.0);
    billboard[0][0]      = viewMatrix[0][0];
    billboard[0][1]      = viewMatrix[1][0];
    billboard[0][2]      = viewMatrix[2][0];
    billboard[1][0]      = viewMatrix[0][1];
    billboard[1][1]      = viewMatrix[1][1];
    billboard[1][2]      = viewMatrix[2][1];
    billboard[2][0]      = viewMatrix[0][2];
    billboard[2][1]      = viewMatrix[1][2];
    billboard[2][2]      = viewMatrix[2][2];

    GlStateManager::MatrixMode(0);
    GlStateManager::LoadIdentity();
    GlStateManager::MultMatrix(viewMatrix);
    GlStateManager::Translated(-cameraPos.x, -cameraPos.y, -cameraPos.z);

    const Entity *renderPlayer = localPlayer ? (const Entity *) localPlayer->GetRenderPlayer() : nullptr;
    PlayerTeam *localTeam      = localPlayer->GetTeam();
    ChatMessageParser parser;
    std::vector<ChatTextSegment> segments;

    for (const std::unique_ptr<Entity> &entity : m_level->GetEntities())
    {
        if (entity.get() == (const Entity *) localPlayer || entity.get() == renderPlayer ||
            (entity->GetSharedFlag(eEntityFlag_Spectator) && !GameType::IsSpectator(localPlayer->GetGameType())) ||
            (entity->GetType() == RemotePlayer::s_type && ((RemotePlayer *) entity.get())->GetDeathTime() >= 20))
        {
            continue;
        }

        glm::dvec3 pos    = entity->GetRenderPosition(partialTicks);
        bool sneaking     = entity->IsSneaking();
        double renderDist = sneaking ? nameTagSneakRenderDistance : nameTagRenderDistance;
        if (glm::distance2(pos, cameraPos) > renderDist * renderDist)
        {
            continue;
        }

        AABB box = entity->GetBoundingBox().Translated(pos);
        if (!m_frustum.TestAABB(box))
        {
            continue;
        }

        const std::wstring &name = entity->GetName();
        if (name.empty())
        {
            continue;
        }

        PlayerTeam *team = entity->GetTeam();
        if (team && localTeam)
        {
            eTEAM_VISIBILITY visibility = team->nameTagVisibility;
            if ((visibility == eTeamVisibility_HideForOtherTeams && team != localTeam) ||
                (visibility == eTeamVisibility_HideForOwnTeam && team == localTeam))
            {
                continue;
            }
        }

        if (team && team->nameTagVisibility == eTeamVisibility_Never)
        {
            continue;
        }

        float height = (float) (box.GetMax().y - box.GetMin().y);
        glm::dvec3 tagPos(pos.x, pos.y + height + nameTagHeadroom - (sneaking ? nameTagSneakDrop : 0.0f), pos.z);
        double tagDist = sqrt(glm::distance2(tagPos, cameraPos));
        float fade     = 1.0f;
        if (GlStateManager::IsFogEnabled())
        {
            float fogStart;
            float fogEnd;
            GlStateManager::GetFogRange(&fogStart, &fogEnd);
            fade = 1.0f - std::clamp(((float) tagDist - fogStart) / (fogEnd - fogStart), 0.0f, 1.0f);
        }

        if (fade <= 0.0f)
        {
            continue;
        }

        // Tags are only dimmed by blocks, testing against the depth buffer would let other entities dim them too
        HitResult *hit = m_level->Clip(cameraPos, tagPos - cameraPos, (float) tagDist, false);
        bool occluded  = hit->IsBlock();
        delete hit;

        uint32_t textColor = sneaking || occluded ? nameTagFaintTextColor : nameTagTextColor;
        uint32_t fillColor = ((uint32_t) (((nameTagFillColor >> 24) & 0xFF) * fade) << 24) | (nameTagFillColor & 0xFFFFFF);
        uint32_t border    = ((uint32_t) (((nameTagBorderColor >> 24) & 0xFF) * fade) << 24) | (nameTagBorderColor & 0xFFFFFF);
        textColor          = ((uint32_t) (((textColor >> 24) & 0xFF) * fade) << 24) | (textColor & 0xFFFFFF);

        GlStateManager::PushMatrix();
        GlStateManager::Translated(tagPos.x, tagPos.y, tagPos.z);
        GlStateManager::MultMatrix(billboard);

        segments.clear();
        parser.Parse(entity->GetDisplayName().dump(), ChatTextStyle(), &segments);
        float width = 0.0f;
        for (size_t i = 0; i < segments.size(); i++)
        {
            width += font->GetWidth(segments[i].text, nameTagScale);
        }

        RenderNameTagPlate(font, width, nameTagScale, nameTagPixelSize, fillColor, border, !sneaking);
        float x = -width * 0.5f;
        for (size_t i = 0; i < segments.size(); i++)
        {
            font->Draw(segments[i].text, x, 0.0f, 0.0f, nameTagScale, (textColor & 0xFF000000) | (segments[i].style.color & 0xFFFFFF));
            x += font->GetWidth(segments[i].text, nameTagScale);
        }
        GlStateManager::PopMatrix();
    }

    GlStateManager::EnableDepthTest();
}

void LevelRenderer::RenderNameTagPlate(Font *font, float width, float scale, float pixelSize, uint32_t fillColor, uint32_t borderColor,
                                       bool throughWalls)
{
    float halfWidth = width * 0.5f;
    float ascent    = font->GetAscent(scale);
    float padX      = pixelSize;
    float padY      = pixelSize;
    float border    = pixelSize;

    float x0 = -halfWidth - padX;
    float x1 = halfWidth + padX;
    float y0 = -padY;
    float y1 = ascent + padY;

    if (throughWalls)
    {
        GlStateManager::DisableDepthTest();
    }
    else
    {
        GlStateManager::EnableDepthTest();
    }

    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::DisableTexture();

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);

    builder->Color(borderColor);
    AppendNameTagQuad(builder, x0 - border, y0 - border, x1 + border, y0);
    AppendNameTagQuad(builder, x0 - border, y1, x1 + border, y1 + border);
    AppendNameTagQuad(builder, x0 - border, y0, x0, y1);
    AppendNameTagQuad(builder, x1, y0, x1 + border, y1);

    builder->Color(fillColor);
    AppendNameTagQuad(builder, x0, y0, x1, y1);

    builder->End();

    GlStateManager::EnableTexture();
    GlStateManager::EnableCull();
}

void LevelRenderer::AppendNameTagQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1)
{
    builder->Vertex(x0, y0, 0.0f);
    builder->Vertex(x1, y0, 0.0f);
    builder->Vertex(x1, y1, 0.0f);
    builder->Vertex(x0, y0, 0.0f);
    builder->Vertex(x1, y1, 0.0f);
    builder->Vertex(x0, y1, 0.0f);
}

uint32_t LevelRenderer::SampleEntityLightColor(const Entity *entity, float partialTicks) const
{
    if (!entity)
    {
        return 0xFFFFFFFF;
    }

    glm::dvec3 pos        = entity->GetRenderPosition(partialTicks);
    AABB box              = entity->GetBoundingBox().Translated(pos);
    const glm::dvec3 &min = box.GetMin();
    const glm::dvec3 &max = box.GetMax();
    glm::dvec3 samplePos((min.x + max.x) * 0.5, min.y + (max.y - min.y) * 0.6, (min.z + max.z) * 0.5);

    return SampleWorldLightColor(samplePos);
}

void LevelRenderer::RenderLeash(const Mob *mob, float partialTicks) const
{
    const int steps = 24;

    Entity *holder = mob->GetLeashHolder();
    if (!holder)
    {
        return;
    }

    glm::dvec3 holderPos = holder->GetRenderPosition(partialTicks);
    if (holder->GetType() == eEntityType_LeashKnot)
    {
        holderPos.y += 0.5 - 0.0625 * 0.7 - 0.25;
    }
    else
    {
        double yaw      = glm::radians((double) holder->GetRenderYaw(partialTicks));
        double pitch    = glm::radians((double) holder->GetRenderPitch(partialTicks));
        double cosYaw   = std::cos(yaw);
        double sinYaw   = std::sin(yaw);
        double cosPitch = std::cos(pitch);
        holderPos.x += -cosYaw * 0.7 - sinYaw * 0.5 * cosPitch;
        holderPos.y += ((const LivingEntity *) holder)->GetEyeHeight() * 0.7 + std::sin(pitch) * 0.5 - 0.25;
        holderPos.z += -sinYaw * 0.7 + cosYaw * 0.5 * cosPitch;
    }

    const AABB &box             = mob->GetBoundingBox();
    double width                = box.GetMax().x - box.GetMin().x;
    double height               = box.GetMax().y - box.GetMin().y;
    double bodyYaw              = glm::radians((double) mob->GetRenderBodyYaw(partialTicks));
    const glm::dvec3 &cameraPos = Everstead::GetInstance()->GetCamera()->GetPosition();
    glm::dvec3 start            = mob->GetRenderPosition(partialTicks);
    glm::dvec3 delta            = holderPos - start;
    start.x += std::cos(bodyYaw) * width * 0.4;
    start.y -= (1.6 - height) * 0.5;
    start.z += std::sin(bodyYaw) * width * 0.4;
    delta.x -= std::cos(bodyYaw) * width * 0.4;
    delta.z -= std::sin(bodyYaw) * width * 0.4;
    start -= cameraPos;

    GlStateManager::DisableTexture();
    GlStateManager::DisableCull();
    GlStateManager::PushMatrix();
    GlStateManager::Translated(cameraPos.x, cameraPos.y, cameraPos.z);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    for (int strip = 0; strip < 2; strip++)
    {
        glm::dvec3 firstOffset  = strip == 0 ? glm::dvec3(0.0) : glm::dvec3(0.0, 0.025, 0.0);
        glm::dvec3 secondOffset = strip == 0 ? glm::dvec3(0.025, 0.025, 0.0) : glm::dvec3(0.025, 0.0, 0.025);
        glm::dvec3 previous[2];
        for (int j = 0; j <= steps; j++)
        {
            double f = (double) j / steps;
            glm::dvec3 point(start.x + delta.x * f, start.y + delta.y * (f * f + f) * 0.5 + (steps - j) / 18.0 + 0.125, start.z + delta.z * f);
            glm::dvec3 a = point + firstOffset;
            glm::dvec3 b = point + secondOffset;
            if (j > 0)
            {
                builder->Color(j % 2 == 0 ? 0xFF80664D : 0xFF5A4736);
                builder->Vertex(previous[0].x, previous[0].y, previous[0].z);
                builder->Vertex(previous[1].x, previous[1].y, previous[1].z);
                builder->Vertex(b.x, b.y, b.z);
                builder->Vertex(previous[0].x, previous[0].y, previous[0].z);
                builder->Vertex(b.x, b.y, b.z);
                builder->Vertex(a.x, a.y, a.z);
            }

            previous[0] = a;
            previous[1] = b;
        }
    }

    builder->End();

    GlStateManager::PopMatrix();
    GlStateManager::EnableCull();
    GlStateManager::EnableTexture();
}

uint32_t LevelRenderer::SampleWorldLightColor(const glm::dvec3 &pos) const
{
    const int offsets[6][3] = {{0, 1, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}};

    glm::dvec3 samplePos = pos;
    BlockPos lightPos((int) floor(pos.x), (int) floor(pos.y), (int) floor(pos.z));
    // Opaque blocks hold no light, so something poking into a wall samples the first open neighbor instead of going black
    Block *block = Block::ById(m_level->GetBlockId(lightPos));
    if (block->IsSolid() && !block->IsTransparent())
    {
        for (int i = 0; i < 6; i++)
        {
            BlockPos neighbor(lightPos.x + offsets[i][0], lightPos.y + offsets[i][1], lightPos.z + offsets[i][2]);
            Block *neighborBlock = Block::ById(m_level->GetBlockId(neighbor));
            if (!neighborBlock->IsSolid() || neighborBlock->IsTransparent())
            {
                lightPos  = neighbor;
                samplePos = glm::dvec3(neighbor.x, neighbor.y, neighbor.z) + 0.5;
                break;
            }
        }
    }

    uint8_t blockR = 0;
    uint8_t blockG = 0;
    uint8_t blockB = 0;
    LightEngine::GetBlockLight(m_level, lightPos, &blockR, &blockG, &blockB);
    m_itemLights.SampleBlockLight(samplePos, &blockR, &blockG, &blockB);
    double vanillaBlockLight = kNimbusEnabled && m_nimbus->IsBlockLighting() ? m_nimbus->GetFloodFade(samplePos) : 1.0;
    blockR                   = (uint8_t) round(blockR * vanillaBlockLight);
    blockG                   = (uint8_t) round(blockG * vanillaBlockLight);
    blockB                   = (uint8_t) round(blockB * vanillaBlockLight);

    uint8_t sky             = m_level->GetSkyLightLevel(lightPos);
    float skyDarken         = LightSource::SampleSkyDarken(m_level->GetWorldProvider(), m_level->GetRainLevel(1.0f), m_level->GetThunderLevel(1.0f));
    glm::dvec3 light        = LightSource::SampleLightColor(blockR, blockG, blockB, sky, skyDarken);
    glm::dvec3 dynamicLight = kNimbusEnabled ? m_nimbus->SampleLight(samplePos, light) : glm::dvec3(0.0);
    float r                 = std::clamp((float) light.x + (float) dynamicLight.x, 0.0f, 1.0f);
    float g                 = std::clamp((float) light.y + (float) dynamicLight.y, 0.0f, 1.0f);
    float b                 = std::clamp((float) light.z + (float) dynamicLight.z, 0.0f, 1.0f);
    uint32_t ir             = (uint32_t) (r * 255.0f + 0.5f);
    uint32_t ig             = (uint32_t) (g * 255.0f + 0.5f);
    uint32_t ib             = (uint32_t) (b * 255.0f + 0.5f);
    return 0xFF000000 | (ir << 16) | (ig << 8) | ib;
}

void LevelRenderer::SetFogUniforms(Shader *shader, bool applyFog)
{
    shader->SetVec3("u_fogColor", m_fogColor.x, m_fogColor.y, m_fogColor.z);
    shader->SetFloat("u_skyFog", m_skyFog);

    const float lavaFogRed     = 0.6f;
    const float lavaFogGreen   = 0.1f;
    const float lavaFogBlue    = 0.0f;
    const float lavaFogDensity = 2.0f;

    glm::dvec3 cameraPosition = Everstead::GetInstance()->GetCamera()->GetPosition();
    bool inLava               = m_level->IsLavaAt(cameraPosition);
    shader->SetInt("u_underwaterFog", applyFog && (inLava || m_level->IsLiquidAt(cameraPosition)));
    if (inLava)
    {
        shader->SetVec3("u_underwaterFogColor", lavaFogRed, lavaFogGreen, lavaFogBlue);
        shader->SetFloat("u_underwaterFogDensity", lavaFogDensity);
    }
    else
    {
        shader->SetVec3("u_underwaterFogColor", s_underwaterFogRed, s_underwaterFogGreen, s_underwaterFogBlue);
        shader->SetFloat("u_underwaterFogDensity", s_underwaterFogDensity);
    }

    shader->SetFloat("u_underwaterFogMinimum", s_underwaterFogMinimum);

    bool fogEnabled = applyFog && GlStateManager::IsFogEnabled();
    shader->SetInt("u_fogEnabled", fogEnabled);
    shader->SetFloat("u_fogStrength", 1.0f);
    shader->SetFloat("u_nimbusFog", fogEnabled && kNimbusEnabled && m_nimbus->HasFog() ? m_nimbus->GetAtmosphere() : 0.0f);
    if (fogEnabled)
    {
        float fogStart;
        float fogEnd;
        GlStateManager::GetFogRange(&fogStart, &fogEnd);
        shader->SetVec2("u_fogRange", fogStart, fogEnd);
    }
}

void LevelRenderer::RenderFogPass(const glm::dmat4 &projection, Framebuffer *sourceFramebuffer, bool applyFog)
{
    GlStateManager::DisableDepthTest();

    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();

    m_fogShader->Bind();

    Textures::Bind(sourceFramebuffer->GetColorTexture(), 0);

    Textures::Bind(sourceFramebuffer->GetDepthTexture(), 1);

    SetFogUniforms(m_fogShader, applyFog);

    Everstead *everstead              = Everstead::GetInstance();
    glm::dmat4 viewMatrix             = everstead->GetGameRenderer()->GetViewMatrix(everstead->GetCamera());
    glm::dmat4 invertedViewProjection = glm::inverse((projection * viewMatrix));
    m_fogShader->SetMat4("u_invViewProj", glm::value_ptr(invertedViewProjection));

    RenderDevice::GetInstance()->DrawArrays(ePrimitiveType_Triangles, 0, 3);

    GlStateManager::EnableCull();
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();
}

void LevelRenderer::RenderNimbusImGui()
{
    m_nimbus->RenderImGui(Everstead::GetInstance()->GetCamera()->GetPosition());
}

const ItemLights &LevelRenderer::GetItemLights() const
{
    return m_itemLights;
}

WorldEffects &LevelRenderer::GetWorldEffects()
{
    return m_worldEffects;
}

void LevelRenderer::UploadPendingMeshes()
{
    LevelChunkMeshingService::UploadPendingMeshes(this);
}

void LevelRenderer::ScheduleMesher()
{
    LevelChunkMeshingService::ScheduleMesher(this);
}

void LevelRenderer::SubmitMesh(const SectionPos &pos, uint64_t generation, uint64_t syncGroup, uint64_t visibility,
                               std::vector<ChunkMesh::PackedData> &&packedResults)
{
    LevelChunkMeshingService::SubmitMesh(this, pos, generation, syncGroup, visibility, std::move(packedResults));
}

void LevelRenderer::RemoveChunkFromPendingSyncGroups(const SectionPos &pos)
{
    LevelChunkMeshingService::RemoveChunkFromPendingSyncGroups(this, pos);
}

void LevelRenderer::SetRegionSection(const SectionPos &pos, std::vector<ChunkMesh::PackedData> &&meshes, bool rebuild)
{
    SectionPos regionPos = ChunkRenderRegion::GetRegionPos(pos);
    auto regionIt        = m_renderRegions.find(regionPos);
    if (regionIt == m_renderRegions.end())
    {
        regionIt        = m_renderRegions.emplace(regionPos, std::make_unique<ChunkRenderRegion>(regionPos, false)).first;
        uint32_t drawId = (uint32_t) m_regionDraws.size();
        if (!m_freeRegionDraws.empty())
        {
            drawId = m_freeRegionDraws.back();
            m_freeRegionDraws.pop_back();
        }
        else
        {
            m_regionDraws.emplace_back();
        }

        m_regionDraws[drawId] = {regionIt->second.get(), regionPos, 0, {}};
        regionIt->second->SetDrawId(drawId);
    }

    regionIt->second->SetSection(pos, std::move(meshes));
    if (rebuild)
    {
        regionIt->second->Rebuild();
        m_dirtyRegions.erase(regionPos);
    }
}

void LevelRenderer::SetSectionVisibility(const SectionPos &pos, uint64_t visibility)
{
    auto columnIt = m_columnVisibility.find(pos.ToChunkPos());
    if (columnIt == m_columnVisibility.end())
    {
        columnIt = m_columnVisibility.emplace(pos.ToChunkPos(), std::array<uint64_t, Chunk::s_sectionCount>()).first;
        columnIt->second.fill(SectionVisibility::s_allVisible);
    }

    m_meshedSections[pos.ToChunkPos()] |= 1 << pos.y;
    m_meshedSectionsVersion++;
    m_nimbus->MarkSectionDirty(pos);

    if (columnIt->second[(size_t) pos.y] != visibility)
    {
        columnIt->second[(size_t) pos.y] = visibility;
        m_occlusionDirty                 = true;
    }
}

void LevelRenderer::UpdateOcclusion(const glm::dvec3 &cameraPos)
{
    Everstead *everstead = Everstead::GetInstance();
    Block *cameraBlock   = Block::ById(m_level->GetBlockId(BlockPos((int) floor(cameraPos.x), (int) floor(cameraPos.y), (int) floor(cameraPos.z))));
    bool cameraInBlock = GameType::IsSpectator(everstead->GetLocalPlayer()->GetGameType()) && cameraBlock->IsSolid() && !cameraBlock->IsTransparent();
    if (!everstead->options.occlusionCulling || everstead->GetGameRenderer()->IsPanoramaCapture() || cameraInBlock)
    {
        m_chunkBatchesDirty |= m_occlusionValid;
        m_occlusionValid = false;
        return;
    }

    if (m_occlusionBusy)
    {
        {
            std::lock_guard<std::mutex> lock(m_occlusionMutex);
            if (!m_occlusionFinished)
            {
                return;
            }

            m_occlusionFinished = false;
        }

        bool changed = !m_occlusionValid || m_visibleSections != m_occlusionResult || m_occlusionOriginX != m_occlusionJobOriginX ||
                       m_occlusionOriginZ != m_occlusionJobOriginZ || m_occlusionRadius != m_occlusionJobRadius;
        m_chunkBatchesDirty |= changed;
        m_visibleSections.swap(m_occlusionResult);
        if (changed)
        {
            RenderDevice *device = RenderDevice::GetInstance();
            device->BindArrayBuffer(m_visibilityBuffer);
            device->UploadArrayBuffer(m_visibleSections.data(), (uint32_t) m_visibleSections.size(), eBufferUsage_Static);
        }

        m_occlusionOriginX = m_occlusionJobOriginX;
        m_occlusionOriginY = m_occlusionJobOriginY;
        m_occlusionOriginZ = m_occlusionJobOriginZ;
        m_occlusionRadius  = m_occlusionJobRadius;
        m_occlusionValid   = true;
        m_occlusionBusy    = false;
    }

    int originX  = Mth::FloorDiv((int) floor(cameraPos.x), Chunk::s_sizeX);
    int originZ  = Mth::FloorDiv((int) floor(cameraPos.z), Chunk::s_sizeZ);
    int cameraY  = Mth::FloorDiv((int) floor(cameraPos.y), ChunkSection::s_size);
    int originY  = std::clamp(cameraY, 0, Chunk::s_sectionCount - 1);
    int radius   = m_level->GetRenderDistance() + 1;
    uint32_t now = Time::GetMilliseconds();
    bool moved   = !m_occlusionValid || originX != m_occlusionOriginX || originY != m_occlusionOriginY || originZ != m_occlusionOriginZ ||
                   radius != m_occlusionRadius;
    if (!moved && (!m_occlusionDirty || now - m_lastOcclusionUpdateMs < s_occlusionUpdateIntervalMs))
    {
        return;
    }

    m_occlusionDirty        = false;
    m_lastOcclusionUpdateMs = now;

    int width = radius * 2 + 1;
    m_occlusionColumns.assign((size_t) (width * width * Chunk::s_sectionCount), SectionVisibility::s_allVisible);
    for (auto columnIt = m_columnVisibility.begin(); columnIt != m_columnVisibility.end(); ++columnIt)
    {
        int columnX = columnIt->first.x - originX;
        int columnZ = columnIt->first.z - originZ;
        if (columnX >= -radius && columnX <= radius && columnZ >= -radius && columnZ <= radius)
        {
            std::copy(columnIt->second.begin(), columnIt->second.end(),
                      m_occlusionColumns.begin() + (ptrdiff_t) (((columnX + radius) + (columnZ + radius) * width) * Chunk::s_sectionCount));
        }
    }

    m_occlusionJobOriginX = originX;
    m_occlusionJobOriginY = originY;
    m_occlusionJobOriginZ = originZ;
    m_occlusionJobCameraY = cameraY;
    m_occlusionJobRadius  = radius;
    m_occlusionBusy       = true;
    {
        std::lock_guard<std::mutex> lock(m_occlusionMutex);
        m_occlusionRequested = true;
    }

    m_occlusionCondition.notify_one();
}

// The worker only touches the job fields, m_occlusionColumns and m_occlusionResult while m_occlusionBusy is set
void LevelRenderer::RunOcclusionWorker()
{
    while (true)
    {
        {
            std::unique_lock<std::mutex> lock(m_occlusionMutex);
            m_occlusionCondition.wait(lock,
                                      [this]
                                      {
                                          return m_occlusionRequested || m_occlusionStop;
                                      });
            if (m_occlusionStop)
            {
                return;
            }

            m_occlusionRequested = false;
        }

        int originX = m_occlusionJobOriginX;
        int originY = m_occlusionJobOriginY;
        int originZ = m_occlusionJobOriginZ;
        int radius  = m_occlusionJobRadius;
        int width   = radius * 2 + 1;
        m_occlusionResult.assign((size_t) (width * width * Chunk::s_sectionCount), 0);
        m_occlusionQueue.clear();
        if (m_occlusionJobCameraY == originY)
        {
            m_occlusionQueue.push_back({originX, originY, originZ});
            m_occlusionResult[(size_t) (((radius + (radius * width)) * Chunk::s_sectionCount) + originY)] = s_occlusionStartMask;
        }
        else
        {
            // Outside the world height every section on the near layer faces the camera directly
            uint8_t entryFace = (uint8_t) (m_occlusionJobCameraY < originY ? 1 << 2 : 1 << 3);
            for (int dz = -radius; dz <= radius; dz++)
            {
                for (int dx = -radius; dx <= radius; dx++)
                {
                    if (dx * dx + dz * dz > radius * radius)
                    {
                        continue;
                    }

                    m_occlusionQueue.push_back({originX + dx, originY, originZ + dz});
                    m_occlusionResult[(size_t) ((((dx + radius) + (dz + radius) * width) * Chunk::s_sectionCount) + originY)] = entryFace;
                }
            }
        }

        for (size_t head = 0; head < m_occlusionQueue.size(); head++)
        {
            OcclusionNode node  = m_occlusionQueue[head];
            int nodeX           = node.x - originX;
            int nodeZ           = node.z - originZ;
            size_t nodeIndex    = (size_t) ((((nodeX + radius) + (nodeZ + radius) * width) * Chunk::s_sectionCount) + node.y);
            uint64_t visibility = m_occlusionColumns[nodeIndex];
            uint8_t incoming    = m_occlusionResult[nodeIndex];

            int outgoing = incoming == s_occlusionStartMask ? s_occlusionAllFaces : 0;
            for (int entry = 0; entry < SectionVisibility::s_faceCount && incoming != s_occlusionStartMask; entry++)
            {
                if ((incoming & (1 << entry)) != 0)
                {
                    outgoing |= (int) ((visibility >> (entry * SectionVisibility::s_faceMaskBits)) & SectionVisibility::s_faceMask);
                }
            }

            int nodeY = node.y - originY;
            outgoing &= (nodeX <= 0 ? 1 << 0 : 0) | (nodeX >= 0 ? 1 << 1 : 0) | (nodeY <= 0 ? 1 << 2 : 0) | (nodeY >= 0 ? 1 << 3 : 0) |
                        (nodeZ <= 0 ? 1 << 4 : 0) | (nodeZ >= 0 ? 1 << 5 : 0);
            for (int face = 0; face < SectionVisibility::s_faceCount; face++)
            {
                if ((outgoing & (1 << face)) == 0)
                {
                    continue;
                }

                int x  = node.x + SectionVisibility::s_faceOffsets[face][0];
                int y  = node.y + SectionVisibility::s_faceOffsets[face][1];
                int z  = node.z + SectionVisibility::s_faceOffsets[face][2];
                int dx = x - originX;
                int dz = z - originZ;
                if (y < 0 || y >= Chunk::s_sectionCount || dx * dx + dz * dz > radius * radius)
                {
                    continue;
                }

                size_t index = (size_t) ((((dx + radius) + (dz + radius) * width) * Chunk::s_sectionCount) + y);
                if (m_occlusionResult[index] == 0)
                {
                    m_occlusionQueue.push_back({x, y, z});
                }

                m_occlusionResult[index] |= (uint8_t) (1 << (face ^ 1));
            }
        }

        std::lock_guard<std::mutex> lock(m_occlusionMutex);
        m_occlusionFinished = true;
    }
}

bool LevelRenderer::IsSectionVisible(const SectionPos &pos) const
{
    int dx = pos.x - m_occlusionOriginX;
    int dz = pos.z - m_occlusionOriginZ;
    if (!m_occlusionValid || dx < -m_occlusionRadius || dx > m_occlusionRadius || dz < -m_occlusionRadius || dz > m_occlusionRadius || pos.y < 0 ||
        pos.y >= Chunk::s_sectionCount)
    {
        return true;
    }

    int width = m_occlusionRadius * 2 + 1;
    return m_visibleSections[(size_t) ((((dx + m_occlusionRadius) + (dz + m_occlusionRadius) * width) * Chunk::s_sectionCount) + pos.y)] != 0;
}

bool LevelRenderer::IsRegionVisible(const SectionPos &regionPos) const
{
    for (int x = 0; x < ChunkRenderRegion::s_size; x++)
    {
        for (int z = 0; z < ChunkRenderRegion::s_size; z++)
        {
            if (IsSectionVisible(SectionPos(regionPos.x * ChunkRenderRegion::s_size + x, regionPos.y, regionPos.z * ChunkRenderRegion::s_size + z)))
            {
                return true;
            }
        }
    }

    return false;
}

void LevelRenderer::RemoveRegionSection(const SectionPos &pos)
{
    SectionPos regionPos = ChunkRenderRegion::GetRegionPos(pos);
    auto regionIt        = m_renderRegions.find(regionPos);
    if (regionIt == m_renderRegions.end())
    {
        return;
    }

    if (!regionIt->second->HasSection(pos))
    {
        return;
    }

    if (m_dirtyRegions.find(regionPos) == m_dirtyRegions.end() && regionIt->second->ClearSection(pos))
    {
        return;
    }

    regionIt->second->RemoveSection(pos);
    if (regionIt->second->IsEmpty())
    {
        RegionDraw &draw = m_regionDraws[regionIt->second->GetDrawId()];
        for (size_t i = 0; i < draw.slots.size(); i++)
        {
            FreeMeshSlot(draw.slots[i]);
        }

        draw = {nullptr, regionPos, 0, {}};
        m_freeRegionDraws.push_back(regionIt->second->GetDrawId());
        m_renderRegions.erase(regionIt);
        m_dirtyRegions.erase(regionPos);
        return;
    }

    m_dirtyRegions.insert(regionPos);
}

void LevelRenderer::RebuildDirtyRegions()
{
    int budget = kDirtyRegionRebuildsPerFrame;
    while (!m_dirtyRegions.empty() && budget > 0)
    {
        auto dirtyIt         = m_dirtyRegions.begin();
        SectionPos regionPos = *dirtyIt;
        m_dirtyRegions.erase(dirtyIt);

        auto regionIt = m_renderRegions.find(regionPos);
        if (regionIt == m_renderRegions.end())
        {
            continue;
        }

        regionIt->second->Rebuild();
        budget--;
    }
}

bool LevelRenderer::HasRegionSection(const SectionPos &pos) const
{
    SectionPos regionPos = ChunkRenderRegion::GetRegionPos(pos);
    auto regionIt        = m_renderRegions.find(regionPos);
    return regionIt != m_renderRegions.end() && regionIt->second->HasSection(pos);
}

void LevelRenderer::SetTranslucentSection(const SectionPos &pos, std::vector<ChunkMesh::PackedData> &&meshes)
{
    SectionPos regionPos = ChunkRenderRegion::GetRegionPos(pos);
    auto regionIt        = m_translucentRegions.find(regionPos);
    if (regionIt == m_translucentRegions.end())
    {
        regionIt = m_translucentRegions.emplace(regionPos, std::make_unique<ChunkRenderRegion>(regionPos, true)).first;
    }

    regionIt->second->UpdateSection(pos, meshes);
    regionIt->second->Rebuild();
}

void LevelRenderer::RemoveTranslucentSection(const SectionPos &pos)
{
    SectionPos regionPos = ChunkRenderRegion::GetRegionPos(pos);
    auto regionIt        = m_translucentRegions.find(regionPos);
    if (regionIt != m_translucentRegions.end() && !regionIt->second->ClearSection(pos))
    {
        m_translucentRegions.erase(regionIt);
    }
}

bool LevelRenderer::HasTranslucentSection(const SectionPos &pos) const
{
    SectionPos regionPos = ChunkRenderRegion::GetRegionPos(pos);
    auto regionIt        = m_translucentRegions.find(regionPos);
    return regionIt != m_translucentRegions.end() && regionIt->second->HasSection(pos);
}

std::vector<float> LevelRenderer::BuildStarVertices()
{
    std::vector<float> vertices = MemoryTracker::GetInstance()->CreateFloatBuffer(1500 * 6 * 3);
    vertices.clear();
    vertices.reserve(1500 * 6 * 3);

    Random random(10842);

    for (int i = 0; i < 1500; i++)
    {
        double x  = random.NextFloat() * 2.0 - 1.0;
        double y  = random.NextFloat() * 2.0 - 1.0;
        double z  = random.NextFloat() * 2.0 - 1.0;
        double ss = 0.15 + random.NextFloat() * 0.10;
        double d  = x * x + y * y + z * z;

        if (d <= 0.01 || d >= 1.0)
        {
            continue;
        }

        d = 1.0 / sqrt(d);
        x *= d;
        y *= d;
        z *= d;

        double xp = x * 160.0;
        double yp = y * 160.0;
        double zp = z * 160.0;

        double yRot = atan2(x, z);
        double ySin = sin(yRot);
        double yCos = cos(yRot);

        double xRot = atan2(sqrt(x * x + z * z), y);
        double xSin = sin(xRot);
        double xCos = cos(xRot);

        double zRot = random.NextDouble() * 3.14159265358979323846 * 2.0;
        double zSin = sin(zRot);
        double zCos = cos(zRot);

        double quad[4][3];

        for (int c = 0; c < 4; c++)
        {
            double ___xo = 0.0;
            double ___yo = (double) ((c & 2) - 1) * ss;
            double ___zo = (double) (((c + 1) & 2) - 1) * ss;

            double __xo = ___xo;
            double __yo = ___yo * zCos - ___zo * zSin;
            double __zo = ___zo * zCos + ___yo * zSin;

            double _zo = __zo;
            double _yo = __yo * xSin + __xo * xCos;
            double _xo = __xo * xSin - __yo * xCos;

            quad[c][0] = xp + (_xo * ySin - _zo * yCos);
            quad[c][1] = yp + _yo;
            quad[c][2] = zp + (_zo * ySin + _xo * yCos);
        }

        PushSkyQuad(&vertices, quad[0][0], quad[0][1], quad[0][2], quad[1][0], quad[1][1], quad[1][2], quad[2][0], quad[2][1], quad[2][2], quad[3][0],
                    quad[3][1], quad[3][2]);
    }

    return vertices;
}

void LevelRenderer::SetupMatrices(const glm::dvec3 &cameraPos)
{
    GlStateManager::MatrixMode(0);
    GlStateManager::LoadIdentity();
    GlStateManager::Translated(-cameraPos.x, -cameraPos.y, -cameraPos.z);
}

void LevelRenderer::SetupFog(const glm::dvec3 &eyePos, float partialTicks)
{
    const int biomeSampleRadius   = 3;
    const int biomeSampleSpacing  = 8;
    const float biomeFogFadeSpeed = 0.5f;

    float biomeColors[6] = {};
    int biomeSamples     = 0;
    int rainySamples     = 0;
    int snowySamples     = 0;
    for (int dz = -biomeSampleRadius; dz <= biomeSampleRadius; dz++)
    {
        for (int dx = -biomeSampleRadius; dx <= biomeSampleRadius; dx++)
        {
            int x              = (int) floor(eyePos.x) + dx * biomeSampleSpacing;
            int z              = (int) floor(eyePos.z) + dz * biomeSampleSpacing;
            const Chunk *chunk = ((const Level *) m_level)->GetChunk(ChunkPos(Mth::FloorDiv(x, Chunk::s_sizeX), Mth::FloorDiv(z, Chunk::s_sizeZ)));
            if (!chunk)
            {
                continue;
            }

            Biome *biome = chunk->GetSurfaceBiome(Mth::FloorMod(x, Chunk::s_sizeX), Mth::FloorMod(z, Chunk::s_sizeZ));
            if (!biome)
            {
                continue;
            }

            if (biome->IsSnowyAt((int) floor(eyePos.y)))
            {
                snowySamples++;
            }
            else if (biome->HasRain())
            {
                rainySamples++;
            }

            uint32_t sky   = biome->GetSkyColor();
            uint32_t color = biome->GetFogColor();
            for (int channel = 0; channel < 3; channel++)
            {
                biomeColors[channel] += (float) ((sky >> (16 - channel * 8)) & 0xFF) / 255.0f;
                biomeColors[channel + 3] += (float) ((color >> (16 - channel * 8)) & 0xFF) / 255.0f;
            }

            biomeSamples++;
        }
    }

    if (biomeSamples > 0)
    {
        Fog &biomeFog = m_level->GetWorldProvider().GetFog();
        glm::dvec3 skyTarget(biomeColors[0] / biomeSamples, biomeColors[1] / biomeSamples, biomeColors[2] / biomeSamples);
        glm::dvec3 fogTarget(biomeColors[3] / biomeSamples, biomeColors[4] / biomeSamples, biomeColors[5] / biomeSamples);
        double fade = m_biomeFogReady ? std::min(Time::GetDelta() * biomeFogFadeSpeed, 1.0) : 1.0;
        biomeFog.SetSkyColor(biomeFog.GetSkyColor() + (skyTarget - biomeFog.GetSkyColor()) * fade);
        biomeFog.SetColor(biomeFog.GetColor() + (fogTarget - biomeFog.GetColor()) * fade);
        m_weatherFog += ((float) (rainySamples + snowySamples) / biomeSamples - m_weatherFog) * (float) fade;
        m_snowFog += ((float) snowySamples / biomeSamples - m_snowFog) * (float) fade;
        m_biomeFogReady = true;
    }

    m_fogDarken = 1.0f;
    m_skyFog    = 0.0f;
    ApplyClearColorScale((float) eyePos.y, &m_fogDarken, &m_fogDarken, &m_fogDarken);

    BlockPos eyeBlock((int) floor(eyePos.x), (int) floor(eyePos.y), (int) floor(eyePos.z));
    const float skyLightFadeSpeed = 2.0f;

    float skyLight = (float) m_level->GetSkyLightLevel(eyeBlock);
    if (m_voidFogSkyLight < 0.0f)
    {
        m_voidFogSkyLight = skyLight;
    }

    m_voidFogSkyLight += (skyLight - m_voidFogSkyLight) * std::min((float) Time::GetDelta() * skyLightFadeSpeed, 1.0f);

    // Unlit caves render near black, so fog fades to black without sky light or it brightens far cave walls
    SampleFogColor(m_level, &m_fogColor.x, &m_fogColor.y, &m_fogColor.z);
    m_fogColor *= m_fogDarken * m_voidFogSkyLight / 15.0f;

    float weather = m_level->GetRainLevel(partialTicks) * m_voidFogSkyLight / 15.0f;
    float rain    = weather * (m_weatherFog - m_snowFog);
    float snow    = weather * m_snowFog;
    float thunder = m_level->GetThunderLevel(partialTicks) * weather * m_weatherFog;
    float grey    = (m_fogColor.x + m_fogColor.y + m_fogColor.z) / 3.0f;
    m_fogColor    = glm::mix(m_fogColor, glm::vec3(grey * 0.92f, grey * 0.97f, grey * 1.08f), rain * 0.4f);
    m_fogColor    = glm::mix(m_fogColor, glm::min(glm::vec3(grey * 1.15f), glm::vec3(1.0f)), snow * 0.4f);

    Fog &fog             = m_level->GetWorldProvider().GetFog();
    Everstead *everstead = Everstead::GetInstance();
    if (!fog.IsEnabled())
    {
        GlStateManager::DisableFog();
        return;
    }

    GlStateManager::EnableFog();
    GlStateManager::SetFogColor(m_fogColor.x, m_fogColor.y, m_fogColor.z);

    int fogChunks =
            everstead->options.lodEnabled ? std::max(m_level->GetRenderDistance(), everstead->options.lodDistance) : m_level->GetRenderDistance();
    float renderEnd = (float) (fogChunks * Chunk::s_sizeX) * everstead->options.fogDistance / 100.0f;
    float fogStart  = renderEnd * fog.GetStartFactor();
    float fogEnd    = renderEnd * fog.GetEndFactor();
    fogStart *= 1.0f - (rain + snow) * 0.6f - thunder * 0.2f;
    fogEnd *= 1.0f - (rain + snow) * 0.3f - thunder * 0.2f;
    double voidFog = m_voidFogSkyLight / 16.0 + (eyePos.y + 4.0) / 32.0;
    if (voidFog < 1.0)
    {
        float fade       = (float) std::max(voidFog, 0.0);
        float fadeSq     = fade * fade;
        float voidFogEnd = std::max(fadeSq * std::lerp(100.0f, fogEnd, fadeSq), 5.0f);
        if (voidFogEnd < fogEnd)
        {
            // Chunks past the void fog end get culled, so the sky behind them has to be fully fogged well before the range bottoms out
            m_skyFog = std::min((1.0f - voidFogEnd / fogEnd) * 4.0f, 1.0f);
            fogStart = voidFogEnd * std::lerp(0.25f, fogStart / fogEnd, fade);
            fogEnd   = voidFogEnd;
        }
    }

    if (fogEnd < fogStart + 0.001f)
    {
        fogEnd = fogStart + 0.001f;
    }

    GlStateManager::SetFogRange(fogStart, fogEnd);
}

void LevelRenderer::RenderSky(const glm::dmat4 &viewMatrix, const glm::dmat4 &projection)
{
    Tesselator *renderer   = Tesselator::GetInstance();
    BufferBuilder *builder = renderer->GetBuilder();
    glm::dmat4 viewRotOnly = viewMatrix;
    viewRotOnly[3][0]      = 0.0f;
    viewRotOnly[3][1]      = 0.0f;
    viewRotOnly[3][2]      = 0.0f;

    float sr;
    float sg;
    float sb;
    SampleSkyColor(m_level, &sr, &sg, &sb);

    float sunriseColor[4];
    bool hasSunrise = GetSunriseColor(m_level, sunriseColor);

    float celestialAngle = m_level->GetCelestialAngle();
    float rainBrightness = 1.0f - m_level->GetRainLevel(1.0f);

    TextureRepository *textures = TextureRepository::GetInstance();
    Texture *sunTexture         = textures ? textures->GetRaw("assets/texture/terrain/sun.png") : nullptr;
    Texture *moonTexture        = textures ? textures->GetRaw("assets/texture/terrain/moon_phases.png") : nullptr;

    bool fogEnabled = GlStateManager::IsFogEnabled();
    float fogStart  = 0.0f;
    float fogEnd    = 0.0f;
    if (fogEnabled)
    {
        GlStateManager::GetFogRange(&fogStart, &fogEnd);
    }

    float skyFogEnd = std::min((float) (m_level->GetRenderDistance() * Chunk::s_sizeX) * 0.8f, fogEnd);
    // The plane edge has to sit well past the fog end or it shows when looking diagonally
    float skyPlaneRadius = std::max(384.0f, skyFogEnd * 2.0f);
    GlStateManager::MatrixMode(1);
    GlStateManager::LoadIdentity();
    GlStateManager::MultMatrix(projection);

    GlStateManager::MatrixMode(0);
    GlStateManager::LoadIdentity();
    GlStateManager::MultMatrix(viewRotOnly);

    GlStateManager::SetAlphaCutoff(-1.0f);
    Textures::Unbind();

    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::EnableCull();
    GlStateManager::SetFrontFace(eFrontFace_Ccw);
    GlStateManager::SetCullFace(eCullFace_Back);
    GlStateManager::DisableBlend();
    if (fogEnabled)
    {
        GlStateManager::SetFogRange(0.0f, skyFogEnd);
        GlStateManager::EnableFog();
    }

    Textures::Unbind();
    builder->Begin(ePrimitiveType_Triangles);
    builder->Color(sr, sg, sb, 1.0f);
    EmitSkyPlane(builder, 16.0f, false, skyPlaneRadius);
    builder->End();

    if (fogEnabled)
    {
        GlStateManager::SetFogRange(fogStart, fogEnd);
        GlStateManager::DisableFog();
    }

    if (hasSunrise)
    {
        GlStateManager::EnableBlend();
        GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
        GlStateManager::DisableCull();

        GlStateManager::PushMatrix();
        GlStateManager::Rotatef(90.0f, 1.0f, 0.0f, 0.0f);
        if (sinf(celestialAngle * 6.28318530718f) < 0.0f)
        {
            GlStateManager::Rotatef(180.0f, 0.0f, 0.0f, 1.0f);
        }

        GlStateManager::Rotatef(90.0f, 0.0f, 0.0f, 1.0f);

        Textures::Unbind();
        builder->Begin(ePrimitiveType_TriangleFan);
        builder->Color(sunriseColor[0], sunriseColor[1], sunriseColor[2], sunriseColor[3]);
        builder->Vertex(0.0f, 100.0f, 0.0f);
        builder->Color(sunriseColor[0], sunriseColor[1], sunriseColor[2], 0.0f);

        int steps = 16;
        for (int i = 0; i <= steps; i++)
        {
            float angle = (float) i * 6.28318530718f / (float) steps;
            float x     = sinf(angle);
            float z     = cosf(angle);
            builder->Vertex(x * 120.0f, z * 120.0f, -z * 40.0f * sunriseColor[3]);
        }

        builder->End();
        GlStateManager::PopMatrix();
        GlStateManager::EnableCull();
        GlStateManager::SetFrontFace(eFrontFace_Ccw);
        GlStateManager::SetCullFace(eCullFace_Back);
    }

    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_One);

    GlStateManager::PushMatrix();
    GlStateManager::Rotatef(-90.0f, 0.0f, 1.0f, 0.0f);
    GlStateManager::Rotatef(celestialAngle * 360.0f, 1.0f, 0.0f, 0.0f);

    if (sunTexture)
    {
        Textures::Bind(sunTexture->GetId());
        builder->Begin(ePrimitiveType_Triangles);
        builder->Color(1.0f, 1.0f, 1.0f, rainBrightness);
        EmitQuad(builder, -30.0f, 100.0f, -30.0f, 0.0f, 0.0f, 30.0f, 100.0f, -30.0f, 1.0f, 0.0f, 30.0f, 100.0f, 30.0f, 1.0f, 1.0f, -30.0f, 100.0f,
                 30.0f, 0.0f, 1.0f);
        builder->End();
    }

    if (moonTexture)
    {
        int phase = GetMoonPhase(m_level);
        int u     = phase % 4;
        int v     = (phase / 4) % 2;
        float u0  = (float) u / 4.0f;
        float v0  = (float) v / 2.0f;
        float u1  = (float) (u + 1) / 4.0f;
        float v1  = (float) (v + 1) / 2.0f;

        Textures::Bind(moonTexture->GetId());
        builder->Begin(ePrimitiveType_Triangles);
        builder->Color(1.0f, 1.0f, 1.0f, rainBrightness);
        EmitQuad(builder, -20.0f, -100.0f, 20.0f, u1, v1, 20.0f, -100.0f, 20.0f, u0, v1, 20.0f, -100.0f, -20.0f, u0, v0, -20.0f, -100.0f, -20.0f, u1,
                 v0);
        builder->End();
    }

    GlStateManager::PopMatrix();

    RenderStars(viewMatrix, projection);

    Textures::Unbind();
    GlStateManager::DisableBlend();

    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    if (fogEnabled)
    {
        GlStateManager::SetFogRange(0.0f, skyFogEnd);
        GlStateManager::EnableFog();
    }

    double belowHorizon = Everstead::GetInstance()->GetCamera()->GetPosition().y - kSkyHorizonHeight;
    if (belowHorizon < 0.0)
    {
        GlStateManager::PushMatrix();
        GlStateManager::Translatef(0.0f, 12.0f, 0.0f);
        builder->Begin(ePrimitiveType_Triangles);
        builder->Color(0.0f, 0.0f, 0.0f, 1.0f);
        EmitSkyPlane(builder, -16.0f, true, skyPlaneRadius);
        builder->End();
        GlStateManager::PopMatrix();

        float bottom = -(float) (belowHorizon + 65.0);
        GlStateManager::DisableCull();
        builder->Begin(ePrimitiveType_Triangles);
        builder->Color(0.0f, 0.0f, 0.0f, 1.0f);
        EmitQuad(builder, -1.0f, bottom, 1.0f, 0.0f, 0.0f, 1.0f, bottom, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, -1.0f, 1.0f, 0.0f,
                 0.0f);
        EmitQuad(builder, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, bottom, -1.0f, 0.0f, 0.0f, -1.0f, bottom, -1.0f,
                 0.0f, 0.0f);
        EmitQuad(builder, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, bottom, 1.0f, 0.0f, 0.0f, 1.0f, bottom, -1.0f, 0.0f,
                 0.0f);
        EmitQuad(builder, -1.0f, bottom, -1.0f, 0.0f, 0.0f, -1.0f, bottom, 1.0f, 0.0f, 0.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, -1.0f, -1.0f,
                 0.0f, 0.0f);
        EmitQuad(builder, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f,
                 0.0f);
        builder->End();
        GlStateManager::EnableCull();
    }

    GlStateManager::PushMatrix();
    GlStateManager::Translatef(0.0f, -(float) (belowHorizon - 16.0), 0.0f);
    builder->Begin(ePrimitiveType_Triangles);
    builder->Color(sr * 0.2f + 0.04f, sg * 0.2f + 0.04f, sb * 0.6f + 0.1f, 1.0f);
    EmitSkyPlane(builder, -16.0f, true, skyPlaneRadius);
    builder->End();
    GlStateManager::PopMatrix();

    // We draw this behind the terrain so it only hazes the sky, nearby terrain gets the weather fog instead
    RenderWeatherHaze(skyFogEnd, builder);

    GlStateManager::MatrixMode(1);
    GlStateManager::LoadIdentity();
    GlStateManager::MultMatrix(projection);

    GlStateManager::MatrixMode(0);
    GlStateManager::LoadIdentity();
    GlStateManager::MultMatrix(viewMatrix);

    GlStateManager::SetDepthMask(true);
    GlStateManager::SetFrontFace(eFrontFace_Ccw);
    GlStateManager::SetCullFace(eCullFace_Back);
    GlStateManager::EnableDepthTest();

    if (fogEnabled)
    {
        GlStateManager::SetFogRange(fogStart, fogEnd);
    }
}

void LevelRenderer::RebuildCloudMesh(int radius, bool noCullMode)
{
    const int cellSize         = 12;
    const int cloudCellCount   = 8;
    const float cloudThickness = 4.0f;
    const float faceOffset     = (1.0f / 1024.0f) * (float) cellSize;

    m_cloudVertices.clear();
    m_cloudSections.clear();
    for (int xPos = -radius + 1; xPos <= radius; xPos++)
    {
        for (int zPos = -radius + 1; zPos <= radius; zPos++)
        {
            CloudSection section;
            section.x           = xPos;
            section.z           = zPos;
            section.firstVertex = (int) (m_cloudVertices.size() / 9);

            bool drawBottom = true;
            bool drawTop    = true;
            bool drawNegX   = true;
            bool drawPosX   = true;
            bool drawNegZ   = true;
            bool drawPosZ   = true;
            if (noCullMode)
            {
                drawBottom = true;
                drawTop    = true;
                drawNegX   = xPos > -1;
                drawPosX   = xPos <= 1;
                drawNegZ   = zPos > -1;
                drawPosZ   = zPos <= 1;
            }

            for (int zt = 0; zt < cloudCellCount; zt++)
            {
                for (int xt = 0; xt < cloudCellCount; xt++)
                {
                    float x0 = (float) (xPos * cloudCellCount + xt) * cellSize;
                    float x1 = x0 + cellSize;
                    float z0 = (float) (zPos * cloudCellCount + zt) * cellSize;
                    float z1 = z0 + cellSize;
                    float u  = ((float) (xPos * cloudCellCount + xt) + 0.5f) / 256.0f;
                    float v  = ((float) (zPos * cloudCellCount + zt) + 0.5f) / 256.0f;

                    if (drawBottom)
                    {
                        PushCloudQuad(&m_cloudVertices, x0, 0.0f, z0, x1, 0.0f, z0, x1, 0.0f, z1, x0, 0.0f, z1, u, v, u, v, u, v, u, v, 0.7f, 0.7f,
                                      0.7f, 0.8f);
                    }

                    if (drawTop)
                    {
                        PushCloudQuad(&m_cloudVertices, x0, cloudThickness, z1, x1, cloudThickness, z1, x1, cloudThickness, z0, x0, cloudThickness,
                                      z0, u, v, u, v, u, v, u, v, 1.0f, 1.0f, 1.0f, 0.8f);
                    }

                    if (drawNegX)
                    {
                        float faceX = x0;
                        if (noCullMode && drawBottom && drawTop && drawNegX && drawPosX)
                        {
                            faceX += faceOffset;
                        }

                        PushCloudQuad(&m_cloudVertices, faceX, 0.0f, z1, faceX, cloudThickness, z1, faceX, cloudThickness, z0, faceX, 0.0f, z0, u, v,
                                      u, v, u, v, u, v, 0.9f, 0.9f, 0.9f, 0.8f);
                    }

                    if (drawPosX)
                    {
                        PushCloudQuad(&m_cloudVertices, x1, 0.0f, z0, x1, cloudThickness, z0, x1, cloudThickness, z1, x1, 0.0f, z1, u, v, u, v, u, v,
                                      u, v, 0.9f, 0.9f, 0.9f, 0.8f);
                    }

                    if (drawNegZ)
                    {
                        float faceZ = z0;
                        if (noCullMode && drawBottom && drawTop && drawNegZ && drawPosZ)
                        {
                            faceZ += faceOffset;
                        }

                        PushCloudQuad(&m_cloudVertices, x0, cloudThickness, faceZ, x1, cloudThickness, faceZ, x1, 0.0f, faceZ, x0, 0.0f, faceZ, u, v,
                                      u, v, u, v, u, v, 0.8f, 0.8f, 0.8f, 0.8f);
                    }

                    if (drawPosZ)
                    {
                        PushCloudQuad(&m_cloudVertices, x0, 0.0f, z1, x1, 0.0f, z1, x1, cloudThickness, z1, x0, cloudThickness, z1, u, v, u, v, u, v,
                                      u, v, 0.8f, 0.8f, 0.8f, 0.8f);
                    }
                }
            }

            section.vertexCount = (int) (m_cloudVertices.size() / 9) - section.firstVertex;
            m_cloudSections.push_back(section);
        }
    }

    m_cloudMesh->Upload(m_cloudVertices);
    m_cloudRadius     = radius;
    m_cloudNoCullMode = noCullMode;
}

void LevelRenderer::RenderClouds(const glm::dmat4 &viewMatrix, const glm::dmat4 &projection, float partialTicks, Framebuffer *outputFramebuffer)
{
    Everstead *everstead = Everstead::GetInstance();
    if (!everstead->options.renderClouds || m_level->GetDimensionId() == eDimensionId_Caves)
    {
        return;
    }

    const glm::dvec3 cameraPos = everstead->GetCamera()->GetPosition();
    int dayStep                = (int) (m_level->GetWorldProvider().GetWorldTime() % (int64_t) WorldProvider::s_dayTicks);
    float timeOfDay            = ((float) dayStep + partialTicks) / (float) WorldProvider::s_dayTicks - 0.25f;
    if (timeOfDay < 0.0f)
    {
        timeOfDay += 1.0f;
    }

    if (timeOfDay > 1.0f)
    {
        timeOfDay -= 1.0f;
    }

    float originalTimeOfDay = timeOfDay;
    timeOfDay               = 1.0f - (float) ((cos((double) timeOfDay * std::numbers::pi) + 1.0) / 2.0);
    timeOfDay               = originalTimeOfDay + (timeOfDay - originalTimeOfDay) / 3.0f;
    float cloudBrightness   = std::clamp(cosf(timeOfDay * std::numbers::pi_v<float> * 2.0f) * 2.0f + 0.5f, 0.0f, 1.0f);
    float cloudR            = cloudBrightness * 0.90f + 0.10f;
    float cloudG            = cloudBrightness * 0.90f + 0.10f;
    float cloudB            = cloudBrightness * 0.85f + 0.15f;

    float rain = m_level->GetRainLevel(partialTicks);
    if (rain > 0.0f)
    {
        float lum = (cloudR * 0.3f + cloudG * 0.59f + cloudB * 0.11f) * 0.6f;
        float mix = rain * 0.95f;
        cloudR    = cloudR * (1.0f - mix) + lum * mix;
        cloudG    = cloudG * (1.0f - mix) + lum * mix;
        cloudB    = cloudB * (1.0f - mix) + lum * mix;
    }

    float thunder = m_level->GetThunderLevel(partialTicks);
    if (thunder > 0.0f)
    {
        float lum = (cloudR * 0.3f + cloudG * 0.59f + cloudB * 0.11f) * 0.2f;
        float mix = thunder * 0.95f;
        cloudR    = cloudR * (1.0f - mix) + lum * mix;
        cloudG    = cloudG * (1.0f - mix) + lum * mix;
        cloudB    = cloudB * (1.0f - mix) + lum * mix;
    }

    m_cloudOffset += (float) Time::GetDelta() * 0.6f;
    if (m_cloudOffset > 24576.0f)
    {
        m_cloudOffset -= 24576.0f;
    }

    const int cellSize         = 12;
    const int cloudCellCount   = 8;
    const int cloudSectionSize = cloudCellCount * cellSize;
    int viewChunks =
            everstead->options.lodEnabled ? std::max(m_level->GetRenderDistance(), everstead->options.lodDistance) : m_level->GetRenderDistance();
    const int cloudRadius      = std::min(12, viewChunks * Chunk::s_sizeX / cloudSectionSize + 1);
    const float cloudThickness = 4.0f;
    const float cloudY         = (float) everstead->options.cloudHeight + 0.33f;
    // Empty cloud texels have alpha 1/255, so a near-zero cutoff lets them draw as faint ghost clouds
    constexpr float kCloudAlphaCutoff = 0.1f;
    const float relativeCloudY        = cloudY - (float) cameraPos.y;
    const bool noCullMode             = relativeCloudY > -cloudThickness - 1.0f && relativeCloudY <= cloudThickness + 1.0f;

    double cloudX = (cameraPos.x + m_cloudOffset) / cellSize;
    double cloudZ = cameraPos.z / cellSize + 0.33;
    // We snap the texture offset to whole sections and shift the mesh by the remainder, so the mesh never rebuilds while moving
    int cloudTextureX   = Mth::FloorDiv((int) floor(cloudX), cloudCellCount) * cloudCellCount;
    int cloudTextureZ   = Mth::FloorDiv((int) floor(cloudZ), cloudCellCount) * cloudCellCount;
    float cloudOffsetX  = (float) (cloudX - cloudTextureX);
    float cloudOffsetZ  = (float) (cloudZ - cloudTextureZ);
    glm::dvec3 flashPos = m_level->GetLightningFlashPosition() - cameraPos;
    glm::vec4 flash((float) flashPos.x, relativeCloudY, (float) flashPos.z, m_level->GetLightningFlash(partialTicks));
    if (kNimbusEnabled && m_nimbus->HasClouds() && outputFramebuffer)
    {
        SetFogUniforms(m_nimbus->GetCloudShader(), true);
        m_nimbus->RenderClouds(outputFramebuffer, m_cloudTexture, glm::inverse(projection * viewMatrix), glm::vec3(cloudR, cloudG, cloudB),
                               glm::vec2((float) (cloudX - floor(cloudX / 256.0) * 256.0), (float) (cloudZ - floor(cloudZ / 256.0) * 256.0)),
                               relativeCloudY, flash, rain, glm::vec3(m_level->GetWorldProvider().GetSunDirection()),
                               (float) (cloudRadius * cloudSectionSize));
        return;
    }

    if (m_cloudRadius != cloudRadius || m_cloudNoCullMode != noCullMode)
    {
        RebuildCloudMesh(cloudRadius, noCullMode);
    }

    std::vector<CloudSection> visibleSections;
    visibleSections.reserve(m_cloudSections.size());
    for (const CloudSection &section : m_cloudSections)
    {
        float x = (float) (section.x * cloudCellCount) * cellSize - cloudOffsetX * cellSize;
        float z = (float) (section.z * cloudCellCount) * cellSize - cloudOffsetZ * cellSize;
        AABB sectionAABB(glm::dvec3(cameraPos.x + x, cloudY, cameraPos.z + z),
                         glm::dvec3(cameraPos.x + x + cloudSectionSize, cloudY + cloudThickness, cameraPos.z + z + cloudSectionSize));
        if (!m_frustum.TestAABB(sectionAABB))
        {
            continue;
        }

        // Sections sit back to back in the vertex buffer, so neighbors merge into one draw
        if (!visibleSections.empty() && visibleSections.back().firstVertex + visibleSections.back().vertexCount == section.firstVertex)
        {
            visibleSections.back().vertexCount += section.vertexCount;
        }
        else
        {
            visibleSections.push_back(section);
        }
    }

    GlStateManager::SetFrontFace(eFrontFace_Ccw);
    if (noCullMode)
    {
        GlStateManager::DisableCull();
    }
    else
    {
        GlStateManager::EnableCull();
    }

    GlStateManager::SetCullFace(eCullFace_Back);

    GlStateManager::EnableDepthTest();
    GlStateManager::SetDepthFunc(eCompareFunc_LessEqual);
    GlStateManager::EnableBlend();
    glm::dmat4 cloudModel = glm::translate(glm::dmat4(1.0), glm::dvec3(-cloudOffsetX * cellSize, cloudY - cameraPos.y, -cloudOffsetZ * cellSize));

    GlStateManager::SetDepthMask(true);
    GlStateManager::SetBlendFunc(eBlendFactor_Zero, eBlendFactor_One);
    m_cloudMesh->Prepare(m_cloudTexture, cloudModel, viewMatrix, projection, cloudR, cloudG, cloudB, kCloudAlphaCutoff,
                         (float) Mth::FloorMod(cloudTextureX, 256) / 256.0f, (float) Mth::FloorMod(cloudTextureZ, 256) / 256.0f, flash);
    for (const CloudSection &section : visibleSections)
    {
        m_cloudMesh->Render(section.firstVertex, section.vertexCount);
    }

    GlStateManager::SetDepthMask(false);
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    m_cloudMesh->Prepare(m_cloudTexture, cloudModel, viewMatrix, projection, cloudR, cloudG, cloudB, kCloudAlphaCutoff,
                         (float) Mth::FloorMod(cloudTextureX, 256) / 256.0f, (float) Mth::FloorMod(cloudTextureZ, 256) / 256.0f, flash);
    for (const CloudSection &section : visibleSections)
    {
        m_cloudMesh->Render(section.firstVertex, section.vertexCount);
    }

    Textures::Unbind();
    GlStateManager::DisableBlend();
    GlStateManager::SetDepthMask(true);
    GlStateManager::SetCullFace(eCullFace_Back);
    GlStateManager::SetFrontFace(eFrontFace_Ccw);
    GlStateManager::EnableCull();
}

void LevelRenderer::RenderStars(const glm::dmat4 &viewMatrix, const glm::dmat4 &projection)
{
    float td    = m_level->GetCelestialAngle();
    float alpha = 1.0f - (cosf(td * 6.28318530718f) * 2.0f + 0.25f);
    if (alpha < 0.0f)
    {
        alpha = 0.0f;
    }

    if (alpha > 1.0f)
    {
        alpha = 1.0f;
    }

    alpha = alpha * alpha * 0.5f;
    if (alpha < 0.01f || m_starVertices.empty())
    {
        return;
    }

    Tesselator *renderer   = Tesselator::GetInstance();
    BufferBuilder *builder = renderer->GetBuilder();

    glm::dmat4 viewRotOnly = viewMatrix;
    viewRotOnly[3][0]      = 0.0f;
    viewRotOnly[3][1]      = 0.0f;
    viewRotOnly[3][2]      = 0.0f;

    GlStateManager::MatrixMode(1);
    GlStateManager::LoadIdentity();
    GlStateManager::MultMatrix(projection);

    GlStateManager::MatrixMode(0);
    GlStateManager::LoadIdentity();
    GlStateManager::MultMatrix(viewRotOnly);

    Textures::Unbind();

    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_One);

    GlStateManager::PushMatrix();
    GlStateManager::Rotatef(-90.0f, 0.0f, 1.0f, 0.0f);
    GlStateManager::Rotatef(td * 360.0f, 1.0f, 0.0f, 0.0f);

    builder->Begin(ePrimitiveType_Triangles);
    builder->Color(alpha, alpha, alpha, alpha);

    for (size_t i = 0; i + 2 < m_starVertices.size(); i += 3)
    {
        builder->Vertex(m_starVertices[i + 0], m_starVertices[i + 1], m_starVertices[i + 2]);
    }

    builder->End();

    GlStateManager::PopMatrix();

    GlStateManager::DisableBlend();
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();
}

#endif
