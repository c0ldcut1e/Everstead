#include "PathFinder.h"

#include <algorithm>
#include <cmath>

#include "../../../world/Level.h"
#include "../../../world/block/Block.h"
#include "../../../world/chunk/Chunk.h"
#include "../../Entity.h"

PathFinder::PathFinder(Level *level, bool avoidWater, bool canFloat)
{
    m_level      = level;
    m_avoidWater = avoidWater;
    m_canFloat   = canFloat;
    for (int i = 0; i < 4; i++)
    {
        m_neighbors[i] = nullptr;
    }
}

std::unique_ptr<Path> PathFinder::FindPath(Entity *entity, double x, double y, double z, float maxDist)
{
    m_openSet.Clear();
    m_nodes.clear();

    const AABB &aabb = entity->GetAABB();
    int startY       = (int) (aabb.GetMin().y + 0.5);
    if (m_canFloat && entity->IsInWater())
    {
        startY       = (int) aabb.GetMin().y;
        Block *block = Block::ById(m_level->GetBlockId(BlockPos((int) floor(entity->GetPosition().x), startY, (int) floor(entity->GetPosition().z))));
        while (block->IsLiquid() && !block->IsLava())
        {
            startY++;
            block = Block::ById(m_level->GetBlockId(BlockPos((int) floor(entity->GetPosition().x), startY, (int) floor(entity->GetPosition().z))));
        }
    }

    const AABB &box = entity->GetBoundingBox();
    double width    = box.GetMax().x - box.GetMin().x;
    double height   = box.GetMax().y - box.GetMin().y;
    Node *from      = GetNode((int) floor(aabb.GetMin().x), startY, (int) floor(aabb.GetMin().z));
    Node *to        = GetNode((int) floor(x - width / 2.0), (int) floor(y), (int) floor(z - width / 2.0));
    Node size((int) floor(width + 1.0), (int) floor(height + 1.0), (int) floor(width + 1.0));
    return FindPath(entity, from, to, size, maxDist);
}

std::unique_ptr<Path> PathFinder::FindPath(Entity *entity, Node *from, Node *to, const Node &size, float maxDist)
{
    from->g    = 0.0f;
    from->h    = from->DistanceTo(to);
    from->f    = from->h;
    from->open = true;
    m_openSet.Clear();
    m_openSet.Push({from->f, from});

    Node *closest = from;
    while (!m_openSet.Empty())
    {
        OpenEntry entry = m_openSet.Top();
        m_openSet.Pop();
        Node *node = entry.node;
        if (node->closed || entry.f != node->f)
        {
            continue;
        }

        if (node == to)
        {
            return ReconstructPath(to);
        }

        if (node->DistanceTo(to) < closest->DistanceTo(to))
        {
            closest = node;
        }

        node->open   = false;
        node->closed = true;
        int count    = GetNeighbors(entity, node, size, to, maxDist);
        for (int i = 0; i < count; i++)
        {
            Node *neighbor = m_neighbors[i];
            float g        = node->g + node->DistanceTo(neighbor);
            if (neighbor->open && g >= neighbor->g)
            {
                continue;
            }

            neighbor->cameFrom = node;
            neighbor->g        = g;
            neighbor->h        = neighbor->DistanceTo(to);
            neighbor->f        = neighbor->g + neighbor->h;
            neighbor->open     = true;
            m_openSet.Push({neighbor->f, neighbor});
        }
    }

    if (closest == from)
    {
        return nullptr;
    }

    return ReconstructPath(closest);
}

int PathFinder::GetNeighbors(Entity *entity, Node *pos, const Node &size, Node *target, float maxDist)
{
    int jumpSize        = IsFree(entity, pos->x, pos->y + 1, pos->z, size) == ePathType_Open ? 1 : 0;
    Node *candidates[4] = {
            GetNode(entity, pos->x, pos->y, pos->z + 1, size, jumpSize),
            GetNode(entity, pos->x - 1, pos->y, pos->z, size, jumpSize),
            GetNode(entity, pos->x + 1, pos->y, pos->z, size, jumpSize),
            GetNode(entity, pos->x, pos->y, pos->z - 1, size, jumpSize),
    };

    int count = 0;
    for (int i = 0; i < 4; i++)
    {
        if (candidates[i] && !candidates[i]->closed && candidates[i]->DistanceTo(target) < maxDist)
        {
            m_neighbors[count++] = candidates[i];
        }
    }

    return count;
}

Node *PathFinder::GetNode(Entity *entity, int x, int y, int z, const Node &size, int jumpSize)
{
    ePATH_TYPE type = IsFree(entity, x, y, z, size);
    if (type == ePathType_Walkable)
    {
        return GetNode(x, y, z);
    }

    Node *best = nullptr;
    if (type == ePathType_Open)
    {
        best = GetNode(x, y, z);
    }

    if (!best && jumpSize > 0 && type != ePathType_Fence && IsFree(entity, x, y + jumpSize, z, size) == ePathType_Open)
    {
        y += jumpSize;
        best = GetNode(x, y, z);
    }

    if (!best)
    {
        return nullptr;
    }

    int drop = 0;
    while (y > 0)
    {
        ePATH_TYPE below = IsFree(entity, x, y - 1, z, size);
        if (m_avoidWater && below == ePathType_Water)
        {
            return nullptr;
        }

        if (below != ePathType_Open)
        {
            if (below == ePathType_Lava)
            {
                return nullptr;
            }

            break;
        }

        if (drop++ >= s_maxFallDistance)
        {
            return nullptr;
        }

        y--;
        if (y > 0)
        {
            best = GetNode(x, y, z);
        }
    }

    return best;
}

Node *PathFinder::GetNode(int x, int y, int z)
{
    std::unique_ptr<Node> &node = m_nodes[Node::CreateHash(x, y, z)];
    if (!node)
    {
        node.reset(new Node(x, y, z));
    }

    return node.get();
}

PathFinder::ePATH_TYPE PathFinder::IsFree(Entity *entity, int x, int y, int z, const Node &size)
{
    return IsFree(m_level, entity, x, y, z, size, m_avoidWater, &m_boxes);
}

PathFinder::ePATH_TYPE PathFinder::IsFree(Level *level, Entity *entity, int x, int y, int z, const Node &size, bool avoidWater,
                                          std::vector<AABB> *boxes)
{
    bool walkable = false;
    for (int xx = x; xx < x + size.x; xx++)
    {
        for (int yy = y; yy < y + size.y; yy++)
        {
            for (int zz = z; zz < z + size.z; zz++)
            {
                if (yy < 0 || yy >= Chunk::s_sizeY)
                {
                    continue;
                }

                BlockPos pos(xx, yy, zz);
                Block *block = Block::ById(level->GetBlockId(pos));
                if (block->IsLava())
                {
                    if (entity->IsInLava())
                    {
                        continue;
                    }

                    return ePathType_Lava;
                }

                if (block->IsLiquid())
                {
                    if (avoidWater)
                    {
                        return ePathType_Water;
                    }

                    walkable = true;
                    continue;
                }

                if (!block->IsCollidable())
                {
                    continue;
                }

                boxes->clear();
                block->GetCollisionBoxes(level->GetBlockData(pos), boxes);
                if (boxes->empty())
                {
                    continue;
                }

                for (size_t i = 0; i < boxes->size(); i++)
                {
                    if ((*boxes)[i].GetMax().y > 1.0)
                    {
                        return ePathType_Fence;
                    }
                }

                return ePathType_Blocked;
            }
        }
    }

    return walkable ? ePathType_Walkable : ePathType_Open;
}

std::unique_ptr<Path> PathFinder::ReconstructPath(Node *to)
{
    std::vector<BlockPos> points;
    for (Node *node = to; node; node = node->cameFrom)
    {
        points.push_back(BlockPos(node->x, node->y, node->z));
    }

    std::reverse(points.begin(), points.end());
    return std::unique_ptr<Path>(new Path(points));
}
