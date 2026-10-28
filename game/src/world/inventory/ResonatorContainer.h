#pragma once

#include "../../utils/Random.h"
#include "../item/ItemInstance.h"
#include "AbstractContainer.h"

class CompoundTag;

class ResonatorContainer : public AbstractContainer
{
public:
    static const int s_gearSlot     = 0;
    static const int s_materialSlot = 1;
    static const int s_resultSlot   = 2;
    static const int s_slotCount    = 3;
    static const int s_workDuration = 100;

    enum eRESONATOR_MODE
    {
        eResonatorMode_None,
        eResonatorMode_Upgrade,
        eResonatorMode_Fracture,
        eResonatorMode_Stabilize
    };

    ResonatorContainer();
    ~ResonatorContainer() override = default;

    int GetColumns() const override;
    int GetRows() const override;
    int GetSize() const override;

    ItemInstance *GetItem(int index) override;
    const ItemInstance *GetItem(int index) const override;
    void SetItem(int index, const ItemInstance &item) override;
    void ClearItem(int index) override;

    bool IsEmpty() const override;

    static Item *GetUpgradeResult(const ItemInstance &gear);
    static bool IsGear(const ItemInstance &item);
    static bool IsMaterial(const ItemInstance &item);

    // This returns true when the progress or items changed, and sets completed to the mode that just finished
    bool Tick(Random *random, eRESONATOR_MODE *completed);
    int GetProgress() const;
    void SetProgress(int progress);

    void Save(CompoundTag *tag) const;
    void Load(const CompoundTag *tag);

private:
    eRESONATOR_MODE GetMode() const;

    ItemInstance m_items[s_slotCount];
    int m_progress;
};
