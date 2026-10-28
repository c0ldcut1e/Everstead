#pragma once

#include <cstddef>
#include <vector>

#include "../../../world/block/BlockPos.h"

#include <glm/vec3.hpp>

class Entity;

class Path
{
public:
    explicit Path(const std::vector<BlockPos> &points);

    void Next();
    bool IsDone() const;
    const BlockPos &GetLast() const;
    const BlockPos &Get(size_t index) const;
    size_t GetSize() const;
    void SetSize(size_t size);
    size_t GetIndex() const;
    void SetIndex(size_t index);
    glm::dvec3 GetEntityPosition(const Entity *entity, size_t index) const;
    glm::dvec3 GetCurrentEntityPosition(const Entity *entity) const;
    bool SameAs(const Path *other) const;

private:
    std::vector<BlockPos> m_points;
    size_t m_index;
};
