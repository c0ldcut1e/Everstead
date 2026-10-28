#pragma once

#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>

class FrameProfiler
{
public:
    struct Result
    {
        const char *name;
        double selfMs;
        double totalMs;
        int calls;
    };

    static void SetEnabled(bool enabled);
    static bool IsEnabled();

    static void SetLogSlowFrames(bool logSlowFrames);
    static bool IsLoggingSlowFrames();

    static void SetThresholdMs(double thresholdMs);
    static double GetThresholdMs();

    static void SetAverageWindowMs(double averageWindowMs);
    static double GetAverageWindowMs();

    static void SetWorstHoldMs(double worstHoldMs);
    static double GetWorstHoldMs();

    static void BeginFrame();
    static void EndFrame();

    static void Push(const char *name);
    static void Pop();

    static void AddGpuTime(const char *name, double ms);
    static const std::vector<Result> &GetGpuResults();

    static const std::vector<Result> &GetResults();
    static double GetFrameMs();
    static double GetUnaccountedMs();

    static uint32_t GetGeneration();

    static const std::vector<Result> &GetWorstResults();
    static double GetWorstFrameMs();
    static double GetWorstAgeSeconds();

private:
    struct Section
    {
        const char *name;
        double totalMs;
        double childMs;
        int calls;
    };

    struct OpenSection
    {
        size_t index;
        double startMs;
        double childMs;
    };

    static double s_averageWindowMs;
    static double s_worstHoldMs;

    static double Now();
    static size_t FindSection(std::vector<Section> *sections, const char *name);
    static void PublishAverage();

    static bool s_enabled;
    static bool s_logSlowFrames;
    static double s_thresholdMs;
    static double s_frameStartMs;
    static double s_windowStartMs;
    static double s_accumulatedFrameMs;
    static int s_accumulatedFrames;

    static std::vector<Section> s_sections;
    static std::vector<Section> s_accumulated;
    static std::vector<OpenSection> s_open;
    static std::thread::id s_mainThread;
    static std::vector<Result> s_results;
    static std::vector<Section> s_gpuAccumulated;
    static std::vector<Result> s_gpuResults;
    static double s_frameMs;
    static double s_unaccountedMs;

    static uint32_t s_generation;
    static std::vector<Result> s_worstResults;
    static double s_worstFrameMs;
    static double s_worstTimeMs;
};

class FrameProfilerScope
{
public:
    explicit FrameProfilerScope(const char *name)
    {
        FrameProfiler::Push(name);
    }

    ~FrameProfilerScope()
    {
        FrameProfiler::Pop();
    }
};
