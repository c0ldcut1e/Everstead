#pragma once

#include <string>

#include "../item/ItemInstance.h"
#include "AbstractContainer.h"

class Player;

class AnvilContainer : public AbstractContainer
{
public:
    static const int s_inputSlot     = 0;
    static const int s_additionSlot  = 1;
    static const int s_resultSlot    = 2;
    static const int s_slotCount     = 3;
    static const int s_maxNameLength = 50;
    static const int s_tooExpensive  = 40;

    explicit AnvilContainer(Player *player);
    ~AnvilContainer() override = default;

    int GetColumns() const override;
    int GetRows() const override;
    int GetSize() const override;

    ItemInstance *GetItem(int index) override;
    const ItemInstance *GetItem(int index) const override;
    void SetItem(int index, const ItemInstance &item) override;
    void ClearItem(int index) override;

    bool IsEmpty() const override;

    void SetItemName(const std::string &itemName);
    void UpdateResult();
    bool MayPickup() const;
    void OnTake();
    // This returns whether a result was taken since the last call and clears it
    bool PopTaken();
    int GetCost() const;
    void SetCost(int cost);

private:
    Player *m_player;
    ItemInstance m_items[s_slotCount];
    std::string m_itemName;
    int m_cost;
    int m_repairItemCountCost;
    bool m_onlyRenaming;
    bool m_taken;
};
