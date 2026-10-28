#include "Goal.h"

Goal::Goal()
{
    m_flags = 0;
}

Goal::~Goal()
{}

bool Goal::CanContinueToUse()
{
    return CanUse();
}

bool Goal::CanInterrupt() const
{
    return true;
}

void Goal::Start()
{}

void Goal::Stop()
{}

void Goal::Tick()
{}

uint8_t Goal::GetFlags() const
{
    return m_flags;
}
