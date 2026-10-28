#pragma once

class Mob;

class BodyControl
{
public:
    explicit BodyControl(Mob *mob);

    void Tick();

private:
    static float Clamp(float clampTo, float clampFrom, float clampAngle);

    Mob *m_mob;
    int m_timeStill;
    float m_lastHeadYaw;
};
