#include "TickRateManager.h"

#include "../entity/player/Player.h"

TickRateManager::TickRateManager()
{
    m_tickRate         = 20.0f;
    m_frozen           = false;
    m_frozenTicksToRun = 0;
    m_runGameElements  = true;
}

void TickRateManager::Tick()
{
    m_runGameElements = !m_frozen || m_frozenTicksToRun > 0;
    if (m_frozenTicksToRun > 0)
    {
        m_frozenTicksToRun--;
    }
}

void TickRateManager::SetTickRate(float tickRate)
{
    m_tickRate = tickRate;
}

float TickRateManager::GetTickRate() const
{
    return m_tickRate;
}

std::chrono::nanoseconds TickRateManager::GetTickInterval() const
{
    return std::chrono::nanoseconds((int64_t) (1000000000.0 / m_tickRate));
}

void TickRateManager::SetFrozen(bool frozen)
{
    m_frozen = frozen;
}

bool TickRateManager::IsFrozen() const
{
    return m_frozen;
}

bool TickRateManager::RunsNormally() const
{
    return m_runGameElements;
}

bool TickRateManager::IsSteppingForward() const
{
    return m_frozen && m_frozenTicksToRun > 0;
}

bool TickRateManager::StepGameIfPaused(int ticks)
{
    if (!m_frozen)
    {
        return false;
    }

    m_frozenTicksToRun = ticks;
    return true;
}

bool TickRateManager::StopStepping()
{
    if (m_frozenTicksToRun <= 0)
    {
        return false;
    }

    m_frozenTicksToRun = 0;
    return true;
}

void TickRateManager::SetFrozenTicksToRun(int ticks)
{
    m_frozenTicksToRun = ticks;
}

int TickRateManager::GetFrozenTicksToRun() const
{
    return m_frozenTicksToRun;
}

bool TickRateManager::IsEntityFrozen(Entity *entity) const
{
    if (m_runGameElements || dynamic_cast<Player *>(entity))
    {
        return false;
    }

    const std::vector<Entity *> &passengers = entity->GetPassengers();
    for (size_t i = 0; i < passengers.size(); i++)
    {
        if (dynamic_cast<Player *>(passengers[i]))
        {
            return false;
        }
    }

    return true;
}
