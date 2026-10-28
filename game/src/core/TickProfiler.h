#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../entity/EntityType.h"

class TickProfiler
{
public:
    struct Result
    {
        std::string name;
        double msPerTick;
        double callsPerTick;
    };

    TickProfiler();

    static TickProfiler *GetActive();

    void BeginTick();
    void EndTick();
    void Push(const char *name);
    void Pop();
    void AddEntityTime(eENTITY_TYPE type, double ms);
    double Now() const;

    const std::vector<Result> &GetSectionResults() const;
    const std::vector<Result> &GetEntityResults() const;
    double GetAverageTickMs() const;
    double GetWorstTickMs() const;

private:
    struct Section
    {
        const char *name;
        double totalMs;
        int calls;
    };

    struct OpenSection
    {
        size_t index;
        double startMs;
    };

    void Publish();

    static thread_local TickProfiler *s_active;

    std::vector<Section> m_sections;
    std::vector<OpenSection> m_open;
    std::vector<double> m_entityMs;
    std::vector<int> m_entityCalls;
    double m_tickStartMs;
    double m_windowTotalMs;
    double m_windowWorstMs;
    int m_windowTicks;
    std::vector<Result> m_sectionResults;
    std::vector<Result> m_entityResults;
    double m_averageTickMs;
    double m_worstTickMs;
};

class TickProfilerEntityScope
{
public:
    explicit TickProfilerEntityScope(eENTITY_TYPE type)
    {
        m_profiler = TickProfiler::GetActive();
        m_type     = type;
        m_startMs  = m_profiler ? m_profiler->Now() : 0.0;
    }

    ~TickProfilerEntityScope()
    {
        if (m_profiler)
        {
            m_profiler->AddEntityTime(m_type, m_profiler->Now() - m_startMs);
        }
    }

private:
    TickProfiler *m_profiler;
    eENTITY_TYPE m_type;
    double m_startMs;
};
