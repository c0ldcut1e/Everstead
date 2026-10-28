#include "PressurePlateBlock.h"

#include "../../entity/Entity.h"
#include "../Level.h"

#include <unordered_map>

// This holds the last tick something stood on each pressed plate since players are not level entities
struct PlateTouch
{
    Level *level;
    int64_t gameTime;
};

static std::unordered_map<BlockPos, PlateTouch> s_plateTouches;

PressurePlateBlock::PressurePlateBlock(const std::string &texturePath) : SwitchBlock(texturePath)
{
    SetBlocksLiquid(true);
    m_poweredProperty = AddProperty("powered", 2);
    SetTransparent(true);
    SetCollidable(false);
    SetRenderShape(eRenderShape_BOXES);
    SetNeedsSupportBelow(true);
}

void PressurePlateBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    const double p = 1.0 / 16.0;
    double height  = IsPowered(data) ? 0.5 * p : p;
    out->push_back(AABB(glm::dvec3(p, 0.0, p), glm::dvec3(15.0 * p, height, 15.0 * p)));
}

bool PressurePlateBlock::HasEntityOnTop(Level *level, const BlockPos &pos) const
{
    auto touch = s_plateTouches.find(pos);
    if (touch != s_plateTouches.end() && touch->second.level == level && level->GetGameTime() - touch->second.gameTime < kCheckTicks)
    {
        return true;
    }

    const double p = 1.0 / 16.0;
    AABB area(glm::dvec3((double) pos.x + p, (double) pos.y, (double) pos.z + p),
              glm::dvec3((double) pos.x + 15.0 * p, (double) pos.y + 0.25, (double) pos.z + 15.0 * p));
    const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i] && entities[i]->GetAABB().Intersects(area))
        {
            return true;
        }
    }

    return false;
}

void PressurePlateBlock::SetPressed(Level *level, const BlockPos &pos, bool pressed)
{
    SetPowered(level, pos, pressed);

    glm::dvec3 center((double) pos.x + 0.5, (double) pos.y + 0.1, (double) pos.z + 0.5);
    level->PlaySound("random.click", center, 0.3f, pressed ? 0.6f : 0.5f);
}

void PressurePlateBlock::OnEntityInside(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide())
    {
        return;
    }

    s_plateTouches[pos] = PlateTouch{level, level->GetGameTime()};
    if (IsPowered(level->GetBlockData(pos)))
    {
        return;
    }

    SetPressed(level, pos, true);
    level->ScheduleBlockForTick(pos, kCheckTicks, 0);
}

void PressurePlateBlock::Tick(Level *level, const BlockPos &pos)
{
    if (!IsPowered(level->GetBlockData(pos)))
    {
        return;
    }

    if (HasEntityOnTop(level, pos))
    {
        level->ScheduleBlockForTick(pos, kCheckTicks, 0);
        return;
    }

    s_plateTouches.erase(pos);
    SetPressed(level, pos, false);
}
