#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_DebugMenu.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <numbers>
#include <string>
#include <vector>

#include "../../core/Everstead.h"
#include "../../core/FrameProfiler.h"
#include "../../core/GameInfo.h"
#include "../../core/ResourceLocation.h"
#include "../../entity/player/MultiPlayerLocalPlayer.h"
#include "../../io/File.h"
#include "../../memory/MemoryTracker.h"
#include "../../network/Connection.h"
#include "../../network/listener/ClientConnection.h"
#include "../../platform/PlatformProcess.h"
#include "../../rendering/Font.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/RenderDevice.h"
#include "../../rendering/Tesselator.h"
#include "../../sound/SoundEngine.h"
#include "../../utils/StringUtils.h"
#include "../../utils/Time.h"
#include "../../utils/math/Mth.h"
#include "../../world/DimensionId.h"
#include "../../world/Level.h"
#include "../../world/LevelRenderer.h"
#include "../../world/block/Block.h"
#include "../../world/chunk/Chunk.h"
#include "../../world/chunk/ChunkManager.h"
#include "../../world/chunk/ChunkMesh.h"
#include "../../world/chunk/ChunkPos.h"
#include "../Screen.h"

const float UIComponent_DebugMenu::s_profilerWheelRadius = 100.0f;
const float UIComponent_DebugMenu::s_profilerWheelTilt   = 0.5f;
const float UIComponent_DebugMenu::s_profilerWheelDepth  = 14.0f;

const uint32_t UIComponent_DebugMenu::s_profilerSliceColors[s_profilerMaxSlices + 1] = {0xFFE05A4A, 0xFFE0A24A, 0xFFD9E04A, 0xFF6FE04A, 0xFF4AE0B4,
                                                                                        0xFF4AA6E0, 0xFF6F4AE0, 0xFFD94AE0, 0xFF808080};

UIComponent_DebugMenu::UIComponent_DebugMenu()
{
    m_cachedFrameStatsLine      = L"";
    m_cachedTickStatsLine       = L"";
    m_statsRefreshTimer         = 0.0;
    m_statsRefreshInterval      = 0.05;
    m_frameStatsRefreshTimer    = 0.0;
    m_frameStatsRefreshInterval = 0.50;
    m_memoryRefreshTimer        = 0.0;
    m_profilerVisible           = false;
    m_cachedProfilerGeneration  = 0;
}

UIComponent_DebugMenu::~UIComponent_DebugMenu()
{}

void UIComponent_DebugMenu::Tick()
{
    m_frameStatsRefreshTimer += Time::GetDelta();
    if (m_frameStatsRefreshTimer >= m_frameStatsRefreshInterval || m_cachedFrameStatsLine.empty())
    {
        RefreshFrameStats();
        m_frameStatsRefreshTimer = 0.0;
    }

    // These stats read /proc, query the driver and walk every server chunk, so we refresh them rarely
    m_memoryRefreshTimer += Time::GetDelta();
    if (m_memoryRefreshTimer >= 1.0 || m_cachedMemoryLines.empty())
    {
        RefreshMemoryLines();
        m_memoryRefreshTimer = 0.0;
    }

    m_statsRefreshTimer += Time::GetDelta();
    if (m_statsRefreshTimer >= m_statsRefreshInterval)
    {
        RefreshLines();
        m_statsRefreshTimer = 0.0;
    }
}

void UIComponent_DebugMenu::Render()
{
    Everstead *everstead = Everstead::GetInstance();

    if (m_cachedWorldLines.empty() && m_cachedGameLines.empty())
    {
        RefreshLines();
        if (m_cachedWorldLines.empty() && m_cachedGameLines.empty())
        {
            return;
        }
    }

    constexpr float kDefaultHudScale = 3.0f;
    Font *font                       = everstead->GetDefaultFont();
    float uiScale   = Screen::ScaleY(everstead->GetHeight()) * (1.0f + Screen::SCALE_ADJUST) * everstead->options.GetHudScale() / kDefaultHudScale;
    float fontScale = font->SnapScale(uiScale * 0.62f);

    float padX       = 4.0f * uiScale;
    float padY       = 2.0f * uiScale;
    float lineHeight = floorf(font->GetLineHeight(fontScale) + padY * 2.0f);
    float baselineY  = floorf(8.0f * uiScale + font->GetAscent(fontScale) + padY);
    float leftX      = floorf(8.0f * uiScale);
    float gap        = floorf(16.0f * uiScale);

    if (m_profilerVisible)
    {
        RenderProfiler(font, uiScale, fontScale, lineHeight);
        return;
    }

    float maxColumnWidth = floorf(((float) everstead->GetWidth() - leftX * 2.0f - gap) * 0.5f);
    float leftWidth      = std::min(GetColumnWidth(font, m_cachedWorldLines, fontScale, padX), maxColumnWidth);
    float rightWidth     = std::min(GetColumnWidth(font, m_cachedGameLines, fontScale, padX), maxColumnWidth);
    DrawColumn(font, m_cachedWorldLines, leftX, baselineY, leftWidth, uiScale, fontScale, lineHeight, false, nullptr);
    DrawColumn(font, m_cachedGameLines, std::max(floorf((float) everstead->GetWidth() - leftX - rightWidth), leftX + leftWidth + gap), baselineY,
               rightWidth, uiScale, fontScale, lineHeight, true, nullptr);
}

void UIComponent_DebugMenu::SetProfilerVisible(bool profilerVisible)
{
    m_profilerVisible = profilerVisible;
    FrameProfiler::SetEnabled(profilerVisible || FrameProfiler::IsLoggingSlowFrames());
}

bool UIComponent_DebugMenu::IsProfilerVisible() const
{
    return m_profilerVisible;
}

void UIComponent_DebugMenu::BuildProfilerLines(std::vector<std::wstring> *outLines)
{
    Everstead *everstead = Everstead::GetInstance();
    wchar_t buffer[0x1FF];

    const std::vector<FrameProfiler::Result> &results = FrameProfiler::GetResults();
    double frameMs                                    = FrameProfiler::GetFrameMs();

    m_cachedProfilerFractions.clear();
    m_cachedProfilerColors.clear();

    swprintf(buffer, 0x1FF, L"frame %.2f ms (%.0f fps)", frameMs, frameMs > 0.0 ? 1000.0 / frameMs : 0.0);
    outLines->push_back(buffer);
    m_cachedProfilerColors.push_back(0xFFFFFFFF);

    double shownMs = 0.0;
    for (size_t i = 0; i < results.size() && (int) i < s_profilerMaxSlices; i++)
    {
        if (results[i].selfMs < 0.01)
        {
            break;
        }

        double share = frameMs > 0.0 ? results[i].selfMs / frameMs : 0.0;
        shownMs += results[i].selfMs;
        m_cachedProfilerFractions.push_back((float) share);

        swprintf(buffer, 0x1FF, L"%.1f%% %hs %.2f ms", share * 100.0, results[i].name, results[i].selfMs);
        outLines->push_back(buffer);
        m_cachedProfilerColors.push_back(s_profilerSliceColors[i]);
    }

    double restMs = frameMs - shownMs;
    if (restMs < 0.0)
    {
        restMs = 0.0;
    }

    m_cachedProfilerFractions.push_back((float) (frameMs > 0.0 ? restMs / frameMs : 1.0));
    swprintf(buffer, 0x1FF, L"%.1f%% unprofiled %.2f ms", frameMs > 0.0 ? restMs / frameMs * 100.0 : 100.0, restMs);
    outLines->push_back(buffer);
    m_cachedProfilerColors.push_back(s_profilerSliceColors[s_profilerMaxSlices]);

    outLines->push_back(L"");
    m_cachedProfilerColors.push_back(0xFFE8E8E8);

    const std::vector<FrameProfiler::Result> &gpu = FrameProfiler::GetGpuResults();
    if (!gpu.empty())
    {
        for (size_t i = 0; i < gpu.size(); i++)
        {
            swprintf(buffer, 0x1FF, L"gpu %hs %.2f ms", gpu[i].name, gpu[i].selfMs);
            outLines->push_back(buffer);
            m_cachedProfilerColors.push_back(0xFF80D0FF);
        }

        outLines->push_back(L"");
        m_cachedProfilerColors.push_back(0xFFE8E8E8);
    }

    const std::vector<FrameProfiler::Result> &worst = FrameProfiler::GetWorstResults();
    double worstFrameMs                             = FrameProfiler::GetWorstFrameMs();
    if (worstFrameMs > 0.0)
    {
        swprintf(buffer, 0x1FF, L"worst %.1f ms  %.0fs ago", worstFrameMs, FrameProfiler::GetWorstAgeSeconds());
        outLines->push_back(buffer);
        m_cachedProfilerColors.push_back(0xFFFF8080);

        double worstShownMs = 0.0;
        for (size_t i = 0; i < worst.size() && i < 6; i++)
        {
            if (worst[i].selfMs < 0.05)
            {
                break;
            }

            worstShownMs += worst[i].selfMs;
            swprintf(buffer, 0x1FF, L"  %hs %.1f ms", worst[i].name, worst[i].selfMs);
            outLines->push_back(buffer);
            m_cachedProfilerColors.push_back(0xFFFFC0C0);
        }

        double worstRestMs = worstFrameMs - worstShownMs;
        if (worstRestMs < 0.0)
        {
            worstRestMs = 0.0;
        }

        swprintf(buffer, 0x1FF, L"  unprofiled %.1f ms", worstRestMs);
        outLines->push_back(buffer);
        m_cachedProfilerColors.push_back(0xFFFFC0C0);

        outLines->push_back(L"");
        m_cachedProfilerColors.push_back(0xFFE8E8E8);
    }

    ChunkManager *chunkManager = everstead->GetChunkManager();
    if (chunkManager)
    {
        swprintf(buffer, 0x1FF, L"gen threads %zu  active %zu/%zu", chunkManager->GetThreadCount(), chunkManager->GetActiveCount(),
                 chunkManager->GetMaxActiveCount());
        outLines->push_back(buffer);
        m_cachedProfilerColors.push_back(0xFF9ED8FF);

        swprintf(buffer, 0x1FF, L"gen pending %zu  finished %zu", chunkManager->GetPendingCount(), chunkManager->GetFinishedCount());
        outLines->push_back(buffer);
        m_cachedProfilerColors.push_back(0xFF9ED8FF);
    }

    LevelRenderer *levelRenderer = everstead->GetLevelRenderer();
    if (levelRenderer)
    {
        swprintf(buffer, 0x1FF, L"mesher threads %zu  active %zu", levelRenderer->GetMesherThreadCount(), levelRenderer->GetActiveMesherTaskCount());
        outLines->push_back(buffer);
        m_cachedProfilerColors.push_back(0xFFB6FFA8);

        swprintf(buffer, 0x1FF, L"mesh queue %zu  urgent %zu", levelRenderer->GetQueuedRebuildCount(), levelRenderer->GetUrgentRebuildCount());
        outLines->push_back(buffer);
        m_cachedProfilerColors.push_back(0xFFB6FFA8);

        swprintf(buffer, 0x1FF, L"mesh uploads pending %zu", levelRenderer->GetPendingMeshCount());
        outLines->push_back(buffer);
        m_cachedProfilerColors.push_back(0xFFB6FFA8);
    }
}

void UIComponent_DebugMenu::RenderProfiler(Font *font, float uiScale, float fontScale, float lineHeight)
{
    Everstead *everstead = Everstead::GetInstance();

    uint32_t generation = FrameProfiler::GetGeneration();
    if (generation != m_cachedProfilerGeneration || m_cachedProfilerLines.empty())
    {
        m_cachedProfilerGeneration = generation;
        m_cachedProfilerLines.clear();
        BuildProfilerLines(&m_cachedProfilerLines);
    }

    float padX      = 4.0f * uiScale;
    float margin    = floorf(8.0f * uiScale);
    float gap       = floorf(10.0f * uiScale);
    float textWidth = GetColumnWidth(font, m_cachedProfilerLines, fontScale, padX);

    float wheelRadius = floorf(s_profilerWheelRadius * uiScale);
    float wheelDepth  = floorf(s_profilerWheelDepth * uiScale);
    float wheelWidth  = wheelRadius * 2.0f;
    float wheelHeight = wheelRadius * 2.0f * s_profilerWheelTilt + wheelDepth;
    float textHeight  = (float) m_cachedProfilerLines.size() * lineHeight;

    float regionWidth  = wheelWidth + gap + textWidth;
    float regionHeight = textHeight > wheelHeight ? textHeight : wheelHeight;

    float regionLeft = (float) everstead->GetWidth() - margin - regionWidth;
    float regionTop  = (float) everstead->GetHeight() - margin - regionHeight;

    float wheelCenterX = regionLeft + wheelRadius;
    float wheelCenterY = regionTop + regionHeight * 0.5f - wheelDepth * 0.5f;

    DrawWheel(wheelCenterX, wheelCenterY, wheelRadius, uiScale, m_cachedProfilerFractions);

    float textLeft  = (float) everstead->GetWidth() - margin - textWidth;
    float baselineY = floorf(regionTop + font->GetAscent(fontScale));
    DrawColumn(font, m_cachedProfilerLines, textLeft, baselineY, textWidth, uiScale, fontScale, lineHeight, true, &m_cachedProfilerColors);
}

void UIComponent_DebugMenu::DrawWheel(float centerX, float centerY, float radius, float uiScale, const std::vector<float> &fractions)
{
    const float twoPi = std::numbers::pi_v<float> * 2.0f;
    float depth       = floorf(s_profilerWheelDepth * uiScale);

    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);

    builder->Color(0x90000000);
    float backdrop = radius + 4.0f * uiScale;
    for (int i = 0; i < s_profilerWheelSegments; i++)
    {
        float a0 = (float) i / (float) s_profilerWheelSegments * twoPi;
        float a1 = (float) (i + 1) / (float) s_profilerWheelSegments * twoPi;
        builder->Vertex(centerX, centerY, 0.0f);
        builder->Vertex(centerX + sinf(a0) * backdrop, centerY - cosf(a0) * backdrop * s_profilerWheelTilt, 0.0f);
        builder->Vertex(centerX + sinf(a1) * backdrop, centerY - cosf(a1) * backdrop * s_profilerWheelTilt, 0.0f);
    }

    float startFraction = 0.0f;
    for (size_t slice = 0; slice < fractions.size(); slice++)
    {
        float sweep = fractions[slice] * twoPi;
        if (sweep <= 0.0f)
        {
            startFraction += fractions[slice];
            continue;
        }

        size_t colorIndex  = slice < (size_t) s_profilerMaxSlices ? slice : (size_t) s_profilerMaxSlices;
        uint32_t color     = s_profilerSliceColors[colorIndex];
        uint32_t sideColor = (color & 0xFF000000) | (((color >> 17) & 0x7F) << 16) | (((color >> 9) & 0x7F) << 8) | ((color >> 1) & 0x7F);

        int segments = (int) ceilf(fractions[slice] * (float) s_profilerWheelSegments);
        if (segments < 1)
        {
            segments = 1;
        }

        float startAngle = startFraction * twoPi;

        builder->Color(color);
        for (int i = 0; i < segments; i++)
        {
            float a0 = startAngle + sweep * (float) i / (float) segments;
            float a1 = startAngle + sweep * (float) (i + 1) / (float) segments;
            builder->Vertex(centerX, centerY, 0.0f);
            builder->Vertex(centerX + sinf(a0) * radius, centerY - cosf(a0) * radius * s_profilerWheelTilt, 0.0f);
            builder->Vertex(centerX + sinf(a1) * radius, centerY - cosf(a1) * radius * s_profilerWheelTilt, 0.0f);
        }

        builder->Color(sideColor);
        for (int i = 0; i < segments; i++)
        {
            float a0 = startAngle + sweep * (float) i / (float) segments;
            float a1 = startAngle + sweep * (float) (i + 1) / (float) segments;
            float y0 = -cosf(a0) * radius * s_profilerWheelTilt;
            float y1 = -cosf(a1) * radius * s_profilerWheelTilt;
            if (y0 < 0.0f && y1 < 0.0f)
            {
                continue;
            }

            float x0 = centerX + sinf(a0) * radius;
            float x1 = centerX + sinf(a1) * radius;
            builder->Vertex(x0, centerY + y0, 0.0f);
            builder->Vertex(x1, centerY + y1, 0.0f);
            builder->Vertex(x1, centerY + y1 + depth, 0.0f);
            builder->Vertex(x0, centerY + y0, 0.0f);
            builder->Vertex(x1, centerY + y1 + depth, 0.0f);
            builder->Vertex(x0, centerY + y0 + depth, 0.0f);
        }

        startFraction += fractions[slice];
    }

    builder->End();

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();
}

float UIComponent_DebugMenu::GetColumnWidth(Font *font, const std::vector<std::wstring> &lines, float fontScale, float padX) const
{
    float width = 0.0f;
    for (size_t i = 0; i < lines.size(); i++)
    {
        if (lines[i].empty())
        {
            continue;
        }

        float rowWidth = font->GetWidth(lines[i], fontScale) + padX * 2.0f;
        if (rowWidth > width)
        {
            width = rowWidth;
        }
    }

    return width;
}

void UIComponent_DebugMenu::DrawColumn(Font *font, const std::vector<std::wstring> &lines, float x, float y, float width, float uiScale,
                                       float fontScale, float lineHeight, bool alignRight, const std::vector<uint32_t> *colors)
{
    float padX   = 4.0f * uiScale;
    float padY   = 2.0f * uiScale;
    float ascent = font->GetAscent(fontScale);

    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    builder->Color(0x90000000);

    for (size_t i = 0; i < lines.size(); i++)
    {
        if (lines[i].empty())
        {
            continue;
        }

        float textWidth = font->GetWidth(lines[i], fontScale);
        float rowWidth  = textWidth + padX * 2.0f;
        if (rowWidth > width)
        {
            rowWidth = width;
        }

        float rowX = alignRight ? x + width - rowWidth : x;
        float rowY = y + (float) i * lineHeight - ascent - padY;
        builder->Vertex(rowX, rowY + lineHeight, 0.0f);
        builder->Vertex(rowX + rowWidth, rowY + lineHeight, 0.0f);
        builder->Vertex(rowX + rowWidth, rowY, 0.0f);
        builder->Vertex(rowX, rowY + lineHeight, 0.0f);
        builder->Vertex(rowX + rowWidth, rowY, 0.0f);
        builder->Vertex(rowX, rowY, 0.0f);
    }

    builder->End();

    for (size_t i = 0; i < lines.size(); i++)
    {
        if (lines[i].empty())
        {
            continue;
        }

        float textWidth = font->GetWidth(lines[i], fontScale);
        float rowWidth  = textWidth + padX * 2.0f;
        if (rowWidth > width)
        {
            rowWidth = width;
        }

        float rowX         = alignRight ? x + width - rowWidth : x;
        float textX        = alignRight ? rowX + rowWidth - padX - textWidth : rowX + padX;
        uint32_t textColor = (colors && i < colors->size()) ? (*colors)[i] : 0xFFE8E8E8;
        font->QueueShadow(lines[i], textX, y + (float) i * lineHeight, fontScale, textColor);
    }

    font->Flush();

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();
}

void UIComponent_DebugMenu::RefreshFrameStats()
{
    Everstead *everstead = Everstead::GetInstance();
    Timer *timer         = everstead->GetTimer();
    double rawDelta      = Time::GetDelta();
    double smoothedDelta = timer ? (double) timer->GetFrameSeconds() : rawDelta;
    wchar_t buffer[0x1FF];

    swprintf(buffer, 0x1FF, L"frame: raw %.3f ms (%.1f fps)  smooth %.3f ms (%.1f fps)", rawDelta * 1000.0, rawDelta > 0.0 ? 1.0 / rawDelta : 0.0,
             smoothedDelta * 1000.0, smoothedDelta > 0.0 ? 1.0 / smoothedDelta : 0.0);
    m_cachedFrameStatsLine = buffer;

    swprintf(buffer, 0x1FF, L"ticks: %d  partial: %.3f  tick dt: %.3f ms", timer ? timer->GetElapsedTicks() : 0,
             timer ? timer->GetPartialTicks() : 0.0f, Time::GetTickDelta() * 1000.0);
    m_cachedTickStatsLine = buffer;
}

void UIComponent_DebugMenu::RefreshMemoryLines()
{
    Everstead *everstead = Everstead::GetInstance();
    std::vector<std::wstring> lines;
    wchar_t buffer[0x1FF];

    uint64_t privateBytes = 0;
    uint64_t limitBytes   = MemoryMonitor::GetInstance()->GetLimitBytes();
    if (PlatformProcess::GetPrivateMemoryBytes(&privateBytes) && limitBytes > 0)
    {
        swprintf(buffer, 0x1FF, L"Mem: %d%% %llu/%lluMB", (int) (privateBytes * 100 / limitBytes),
                 (unsigned long long) (privateBytes / (1024 * 1024)), (unsigned long long) (limitBytes / (1024 * 1024)));
        lines.emplace_back(buffer);
    }

    Level *memoryLevel    = everstead->GetLevel();
    Level *serverLevel    = everstead->GetWorldLevel();
    size_t scheduledTicks = memoryLevel ? memoryLevel->GetScheduledBlockTickCount() : 0;
    size_t serverTicks    = (serverLevel && serverLevel != memoryLevel) ? serverLevel->GetScheduledBlockTickCount() : 0;
    swprintf(buffer, 0x1FF, L"block ticks queued: client %zu  server %zu", scheduledTicks, serverTicks);
    lines.emplace_back(buffer);

    MemoryTracker::Stats memoryStats = MemoryTracker::GetInstance()->GetStats();
    VramStats vramStats              = RenderDevice::GetInstance()->QueryVramStats();
    std::wstring vram                = L"unavailable";
    if (vramStats.available)
    {
        vram = vramStats.hasTotal ? FormatBytes(vramStats.freeBytes) + L" / " + FormatBytes(vramStats.totalBytes) : FormatBytes(vramStats.freeBytes);
    }

    swprintf(buffer, 0x1FF, L"gpu objs: tex %d  buf %d  vao %d  vram free %ls", memoryStats.textureCount, memoryStats.bufferCount,
             memoryStats.vertexArrayCount, vram.c_str());
    lines.emplace_back(buffer);

    m_cachedMemoryLines = std::move(lines);
}

void UIComponent_DebugMenu::RefreshLines()
{
    Everstead *everstead = Everstead::GetInstance();
    LocalPlayer *me      = everstead->GetLocalPlayer();
    if (!me)
    {
        m_cachedWorldLines.clear();
        m_cachedGameLines.clear();
        return;
    }

    Level *level = me->GetLevel();

    std::vector<std::wstring> worldLines;
    std::vector<std::wstring> gameLines;

    wchar_t buffer[0x1FF];
    swprintf(buffer, 0x1FF, L"Everstead %u.%u.%u  display %d x %d", g_gameVersionMajor, g_gameVersionMinor, g_gameVersionPatch, everstead->GetWidth(),
             everstead->GetHeight());
    gameLines.emplace_back(buffer);
    gameLines.emplace_back(m_cachedFrameStatsLine);
    gameLines.emplace_back(m_cachedTickStatsLine);

    size_t sentPackets                  = 0;
    size_t receivedPackets              = 0;
    MultiPlayerLocalPlayer *queuePlayer = everstead->GetLocalPlayer();
    if (queuePlayer)
    {
        ClientConnection *clientConnection = queuePlayer->GetClientConnection();
        if (clientConnection && clientConnection->GetConnection())
        {
            sentPackets     = clientConnection->GetConnection()->GetSentPacketCount();
            receivedPackets = clientConnection->GetConnection()->GetReceivedPacketCount();
        }
    }

    swprintf(buffer, 0x1FF, L"packets: out %zu  in %zu", sentPackets, receivedPackets);
    gameLines.emplace_back(buffer);

    gameLines.insert(gameLines.end(), m_cachedMemoryLines.begin(), m_cachedMemoryLines.end());
    std::wstring renderer = StringUtils::ToUtf16(RenderDevice::GetInstance()->GetRenderer());
    std::wstring version  = StringUtils::ToUtf16(RenderDevice::GetInstance()->GetVersion());
    swprintf(buffer, 0x1FF, L"gpu: %ls  %ls", renderer.c_str(), version.c_str());
    gameLines.emplace_back(buffer);

    const glm::dvec3 &pos   = me->GetPosition();
    const glm::dvec3 &front = me->GetFront();
    int cx                  = Mth::FloorDiv((int) floor(pos.x), Chunk::s_sizeX);
    int cz                  = Mth::FloorDiv((int) floor(pos.z), Chunk::s_sizeZ);
    int lx                  = Mth::FloorMod((int) floor(pos.x), Chunk::s_sizeX);
    int ly                  = (int) floor(pos.y);
    int lz                  = Mth::FloorMod((int) floor(pos.z), Chunk::s_sizeZ);

    swprintf(buffer, 0xFF, L"pos: %.3f / %.3f / %.3f", (float) pos.x, (float) pos.y, (float) pos.z);
    worldLines.emplace_back(buffer);

    swprintf(buffer, 0xFF, L"facing: yaw %.2f  pitch %.2f  %.3f / %.3f / %.3f", me->GetYaw(), me->GetPitch(), (float) front.x, (float) front.y,
             (float) front.z);
    worldLines.emplace_back(buffer);

    swprintf(buffer, 0xFF, L"chunk: %d / %d  local: %d / %d / %d", cx, cz, lx, ly, lz);
    worldLines.emplace_back(buffer);

    if (level)
    {
        const Chunk *playerChunk = level->GetChunk(ChunkPos(cx, cz));

        if (playerChunk && ly >= 0 && ly < Chunk::s_sizeY)
        {
            uint8_t lightR = 0;
            uint8_t lightG = 0;
            uint8_t lightB = 0;
            uint8_t sky    = 0;
            playerChunk->GetBlockLight(lx, ly, lz, &lightR, &lightG, &lightB);
            sky = playerChunk->GetSkyLight(lx, ly, lz);

            swprintf(buffer, 0x1FF, L"light: feet %u/%u/%u sky %u", lightR, lightG, lightB, sky);

            glm::dvec3 eyePos     = me->GetEyePosition(1.0f);
            int eyeX              = (int) floor(eyePos.x);
            int eyeY              = (int) floor(eyePos.y);
            int eyeZ              = (int) floor(eyePos.z);
            const Chunk *eyeChunk = level->GetChunk(ChunkPos(Mth::FloorDiv(eyeX, Chunk::s_sizeX), Mth::FloorDiv(eyeZ, Chunk::s_sizeZ)));
            if (eyeChunk && eyeY >= 0 && eyeY < Chunk::s_sizeY)
            {
                int eyeLx = Mth::FloorMod(eyeX, Chunk::s_sizeX);
                int eyeLz = Mth::FloorMod(eyeZ, Chunk::s_sizeZ);
                eyeChunk->GetBlockLight(eyeLx, eyeY, eyeLz, &lightR, &lightG, &lightB);
                size_t length = wcslen(buffer);
                swprintf(buffer + length, 0x1FF - length, L"  head %u/%u/%u sky %u", lightR, lightG, lightB,
                         eyeChunk->GetSkyLight(eyeLx, eyeY, eyeLz));
            }

            worldLines.emplace_back(buffer);

            Biome *biome = playerChunk->GetBiomeAt(lx, ly, lz);
            if (biome)
            {
                std::wstring biomeName = StringUtils::ToUtf16(biome->GetResourceLocation().ToString());
                swprintf(buffer, 0x1FF, L"biome: %ls", biomeName.c_str());
                worldLines.emplace_back(buffer);
            }

            swprintf(buffer, 0xFF, L"cave mood: %.1f%%  sounds %d", me->GetCaveMood() * 100.0f, me->GetCaveSoundsPlayed());
            worldLines.emplace_back(buffer);

            SoundEngine *soundEngine = everstead->GetSoundEngine();
            if (soundEngine->IsAcousticsActive())
            {
                const AcousticEnvironment &environment = soundEngine->GetAcousticEnvironment();
                swprintf(buffer, 0xFF, L"acoustics: enclosed %.0f%%  reverb %.0f%% %.1fs  echo %.0f%% %.0fms", environment.enclosure * 100.0f,
                         environment.reverbGain * 100.0f, environment.decayTime, environment.echoGain * 100.0f, environment.echoDelay * 1000.0f);
                worldLines.emplace_back(buffer);
            }
        }

        std::wstring dimensionName =
                StringUtils::ToUtf16(ResourceLocation(ResourceLocation::kDefaultNamespace, kDimensionNames[level->GetDimensionId()]).ToString());
        int64_t ticks = level->GetWorldProvider().GetWorldTime();
        swprintf(buffer, 0x1FF, L"dimension: %ls  time %lld  day %lld", dimensionName.c_str(), (long long) ticks,
                 (long long) (ticks / WorldProvider::s_dayTicks));
        worldLines.emplace_back(buffer);

        worldLines.emplace_back(L"");

        LevelRenderer *levelRenderer = everstead->GetLevelRenderer();
        uint32_t loadedChunks        = (uint32_t) level->GetLoadedChunkCount();
        uint32_t meshedChunks        = levelRenderer ? (uint32_t) levelRenderer->GetMeshedChunkCount() : 0;
        uint32_t visibleChunks       = levelRenderer ? (uint32_t) levelRenderer->GetVisibleChunkCount() : 0;
        uint32_t renderedMeshes      = levelRenderer ? (uint32_t) levelRenderer->GetRenderedMeshCount() : 0;

        swprintf(buffer, 0xFF, L"chunks: loaded %u  meshed %u  visible %u  drawn %u  distance %d", loadedChunks, meshedChunks, visibleChunks,
                 renderedMeshes, level->GetRenderDistance());
        worldLines.emplace_back(buffer);
        gameLines.emplace_back(L"");

        int ticksUntilAutosave = everstead->GetTicksUntilAutosave();
        if (ticksUntilAutosave < 0)
        {
            gameLines.emplace_back(L"world save: autosave unavailable");
        }
        else
        {
            int secondsUntilAutosave = ticksUntilAutosave / 20;
            int minutesUntilAutosave = secondsUntilAutosave / 60;
            int seconds              = secondsUntilAutosave % 60;
            if (everstead->IsSaving())
            {
                swprintf(buffer, 0xFF, L"world save: saving  next %d:%02d", minutesUntilAutosave, seconds);
            }
            else
            {
                swprintf(buffer, 0xFF, L"world save: next %d:%02d", minutesUntilAutosave, seconds);
            }

            gameLines.emplace_back(buffer);
        }

        if (levelRenderer)
        {
            swprintf(buffer, 0xFF, L"mesher: upload %u  ready %u  rebuild %u  urgent %u", (uint32_t) levelRenderer->GetPendingMeshCount(),
                     (uint32_t) levelRenderer->GetReadyMeshCount(), (uint32_t) levelRenderer->GetQueuedRebuildCount(),
                     (uint32_t) levelRenderer->GetUrgentRebuildCount());
            gameLines.emplace_back(buffer);
        }

        swprintf(buffer, 0xFF, L"level q: dirty %u  urgent %u  light %u  entities %u", (uint32_t) level->GetQueuedDirtyChunkCount(),
                 (uint32_t) level->GetUrgentDirtyChunkCount(), (uint32_t) level->GetQueuedLightUpdateCount(), (uint32_t) level->GetEntities().size());
        worldLines.emplace_back(buffer);

        worldLines.emplace_back(L"");

        HitResult *result = level->Clip(pos + glm::dvec3(0.0, 1.6, 0.0), front, 8.0f);
        if (result)
        {
            if (result->IsBlock())
            {
                BlockPos hit       = result->GetBlockPos();
                Direction *hitFace = result->GetBlockFace();

                int bcx = Mth::FloorDiv(hit.x, Chunk::s_sizeX);
                int bcz = Mth::FloorDiv(hit.z, Chunk::s_sizeZ);

                int targetLx = Mth::FloorMod(hit.x, Chunk::s_sizeX);
                int targetLy = hit.y;
                int targetLz = Mth::FloorMod(hit.z, Chunk::s_sizeZ);

                const Chunk *chunk = level->GetChunk({bcx, bcz});
                uint32_t blockId   = 0;
                Block *block       = nullptr;

                if (chunk && targetLy >= 0 && targetLy < Chunk::s_sizeY)
                {
                    uint8_t lightR = 0;
                    uint8_t lightG = 0;
                    uint8_t lightB = 0;
                    uint8_t sky    = 0;
                    blockId        = chunk->GetBlockId(targetLx, targetLy, targetLz);
                    block          = Block::ById(blockId);
                    chunk->GetBlockLight(targetLx, targetLy, targetLz, &lightR, &lightG, &lightB);
                    sky = chunk->GetSkyLight(targetLx, targetLy, targetLz);

                    swprintf(buffer, 0x1FF, L"target light: block %u/%u/%u  sky %u", lightR, lightG, lightB, sky);
                    worldLines.emplace_back(buffer);
                }

                std::wstring blockName;
                if (block)
                {
                    blockName = StringUtils::ToUtf16(block->GetResourceLocation().ToString());
                }
                else
                {
                    blockName = L"(null)";
                }

                std::wstring faceName = hitFace ? StringUtils::ToUtf16(hitFace->name) : std::wstring(L"none");
                swprintf(buffer, 0x1FF, L"target: %d %d %d  %ls  %u %ls", hit.x, hit.y, hit.z, faceName.c_str(), blockId, blockName.c_str());
                worldLines.emplace_back(buffer);
            }
            else if (result->IsEntity())
            {
                swprintf(buffer, 0xFF, L"target entity: type 0x%016llx", result->GetEntityType());
                worldLines.emplace_back(buffer);
            }
            else
            {
                worldLines.emplace_back(L"target: (none)");
            }

            delete result;
        }
        else
        {
            worldLines.emplace_back(L"target: (none)");
        }
    }

    if (!level)
    {
        worldLines.emplace_back(L"level: (none)");
    }

    m_cachedWorldLines.swap(worldLines);
    m_cachedGameLines.swap(gameLines);
}

std::wstring UIComponent_DebugMenu::FormatBytes(uint64_t bytes)
{
    wchar_t buffer[64];

    const uint64_t kib = 1024;
    const uint64_t mib = kib * 1024;
    const uint64_t gib = mib * 1024;
    if (bytes >= gib)
    {
        swprintf(buffer, 64, L"%.2f GiB", (double) bytes / (1024.0 * 1024.0 * 1024.0));
        return buffer;
    }

    if (bytes >= mib)
    {
        swprintf(buffer, 64, L"%.2f MiB", (double) bytes / (1024.0 * 1024.0));
        return buffer;
    }

    if (bytes >= kib)
    {
        swprintf(buffer, 64, L"%.2f KiB", (double) bytes / 1024.0);
        return buffer;
    }

    swprintf(buffer, 64, L"%llu B", (unsigned long long) bytes);
    return buffer;
}

#endif
