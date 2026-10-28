#include "JumpControl.h"

#include "../../Mob.h"

JumpControl::JumpControl(Mob *mob)
{
    m_mob  = mob;
    m_jump = false;
}

void JumpControl::Jump()
{
    m_jump = true;
}

void JumpControl::Tick()
{
    m_mob->SetJumping(m_jump);
    m_jump = false;
}
