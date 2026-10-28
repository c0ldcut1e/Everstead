#pragma once

#include "../../world/inventory/CreativeContainer.h"
#include "../../world/item/CreativeTabs.h"
#include "../UIContainerInput.h"
#include "../component/UIComponent_Image.h"
#include "../component/UIComponent_PageSwitcher.h"
#include "../component/UIComponent_Panel.h"
#include "../component/UIComponent_PlayerPreview.h"
#include "../component/UIComponent_Scrollbar.h"
#include "../component/UIComponent_Tooltip.h"
#include "../control/UIControl_SlotList.h"
#include "../control/UIControl_TextInput.h"
#include "UIScene.h"

class UIScene_Inventory : public UIScene
{
public:
    UIScene_Inventory();
    ~UIScene_Inventory() override;

    void Tick() override;
    void Render() override;
    void OnExit() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static constexpr int kTabCount                   = CreativeTabs::s_tabCount;
    static constexpr int kTabsPerPage                = 5;
    static constexpr int kTabPositions               = kTabsPerPage + 3;
    static constexpr int kColumns                    = 10;
    static constexpr int kVisibleRows                = 6;
    static constexpr float kStickScrollRowsPerSecond = 12.0f;

    void UpdateLayout();
    void SelectTab(int index);
    void RefreshItems();
    void UpdateVisibleItems();
    void SetSearchFocused(bool focused);
    int GetRowCount() const;
    void SelectNextTab(int direction);
    bool IsTabShown(int tab) const;
    int GetTabPosition(int tab) const;
    int GetTabPageCount() const;
    void ShowTabPage(int page);
    int GetTabAt(float virtualX, float virtualY) const;

    UIGroup *m_groupRoot;
    UIComponent_Image *m_tabs[kTabCount];
    UIComponent_Image *m_selectedFirstTab;
    UIComponent_Image *m_selectedMiddleTab;
    UIComponent_Image *m_selectedLastTab;
    float m_tabX[kTabCount];
    float m_tabY;
    float m_tabWidth;
    float m_tabHeight;
    int m_tabPage;
    UIComponent_PageSwitcher *m_pageSwitcher;
    bool m_leftDown;
    float m_stickScroll;
    UIComponent_Panel *m_background;
    UIComponent_Scrollbar *m_scrollbar;
    UIControl_SlotList *m_hotbarSlots;
    UIControl_SlotList *m_itemSlots;
    UIControl_SlotList *m_inventorySlots;
    UIControl_SlotList *m_armorSlots;
    UIControl_SlotList *m_craftingSlots;
    UIControl_SlotList *m_craftingOutputSlots;
    UIControl_SlotList *m_deleteSlot;
    UIComponent_Image *m_playerBackground;
    UIComponent_PlayerPreview *m_playerPreview;
    UIComponent_Image *m_craftingArrow;
    CreativeContainer *m_itemContainer;
    UIContainerInput *m_input;
    UIControl_TextInput *m_searchInput;
    UIComponent_Tooltip m_tabTooltip;
    std::vector<int> m_visibleItems;
    std::wstring m_searchText;
    int m_selectedTabIndex;
    bool m_creative;

    static int s_lastTabIndex;
};
