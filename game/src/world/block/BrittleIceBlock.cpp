#include "BrittleIceBlock.h"

#include <string>
#include <unordered_map>

#include "../../utils/Random.h"
#include "../Level.h"

struct IceTouch
{
    Level *level;
    int64_t gameTime;
};

// This holds the last tick a player stood on each cracking block since players are not level entities
static std::unordered_map<BlockPos, IceTouch> s_iceTouches;
static Random s_brittleIceRandom;

BrittleIceBlock::BrittleIceBlock() : Block(true, "assets/texture/block/brittle_ice_0.png")
{
    m_stageProperty = AddProperty("stage", kStageCount);
    for (int i = 0; i < kStageCount; i++)
    {
        AddTextureVariant("assets/texture/block/brittle_ice_" + std::to_string(i) + ".png");
    }

    SetTransparent(true);
    SetTranslucent(true);
    SetSlipperiness(0.98f);
}

void BrittleIceBlock::OnSteppedOn(Level *level, const BlockPos &pos)
{
    // A touch older than one stage means its scheduled tick already ran, so we have to schedule a new one
    auto touch        = s_iceTouches.find(pos);
    bool scheduled    = touch != s_iceTouches.end() && touch->second.level == level && level->GetGameTime() - touch->second.gameTime <= kStageTicks;
    s_iceTouches[pos] = IceTouch{level, level->GetGameTime()};
    if (!scheduled)
    {
        level->ScheduleBlockForTick(pos, kStageTicks, 0);
    }
}

void BrittleIceBlock::Tick(Level *level, const BlockPos &pos)
{
    auto touch = s_iceTouches.find(pos);
    if (touch == s_iceTouches.end() || touch->second.level != level)
    {
        return;
    }

    if (level->GetGameTime() - touch->second.gameTime > 2)
    {
        s_iceTouches.erase(touch);
        return;
    }

    glm::dvec3 center(pos.x + 0.5, pos.y + 1.0, pos.z + 0.5);
    uint16_t data = level->GetBlockData(pos);
    int stage     = GetPropertyValue(data, m_stageProperty);
    if (stage == kStageCount - 1)
    {
        s_iceTouches.erase(touch);
        level->SetBlock(pos, Block::ByName("air"));
        level->RecordBlockChange(pos);
        level->PlaySound("random.glass" + std::to_string(s_brittleIceRandom.NextInt(1, 3)), center, 1.0f,
                         0.9f + s_brittleIceRandom.NextFloat() * 0.2f);
        return;
    }

    level->SetBlockData(pos, WithPropertyValue(data, m_stageProperty, stage + 1));
    level->RecordBlockChange(pos);
    level->PlaySound("random.glass" + std::to_string(s_brittleIceRandom.NextInt(1, 3)), center, 0.3f + stage * 0.15f,
                     1.8f - stage * 0.2f + s_brittleIceRandom.NextFloat() * 0.1f);
    level->ScheduleBlockForTick(pos, kStageTicks, 0);
}

Block::UVRect BrittleIceBlock::GetAtlasUVRect(Direction *direction, uint16_t data) const
{
    (void) direction;

    return GetTextureVariantAtlasUVRect(GetPropertyValue(data, m_stageProperty));
}
