#include "DataFixer.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../core/Logger.h"
#include "../../entity/attribute/Attribute.h"
#include "../../tag/CompoundTag.h"
#include "../../tag/DoubleTag.h"
#include "../../tag/FloatTag.h"
#include "../../tag/ListTag.h"
#include "../../tag/StringTag.h"
#include "../block/Block.h"
#include "../block/BlockRegistry.h"
#include "../chunk/Chunk.h"
#include "../chunk/ChunkSection.h"
#include "WorldSave.h"

// Saves before version 6 still have the removed world border block at id 1, so older fixes shift ids down to find the right block
static Block *LegacyBlockById(uint32_t id)
{
    return Block::ById(id > 1 ? id - 1 : 0);
}

const char *DataFixer::s_versionTagName = "DataVersion";

const DataFixer::Fix DataFixer::s_fixes[] = {
        {2, eDataType_Chunk, &DataFixer::ChunkFlatArraysToPalettedSections},
        {3, eDataType_Chunk, &DataFixer::ChunkFacesToBlockStateProperties},
        {4, eDataType_Chunk, &DataFixer::ChunkColumnBiomesToSectionBiomes},
        {5, eDataType_Chunk, &DataFixer::ChunkIceSheetFacesToDirectionOrder},
        {6, eDataType_Chunk, &DataFixer::ChunkRemoveWorldBorderBlock},
        {7, eDataType_Player, &DataFixer::PlayerBlockInteractionRangeDefault},
        {8, eDataType_Player, &DataFixer::ItemWhiteDyeToBoneMeal},
        {8, eDataType_EntityRegion, &DataFixer::ItemWhiteDyeToBoneMeal},
        {9, eDataType_Player, &DataFixer::EntityPositionToLists},
        {9, eDataType_EntityRegion, &DataFixer::EntityRegionPositionToLists},
        {10, eDataType_Player, &DataFixer::PlayerStepHeightDefault},
        {11, eDataType_Player, &DataFixer::ItemBedToRedBed},
        {11, eDataType_EntityRegion, &DataFixer::ItemBedToRedBed},
        {11, eDataType_Chunk, &DataFixer::ItemBedToRedBed},
        {12, eDataType_Player, &DataFixer::ItemBrownDyeToCocoaBeans},
        {12, eDataType_EntityRegion, &DataFixer::ItemBrownDyeToCocoaBeans},
        {12, eDataType_Chunk, &DataFixer::ItemBrownDyeToCocoaBeans},
        {13, eDataType_Player, &DataFixer::ItemLanternToOrangeLantern},
        {13, eDataType_EntityRegion, &DataFixer::ItemLanternToOrangeLantern},
        {13, eDataType_Chunk, &DataFixer::ItemLanternToOrangeLantern},
        {14, eDataType_Chunk, &DataFixer::ChunkBlockIdsToNames},
};

void DataFixer::Stamp(CompoundTag *tag)
{
    tag->PutInt(s_versionTagName, kCurrentVersion);
}

void DataFixer::Update(eDATA_TYPE type, CompoundTag *tag)
{
    int version = tag->Contains(s_versionTagName) ? tag->GetInt(s_versionTagName) : kLegacyVersion;
    if (version > kCurrentVersion)
    {
        Logger::LogWarn("Save data version %d is newer than supported version %d", version, kCurrentVersion);
        return;
    }

    for (size_t i = 0; i < sizeof(s_fixes) / sizeof(s_fixes[0]); i++)
    {
        if (s_fixes[i].type == type && s_fixes[i].version > version)
        {
            s_fixes[i].apply(tag);
        }
    }

    Stamp(tag);
}

void DataFixer::ChunkFlatArraysToPalettedSections(CompoundTag *tag)
{
    int volume                      = Chunk::s_sizeX * Chunk::s_sizeY * Chunk::s_sizeZ;
    std::vector<int> blocks         = tag->GetIntArray("Blocks");
    std::vector<uint8_t> faces      = tag->GetByteArray("AttachmentFaces");
    std::vector<uint8_t> blockData  = tag->GetByteArray("BlockData");
    std::vector<uint8_t> skyLight   = tag->GetByteArray("SkyLight");
    std::vector<uint8_t> blockLight = tag->GetByteArray("BlockLight");
    if ((int) blocks.size() != volume || (int) faces.size() != volume || (int) skyLight.size() != volume || (int) blockLight.size() != volume * 3)
    {
        return;
    }

    bool hasBlockData = (int) blockData.size() == volume;

    std::unique_ptr<ListTag> sections(new ListTag());
    std::vector<uint32_t> states((size_t) ChunkSection::s_volume);
    std::vector<uint8_t> sky((size_t) ChunkSection::s_volume);
    std::vector<uint8_t> light((size_t) ChunkSection::s_volume * 3);
    for (int section = 0; section < Chunk::s_sectionCount; section++)
    {
        for (int localY = 0; localY < ChunkSection::s_size; localY++)
        {
            for (int z = 0; z < Chunk::s_sizeZ; z++)
            {
                for (int x = 0; x < Chunk::s_sizeX; x++)
                {
                    int y                 = section * ChunkSection::s_size + localY;
                    size_t source         = (size_t) (x + Chunk::s_sizeX * (z + Chunk::s_sizeZ * y));
                    size_t target         = (size_t) ChunkSection::Index(x, localY, z);
                    uint8_t data          = hasBlockData ? blockData[source] : 0;
                    states[target]        = ((uint32_t) blocks[source] & 0xFFFF) | ((uint32_t) data << 16) | ((uint32_t) faces[source] << 24);
                    sky[target]           = skyLight[source];
                    light[target * 3]     = blockLight[source * 3];
                    light[target * 3 + 1] = blockLight[source * 3 + 1];
                    light[target * 3 + 2] = blockLight[source * 3 + 2];
                }
            }
        }

        sections->Add(WorldSave::EncodeSection(section, states.data(), sky.data(), light.data()));
    }

    tag->Remove("Blocks");
    tag->Remove("AttachmentFaces");
    tag->Remove("BlockData");
    tag->Remove("SkyLight");
    tag->Remove("BlockLight");
    tag->Put("Sections", std::move(sections));
}

void DataFixer::ChunkFacesToBlockStateProperties(CompoundTag *tag)
{
    ListTag *sections = tag->GetList("Sections");
    if (!sections)
    {
        return;
    }

    for (size_t i = 0; i < sections->Size(); i++)
    {
        CompoundTag *section = (CompoundTag *) sections->Get(i);
        if (!section->Contains("Palette"))
        {
            continue;
        }

        std::vector<int> palette = section->GetIntArray("Palette");
        for (size_t entry = 0; entry < palette.size(); entry++)
        {
            uint32_t state = (uint32_t) palette[entry];
            uint32_t id    = state & 0xFFFF;
            uint8_t data   = (uint8_t) ((state >> 16) & 0xFF);
            uint8_t face   = (uint8_t) (state >> 24);
            Block *block   = LegacyBlockById(id);
            uint16_t bits  = 0;
            if (block && block->IsLiquid())
            {
                bits = data;
            }
            else if (block)
            {
                bits = block->GetPlacementData(Direction::Opposite(Direction::ById(face)));
            }

            palette[entry] = (int) ChunkSection::MakeState(id, bits);
        }

        section->PutIntArray("Palette", palette);
    }
}

void DataFixer::ChunkColumnBiomesToSectionBiomes(CompoundTag *tag)
{
    std::vector<int> columns = tag->GetIntArray("Biomes");
    if ((int) columns.size() != Chunk::s_sizeX * Chunk::s_sizeZ)
    {
        return;
    }

    std::vector<uint8_t> biomes;
    for (int section = 0; section < Chunk::s_sectionCount; section++)
    {
        biomes.push_back(1);
        for (int y = 0; y < ChunkSection::s_size; y += ChunkSection::s_biomeHeight)
        {
            for (size_t i = 0; i < columns.size(); i++)
            {
                biomes.push_back((uint8_t) columns[i]);
            }
        }
    }

    tag->Remove("Biomes");
    tag->PutByteArray("Biomes", biomes);
}

// Old saves store ice sheet faces as down, up, north, south, west, east instead of direction id order
void DataFixer::ChunkIceSheetFacesToDirectionOrder(CompoundTag *tag)
{
    ListTag *sections = tag->GetList("Sections");
    if (!sections)
    {
        return;
    }

    Block *iceSheet = Block::ByName("ice_sheet");
    for (size_t i = 0; i < sections->Size(); i++)
    {
        CompoundTag *section = (CompoundTag *) sections->Get(i);
        if (!section->Contains("Palette"))
        {
            continue;
        }

        std::vector<int> palette = section->GetIntArray("Palette");
        for (size_t entry = 0; entry < palette.size(); entry++)
        {
            uint32_t state = (uint32_t) palette[entry];
            uint32_t id    = state & 0xFFFF;
            if (LegacyBlockById(id) != iceSheet)
            {
                continue;
            }

            uint16_t data  = (uint16_t) (state >> 16);
            uint16_t bits  = (uint16_t) (((data & 0x3) << 4) | ((data >> 2) & 0xF));
            palette[entry] = (int) ChunkSection::MakeState(id, bits);
        }

        section->PutIntArray("Palette", palette);
    }
}

void DataFixer::ChunkRemoveWorldBorderBlock(CompoundTag *tag)
{
    ListTag *sections = tag->GetList("Sections");
    if (!sections)
    {
        return;
    }

    for (size_t i = 0; i < sections->Size(); i++)
    {
        CompoundTag *section = (CompoundTag *) sections->Get(i);
        if (!section->Contains("Palette"))
        {
            continue;
        }

        std::vector<int> palette = section->GetIntArray("Palette");
        for (size_t entry = 0; entry < palette.size(); entry++)
        {
            uint32_t state = (uint32_t) palette[entry];
            uint32_t id    = state & 0xFFFF;
            if (id == 1)
            {
                palette[entry] = 0;
            }
            else if (id > 1)
            {
                palette[entry] = (int) (state - 1);
            }
        }

        section->PutIntArray("Palette", palette);
    }
}

void DataFixer::ChunkBlockIdsToNames(CompoundTag *tag)
{
    ListTag *sections = tag->GetList("Sections");
    if (!sections)
    {
        return;
    }

    // Older saves use the registry order from before podzol and the colored lanterns, with orange_lantern in the old lantern slot
    BlockRegistry *registry = BlockRegistry::GetInstance();
    std::vector<std::string> legacyNames;
    for (uint32_t id = 0; id < registry->Size(); id++)
    {
        std::string name = registry->GetKey(id).GetPath();
        if (name == "podzol" || (name != "orange_lantern" && name.size() > 8 && name.compare(name.size() - 8, 8, "_lantern") == 0))
        {
            continue;
        }

        legacyNames.push_back(registry->GetKey(id).ToString());
    }

    std::unordered_map<uint32_t, uint32_t> localIds;
    std::unique_ptr<ListTag> blockNames(new ListTag());
    for (size_t i = 0; i < sections->Size(); i++)
    {
        CompoundTag *section = (CompoundTag *) sections->Get(i);
        if (!section->Contains("Palette"))
        {
            continue;
        }

        std::vector<int> palette = section->GetIntArray("Palette");
        for (size_t entry = 0; entry < palette.size(); entry++)
        {
            uint32_t state = (uint32_t) palette[entry];
            uint32_t id    = ChunkSection::StateId(state);
            auto inserted  = localIds.emplace(id, (uint32_t) blockNames->Size());
            if (inserted.second)
            {
                blockNames->Add(std::unique_ptr<Tag>(new StringTag(id < legacyNames.size() ? legacyNames[id] : legacyNames[0])));
            }

            palette[entry] = (int) ChunkSection::MakeState(inserted.first->second, ChunkSection::StateData(state));
        }

        section->PutIntArray("Palette", palette);
    }

    tag->Put("BlockNames", std::move(blockNames));
}

void DataFixer::PlayerBlockInteractionRangeDefault(CompoundTag *tag)
{
    ListTag *attributes = tag->GetList("attributes");
    for (size_t i = 0; attributes && i < attributes->Size(); i++)
    {
        CompoundTag *attribute = (CompoundTag *) attributes->Get(i);
        if (attribute->GetString("id") == "block_interaction_range" && attribute->GetDouble("base") == 6.0)
        {
            attribute->PutDouble("base", 4.5);
        }
    }
}

void DataFixer::PlayerStepHeightDefault(CompoundTag *tag)
{
    ListTag *attributes = tag->GetList("attributes");
    for (size_t i = 0; attributes && i < attributes->Size(); i++)
    {
        CompoundTag *attribute = (CompoundTag *) attributes->Get(i);
        if (ResourceLocation(attribute->GetString("id")) == Attribute::s_stepHeight.GetResourceLocation() && attribute->GetDouble("base") == 0.5)
        {
            attribute->PutDouble("base", Attribute::s_stepHeight.GetDefaultValue());
        }
    }
}

void DataFixer::RenameItemId(CompoundTag *tag, const char *from, const char *to)
{
    if (tag->GetString("id") == from)
    {
        tag->PutString("id", to);
    }

    for (const std::pair<const std::string, std::unique_ptr<Tag>> &entry : tag->GetTags())
    {
        if (entry.second->GetType() == Tag::eTagType_Compound)
        {
            RenameItemId((CompoundTag *) entry.second.get(), from, to);
        }
        else if (entry.second->GetType() == Tag::eTagType_List)
        {
            ListTag *list = (ListTag *) entry.second.get();
            for (size_t i = 0; i < list->Size(); i++)
            {
                if (list->Get(i)->GetType() == Tag::eTagType_Compound)
                {
                    RenameItemId((CompoundTag *) list->Get(i), from, to);
                }
            }
        }
    }
}

void DataFixer::ItemWhiteDyeToBoneMeal(CompoundTag *tag)
{
    RenameItemId(tag, "everstead:white_dye", "everstead:bone_meal");
}

void DataFixer::ItemBedToRedBed(CompoundTag *tag)
{
    RenameItemId(tag, "everstead:bed", "everstead:red_bed");
}

void DataFixer::ItemBrownDyeToCocoaBeans(CompoundTag *tag)
{
    RenameItemId(tag, "everstead:brown_dye", "everstead:cocoa_beans");
}

void DataFixer::ItemLanternToOrangeLantern(CompoundTag *tag)
{
    RenameItemId(tag, "everstead:lantern", "everstead:orange_lantern");
}

void DataFixer::EntityPositionToLists(CompoundTag *tag)
{
    const char *keys[][3] = {{"PosX", "PosY", "PosZ"}, {"MotionX", "MotionY", "MotionZ"}};
    const char *lists[]   = {"Pos", "Motion"};
    for (size_t i = 0; i < std::size(lists); i++)
    {
        if (!tag->Contains(keys[i][0]))
        {
            continue;
        }

        std::unique_ptr<ListTag> list(new ListTag());
        for (size_t axis = 0; axis < 3; axis++)
        {
            list->Add(std::unique_ptr<Tag>(new DoubleTag(tag->GetDouble(keys[i][axis]))));
            tag->Remove(keys[i][axis]);
        }

        tag->Put(lists[i], std::move(list));
    }

    if (tag->Contains("Yaw"))
    {
        std::unique_ptr<ListTag> rotation(new ListTag());
        rotation->Add(std::unique_ptr<Tag>(new FloatTag(tag->GetFloat("Yaw"))));
        rotation->Add(std::unique_ptr<Tag>(new FloatTag(tag->GetFloat("Pitch"))));
        tag->Remove("Yaw");
        tag->Remove("Pitch");
        tag->Put("Rotation", std::move(rotation));
    }
}

void DataFixer::EntityRegionPositionToLists(CompoundTag *tag)
{
    ListTag *entities = tag->GetList("Entities");
    for (size_t i = 0; entities && i < entities->Size(); i++)
    {
        EntityPositionToLists((CompoundTag *) entities->Get(i));
    }
}
