#pragma once

#include <glm/vec3.hpp>

class Mob;

class MoveControl
{
public:
    explicit MoveControl(Mob *mob);

    bool HasWanted() const;
    double GetSpeed() const;
    glm::dvec3 GetWantedPosition() const;
    void SetWantedPosition(double x, double y, double z, double speed);
    void Tick();

    static float RotLerp(float from, float to, float max);

    static constexpr float kMinSpeedSquared = 0.0005f * 0.0005f;

private:
    Mob *m_mob;
    double m_wantedX;
    double m_wantedY;
    double m_wantedZ;
    double m_speed;
    bool m_hasWanted;
};
