#pragma once

#include <cstdint>

class CompoundTag;

enum eDATA_TYPE : uint8_t
{
    eDataType_Level        = 0,
    eDataType_Chunk        = 1,
    eDataType_Player       = 2,
    eDataType_EntityRegion = 3,
};

class DataFixer
{
public:
    static constexpr int kLegacyVersion  = 1;
    static constexpr int kCurrentVersion = 14;

    static void Stamp(CompoundTag *tag);
    static void Update(eDATA_TYPE type, CompoundTag *tag);

private:
    struct Fix
    {
        int version;
        eDATA_TYPE type;
        void (*apply)(CompoundTag *tag);
    };

    static const char *s_versionTagName;
    static const Fix s_fixes[];

    static void ChunkFlatArraysToPalettedSections(CompoundTag *tag);
    static void ChunkFacesToBlockStateProperties(CompoundTag *tag);
    static void ChunkColumnBiomesToSectionBiomes(CompoundTag *tag);
    static void ChunkIceSheetFacesToDirectionOrder(CompoundTag *tag);
    static void ChunkRemoveWorldBorderBlock(CompoundTag *tag);
    static void ChunkBlockIdsToNames(CompoundTag *tag);
    static void PlayerBlockInteractionRangeDefault(CompoundTag *tag);
    static void PlayerStepHeightDefault(CompoundTag *tag);
    static void RenameItemId(CompoundTag *tag, const char *from, const char *to);
    static void ItemWhiteDyeToBoneMeal(CompoundTag *tag);
    static void ItemBedToRedBed(CompoundTag *tag);
    static void ItemBrownDyeToCocoaBeans(CompoundTag *tag);
    static void ItemLanternToOrangeLantern(CompoundTag *tag);
    static void EntityPositionToLists(CompoundTag *tag);
    static void EntityRegionPositionToLists(CompoundTag *tag);
};
