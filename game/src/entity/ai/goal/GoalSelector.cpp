#include "GoalSelector.h"

GoalSelector::GoalSelector()
{
    m_tickCount = 0;
}

GoalSelector::~GoalSelector()
{}

void GoalSelector::AddGoal(int priority, Goal *goal)
{
    m_goals.push_back({priority, std::unique_ptr<Goal>(goal), false});
}

void GoalSelector::Tick()
{
    const int newGoalRate = 3;

    m_toStart.clear();
    if (m_tickCount++ % newGoalRate == 0)
    {
        for (size_t i = 0; i < m_goals.size(); i++)
        {
            Entry &entry = m_goals[i];
            if (entry.running)
            {
                if (CanUseInSystem(i) && entry.goal->CanContinueToUse())
                {
                    continue;
                }

                entry.goal->Stop();
                entry.running = false;
            }

            if (!CanUseInSystem(i) || !entry.goal->CanUse())
            {
                continue;
            }

            m_toStart.push_back(i);
            entry.running = true;
        }
    }
    else
    {
        for (size_t i = 0; i < m_goals.size(); i++)
        {
            Entry &entry = m_goals[i];
            if (entry.running && !entry.goal->CanContinueToUse())
            {
                entry.goal->Stop();
                entry.running = false;
            }
        }
    }

    for (size_t i = 0; i < m_toStart.size(); i++)
    {
        m_goals[m_toStart[i]].goal->Start();
    }

    for (size_t i = 0; i < m_goals.size(); i++)
    {
        if (m_goals[i].running)
        {
            m_goals[i].goal->Tick();
        }
    }
}

bool GoalSelector::CanUseInSystem(size_t index) const
{
    const Entry &entry = m_goals[index];
    for (size_t i = 0; i < m_goals.size(); i++)
    {
        const Entry &other = m_goals[i];
        if (i == index || !other.running)
        {
            continue;
        }

        if (entry.priority >= other.priority)
        {
            if ((entry.goal->GetFlags() & other.goal->GetFlags()) != 0)
            {
                return false;
            }
        }
        else if (!other.goal->CanInterrupt())
        {
            return false;
        }
    }

    return true;
}
