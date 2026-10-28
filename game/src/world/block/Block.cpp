#include "Block.h"

#include <algorithm>
#include <bit>
#include <cmath>

#include "../../entity/item/ExperienceOrb.h"
#include "../../entity/item/ItemEntity.h"
#include "../Level.h"
#include "../biome/Biome.h"
#include "../chunk/Chunk.h"
#include "../item/Item.h"
#include "../item/ItemInstance.h"
#include "../loot/LootTables.h"
#include "BlockRegistry.h"

thread_local Random Block::s_dropRandom;

Block::Block()
{
    m_id                          = 0;
    m_breakSound                  = "dig.stone";
    m_breakSoundVariants          = 4;
    m_foliageTint                 = false;
    m_hasFixedTint                = false;
    m_fixedTint                   = 0xFFFFFF;
    m_placeSound                  = "";
    m_placeSoundVariants          = 4;
    m_placeSoundPitch             = 0.8f;
    m_noteInstrument              = "note.harp";
    m_solid                       = false;
    m_collidable                  = false;
    m_selectable                  = true;
    m_transparent                 = false;
    m_lightOpacity                = 0;
    m_cullsSameNeighbor           = true;
    m_liquid                      = false;
    m_translucent                 = false;
    m_replaceable                 = false;
    m_randomTicking               = false;
    m_slipperiness                = 0.6f;
    m_liquidReplaceable           = false;
    m_blocksLiquid                = false;
    m_needsSupportBelow           = false;
    m_needsCenterSupport          = false;
    m_needsAttachedSupport        = false;
    m_flatItem                    = false;
    m_climbable                   = false;
    m_wallVariant                 = nullptr;
    m_aabb                        = {glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(0.0, 0.0, 0.0)};
    m_interactionAABB             = {glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(0.0, 0.0, 0.0)};
    m_hasInteractionAABB          = false;
    m_interactionAttachmentOffset = 0.0f;
    m_fireEncouragement           = 0;
    m_flammability                = 0;
    m_explosionResistance         = 6.0f;
    m_destroyTime                 = 0.0f;
    m_toolType                    = eToolType_None;
    m_requiresTool                = false;
    m_harvestLevel                = 0;
    m_lightEmission               = 0;
    m_lightR                      = 0;
    m_lightG                      = 0;
    m_lightB                      = 0;
    m_renderShape                 = eRenderShape_CUBE;
    m_entityRendered              = false;
    m_hasWallMountedTransform     = false;
    m_wallMountedTiltDegrees      = 0.0f;
    m_wallMountedInset            = 0.0f;
    m_hasTorchPlanes              = false;
    m_facingProperty              = -1;
    m_axisProperty                = -1;
    m_rotatedSideAtlasUVRect      = {0.0f, 0.0f, 1.0f, 1.0f};
    m_sideOverlayAtlasUVRect      = {0.0f, 0.0f, 1.0f, 1.0f};
    m_secondaryAtlasUVRect        = {0.0f, 0.0f, 1.0f, 1.0f};
    m_crossJitter                 = eCrossJitter_Full;
    m_crossFollowsAttachment      = false;
}

Block::Block(bool solid, const std::string &texturePath)
{
    m_id                          = 0;
    m_breakSound                  = "dig.stone";
    m_breakSoundVariants          = 4;
    m_foliageTint                 = false;
    m_hasFixedTint                = false;
    m_fixedTint                   = 0xFFFFFF;
    m_placeSound                  = "";
    m_placeSoundVariants          = 4;
    m_placeSoundPitch             = 0.8f;
    m_noteInstrument              = "note.harp";
    m_solid                       = solid;
    m_collidable                  = solid;
    m_selectable                  = true;
    m_transparent                 = false;
    m_lightOpacity                = 0;
    m_cullsSameNeighbor           = true;
    m_liquid                      = false;
    m_translucent                 = false;
    m_replaceable                 = false;
    m_randomTicking               = false;
    m_slipperiness                = 0.6f;
    m_liquidReplaceable           = false;
    m_blocksLiquid                = false;
    m_needsSupportBelow           = false;
    m_needsCenterSupport          = false;
    m_needsAttachedSupport        = false;
    m_flatItem                    = false;
    m_climbable                   = false;
    m_wallVariant                 = nullptr;
    m_aabb                        = {glm::dvec3(0.0, 0.0, 0.0), solid ? glm::dvec3(1.0, 1.0, 1.0) : glm::dvec3(0.0, 0.0, 0.0)};
    m_fireEncouragement           = 0;
    m_flammability                = 0;
    m_explosionResistance         = 6.0f;
    m_destroyTime                 = 0.0f;
    m_toolType                    = eToolType_None;
    m_requiresTool                = false;
    m_harvestLevel                = 0;
    m_lightEmission               = 0;
    m_lightR                      = 0;
    m_lightG                      = 0;
    m_lightB                      = 0;
    m_renderShape                 = eRenderShape_CUBE;
    m_entityRendered              = false;
    m_hasWallMountedTransform     = false;
    m_wallMountedTiltDegrees      = 0.0f;
    m_wallMountedInset            = 0.0f;
    m_hasTorchPlanes              = false;
    m_facingProperty              = -1;
    m_axisProperty                = -1;
    m_rotatedSideAtlasUVRect      = {0.0f, 0.0f, 1.0f, 1.0f};
    m_sideOverlayAtlasUVRect      = {0.0f, 0.0f, 1.0f, 1.0f};
    m_secondaryAtlasUVRect        = {0.0f, 0.0f, 1.0f, 1.0f};
    m_crossJitter                 = eCrossJitter_Full;
    m_crossFollowsAttachment      = false;
    m_interactionAABB             = m_aabb;
    m_hasInteractionAABB          = false;
    m_interactionAttachmentOffset = 0.0f;
    if (!texturePath.empty())
    {
        SetTexturePath(Direction::up, texturePath);
        SetTexturePath(Direction::down, texturePath);
        SetTexturePath(Direction::north, texturePath);
        SetTexturePath(Direction::south, texturePath);
        SetTexturePath(Direction::east, texturePath);
        SetTexturePath(Direction::west, texturePath);
    }
}

Block *Block::ById(uint32_t id)
{
    return BlockRegistry::GetInstance()->ById(id);
}

Block *Block::ByName(const ResourceLocation &location)
{
    return BlockRegistry::GetInstance()->Get(location);
}

void Block::OnPlace(Level *level, const BlockPos &pos)
{
    (void) level;
    (void) pos;
}

bool Block::DropsFromExplosion() const
{
    return true;
}

void Block::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    out->push_back(m_aabb);
}

void Block::GetPlacedBoxes(const Level *level, const BlockPos &pos, uint16_t data, std::vector<AABB> *out) const
{
    (void) level;
    (void) pos;
    GetBoxes(data, out);
}

void Block::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    GetBoxes(data, out);
}

void Block::GetItemBoxes(std::vector<AABB> *out) const
{
    GetRenderBoxes(GetItemData(), out);
}

bool Block::HasBoxFace(uint16_t data, size_t boxIndex, Direction *direction) const
{
    (void) data;
    (void) boxIndex;
    (void) direction;
    return true;
}

bool Block::IsBoxFaceDoubleSided(uint16_t data, size_t boxIndex, Direction *direction) const
{
    (void) data;
    (void) boxIndex;
    (void) direction;
    return false;
}

bool Block::HasAmbientOcclusion() const
{
    return true;
}

bool Block::HasDirectionalShade() const
{
    return true;
}

uint16_t Block::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) level;
    (void) pos;
    (void) hitPos;
    (void) look;
    return GetPlacementData(face);
}

Block *Block::GetCombinedBlock(uint16_t existingData, uint16_t placedData) const
{
    (void) existingData;
    (void) placedData;
    return nullptr;
}

uint16_t Block::GetItemData() const
{
    return 0;
}

void Block::OnBreak(Level *level, const BlockPos &pos)
{
    (void) level;
    (void) pos;
}

void Block::OnRemoved(Level *level, const BlockPos &pos, uint16_t data)
{
    (void) level;
    (void) pos;
    (void) data;
}

void Block::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide() || CanSurvive(level, pos, GetAttachment(level->GetBlockData(pos))))
    {
        return;
    }

    DropAsItem(level, pos, level->GetBlockData(pos));
    level->SetBlock(pos, Block::ByName("air"));
    level->RecordBlockChange(pos);
}

void Block::OnClientPlace(Level *level, const BlockPos &pos)
{
    UpdateShape(level, pos);
}

void Block::UpdateShape(Level *level, const BlockPos &pos)
{
    (void) level;
    (void) pos;
}

void Block::OnNeighborStateChanged(Level *level, const BlockPos &pos, Direction *direction)
{
    (void) level;
    (void) pos;
    (void) direction;
}

void Block::DropAsItem(Level *level, const BlockPos &pos, uint16_t data, const ItemInstance *tool) const
{
    if (level->IsClientSide())
    {
        return;
    }

    const LootTable *table = LootTables::ForBlock(this);
    if (!table)
    {
        Item *item = Item::ByName(GetResourceLocation());
        if (item && !IsReplaceable())
        {
            SpawnDrop(level, pos, item, 0);
        }

        return;
    }

    LootContext context = {&s_dropRandom, {}, tool, false, false};
    for (size_t i = 0; i < m_properties.size(); i++)
    {
        context.properties.push_back({m_properties[i].name, GetPropertyValue(data, (int) i)});
    }

    std::vector<ItemInstance> items;
    table->GetRandomItems(context, &items);
    for (size_t i = 0; i < items.size(); i++)
    {
        for (int j = 0; j < items[i].GetCount(); j++)
        {
            SpawnDrop(level, pos, items[i].GetItem(), items[i].GetAuxValue());
        }
    }

    int experience = table->GetExperience(context);
    if (experience > 0)
    {
        ExperienceOrb::SpawnOrbs(level, glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), experience);
    }
}

void Block::SpawnDrop(Level *level, const BlockPos &pos, Item *item, int auxValue) const
{
    glm::dvec3 position(pos.x + 0.5 + s_dropRandom.NextDouble() * 0.5 - 0.25, pos.y + 0.5 - 0.125 + s_dropRandom.NextDouble() * 0.5 - 0.25,
                        pos.z + 0.5 + s_dropRandom.NextDouble() * 0.5 - 0.25);
    glm::dvec3 velocity(s_dropRandom.NextDouble() * 0.2 - 0.1, 0.2, s_dropRandom.NextDouble() * 0.2 - 0.1);

    std::unique_ptr<ItemEntity> itemEntity(new ItemEntity(level));
    itemEntity->Sync(position, velocity, ItemInstance(item, 1, auxValue), 0, 10);
    level->AddEntity(std::move(itemEntity));
}

void Block::AnimateTick(Level *level, const BlockPos &pos, Random *random)
{
    (void) level;
    (void) pos;
    (void) random;
}

void Block::Tick(Level *level, const BlockPos &pos)
{
    (void) level;
    (void) pos;
}

void Block::TriggerEvent(Level *level, const BlockPos &pos, int id)
{
    (void) level;
    (void) pos;
    (void) id;
}

float Block::GetLiquidHeight(uint8_t data) const
{
    (void) data;
    return 1.0f;
}

glm::dvec3 Block::GetFlow(Level *level, const BlockPos &pos) const
{
    (void) level;
    (void) pos;
    return glm::dvec3();
}

void Block::SetTexture(Direction *direction, Texture *texture)
{
    m_textures[direction] = texture;
}

Texture *Block::GetTexture(Direction *direction) const
{
    auto it = m_textures.find(direction);
    if (it == m_textures.end())
    {
        return nullptr;
    }

    return it->second;
}

void Block::SetTexturePath(Direction *direction, const std::string &path)
{
    m_texturePaths[direction] = path;
}

void Block::SetFaceTexture(Direction *direction, const std::string &path)
{
    SetTexturePath(direction, path);
}

int Block::AddProperty(const std::string &name, int valueCount)
{
    uint8_t shift = 0;
    if (!m_properties.empty())
    {
        const Property &last = m_properties.back();
        shift                = (uint8_t) (last.shift + std::popcount((uint32_t) last.mask));
    }

    int bits = 1;
    while ((1 << bits) < valueCount)
    {
        bits++;
    }

    m_properties.push_back({name, (uint8_t) valueCount, shift, (uint16_t) (((1u << bits) - 1u) << shift)});
    return (int) m_properties.size() - 1;
}

int Block::FindProperty(const std::string &name) const
{
    for (size_t i = 0; i < m_properties.size(); i++)
    {
        if (m_properties[i].name == name)
        {
            return (int) i;
        }
    }

    return -1;
}

const std::vector<Block::Property> &Block::GetProperties() const
{
    return m_properties;
}

int Block::GetPropertyValue(uint16_t data, int property) const
{
    const Property &entry = m_properties[(size_t) property];
    return (data & entry.mask) >> entry.shift;
}

uint16_t Block::WithPropertyValue(uint16_t data, int property, int value) const
{
    const Property &entry = m_properties[(size_t) property];
    return (uint16_t) ((data & ~entry.mask) | (((uint32_t) value << entry.shift) & entry.mask));
}

static Direction *s_horizontalDirections[] = {Direction::north, Direction::south, Direction::west, Direction::east};

void Block::SetHorizontalFacing()
{
    m_facingProperty = AddProperty("facing", 4);
}

uint16_t Block::GetPlacementData(Direction *placedAgainst) const
{
    uint16_t data = 0;
    if (m_facingProperty >= 0)
    {
        for (int i = 0; i < 4; i++)
        {
            if (s_horizontalDirections[i] == placedAgainst)
            {
                data = WithPropertyValue(data, m_facingProperty, i);
            }
        }
    }

    if (m_axisProperty >= 0)
    {
        int axis = 0;
        if (placedAgainst == Direction::east || placedAgainst == Direction::west)
        {
            axis = 1;
        }
        else if (placedAgainst == Direction::north || placedAgainst == Direction::south)
        {
            axis = 2;
        }

        data = WithPropertyValue(data, m_axisProperty, axis);
    }

    return data;
}

Direction *Block::GetAttachment(uint16_t data) const
{
    if (m_facingProperty >= 0)
    {
        return Direction::Opposite(s_horizontalDirections[GetPropertyValue(data, m_facingProperty)]);
    }

    return m_needsAttachedSupport || m_needsSupportBelow ? Direction::down : nullptr;
}

void Block::SetPillar(const std::string &topPath)
{
    m_axisProperty = AddProperty("axis", 3);
    SetFaceTexture(Direction::up, topPath);
    SetFaceTexture(Direction::down, topPath);
}

bool Block::IsPillar() const
{
    return m_axisProperty >= 0;
}

std::string Block::GetRotatedSidePath() const
{
    return GetTexturePath(Direction::north) + "#rotated";
}

void Block::SetRotatedSideAtlasUVRect(float u0, float v0, float u1, float v1)
{
    m_rotatedSideAtlasUVRect = {u0, v0, u1, v1};
}

void Block::SetSideOverlay(const std::string &path)
{
    m_sideOverlayPath = path;
}

bool Block::HasSideOverlay() const
{
    return !m_sideOverlayPath.empty();
}

const std::string &Block::GetSideOverlayPath() const
{
    return m_sideOverlayPath;
}

void Block::SetSideOverlayAtlasUVRect(float u0, float v0, float u1, float v1)
{
    m_sideOverlayAtlasUVRect = {u0, v0, u1, v1};
}

Block::UVRect Block::GetSideOverlayAtlasUVRect() const
{
    return m_sideOverlayAtlasUVRect;
}

void Block::SetSecondaryTexture(const std::string &path)
{
    m_secondaryTexturePath = path;
}

bool Block::HasSecondaryTexture() const
{
    return !m_secondaryTexturePath.empty();
}

const std::string &Block::GetSecondaryTexturePath() const
{
    return m_secondaryTexturePath;
}

void Block::SetSecondaryAtlasUVRect(float u0, float v0, float u1, float v1)
{
    m_secondaryAtlasUVRect = {u0, v0, u1, v1};
}

Block::UVRect Block::GetSecondaryAtlasUVRect() const
{
    return m_secondaryAtlasUVRect;
}

int Block::AddTextureVariant(const std::string &path)
{
    m_textureVariantPaths.push_back(path);
    m_textureVariantAtlasUVRects.push_back({0.0f, 0.0f, 1.0f, 1.0f});
    return (int) m_textureVariantPaths.size() - 1;
}

const std::vector<std::string> &Block::GetTextureVariantPaths() const
{
    return m_textureVariantPaths;
}

void Block::SetTextureVariantAtlasUVRect(int index, float u0, float v0, float u1, float v1)
{
    m_textureVariantAtlasUVRects[index] = {u0, v0, u1, v1};
}

Block::UVRect Block::GetTextureVariantAtlasUVRect(int index) const
{
    return m_textureVariantAtlasUVRects[index];
}

Block::UVRect Block::GetCrossAtlasUVRect(uint16_t data) const
{
    (void) data;
    return GetAtlasUVRect(Direction::up);
}

int Block::GetCrossTextureVariant(uint16_t data) const
{
    (void) data;
    return -1;
}

void Block::SetCrossJitter(eCROSS_JITTER crossJitter)
{
    m_crossJitter = crossJitter;
}

Block::eCROSS_JITTER Block::GetCrossJitter() const
{
    return m_crossJitter;
}

glm::dvec3 Block::GetCrossOffset(const BlockPos &pos, uint16_t data) const
{
    if (m_renderShape != eRenderShape_CROSS || m_crossJitter == eCrossJitter_None ||
        (m_crossFollowsAttachment && GetAttachment(data) != Direction::down))
    {
        return glm::dvec3();
    }

    int64_t seed  = ((int64_t) pos.x * 3129871) ^ ((int64_t) pos.z * 116129781) ^ (int64_t) pos.y;
    seed          = seed * seed * 42317861 + seed * 11;
    double spread = m_crossJitter == eCrossJitter_Full ? 0.5 : 0.3;
    double y      = m_crossJitter == eCrossJitter_Full ? ((double) ((seed >> 20) & 0xF) / 15.0 - 1.0) * 0.2 : 0.0;
    return glm::dvec3(((double) ((seed >> 16) & 0xF) / 15.0 - 0.5) * spread, y, ((double) ((seed >> 24) & 0xF) / 15.0 - 0.5) * spread);
}

void Block::SetCrossFollowsAttachment(bool crossFollowsAttachment)
{
    m_crossFollowsAttachment = crossFollowsAttachment;
}

bool Block::CrossFollowsAttachment() const
{
    return m_crossFollowsAttachment;
}

bool Block::IsUsable(uint16_t data) const
{
    (void) data;

    return false;
}

bool Block::OnUse(Level *level, const BlockPos &pos)
{
    (void) level;
    (void) pos;
    return false;
}

void Block::OnAttack(Level *level, const BlockPos &pos)
{
    (void) level;
    (void) pos;
}

Block::UVRect Block::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    (void) data;
    (void) box;
    *flipU = false;
    return GetAtlasUVRect(direction);
}

void Block::AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const
{
    (void) data;
    (void) box;
    (void) direction;
    (void) u0;
    (void) v0;
    (void) u1;
    (void) v1;
}

void Block::GetRenderPlanes(uint16_t data, std::vector<RenderPlane> *out) const
{
    (void) data;
    (void) out;
}

const std::string &Block::GetTexturePath(Direction *direction) const
{
    auto it = m_texturePaths.find(direction);
    static const std::string empty;
    if (it == m_texturePaths.end())
    {
        return empty;
    }

    return it->second;
}

void Block::SetBiomeTint()
{
    m_biomeTintFaces[Direction::up]    = true;
    m_biomeTintFaces[Direction::down]  = true;
    m_biomeTintFaces[Direction::north] = true;
    m_biomeTintFaces[Direction::south] = true;
    m_biomeTintFaces[Direction::east]  = true;
    m_biomeTintFaces[Direction::west]  = true;
}

void Block::SetBiomeTint(Direction *direction)
{
    m_biomeTintFaces[direction] = true;
}

void Block::SetFoliageTint()
{
    SetBiomeTint();
    m_foliageTint = true;
}

void Block::SetFixedTint(uint32_t color)
{
    SetBiomeTint();
    m_hasFixedTint = true;
    m_fixedTint    = color;
}

uint32_t Block::GetItemTint() const
{
    if (m_hasFixedTint)
    {
        return m_fixedTint;
    }

    if (m_foliageTint)
    {
        return 0x48B518;
    }

    return GetItemGrassTint();
}

uint32_t Block::GetDataTint(uint16_t data) const
{
    (void) data;

    return 0xFFFFFF;
}

uint32_t Block::GetItemGrassTint()
{
    Biome *plains = Biome::ByName("plains");
    return plains ? plains->GetGrassColor() : 0xFFFFFF;
}

bool Block::HasBiomeTint(Direction *direction) const
{
    return m_biomeTintFaces.find(direction) != m_biomeTintFaces.end();
}

uint32_t Block::ResolveTint(Direction *direction, uint32_t grassColor, uint32_t foliageColor) const
{
    if (!HasBiomeTint(direction))
    {
        return 0xFFFFFF;
    }

    if (m_hasFixedTint)
    {
        return m_fixedTint;
    }

    return m_foliageTint ? foliageColor : grassColor;
}

uint32_t Block::ResolveTint(Direction *direction, Level *level, const Chunk *chunk, int localX, int localZ) const
{
    if (!HasBiomeTint(direction))
    {
        return 0xFFFFFF;
    }

    if (m_hasFixedTint)
    {
        return m_fixedTint;
    }

    static const int radius = 2;
    uint32_t totalR         = 0;
    uint32_t totalG         = 0;
    uint32_t totalB         = 0;
    int samples             = 0;

    for (int dz = -radius; dz <= radius; dz++)
    {
        for (int dx = -radius; dx <= radius; dx++)
        {
            int lx = localX + dx;
            int lz = localZ + dz;

            int chunkOffsetX = 0;
            int chunkOffsetZ = 0;

            if (lx < 0)
            {
                chunkOffsetX = -1;
                lx += Chunk::s_sizeX;
            }
            else if (lx >= Chunk::s_sizeX)
            {
                chunkOffsetX = 1;
                lx -= Chunk::s_sizeX;
            }

            if (lz < 0)
            {
                chunkOffsetZ = -1;
                lz += Chunk::s_sizeZ;
            }
            else if (lz >= Chunk::s_sizeZ)
            {
                chunkOffsetZ = 1;
                lz -= Chunk::s_sizeZ;
            }

            const Chunk *target = chunk;
            if (chunkOffsetX != 0 || chunkOffsetZ != 0)
            {
                ChunkPos pos = chunk->GetPos();
                pos.x += chunkOffsetX;
                pos.z += chunkOffsetZ;
                target = level->GetChunk(pos);
                if (!target)
                {
                    continue;
                }
            }

            Biome *biome = target->GetSurfaceBiome(lx, lz);
            if (!biome)
            {
                continue;
            }

            uint32_t color = m_foliageTint ? biome->GetFoliageColor() : biome->GetGrassColor();
            totalR += (color >> 16) & 0xFF;
            totalG += (color >> 8) & 0xFF;
            totalB += color & 0xFF;
            samples++;
        }
    }

    if (samples == 0)
    {
        return 0xFFFFFF;
    }

    return ((totalR / (uint32_t) samples) << 16) | ((totalG / (uint32_t) samples) << 8) | (totalB / (uint32_t) samples);
}

const ResourceLocation &Block::GetResourceLocation() const
{
    BlockRegistry *registry = BlockRegistry::GetInstance();
    return registry->GetKey(registry->IdOf((Block *) this));
}

void Block::SetBreakSound(const std::string &breakSound)
{
    m_breakSound = breakSound;
}

void Block::SetBreakSound(const std::string &breakSound, int variants)
{
    m_breakSound         = breakSound;
    m_breakSoundVariants = variants;
}

const std::string &Block::GetBreakSound() const
{
    return m_breakSound;
}

std::string Block::GetStepSound(int *outVariants) const
{
    if (m_breakSound == "dig.grass" || m_breakSound == "sweet_berry_bush.break")
    {
        *outVariants = 6;
        return "step.grass";
    }

    if (m_breakSound == "dig.wood")
    {
        *outVariants = 6;
        return "step.wood";
    }

    if (m_breakSound == "dig.sand")
    {
        *outVariants = 5;
        return "step.sand";
    }

    if (m_breakSound == "dig.dirt" || m_breakSound == "dig.gravel")
    {
        *outVariants = 4;
        return "step.gravel";
    }

    if (m_breakSound == "dig.snow")
    {
        *outVariants = 4;
        return "step.snow";
    }

    if (m_breakSound == "bamboo.place" || m_breakSound == "bamboo.sapling_place")
    {
        *outVariants = 6;
        return "bamboo.step";
    }

    if (m_breakSound == "moss.break")
    {
        *outVariants = 5;
        return "moss.step";
    }

    if (m_breakSound == "scaffolding.place")
    {
        *outVariants = 7;
        return "scaffolding.step";
    }

    if (m_breakSound == "dripstone.break")
    {
        *outVariants = 6;
        return "dripstone.step";
    }

    if (m_breakSound == "basalt.break")
    {
        *outVariants = 6;
        return "basalt.step";
    }

    if (m_breakSound == "chain.break")
    {
        *outVariants = 6;
        return "chain.step";
    }

    if (m_breakSound == "lantern.break")
    {
        *outVariants = 6;
        return "lantern.break";
    }

    if (m_breakSound == "cherry_wood.break")
    {
        *outVariants = 6;
        return "cherry_wood.step";
    }

    if (m_breakSound == "cherry_leaves.break")
    {
        *outVariants = 5;
        return "cherry_leaves.step";
    }

    if (m_breakSound == "bamboo_wood.break")
    {
        *outVariants = 6;
        return "bamboo_wood.step";
    }

    if (m_breakSound == "dig.cloth")
    {
        *outVariants = 4;
        return "step.cloth";
    }

    *outVariants = 6;
    return "step.stone";
}

int Block::GetBreakSoundVariants() const
{
    return m_breakSoundVariants;
}

void Block::SetPlaceSound(const std::string &placeSound, int variants, float pitch)
{
    m_placeSound         = placeSound;
    m_placeSoundVariants = variants;
    m_placeSoundPitch    = pitch;
}

const std::string &Block::GetPlaceSound() const
{
    return m_placeSound.empty() ? m_breakSound : m_placeSound;
}

int Block::GetPlaceSoundVariants() const
{
    return m_placeSound.empty() ? m_breakSoundVariants : m_placeSoundVariants;
}

float Block::GetPlaceSoundPitch() const
{
    return m_placeSoundPitch;
}

void Block::SetNoteInstrument(const std::string &noteInstrument)
{
    m_noteInstrument = noteInstrument;
}

const std::string &Block::GetNoteInstrument() const
{
    return m_noteInstrument;
}

void Block::SetId(uint32_t id)
{
    m_id = id;
}

bool Block::IsSolid() const
{
    return m_solid;
}

bool Block::IsSignalConductor() const
{
    return m_solid && !m_transparent && m_renderShape == eRenderShape_CUBE && !IsSignalSource();
}

int Block::GetSignal(uint16_t data, Direction *direction) const
{
    (void) data;
    (void) direction;

    return 0;
}

int Block::GetDirectSignal(uint16_t data, Direction *direction) const
{
    (void) data;
    (void) direction;

    return 0;
}

bool Block::IsSignalSource() const
{
    return false;
}

bool Block::CanConnectRedstone(uint16_t data, Direction *direction) const
{
    (void) data;
    (void) direction;
    return IsSignalSource();
}

bool Block::HasAnalogOutput() const
{
    return false;
}

int Block::GetAnalogOutput(Level *level, const BlockPos &pos) const
{
    (void) level;
    (void) pos;
    return 0;
}

bool Block::BlocksLight() const
{
    return m_solid && !m_transparent;
}

void Block::SetLightOpacity(int opacity)
{
    m_lightOpacity = opacity;
}

int Block::GetLightOpacity() const
{
    return m_lightOpacity;
}

void Block::SetCollidable(bool collidable)
{
    m_collidable = collidable;
}

bool Block::IsCollidable() const
{
    return m_collidable;
}

void Block::SetSelectable(bool selectable)
{
    m_selectable = selectable;
}

bool Block::IsSelectable() const
{
    return m_selectable;
}

void Block::SetTransparent(bool transparent)
{
    m_transparent = transparent;
}

bool Block::CullsSameNeighbor() const
{
    return m_cullsSameNeighbor;
}

void Block::SetCullsSameNeighbor(bool cullsSameNeighbor)
{
    m_cullsSameNeighbor = cullsSameNeighbor;
}

bool Block::IsTransparent() const
{
    return m_transparent;
}

void Block::SetLiquid(bool liquid)
{
    m_liquid = liquid;
}

bool Block::IsLava() const
{
    return false;
}

bool Block::IsTranslucent() const
{
    return m_translucent;
}

void Block::SetTranslucent(bool translucent)
{
    m_translucent = translucent;
}

bool Block::IsLiquid() const
{
    return m_liquid;
}

void Block::SetReplaceable(bool replaceable)
{
    m_replaceable = replaceable;
}

bool Block::IsReplaceable() const
{
    return m_replaceable;
}

bool Block::CanBeReplaced(uint16_t data) const
{
    (void) data;
    return m_replaceable;
}

void Block::SetRandomTicking(bool randomTicking)
{
    m_randomTicking = randomTicking;
}

bool Block::IsRandomTicking() const
{
    return m_randomTicking;
}

void Block::SetSlipperiness(float slipperiness)
{
    m_slipperiness = slipperiness;
}

float Block::GetSlipperiness() const
{
    return m_slipperiness;
}

void Block::OnEntityLanded(Entity *entity, double velocityY) const
{
    (void) entity;
    (void) velocityY;
}

void Block::OnEntityWalk(Entity *entity) const
{
    (void) entity;
}

void Block::OnProjectileHit(Level *level, const BlockPos &pos, Entity *projectile, const glm::dvec3 &hitPos, Direction *face)
{
    (void) level;
    (void) pos;
    (void) projectile;
    (void) hitPos;
    (void) face;
}

float Block::GetFallDamageMultiplier(Entity *entity) const
{
    (void) entity;
    return 1.0f;
}

void Block::SetLiquidReplaceable(bool liquidReplaceable)
{
    m_liquidReplaceable = liquidReplaceable;
}

bool Block::IsLiquidReplaceable() const
{
    return m_liquidReplaceable;
}

void Block::SetBlocksLiquid(bool blocksLiquid)
{
    m_blocksLiquid = blocksLiquid;
}

bool Block::BlocksLiquid() const
{
    return m_blocksLiquid;
}

void Block::GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const
{
    GetBoxes(data, out);
}

void Block::GetPlacedCollisionBoxes(const Level *level, const BlockPos &pos, uint16_t data, std::vector<AABB> *out) const
{
    (void) level;
    (void) pos;
    GetCollisionBoxes(data, out);
}

void Block::RandomTick(Level *level, const BlockPos &pos)
{
    (void) level;
    (void) pos;
}

void Block::SetNeedsCenterSupport(bool needsSupport)
{
    m_needsCenterSupport = needsSupport;
}

void Block::SetNeedsSupportBelow(bool needsSupport)
{
    m_needsSupportBelow = needsSupport;
}

bool Block::NeedsSupportBelow() const
{
    return m_needsSupportBelow;
}

void Block::SetNeedsAttachedSupport(bool needsSupport)
{
    m_needsAttachedSupport = needsSupport;
}

void Block::SetSupportBlocks(const std::vector<std::string> &supportBlocks)
{
    m_supportBlocks = supportBlocks;
}

BlockPos Block::GetOtherPos(const BlockPos &pos, uint16_t data) const
{
    (void) data;
    return pos;
}

bool Block::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    Direction *supportDirection = nullptr;
    if (m_needsSupportBelow)
    {
        supportDirection = Direction::down;
    }

    if (m_needsAttachedSupport)
    {
        supportDirection = attachment ? attachment : Direction::down;
    }

    if (!supportDirection)
    {
        return true;
    }

    BlockPos supportPos(pos.x + supportDirection->dx, pos.y + supportDirection->dy, pos.z + supportDirection->dz);
    Block *support = Block::ById(level->GetBlockId(supportPos));
    if (!m_supportBlocks.empty())
    {
        for (size_t i = 0; i < m_supportBlocks.size(); i++)
        {
            if (support == Block::ByName(m_supportBlocks[i]))
            {
                return true;
            }
        }

        return false;
    }

    if (!support)
    {
        return false;
    }

    if (m_needsCenterSupport && supportDirection == Direction::down)
    {
        return support->CanSupportCenter(level->GetBlockData(supportPos), Direction::up);
    }

    return support->IsFaceSturdy(level->GetBlockData(supportPos), Direction::Opposite(supportDirection));
}

bool Block::IsFaceSturdy(uint16_t data, Direction *face) const
{
    return CoversFace(data, face, 0.0, 1.0);
}

bool Block::CanSupportCenter(uint16_t data, Direction *face) const
{
    return CoversFace(data, face, 7.0 / 16.0, 9.0 / 16.0);
}

// A rigid face only needs the outer two pixel ring covered, which can be spread across several boxes
bool Block::CanSupportRigid(uint16_t data, Direction *face) const
{
    if (IsFaceSturdy(data, face))
    {
        return true;
    }

    if (!IsCollidable() || IsLiquid() || GetRenderShape() != eRenderShape_BOXES)
    {
        return false;
    }

    std::vector<AABB> boxes;
    GetBoxes(data, &boxes);
    for (int a = 0; a < 16; a++)
    {
        for (int b = 0; b < 16; b++)
        {
            if (a >= 2 && a < 14 && b >= 2 && b < 14)
            {
                continue;
            }

            double u = (a + 0.5) / 16.0;
            double v = (b + 0.5) / 16.0;
            glm::dvec3 point(face->dx != 0 ? 0.5 : u, face->dy != 0 ? 0.5 : (face->dx != 0 ? u : v), face->dz != 0 ? 0.5 : v);
            bool covered = false;
            for (size_t i = 0; i < boxes.size() && !covered; i++)
            {
                const glm::dvec3 &min = boxes[i].GetMin();
                const glm::dvec3 &max = boxes[i].GetMax();
                bool touchesX         = face->dx == 0 ? min.x <= point.x && max.x >= point.x : (face->dx > 0 ? max.x >= 1.0 : min.x <= 0.0);
                bool touchesY         = face->dy == 0 ? min.y <= point.y && max.y >= point.y : (face->dy > 0 ? max.y >= 1.0 : min.y <= 0.0);
                bool touchesZ         = face->dz == 0 ? min.z <= point.z && max.z >= point.z : (face->dz > 0 ? max.z >= 1.0 : min.z <= 0.0);
                covered               = touchesX && touchesY && touchesZ;
            }

            if (!covered)
            {
                return false;
            }
        }
    }

    return true;
}

bool Block::CoversFace(uint16_t data, Direction *face, double areaMin, double areaMax) const
{
    if (!IsSolid() || IsLiquid())
    {
        return false;
    }

    if (GetRenderShape() == eRenderShape_CUBE)
    {
        return true;
    }

    if (GetRenderShape() != eRenderShape_BOXES)
    {
        return false;
    }

    std::vector<AABB> boxes;
    GetBoxes(data, &boxes);
    for (size_t i = 0; i < boxes.size(); i++)
    {
        const glm::dvec3 &min = boxes[i].GetMin();
        const glm::dvec3 &max = boxes[i].GetMax();
        bool coversX          = face->dx != 0 || (min.x <= areaMin && max.x >= areaMax);
        bool coversY          = face->dy != 0 || (min.y <= areaMin && max.y >= areaMax);
        bool coversZ          = face->dz != 0 || (min.z <= areaMin && max.z >= areaMax);
        bool touchesX         = face->dx == 0 || (face->dx > 0 ? max.x >= 1.0 : min.x <= 0.0);
        bool touchesY         = face->dy == 0 || (face->dy > 0 ? max.y >= 1.0 : min.y <= 0.0);
        bool touchesZ         = face->dz == 0 || (face->dz > 0 ? max.z >= 1.0 : min.z <= 0.0);
        if (coversX && coversY && coversZ && touchesX && touchesY && touchesZ)
        {
            return true;
        }
    }

    return false;
}

void Block::GetBoxFaceCorners(const AABB &box, Direction *direction, glm::dvec3 *corners) const
{
    const glm::dvec3 &min = box.GetMin();
    const glm::dvec3 &max = box.GetMax();
    if (direction == Direction::north)
    {
        corners[0] = glm::dvec3(min.x, min.y, min.z);
        corners[1] = glm::dvec3(min.x, max.y, min.z);
        corners[2] = glm::dvec3(max.x, max.y, min.z);
    }
    else if (direction == Direction::south)
    {
        corners[0] = glm::dvec3(max.x, min.y, max.z);
        corners[1] = glm::dvec3(max.x, max.y, max.z);
        corners[2] = glm::dvec3(min.x, max.y, max.z);
    }
    else if (direction == Direction::east)
    {
        corners[0] = glm::dvec3(max.x, min.y, min.z);
        corners[1] = glm::dvec3(max.x, max.y, min.z);
        corners[2] = glm::dvec3(max.x, max.y, max.z);
    }
    else if (direction == Direction::west)
    {
        corners[0] = glm::dvec3(min.x, min.y, max.z);
        corners[1] = glm::dvec3(min.x, max.y, max.z);
        corners[2] = glm::dvec3(min.x, max.y, min.z);
    }
    else if (direction == Direction::up)
    {
        corners[0] = glm::dvec3(min.x, max.y, min.z);
        corners[1] = glm::dvec3(min.x, max.y, max.z);
        corners[2] = glm::dvec3(max.x, max.y, max.z);
    }
    else
    {
        corners[0] = glm::dvec3(min.x, min.y, max.z);
        corners[1] = glm::dvec3(min.x, min.y, min.z);
        corners[2] = glm::dvec3(max.x, min.y, min.z);
    }
}

glm::vec2 Block::GetModelFaceAxes(Direction *face, const glm::vec3 &model) const
{
    if (face == Direction::down)
    {
        return glm::vec2(model.x, 1.0f - model.z);
    }

    if (face == Direction::up)
    {
        return glm::vec2(model.x, model.z);
    }

    if (face == Direction::north)
    {
        return glm::vec2(1.0f - model.x, 1.0f - model.y);
    }

    if (face == Direction::south)
    {
        return glm::vec2(model.x, 1.0f - model.y);
    }

    if (face == Direction::west)
    {
        return glm::vec2(model.z, 1.0f - model.y);
    }

    return glm::vec2(1.0f - model.z, 1.0f - model.y);
}

void Block::ResolveFaceUV(int texture, glm::vec2 *st, int *outTexture, float *u0, float *v0, float *u1, float *v1) const
{
    *outTexture = texture;
    if (fabsf(st[0].x - st[1].x) > 1e-4f)
    {
        *outTexture = texture + 1;
        for (int i = 0; i < 3; i++)
        {
            st[i] = glm::vec2(1.0f - st[i].y, st[i].x);
        }
    }

    *u1 = st[0].x;
    *v1 = st[0].y;
    *u0 = st[2].x;
    *v0 = st[2].y;
}

void Block::SetWallVariant(Block *wallVariant)
{
    m_wallVariant = wallVariant;
}

Block *Block::GetPlacementBlock(Direction *face)
{
    if (!m_wallVariant || face == Direction::up)
    {
        return this;
    }

    return face == Direction::down ? nullptr : m_wallVariant;
}

void Block::SetAABB(const AABB &aabb)
{
    m_aabb = aabb;
}

const AABB &Block::GetAABB() const
{
    return m_aabb;
}

bool Block::HasInteractionAABB() const
{
    return m_hasInteractionAABB;
}

void Block::SetInteractionAABB(const AABB &aabb)
{
    m_interactionAABB    = aabb;
    m_hasInteractionAABB = true;
}

const AABB &Block::GetInteractionAABB() const
{
    if (m_hasInteractionAABB)
    {
        return m_interactionAABB;
    }

    return m_aabb;
}

void Block::SetInteractionAttachmentOffset(float offset)
{
    m_interactionAttachmentOffset = offset;
}

float Block::GetInteractionAttachmentOffset() const
{
    return m_interactionAttachmentOffset;
}

AABB Block::GetPlacedAABB(const BlockPos &pos, uint16_t data) const
{
    if (m_renderShape != eRenderShape_BOXES)
    {
        return GetPlacedAABB(pos, GetAttachment(data)).Translated(GetCrossOffset(pos, data));
    }

    std::vector<AABB> boxes;
    GetBoxes(data, &boxes);
    glm::dvec3 min = boxes[0].GetMin();
    glm::dvec3 max = boxes[0].GetMax();
    for (size_t i = 1; i < boxes.size(); i++)
    {
        min = glm::dvec3(std::min(min.x, boxes[i].GetMin().x), std::min(min.y, boxes[i].GetMin().y), std::min(min.z, boxes[i].GetMin().z));
        max = glm::dvec3(std::max(max.x, boxes[i].GetMax().x), std::max(max.y, boxes[i].GetMax().y), std::max(max.z, boxes[i].GetMax().z));
    }

    return AABB(min, max).Translated(glm::dvec3(pos.x, pos.y, pos.z));
}

void Block::GetSelectionBoxes(const Level *level, const BlockPos &pos, uint16_t data, std::vector<AABB> *out) const
{
    (void) level;
    out->push_back(GetPlacedAABB(pos, data));
}

void Block::SetFlatItem(bool flatItem)
{
    m_flatItem = flatItem;
}

void Block::SetClimbable(bool climbable)
{
    m_climbable = climbable;
}

bool Block::IsClimbable() const
{
    return m_climbable;
}

bool Block::RendersAsItemBlock() const
{
    return !m_flatItem && (m_renderShape == eRenderShape_CUBE || m_renderShape == eRenderShape_BOXES);
}

AABB Block::GetPlacedAABB(const BlockPos &pos, Direction *attachmentFace) const
{
    AABB box = GetInteractionAABB().Translated(glm::dvec3(pos.x, pos.y, pos.z));
    if (!m_hasWallMountedTransform)
    {
        return box;
    }

    if (attachmentFace == Direction::north)
    {
        return box.Translated(glm::dvec3(0.0, 0.0, -(m_wallMountedInset + m_interactionAttachmentOffset)));
    }

    if (attachmentFace == Direction::south)
    {
        return box.Translated(glm::dvec3(0.0, 0.0, m_wallMountedInset + m_interactionAttachmentOffset));
    }

    if (attachmentFace == Direction::west)
    {
        return box.Translated(glm::dvec3(-(m_wallMountedInset + m_interactionAttachmentOffset), 0.0, 0.0));
    }

    if (attachmentFace == Direction::east)
    {
        return box.Translated(glm::dvec3(m_wallMountedInset + m_interactionAttachmentOffset, 0.0, 0.0));
    }

    return box;
}

void Block::SetLightEmission(uint8_t value)
{
    m_lightEmission = value;
}

uint8_t Block::GetLightEmission() const
{
    return m_lightEmission;
}

void Block::SetLightColor(uint8_t r, uint8_t g, uint8_t b)
{
    m_lightR = r;
    m_lightG = g;
    m_lightB = b;
}

void Block::GetLightColor(uint8_t *r, uint8_t *g, uint8_t *b) const
{
    *r = m_lightR;
    *g = m_lightG;
    *b = m_lightB;
}

void Block::SetFlammable(int encouragement, int flammability)
{
    m_fireEncouragement = encouragement;
    m_flammability      = flammability;
}

int Block::GetFireEncouragement() const
{
    return m_fireEncouragement;
}

int Block::GetFlammability() const
{
    return m_flammability;
}

void Block::SetExplosionResistance(float resistance)
{
    m_explosionResistance = resistance;
}

float Block::GetExplosionResistance() const
{
    return m_explosionResistance;
}

void Block::SetDestroyTime(float destroyTime)
{
    m_destroyTime = destroyTime;
}

float Block::GetDestroyTime() const
{
    return m_destroyTime;
}

void Block::SetTool(eTOOL_TYPE toolType, bool requiresTool, int harvestLevel)
{
    m_toolType     = toolType;
    m_requiresTool = requiresTool;
    m_harvestLevel = harvestLevel;
}

Block::eTOOL_TYPE Block::GetToolType() const
{
    return m_toolType;
}

bool Block::RequiresTool() const
{
    return m_requiresTool;
}

int Block::GetHarvestLevel() const
{
    return m_harvestLevel;
}

void Block::OnExploded(Level *level, const BlockPos &pos)
{
    (void) level;
    (void) pos;
}

void Block::SetRenderShape(eRENDER_SHAPE type)
{
    m_renderShape = type;
}

Block::eRENDER_SHAPE Block::GetRenderShape() const
{
    return m_renderShape;
}

void Block::SetEntityRendered(bool entityRendered)
{
    m_entityRendered = entityRendered;
}

bool Block::IsEntityRendered() const
{
    return m_entityRendered;
}

const char *Block::GetBlockEntityId() const
{
    return nullptr;
}

void Block::SetUVRect(Direction *direction, float u0, float v0, float u1, float v1)
{
    m_uvRects[direction] = {u0, v0, u1, v1};
}

Block::UVRect Block::GetUVRect(Direction *direction) const
{
    auto it = m_uvRects.find(direction);
    if (it == m_uvRects.end())
    {
        return {0.0f, 0.0f, 1.0f, 1.0f};
    }

    return it->second;
}

void Block::SetAtlasUVRect(Direction *direction, float u0, float v0, float u1, float v1)
{
    m_atlasUVRects[direction] = {u0, v0, u1, v1};
}

Block::UVRect Block::GetAtlasUVRect(Direction *direction) const
{
    auto it = m_atlasUVRects.find(direction);
    if (it == m_atlasUVRects.end())
    {
        return {0.0f, 0.0f, 1.0f, 1.0f};
    }

    return it->second;
}

Block::UVRect Block::GetAtlasUVRect(Direction *direction, uint16_t data) const
{
    int axis = m_axisProperty >= 0 ? GetPropertyValue(data, m_axisProperty) : 0;
    if (axis == 0)
    {
        return GetAtlasUVRect(direction);
    }

    bool axisX = axis == 1;
    bool alongAxis =
            axisX ? (direction == Direction::east || direction == Direction::west) : (direction == Direction::north || direction == Direction::south);
    bool verticalFace = direction == Direction::up || direction == Direction::down;
    if (alongAxis)
    {
        return GetAtlasUVRect(Direction::up);
    }

    if (!axisX && verticalFace)
    {
        return GetAtlasUVRect(Direction::north);
    }

    return m_rotatedSideAtlasUVRect;
}

void Block::SetWallMountedTransform(float tiltDegrees, float wallInset)
{
    m_hasWallMountedTransform = true;
    m_wallMountedTiltDegrees  = tiltDegrees;
    m_wallMountedInset        = wallInset;
}

bool Block::HasWallMountedTransform() const
{
    return m_hasWallMountedTransform;
}

float Block::GetWallMountedTiltDegrees() const
{
    return m_wallMountedTiltDegrees;
}

float Block::GetWallMountedInset() const
{
    return m_wallMountedInset;
}

void Block::SetTorchPlanes()
{
    m_hasTorchPlanes = true;
}

bool Block::HasTorchPlanes() const
{
    return m_hasTorchPlanes;
}
