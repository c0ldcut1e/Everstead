#pragma once

#include <cstdint>

enum eGOAL_FLAG : uint8_t
{
    eGoalFlag_Move   = 1,
    eGoalFlag_Look   = 2,
    eGoalFlag_Jump   = 4,
    eGoalFlag_Target = 8,
};

class Goal
{
public:
    Goal();
    virtual ~Goal();

    virtual bool CanUse() = 0;
    virtual bool CanContinueToUse();
    virtual bool CanInterrupt() const;
    virtual void Start();
    virtual void Stop();
    virtual void Tick();

    uint8_t GetFlags() const;

protected:
    uint8_t m_flags;
};
