#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "../../../utils/AABB.h"
#include "../../../utils/heap/BinaryHeap.h"
#include "Node.h"
#include "Path.h"

class Entity;
class Level;

class PathFinder
{
public:
    PathFinder(Level *level, bool avoidWater, bool canFloat);

    std::unique_ptr<Path> FindPath(Entity *entity, double x, double y, double z, float maxDist);

    enum ePATH_TYPE
    {
        ePathType_Fence    = -3,
        ePathType_Lava     = -2,
        ePathType_Water    = -1,
        ePathType_Blocked  = 0,
        ePathType_Open     = 1,
        ePathType_Walkable = 2,
    };

    static ePATH_TYPE IsFree(Level *level, Entity *entity, int x, int y, int z, const Node &size, bool avoidWater, std::vector<AABB> *boxes);

private:
    std::unique_ptr<Path> FindPath(Entity *entity, Node *from, Node *to, const Node &size, float maxDist);
    int GetNeighbors(Entity *entity, Node *pos, const Node &size, Node *target, float maxDist);
    Node *GetNode(Entity *entity, int x, int y, int z, const Node &size, int jumpSize);
    Node *GetNode(int x, int y, int z);
    ePATH_TYPE IsFree(Entity *entity, int x, int y, int z, const Node &size);
    std::unique_ptr<Path> ReconstructPath(Node *to);

    static const int s_maxFallDistance = 4;

    struct OpenEntry
    {
        float f;
        Node *node;
    };

    struct OpenEntryCompare
    {
        bool operator()(const OpenEntry &a, const OpenEntry &b) const
        {
            return a.f > b.f;
        }
    };

    Level *m_level;
    bool m_avoidWater;
    bool m_canFloat;
    BinaryHeap<OpenEntry, OpenEntryCompare> m_openSet;
    std::unordered_map<int, std::unique_ptr<Node>> m_nodes;
    Node *m_neighbors[4];
    std::vector<AABB> m_boxes;
};
