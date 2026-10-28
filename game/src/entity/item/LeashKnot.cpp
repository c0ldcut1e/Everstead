#include "LeashKnot.h"

#include <cmath>
#include <memory>
#include <vector>

#include "../../world/Level.h"
#include "../../world/block/Block.h"
#include "../../world/block/FenceBlock.h"
#include "../../world/block/WallBlock.h"
#include "../Mob.h"
#include "../player/GameType.h"
#include "../player/Player.h"

LeashKnot::LeashKnot(Level *level) : Entity(level)
{
    m_noGravity   = true;
    m_boundingBox = {glm::dvec3(-0.1875, 0.0, -0.1875), glm::dvec3(0.1875, 0.5, 0.1875)};
}

LeashKnot::~LeashKnot()
{}

eENTITY_TYPE LeashKnot::GetType()
{
    return s_type;
}

void LeashKnot::Tick()
{
    m_totalTickCount++;
    if (m_level->IsClientSide() || m_totalTickCount % 100 != 0)
    {
        return;
    }

    Block *block = Block::ById(m_level->GetBlockId(GetFencePos()));
    if (!dynamic_cast<FenceBlock *>(block) || dynamic_cast<WallBlock *>(block))
    {
        m_level->RemoveEntity(this);
    }
}

void LeashKnot::Interact(Player *player)
{
    if (BindPlayerMobs(player, GetFencePos()))
    {
        return;
    }

    if (GameType::HasInfiniteItems(player->GetGameType()))
    {
        const std::vector<std::unique_ptr<Entity>> &entities = m_level->GetEntities();
        for (size_t i = 0; i < entities.size(); i++)
        {
            Mob *mob = dynamic_cast<Mob *>(entities[i].get());
            if (mob && mob->GetLeashHolderId() == GetId())
            {
                mob->DropLeash(false);
            }
        }
    }

    Hurt();
}

void LeashKnot::Hurt()
{
    m_level->PlaySound(m_level->GetRandom().NextInt(0, 1) == 0 ? "leashknot.break1" : "leashknot.break2", m_position, 1.0f, 1.0f);
    m_level->RemoveEntity(this);
}

BlockPos LeashKnot::GetFencePos() const
{
    return BlockPos((int) floor(m_position.x), (int) floor(m_position.y), (int) floor(m_position.z));
}

LeashKnot *LeashKnot::GetOrCreate(Level *level, const BlockPos &pos)
{
    const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i]->GetType() == s_type && ((LeashKnot *) entities[i].get())->GetFencePos() == pos)
        {
            return (LeashKnot *) entities[i].get();
        }
    }

    std::unique_ptr<LeashKnot> knot(new LeashKnot(level));
    LeashKnot *result = knot.get();
    knot->SetPosition(glm::dvec3(pos.x + 0.5, pos.y + 0.375, pos.z + 0.5));
    level->AddEntity(std::move(knot));
    level->PlaySound("leashknot.place1", result->GetPosition(), 1.0f, 1.0f);
    return result;
}

bool LeashKnot::BindPlayerMobs(Player *player, const BlockPos &pos)
{
    Level *level = player->GetLevel();
    glm::dvec3 center(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5);
    std::vector<Mob *> mobs;
    const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        Mob *mob = dynamic_cast<Mob *>(entities[i].get());
        if (mob && mob->GetLeashHolderId() == player->GetId() && std::abs(mob->GetPosition().x - center.x) <= 7.0 &&
            std::abs(mob->GetPosition().y - center.y) <= 7.0 && std::abs(mob->GetPosition().z - center.z) <= 7.0)
        {
            mobs.push_back(mob);
        }
    }

    if (mobs.empty())
    {
        return false;
    }

    LeashKnot *knot = GetOrCreate(level, pos);
    for (size_t i = 0; i < mobs.size(); i++)
    {
        mobs[i]->SetLeashHolder(knot);
    }

    return true;
}
