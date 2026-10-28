#include "TickProfiler.h"

#include <algorithm>
#include <chrono>

#include "../entity/EntityFactory.h"

static bool CompareResults(const TickProfiler::Result &a, const TickProfiler::Result &b)
{
    return a.msPerTick > b.msPerTick;
}

thread_local TickProfiler *TickProfiler::s_active = nullptr;

TickProfiler::TickProfiler()
{
    m_sections      = {};
    m_open          = {};
    m_entityMs      = {};
    m_entityCalls   = {};
    m_tickStartMs   = 0.0;
    m_windowTotalMs = 0.0;
    m_windowWorstMs = 0.0;
    m_windowTicks   = 0;
    m_averageTickMs = 0.0;
    m_worstTickMs   = 0.0;
}

TickProfiler *TickProfiler::GetActive()
{
    return s_active;
}

void TickProfiler::BeginTick()
{
    s_active      = this;
    m_tickStartMs = Now();
}

void TickProfiler::EndTick()
{
    s_active      = nullptr;
    double tickMs = Now() - m_tickStartMs;
    m_windowTotalMs += tickMs;
    m_windowWorstMs = std::max(m_windowWorstMs, tickMs);
    m_windowTicks++;
    if (m_windowTicks >= 100)
    {
        Publish();
    }
}

void TickProfiler::Push(const char *name)
{
    size_t index = m_sections.size();
    for (size_t i = 0; i < m_sections.size(); i++)
    {
        if (m_sections[i].name == name)
        {
            index = i;
            break;
        }
    }

    if (index == m_sections.size())
    {
        m_sections.push_back({name, 0.0, 0});
    }

    m_open.push_back({index, Now()});
}

void TickProfiler::Pop()
{
    OpenSection open = m_open.back();
    m_open.pop_back();
    m_sections[open.index].totalMs += Now() - open.startMs;
    m_sections[open.index].calls++;
}

void TickProfiler::AddEntityTime(eENTITY_TYPE type, double ms)
{
    if (type >= m_entityMs.size())
    {
        m_entityMs.resize(type + 1, 0.0);
        m_entityCalls.resize(type + 1, 0);
    }

    m_entityMs[type] += ms;
    m_entityCalls[type]++;
}

double TickProfiler::Now() const
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

void TickProfiler::Publish()
{
    double ticks = (double) m_windowTicks;
    m_sectionResults.clear();
    for (size_t i = 0; i < m_sections.size(); i++)
    {
        m_sectionResults.push_back({m_sections[i].name, m_sections[i].totalMs / ticks, m_sections[i].calls / ticks});
        m_sections[i].totalMs = 0.0;
        m_sections[i].calls   = 0;
    }

    m_entityResults.clear();
    for (size_t i = 0; i < m_entityMs.size(); i++)
    {
        if (m_entityCalls[i] > 0)
        {
            m_entityResults.push_back(
                    {EntityFactory::GetResourceLocation((eENTITY_TYPE) i).ToString(), m_entityMs[i] / ticks, m_entityCalls[i] / ticks});
        }

        m_entityMs[i]    = 0.0;
        m_entityCalls[i] = 0;
    }

    std::sort(m_sectionResults.begin(), m_sectionResults.end(), CompareResults);
    std::sort(m_entityResults.begin(), m_entityResults.end(), CompareResults);
    m_averageTickMs = m_windowTotalMs / ticks;
    m_worstTickMs   = m_windowWorstMs;
    m_windowTotalMs = 0.0;
    m_windowWorstMs = 0.0;
    m_windowTicks   = 0;
}

const std::vector<TickProfiler::Result> &TickProfiler::GetSectionResults() const
{
    return m_sectionResults;
}

const std::vector<TickProfiler::Result> &TickProfiler::GetEntityResults() const
{
    return m_entityResults;
}

double TickProfiler::GetAverageTickMs() const
{
    return m_averageTickMs;
}

double TickProfiler::GetWorstTickMs() const
{
    return m_worstTickMs;
}
