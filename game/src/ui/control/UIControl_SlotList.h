#pragma once

#include <string>
#include <utility>
#include <vector>

#include "../../entity/Inventory.h"
#include "../../world/inventory/AbstractContainer.h"
#include "../Screen.h"
#include "../Slot.h"
#include "../UITextureAtlas.h"
#include "UIControl.h"

class UIControl_SlotList : public UIControl
{
public:
    UIControl_SlotList(int columns, int rows);
    ~UIControl_SlotList() override;

    void SetContainer(AbstractContainer *container);
    void SetContainerSlots(AbstractContainer *container, int firstSlot);
    AbstractContainer *GetContainer() const;
    void SetInventory(Inventory *inventory, int firstSlot);
    void SetEmptyIcons(const std::vector<std::string> &paths);
    void SetCenterOffset(float x, float y);
    void SetSlotSize(int slotSize);
    void SetMenuSlotBase(int base);
    void SetMenuSlots(const std::vector<int> &menuSlots);
    int GetMenuSlot(int index) const;
    int GetSlotIndex(int menuSlot) const;
    int GetMenuSlotAt(float actualX, float actualY) const;
    void SetHoveredMenuSlot(int menuSlot);
    int GetMenuSlotBase() const;
    Screen::Rect GetSlotRect(int index) const;
    void SetHighlightedMenuSlots(const std::vector<int> *slots);
    void SetPreviewItems(const std::vector<std::pair<int, ItemInstance>> *previewItems);

    int GetColumns() const;
    int GetRows() const;
    const std::vector<Slot> &GetSlots() const;

    void Tick() override;
    void Render() override;

private:
    static constexpr int kMaxRows = 9;
    static const int s_padding    = 7;

    void UpdateLayout();
    const ItemInstance *GetSlotItem(int index) const;
    bool IsSlotHighlighted(int index) const;
    void DrawSlotHighlight(int index, float slotSize, int actualWidth, int actualHeight);
    float GetHudScale() const;

    std::vector<Slot> m_slots;
    AbstractContainer *m_container;
    Inventory *m_inventory;
    UITextureAtlas::Region m_slotRegion;
    int m_columns;
    int m_rows;
    int m_slotSize;
    int m_hoveredSlot;
    int m_menuSlotBase;
    std::vector<int> m_menuSlots;
    const std::vector<int> *m_highlightedMenuSlots;
    const std::vector<std::pair<int, ItemInstance>> *m_previewItems;
    std::vector<UITextureAtlas::Region> m_emptyIcons;
    int m_firstSlot;
    float m_centerOffsetX;
    float m_centerOffsetY;
};
