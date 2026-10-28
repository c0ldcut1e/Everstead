#pragma once

#include <chrono>

class Entity;

class TickRateManager
{
public:
    TickRateManager();

    void Tick();
    void SetTickRate(float tickRate);
    float GetTickRate() const;
    std::chrono::nanoseconds GetTickInterval() const;
    void SetFrozen(bool frozen);
    bool IsFrozen() const;
    bool RunsNormally() const;
    bool IsSteppingForward() const;
    bool StepGameIfPaused(int ticks);
    bool StopStepping();
    void SetFrozenTicksToRun(int ticks);
    int GetFrozenTicksToRun() const;
    bool IsEntityFrozen(Entity *entity) const;

private:
    float m_tickRate;
    bool m_frozen;
    int m_frozenTicksToRun;
    bool m_runGameElements;
};
