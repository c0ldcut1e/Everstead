#include "PathNavigation.h"

#include <cmath>

#include "../../../world/Level.h"
#include "../../../world/block/Block.h"
#include "../../Mob.h"
#include "PathFinder.h"

#include <glm/gtx/norm.hpp>

PathNavigation::PathNavigation(Mob *mob, Level *level)
{
    m_mob               = mob;
    m_level             = level;
    m_path              = nullptr;
    m_speed             = 0.0;
    m_maxDist           = 16.0f;
    m_tick              = 0;
    m_lastStuckCheck    = 0;
    m_lastStuckCheckPos = glm::dvec3(0.0);
    m_avoidWater        = false;
    m_canFloat          = false;
}

void PathNavigation::SetAvoidWater(bool avoidWater)
{
    m_avoidWater = avoidWater;
}

bool PathNavigation::GetAvoidWater() const
{
    return m_avoidWater;
}

void PathNavigation::SetCanFloat(bool canFloat)
{
    m_canFloat = canFloat;
}

void PathNavigation::SetSpeed(double speed)
{
    m_speed = speed;
}

std::unique_ptr<Path> PathNavigation::CreatePath(double x, double y, double z)
{
    if (!CanUpdatePath())
    {
        return nullptr;
    }

    PathFinder finder(m_level, m_avoidWater, m_canFloat);
    return finder.FindPath(m_mob, x, y, z, m_maxDist);
}

bool PathNavigation::MoveTo(double x, double y, double z, double speed)
{
    return MoveTo(CreatePath(floor(x), (int) y, floor(z)), speed);
}

bool PathNavigation::MoveTo(Entity *target, double speed)
{
    const glm::dvec3 &position       = target->GetPosition();
    std::unique_ptr<Path> path = CreatePath(position.x, target->GetAABB().GetMin().y, position.z);
    if (!path)
    {
        return false;
    }

    return MoveTo(std::move(path), speed);
}

bool PathNavigation::MoveTo(std::unique_ptr<Path> path, double speed)
{
    if (!path)
    {
        m_path = nullptr;
        return false;
    }

    if (!path->SameAs(m_path.get()))
    {
        m_path = std::move(path);
    }

    if (m_path->GetSize() == 0)
    {
        return false;
    }

    m_speed             = speed;
    m_lastStuckCheck    = m_tick;
    m_lastStuckCheckPos = GetMobPosition();
    return true;
}

void PathNavigation::Tick()
{
    m_tick++;
    if (IsDone())
    {
        return;
    }

    if (CanUpdatePath())
    {
        UpdatePath();
    }

    if (IsDone())
    {
        return;
    }

    glm::dvec3 target = m_path->GetCurrentEntityPosition(m_mob);
    m_mob->GetMoveControl()->SetWantedPosition(target.x, target.y, target.z, m_speed);
}

void PathNavigation::UpdatePath()
{
    glm::dvec3 mobPos = GetMobPosition();

    size_t firstElevation = m_path->GetSize();
    for (size_t i = m_path->GetIndex(); i < m_path->GetSize(); i++)
    {
        if (m_path->Get(i).y != (int) mobPos.y)
        {
            firstElevation = i;
            break;
        }
    }

    const AABB &box         = m_mob->GetBoundingBox();
    double width            = box.GetMax().x - box.GetMin().x;
    double waypointRadiusSq = width * width;
    for (size_t i = m_path->GetIndex(); i < firstElevation; i++)
    {
        if (glm::distance2(mobPos, m_path->GetEntityPosition(m_mob, i)) < waypointRadiusSq)
        {
            m_path->SetIndex(i + 1);
        }
    }

    int sx = (int) ceil(width);
    int sy = (int) (box.GetMax().y - box.GetMin().y) + 1;
    for (size_t i = firstElevation; i > m_path->GetIndex(); i--)
    {
        if (CanMoveDirectly(mobPos, m_path->GetEntityPosition(m_mob, i - 1), sx, sy, sx))
        {
            m_path->SetIndex(i - 1);
            break;
        }
    }

    if (m_tick - m_lastStuckCheck > 100)
    {
        if (glm::distance2(mobPos, m_lastStuckCheckPos) < 1.5 * 1.5)
        {
            Stop();
        }

        m_lastStuckCheck    = m_tick;
        m_lastStuckCheckPos = mobPos;
    }
}

bool PathNavigation::IsDone() const
{
    return !m_path || m_path->IsDone();
}

void PathNavigation::Stop()
{
    m_path = nullptr;
}

glm::dvec3 PathNavigation::GetMobPosition() const
{
    const glm::dvec3 &position = m_mob->GetPosition();
    return glm::dvec3(position.x, (double) GetSurfaceY(), position.z);
}

int PathNavigation::GetSurfaceY() const
{
    double minY = m_mob->GetAABB().GetMin().y;
    if (!m_mob->IsInWater() || !m_canFloat)
    {
        return (int) (minY + 0.5);
    }

    const glm::dvec3 &position = m_mob->GetPosition();
    int x                = (int) floor(position.x);
    int z                = (int) floor(position.z);
    int surface          = (int) minY;
    for (int steps = 0; Block::ById(m_level->GetBlockId(BlockPos(x, surface, z)))->IsLiquid(); steps++)
    {
        if (steps >= 16)
        {
            return (int) minY;
        }

        surface++;
    }

    return surface;
}

bool PathNavigation::CanUpdatePath() const
{
    return m_mob->IsOnGround() || (m_canFloat && (m_mob->IsInWater() || m_mob->IsInLava()));
}

bool PathNavigation::CanMoveDirectly(const glm::dvec3 &start, const glm::dvec3 &stop, int sx, int sy, int sz) const
{
    int gridX = (int) floor(start.x);
    int gridZ = (int) floor(start.z);

    double dirX   = stop.x - start.x;
    double dirZ   = stop.z - start.z;
    double distSq = dirX * dirX + dirZ * dirZ;
    if (distSq < 0.00000001)
    {
        return false;
    }

    double nf = 1.0 / sqrt(distSq);
    dirX *= nf;
    dirZ *= nf;

    if (!CanWalkOn(gridX, (int) start.y, gridZ, sx + 2, sy, sz + 2, start, dirX, dirZ))
    {
        return false;
    }

    double deltaX = 1.0 / fabs(dirX);
    double deltaZ = 1.0 / fabs(dirZ);
    double maxX   = gridX - start.x;
    double maxZ   = gridZ - start.z;
    if (dirX >= 0.0)
    {
        maxX += 1.0;
    }

    if (dirZ >= 0.0)
    {
        maxZ += 1.0;
    }

    maxX /= dirX;
    maxZ /= dirZ;

    int stepX       = dirX < 0.0 ? -1 : 1;
    int stepZ       = dirZ < 0.0 ? -1 : 1;
    int goalX       = (int) floor(stop.x);
    int goalZ       = (int) floor(stop.z);
    int currentDirX = goalX - gridX;
    int currentDirZ = goalZ - gridZ;
    while (currentDirX * stepX > 0 || currentDirZ * stepZ > 0)
    {
        if (maxX < maxZ)
        {
            maxX += deltaX;
            gridX += stepX;
            currentDirX = goalX - gridX;
        }
        else
        {
            maxZ += deltaZ;
            gridZ += stepZ;
            currentDirZ = goalZ - gridZ;
        }

        if (!CanWalkOn(gridX, (int) start.y, gridZ, sx, sy, sz, start, dirX, dirZ))
        {
            return false;
        }
    }

    return true;
}

bool PathNavigation::CanWalkOn(int x, int y, int z, int sx, int sy, int sz, const glm::dvec3 &start, double goalDirX, double goalDirZ) const
{
    int startX = x - sx / 2;
    int startZ = z - sz / 2;
    if (!CanWalkAbove(startX, y, startZ, sx, sy, sz, start, goalDirX, goalDirZ))
    {
        return false;
    }

    for (int xx = startX; xx < startX + sx; xx++)
    {
        for (int zz = startZ; zz < startZ + sz; zz++)
        {
            double dirX = xx + 0.5 - start.x;
            double dirZ = zz + 0.5 - start.z;
            if (dirX * goalDirX + dirZ * goalDirZ < 0.0)
            {
                continue;
            }

            uint32_t id = m_level->GetBlockId(BlockPos(xx, y - 1, zz));
            if (id == 0)
            {
                return false;
            }

            Block *block = Block::ById(id);
            if (block->IsLava() || (block->IsLiquid() && !m_mob->IsInWater()))
            {
                return false;
            }
        }
    }

    return true;
}

bool PathNavigation::CanWalkAbove(int startX, int startY, int startZ, int sx, int sy, int sz, const glm::dvec3 &start, double goalDirX,
                                  double goalDirZ) const
{
    for (int xx = startX; xx < startX + sx; xx++)
    {
        for (int yy = startY; yy < startY + sy; yy++)
        {
            for (int zz = startZ; zz < startZ + sz; zz++)
            {
                double dirX = xx + 0.5 - start.x;
                double dirZ = zz + 0.5 - start.z;
                if (dirX * goalDirX + dirZ * goalDirZ < 0.0)
                {
                    continue;
                }

                if (Block::ById(m_level->GetBlockId(BlockPos(xx, yy, zz)))->IsCollidable())
                {
                    return false;
                }
            }
        }
    }

    return true;
}
