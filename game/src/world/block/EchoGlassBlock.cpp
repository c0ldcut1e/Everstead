#include "EchoGlassBlock.h"

#include "../Level.h"

EchoGlassBlock::EchoGlassBlock(const std::string &path, const std::string &solidPath) : Block(true, path)
{
    m_solidProperty = AddProperty("hidden", 64);
    SetRenderShape(eRenderShape_BOXES);
    SetSecondaryTexture(solidPath);
}

void EchoGlassBlock::ToggleFace(Level *level, const BlockPos &pos, Direction *face) const
{
    uint16_t data = level->GetBlockData(pos);
    int solid     = GetPropertyValue(data, m_solidProperty) ^ (1 << (face->GetId() - 1));
    level->SetBlockData(pos, WithPropertyValue(data, m_solidProperty, solid));
    level->RecordBlockChange(pos);
    level->PlaySound("step.stone1", glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 0.6f, solid & (1 << (face->GetId() - 1)) ? 1.2f : 1.5f);
}

Block::UVRect EchoGlassBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    (void) box;
    *flipU = false;
    if (GetPropertyValue(data, m_solidProperty) & (1 << (direction->GetId() - 1)))
    {
        return GetSecondaryAtlasUVRect();
    }

    return GetAtlasUVRect(direction);
}
