#include "CropBlock.h"

#include <algorithm>

#include "../Level.h"
#include "FarmlandBlock.h"

CropBlock::CropBlock(const std::string &texturePrefix, int stageCount, int maxAge, const std::string &seedName)
    : Block(false, texturePrefix + "0.png")
{
    m_ageProperty = AddProperty("age", maxAge + 1);
    m_maxAge      = maxAge;
    m_stageCount  = stageCount;
    m_seedName    = seedName;
    for (int i = 0; i < stageCount; i++)
    {
        AddTextureVariant(texturePrefix + std::to_string(i) + ".png");
    }

    SetTransparent(true);
    SetCollidable(false);
    SetRenderShape(eRenderShape_BOXES);
    SetFlatItem(true);
    SetRandomTicking(true);
    SetNeedsSupportBelow(true);
    SetBreakSound("dig.grass");
    SetAABB(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(1.0, 0.25, 1.0)));
}

float CropBlock::GetGrowthSpeed(Level *level, const BlockPos &pos, const Block *crop)
{
    float speed = 1.0f;
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dz = -1; dz <= 1; dz++)
        {
            BlockPos below(pos.x + dx, pos.y - 1, pos.z + dz);
            FarmlandBlock *farmland = dynamic_cast<FarmlandBlock *>(Block::ById(level->GetBlockId(below)));
            if (!farmland)
            {
                continue;
            }

            float bonus = farmland->GetPropertyValue(level->GetBlockData(below), farmland->FindProperty("moisture")) > 0 ? 3.0f : 1.0f;
            speed += dx == 0 && dz == 0 ? bonus : bonus / 4.0f;
        }
    }

    bool west     = Block::ById(level->GetBlockId(BlockPos(pos.x - 1, pos.y, pos.z))) == crop;
    bool east     = Block::ById(level->GetBlockId(BlockPos(pos.x + 1, pos.y, pos.z))) == crop;
    bool north    = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y, pos.z - 1))) == crop;
    bool south    = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y, pos.z + 1))) == crop;
    bool diagonal = Block::ById(level->GetBlockId(BlockPos(pos.x - 1, pos.y, pos.z - 1))) == crop ||
                    Block::ById(level->GetBlockId(BlockPos(pos.x + 1, pos.y, pos.z - 1))) == crop ||
                    Block::ById(level->GetBlockId(BlockPos(pos.x + 1, pos.y, pos.z + 1))) == crop ||
                    Block::ById(level->GetBlockId(BlockPos(pos.x - 1, pos.y, pos.z + 1))) == crop;
    if (diagonal || ((west || east) && (north || south)))
    {
        speed /= 2.0f;
    }

    return speed;
}

void CropBlock::RandomTick(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide() || level->GetRawBrightness(BlockPos(pos.x, pos.y + 1, pos.z)) < 9)
    {
        return;
    }

    uint16_t data = level->GetBlockData(pos);
    int age       = GetPropertyValue(data, m_ageProperty);
    if (age >= m_maxAge || (m_seedName == "beetroot_seeds" && level->GetRandom().NextInt(0, 2) == 0))
    {
        return;
    }

    if (level->GetRandom().NextInt(0, (int) (25.0f / GetGrowthSpeed(level, pos, this))) == 0)
    {
        level->SetBlockData(pos, WithPropertyValue(data, m_ageProperty, age + 1));
        level->RecordBlockChange(pos);
    }
}

bool CropBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;

    // Generated crops sit on grass or dirt, so only placement requires farmland
    return level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)) != 0 && (level->GetLightLevel(pos) >= 8 || level->GetSkyLightLevel(pos) == 15);
}

bool CropBlock::CanPlaceAt(Level *level, const BlockPos &pos) const
{
    return Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z))) == Block::ByName("farmland");
}

int CropBlock::GetMaxAge() const
{
    return m_maxAge;
}

uint16_t CropBlock::GetAgeData(int age) const
{
    return WithPropertyValue(0, m_ageProperty, age);
}

bool CropBlock::CanBoneMeal(uint16_t data) const
{
    return GetPropertyValue(data, m_ageProperty) < m_maxAge;
}

void CropBlock::BoneMeal(Level *level, const BlockPos &pos) const
{
    uint16_t data = level->GetBlockData(pos);
    int growth    = level->GetRandom().NextInt(2, 5);
    if (m_seedName == "beetroot_seeds")
    {
        growth /= 3;
    }

    level->SetBlockData(pos, WithPropertyValue(data, m_ageProperty, std::min(GetPropertyValue(data, m_ageProperty) + growth, m_maxAge)));
    level->RecordBlockChange(pos);
}

void CropBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;

    out->push_back(AABB(glm::dvec3(0.25, -1.0 / 16.0, 0.0), glm::dvec3(0.25, 15.0 / 16.0, 1.0)));
    out->push_back(AABB(glm::dvec3(0.75, -1.0 / 16.0, 0.0), glm::dvec3(0.75, 15.0 / 16.0, 1.0)));
    out->push_back(AABB(glm::dvec3(0.0, -1.0 / 16.0, 0.25), glm::dvec3(1.0, 15.0 / 16.0, 0.25)));
    out->push_back(AABB(glm::dvec3(0.0, -1.0 / 16.0, 0.75), glm::dvec3(1.0, 15.0 / 16.0, 0.75)));
}

Block::UVRect CropBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    (void) box;
    (void) direction;

    *flipU = false;
    return GetTextureVariantAtlasUVRect(GetStage(data));
}

int CropBlock::GetStage(uint16_t data) const
{
    int age = GetPropertyValue(data, m_ageProperty);
    if (m_stageCount == m_maxAge + 1)
    {
        return age;
    }

    constexpr int kFourStages[] = {0, 0, 1, 1, 2, 2, 2, 3};
    return kFourStages[age];
}

void CropBlock::AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const
{
    (void) data;
    (void) box;
    (void) direction;
    (void) u0;
    (void) u1;

    *v0 = 0.0f;
    *v1 = 1.0f;
}

AABB CropBlock::GetPlacedAABB(const BlockPos &pos, uint16_t data) const
{
    double height = (GetPropertyValue(data, m_ageProperty) + 1) * 2.0 / 16.0;
    return AABB(glm::dvec3(pos.x, pos.y, pos.z), glm::dvec3(pos.x + 1.0, pos.y + height, pos.z + 1.0));
}

bool CropBlock::HasDirectionalShade() const
{
    return false;
}
