#pragma once

#include "UIComponent.h"

#include <cstdint>
#include <string>
#include <vector>

class Font;

class UIComponent_DebugMenu : public UIComponent
{
public:
    UIComponent_DebugMenu();
    ~UIComponent_DebugMenu() override;

    void Tick() override;
    void Render() override;

    void SetProfilerVisible(bool profilerVisible);
    bool IsProfilerVisible() const;

private:
    static const int s_profilerMaxSlices     = 8;
    static const int s_profilerWheelSegments = 64;
    static const float s_profilerWheelRadius;
    static const float s_profilerWheelTilt;
    static const float s_profilerWheelDepth;
    static const uint32_t s_profilerSliceColors[s_profilerMaxSlices + 1];

    float GetColumnWidth(Font *font, const std::vector<std::wstring> &lines, float fontScale, float padX) const;
    void DrawColumn(Font *font, const std::vector<std::wstring> &lines, float x, float y, float width, float uiScale, float fontScale,
                    float lineHeight, bool alignRight, const std::vector<uint32_t> *colors);
    void RefreshFrameStats();
    void RenderProfiler(Font *font, float uiScale, float fontScale, float lineHeight);
    void DrawWheel(float centerX, float centerY, float radius, float uiScale, const std::vector<float> &fractions);
    void BuildProfilerLines(std::vector<std::wstring> *outLines);
    void RefreshLines();
    void RefreshMemoryLines();
    std::wstring FormatBytes(uint64_t bytes);

    std::vector<std::wstring> m_cachedWorldLines;
    std::vector<std::wstring> m_cachedGameLines;
    std::wstring m_cachedFrameStatsLine;
    std::wstring m_cachedTickStatsLine;
    double m_statsRefreshTimer;
    double m_statsRefreshInterval;
    double m_frameStatsRefreshTimer;
    double m_frameStatsRefreshInterval;
    double m_memoryRefreshTimer;
    std::vector<std::wstring> m_cachedMemoryLines;
    bool m_profilerVisible;
    std::vector<std::wstring> m_cachedProfilerLines;
    std::vector<float> m_cachedProfilerFractions;
    std::vector<uint32_t> m_cachedProfilerColors;
    uint32_t m_cachedProfilerGeneration;
};
