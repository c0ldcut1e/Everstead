#include "Path.h"

#include "../../Entity.h"

Path::Path(const std::vector<BlockPos> &points)
{
    m_points = points;
    m_index  = 0;
}

void Path::Next()
{
    m_index++;
}

bool Path::IsDone() const
{
    return m_index >= m_points.size();
}

const BlockPos &Path::GetLast() const
{
    return m_points.back();
}

const BlockPos &Path::Get(size_t index) const
{
    return m_points[index];
}

size_t Path::GetSize() const
{
    return m_points.size();
}

void Path::SetSize(size_t size)
{
    m_points.resize(size);
}

size_t Path::GetIndex() const
{
    return m_index;
}

void Path::SetIndex(size_t index)
{
    m_index = index;
}

// We aim at the middle of the footprint the entity occupies, which is wider than one block for big mobs
glm::dvec3 Path::GetEntityPosition(const Entity *entity, size_t index) const
{
    const AABB &box = entity->GetBoundingBox();
    double offset   = (double) (int) (box.GetMax().x - box.GetMin().x + 1.0) * 0.5;
    return glm::dvec3((double) m_points[index].x + offset, (double) m_points[index].y, (double) m_points[index].z + offset);
}

glm::dvec3 Path::GetCurrentEntityPosition(const Entity *entity) const
{
    return GetEntityPosition(entity, m_index);
}

bool Path::SameAs(const Path *other) const
{
    return other && other->m_points == m_points;
}
