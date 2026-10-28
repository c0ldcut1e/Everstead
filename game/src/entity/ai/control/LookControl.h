#pragma once

class Entity;
class Mob;

class LookControl
{
public:
    explicit LookControl(Mob *mob);

    void SetLookAt(Entity *target, float yawMax, float pitchMax);
    void SetLookAt(double x, double y, double z, float yawMax, float pitchMax);
    void Tick();

private:
    Mob *m_mob;
    float m_yawMax;
    float m_pitchMax;
    bool m_hasWanted;
    double m_wantedX;
    double m_wantedY;
    double m_wantedZ;
};
