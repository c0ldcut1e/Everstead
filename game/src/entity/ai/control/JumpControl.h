#pragma once

class Mob;

class JumpControl
{
public:
    explicit JumpControl(Mob *mob);

    void Jump();
    void Tick();

private:
    Mob *m_mob;
    bool m_jump;
};
