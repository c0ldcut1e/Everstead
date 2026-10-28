#include "FrameProfiler.h"

#include <algorithm>
#include <chrono>

#include "Logger.h"
#include "TickProfiler.h"

double FrameProfiler::s_averageWindowMs = 500.0;
double FrameProfiler::s_worstHoldMs     = 10000.0;

bool FrameProfiler::s_enabled              = false;
bool FrameProfiler::s_logSlowFrames        = false;
double FrameProfiler::s_thresholdMs        = 12.0;
double FrameProfiler::s_frameStartMs       = 0.0;
double FrameProfiler::s_windowStartMs      = 0.0;
double FrameProfiler::s_accumulatedFrameMs = 0.0;
int FrameProfiler::s_accumulatedFrames     = 0;
double FrameProfiler::s_frameMs            = 0.0;
double FrameProfiler::s_unaccountedMs      = 0.0;

std::vector<FrameProfiler::Section> FrameProfiler::s_sections;
std::vector<FrameProfiler::Section> FrameProfiler::s_accumulated;
std::vector<FrameProfiler::OpenSection> FrameProfiler::s_open;
std::thread::id FrameProfiler::s_mainThread = std::this_thread::get_id();
std::vector<FrameProfiler::Result> FrameProfiler::s_results;
std::vector<FrameProfiler::Section> FrameProfiler::s_gpuAccumulated;
std::vector<FrameProfiler::Result> FrameProfiler::s_gpuResults;
uint32_t FrameProfiler::s_generation = 0;
std::vector<FrameProfiler::Result> FrameProfiler::s_worstResults;
double FrameProfiler::s_worstFrameMs = 0.0;
double FrameProfiler::s_worstTimeMs  = 0.0;

void FrameProfiler::SetEnabled(bool enabled)
{
    if (s_enabled == enabled)
    {
        return;
    }

    s_enabled = enabled;
    s_sections.clear();
    s_accumulated.clear();
    s_open.clear();
    s_results.clear();
    s_gpuAccumulated.clear();
    s_gpuResults.clear();
    s_windowStartMs      = Now();
    s_accumulatedFrameMs = 0.0;
    s_accumulatedFrames  = 0;
    s_frameMs            = 0.0;
    s_unaccountedMs      = 0.0;
    s_worstResults.clear();
    s_worstFrameMs = 0.0;
    s_worstTimeMs  = 0.0;
}

bool FrameProfiler::IsEnabled()
{
    return s_enabled;
}

void FrameProfiler::SetLogSlowFrames(bool logSlowFrames)
{
    s_logSlowFrames = logSlowFrames;
}

bool FrameProfiler::IsLoggingSlowFrames()
{
    return s_logSlowFrames;
}

void FrameProfiler::SetThresholdMs(double thresholdMs)
{
    s_thresholdMs = thresholdMs;
}

double FrameProfiler::GetThresholdMs()
{
    return s_thresholdMs;
}

void FrameProfiler::SetAverageWindowMs(double averageWindowMs)
{
    if (averageWindowMs < 16.0)
    {
        averageWindowMs = 16.0;
    }

    s_averageWindowMs = averageWindowMs;
}

double FrameProfiler::GetAverageWindowMs()
{
    return s_averageWindowMs;
}

void FrameProfiler::SetWorstHoldMs(double worstHoldMs)
{
    if (worstHoldMs < 1000.0)
    {
        worstHoldMs = 1000.0;
    }

    s_worstHoldMs = worstHoldMs;
}

double FrameProfiler::GetWorstHoldMs()
{
    return s_worstHoldMs;
}

double FrameProfiler::Now()
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

size_t FrameProfiler::FindSection(std::vector<Section> *sections, const char *name)
{
    for (size_t i = 0; i < sections->size(); i++)
    {
        if ((*sections)[i].name == name)
        {
            return i;
        }
    }

    sections->push_back({name, 0.0, 0.0, 0});
    return sections->size() - 1;
}

void FrameProfiler::BeginFrame()
{
    if (!s_enabled)
    {
        return;
    }

    s_sections.clear();
    s_open.clear();
    s_frameStartMs = Now();
}

void FrameProfiler::Push(const char *name)
{
    // Server ticks also feed the tick profiler so /tps sees the same sections without duplicate scopes
    TickProfiler *tickProfiler = TickProfiler::GetActive();
    if (tickProfiler)
    {
        tickProfiler->Push(name);
    }

    // Only the main thread records sections because the profiler state is shared and unlocked
    if (!s_enabled || std::this_thread::get_id() != s_mainThread)
    {
        return;
    }

    s_open.push_back({FindSection(&s_sections, name), Now(), 0.0});
}

void FrameProfiler::Pop()
{
    TickProfiler *tickProfiler = TickProfiler::GetActive();
    if (tickProfiler)
    {
        tickProfiler->Pop();
    }

    if (!s_enabled || std::this_thread::get_id() != s_mainThread || s_open.empty())
    {
        return;
    }

    OpenSection open = s_open.back();
    s_open.pop_back();

    double elapsedMs = Now() - open.startMs;

    Section &section = s_sections[open.index];
    section.totalMs += elapsedMs;
    section.childMs += open.childMs;
    section.calls++;

    if (!s_open.empty())
    {
        s_open.back().childMs += elapsedMs;
    }
}

void FrameProfiler::AddGpuTime(const char *name, double ms)
{
    if (!s_enabled)
    {
        return;
    }

    Section &section = s_gpuAccumulated[FindSection(&s_gpuAccumulated, name)];
    section.totalMs += ms;
    section.calls++;
}

const std::vector<FrameProfiler::Result> &FrameProfiler::GetGpuResults()
{
    return s_gpuResults;
}

void FrameProfiler::EndFrame()
{
    if (!s_enabled)
    {
        return;
    }

    double frameMs = Now() - s_frameStartMs;

    for (size_t i = 0; i < s_sections.size(); i++)
    {
        size_t index = FindSection(&s_accumulated, s_sections[i].name);
        s_accumulated[index].totalMs += s_sections[i].totalMs;
        s_accumulated[index].childMs += s_sections[i].childMs;
        s_accumulated[index].calls += s_sections[i].calls;
    }

    s_accumulatedFrameMs += frameMs;
    s_accumulatedFrames++;

    double nowMs = Now();
    if (frameMs > s_worstFrameMs || nowMs - s_worstTimeMs > s_worstHoldMs)
    {
        s_worstFrameMs = frameMs;
        s_worstTimeMs  = nowMs;
        s_generation++;

        s_worstResults.clear();
        s_worstResults.reserve(s_sections.size());
        for (size_t i = 0; i < s_sections.size(); i++)
        {
            s_worstResults.push_back({s_sections[i].name, s_sections[i].totalMs - s_sections[i].childMs, s_sections[i].totalMs, s_sections[i].calls});
        }

        std::sort(s_worstResults.begin(), s_worstResults.end(),
                  [](const Result &a, const Result &b)
                  {
                      return a.selfMs > b.selfMs;
                  });
    }

    if (nowMs - s_windowStartMs >= s_averageWindowMs)
    {
        PublishAverage();
    }

    if (!s_logSlowFrames || frameMs < s_thresholdMs)
    {
        return;
    }

    std::vector<Section> sorted = s_sections;
    std::sort(sorted.begin(), sorted.end(),
              [](const Section &a, const Section &b)
              {
                  return (a.totalMs - a.childMs) > (b.totalMs - b.childMs);
              });

    Logger::LogWarn("Slow frame: %.2fms", frameMs);
    for (size_t i = 0; i < sorted.size() && i < 8; i++)
    {
        double selfMs = sorted[i].totalMs - sorted[i].childMs;
        if (selfMs < 0.05)
        {
            break;
        }

        Logger::LogWarn("  %-28s self %6.2fms  total %6.2fms  x%d", sorted[i].name, selfMs, sorted[i].totalMs, sorted[i].calls);
    }
}

void FrameProfiler::PublishAverage()
{
    double frames = (double) (s_accumulatedFrames > 0 ? s_accumulatedFrames : 1);

    s_results.clear();
    s_results.reserve(s_accumulated.size());

    double totalSelfMs = 0.0;
    for (size_t i = 0; i < s_accumulated.size(); i++)
    {
        double selfMs = (s_accumulated[i].totalMs - s_accumulated[i].childMs) / frames;
        totalSelfMs += selfMs;
        s_results.push_back(
                {s_accumulated[i].name, selfMs, s_accumulated[i].totalMs / frames, (int) ((double) s_accumulated[i].calls / frames + 0.5)});
    }

    std::sort(s_results.begin(), s_results.end(),
              [](const Result &a, const Result &b)
              {
                  return a.selfMs > b.selfMs;
              });

    s_gpuResults.clear();
    for (size_t i = 0; i < s_gpuAccumulated.size(); i++)
    {
        double ms = s_gpuAccumulated[i].totalMs / frames;
        s_gpuResults.push_back({s_gpuAccumulated[i].name, ms, ms, (int) ((double) s_gpuAccumulated[i].calls / frames + 0.5)});
    }

    std::sort(s_gpuResults.begin(), s_gpuResults.end(),
              [](const Result &a, const Result &b)
              {
                  return a.selfMs > b.selfMs;
              });

    s_frameMs       = s_accumulatedFrameMs / frames;
    s_unaccountedMs = s_frameMs - totalSelfMs;
    if (s_unaccountedMs < 0.0)
    {
        s_unaccountedMs = 0.0;
    }

    s_generation++;

    s_accumulated.clear();
    s_gpuAccumulated.clear();
    s_windowStartMs      = Now();
    s_accumulatedFrameMs = 0.0;
    s_accumulatedFrames  = 0;
}

const std::vector<FrameProfiler::Result> &FrameProfiler::GetResults()
{
    return s_results;
}

double FrameProfiler::GetFrameMs()
{
    return s_frameMs;
}

double FrameProfiler::GetUnaccountedMs()
{
    return s_unaccountedMs;
}

uint32_t FrameProfiler::GetGeneration()
{
    return s_generation;
}

const std::vector<FrameProfiler::Result> &FrameProfiler::GetWorstResults()
{
    return s_worstResults;
}

double FrameProfiler::GetWorstFrameMs()
{
    return s_worstFrameMs;
}

double FrameProfiler::GetWorstAgeSeconds()
{
    return (Now() - s_worstTimeMs) / 1000.0;
}
