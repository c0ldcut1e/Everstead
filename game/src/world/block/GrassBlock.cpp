#include "GrassBlock.h"

#include "../../utils/Random.h"
#include "../Level.h"
#include "../chunk/Chunk.h"

GrassBlock::GrassBlock(const std::string &texturePath) : Block(true, texturePath)
{
    SetRandomTicking(true);
    m_snowySide = AddTextureVariant("assets/texture/block/grass_side_snowed.png");
}

Block::UVRect GrassBlock::GetSnowySideAtlasUVRect() const
{
    return GetTextureVariantAtlasUVRect(m_snowySide);
}

bool GrassBlock::IsSmothered(Level *level, const BlockPos &pos)
{
    Block *above = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y + 1, pos.z)));
    return above && (above->BlocksLight() || above->IsLiquid() || above->GetLightOpacity() > 2);
}

void GrassBlock::RandomTick(Level *level, const BlockPos &pos)
{
    static Block *dirt = Block::ByName("dirt");

    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    if (abovePos.y >= Chunk::s_sizeY)
    {
        return;
    }

    int light = level->GetLightLevel(abovePos);
    if (light < kMinSurviveLight && IsSmothered(level, pos))
    {
        level->SetBlock(pos, dirt);
        level->RecordBlockChange(pos);
        return;
    }

    if (light < kMinSpreadLight || !level->IsAreaLoaded(pos, 1))
    {
        return;
    }

    for (int i = 0; i < kSpreadAttempts; i++)
    {
        uint32_t value = level->GetRandom().NextUInt();
        BlockPos target(pos.x + (int) (value % 3) - 1, pos.y + (int) ((value >> 8) % 5) - 3, pos.z + (int) ((value >> 16) % 3) - 1);
        if (target.y <= 0 || target.y + 1 >= Chunk::s_sizeY || Block::ById(level->GetBlockId(target)) != dirt)
        {
            continue;
        }

        if (level->GetLightLevel(BlockPos(target.x, target.y + 1, target.z)) >= kMinSurviveLight && !IsSmothered(level, target))
        {
            level->SetBlock(target, this);
            level->RecordBlockChange(target);
        }
    }
}
