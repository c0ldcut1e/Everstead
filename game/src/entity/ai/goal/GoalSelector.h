#pragma once

#include <memory>
#include <vector>

#include "Goal.h"

class GoalSelector
{
public:
    GoalSelector();
    ~GoalSelector();

    void AddGoal(int priority, Goal *goal);
    void Tick();

private:
    struct Entry
    {
        int priority;
        std::unique_ptr<Goal> goal;
        bool running;
    };

    bool CanUseInSystem(size_t index) const;

    std::vector<Entry> m_goals;
    std::vector<size_t> m_toStart;
    int m_tickCount;
};
