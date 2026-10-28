#pragma once

#include <memory>

#include "Path.h"

#include <glm/vec3.hpp>

class Entity;
class Level;
class Mob;

class PathNavigation
{
public:
    PathNavigation(Mob *mob, Level *level);

    void SetAvoidWater(bool avoidWater);
    bool GetAvoidWater() const;
    void SetCanFloat(bool canFloat);
    void SetSpeed(double speed);
    bool MoveTo(double x, double y, double z, double speed);
    bool MoveTo(Entity *target, double speed);
    void Tick();
    bool IsDone() const;
    void Stop();

private:
    bool MoveTo(std::unique_ptr<Path> path, double speed);
    std::unique_ptr<Path> CreatePath(double x, double y, double z);
    void UpdatePath();
    glm::dvec3 GetMobPosition() const;
    int GetSurfaceY() const;
    bool CanUpdatePath() const;
    bool CanMoveDirectly(const glm::dvec3 &start, const glm::dvec3 &stop, int sx, int sy, int sz) const;
    bool CanWalkOn(int x, int y, int z, int sx, int sy, int sz, const glm::dvec3 &start, double goalDirX, double goalDirZ) const;
    bool CanWalkAbove(int startX, int startY, int startZ, int sx, int sy, int sz, const glm::dvec3 &start, double goalDirX, double goalDirZ) const;

    Mob *m_mob;
    Level *m_level;
    std::unique_ptr<Path> m_path;
    double m_speed;
    float m_maxDist;
    int m_tick;
    int m_lastStuckCheck;
    glm::dvec3 m_lastStuckCheckPos;
    bool m_avoidWater;
    bool m_canFloat;
};
