#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_Inventory.h"

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <memory>

#include <SDL2/SDL.h>

#include "../../core/Everstead.h"
#include "../../core/GameInfo.h"
#include "../../entity/player/GameType.h"
#include "../../entity/player/LocalPlayer.h"
#include "../../input/InputManager.h"
#include "../../rendering/Font.h"
#include "../../rendering/GlStateManager.h"
#include "../../utils/Time.h"
#include "../../world/item/Item.h"
#include "../Screen.h"
#include "../UIItemRenderer.h"
#include "../UIPanelProperties.h"

class UIPanelProperties_Inventory : public UIPanelProperties
{
public:
    UIPanelProperties_Inventory() : UIPanelProperties(0.0f, 0.0f, 0.0f, 0.0f, 1.0f)
    {}

    const char *GetTexturePath() const override
    {
        return "assets/texture/ui/control/tabbedpanel9grid.png";
    }
};

int UIScene_Inventory::s_lastTabIndex = 0;

UIScene_Inventory::UIScene_Inventory() : UIScene(eUIScene_Inventory)
{
    m_groupRoot           = new UIGroup("Group_Inventory");
    m_background          = new UIComponent_Panel(std::make_unique<UIPanelProperties_Inventory>());
    m_scrollbar           = new UIComponent_Scrollbar();
    m_hotbarSlots         = new UIControl_SlotList(Inventory::s_hotbarSize, 1);
    m_itemSlots           = new UIControl_SlotList(kColumns, kVisibleRows);
    m_itemContainer       = new CreativeContainer(kColumns, kVisibleRows);
    m_searchInput         = new UIControl_TextInput(0.0f, 0.0f, 0.0f, 0.0f);
    m_inventorySlots      = new UIControl_SlotList(Inventory::s_hotbarSize, 3);
    m_armorSlots          = new UIControl_SlotList(1, 4);
    m_craftingSlots       = new UIControl_SlotList(2, 2);
    m_craftingOutputSlots = new UIControl_SlotList(1, 1);
    m_deleteSlot          = new UIControl_SlotList(1, 1);
    m_playerBackground    = new UIComponent_Image("assets/texture/ui/scene/inventory_player_background.png");
    m_playerPreview       = new UIComponent_PlayerPreview();
    m_craftingArrow       = new UIComponent_Image("assets/texture/ui/scene/arrow.png");
    m_selectedFirstTab    = new UIComponent_Image("assets/texture/ui/scene/creative_tab_selected_2.png");
    m_selectedMiddleTab   = new UIComponent_Image("assets/texture/ui/scene/creative_tab_selected_3.png");
    m_selectedLastTab     = new UIComponent_Image("assets/texture/ui/scene/creative_tab_selected.png");
    m_selectedTabIndex    = 0;
    m_tabY                = 0.0f;
    m_tabWidth            = 0.0f;
    m_tabHeight           = 0.0f;
    m_tabPage             = 0;
    m_pageSwitcher        = new UIComponent_PageSwitcher();
    m_leftDown            = false;
    m_stickScroll         = 0.0f;
    m_creative            = GameType::HasInfiniteItems(Everstead::GetInstance()->GetLocalPlayer()->GetGameType());
    Inventory *inventory  = Everstead::GetInstance()->GetLocalPlayer()->GetInventory();
    m_hotbarSlots->SetInventory(inventory, 0);
    m_inventorySlots->SetInventory(inventory, Inventory::s_hotbarSize);
    m_armorSlots->SetInventory(inventory, Inventory::s_totalSize);
    m_armorSlots->SetEmptyIcons({"assets/texture/ui/control/armor_slot/helmet.png", "assets/texture/ui/control/armor_slot/chestplate.png",
                                 "assets/texture/ui/control/armor_slot/leggings.png", "assets/texture/ui/control/armor_slot/boots.png"});

    CraftingContainer *crafting = Everstead::GetInstance()->GetLocalPlayer()->GetInventoryCraftingContainer();
    m_input                     = new UIContainerInput(InventoryMenu::s_creativeMenuId, crafting);
    m_itemSlots->SetContainer(m_itemContainer);
    m_input->AddSlotList(m_hotbarSlots, 0);
    m_input->AddSlotList(m_itemSlots, InventoryMenu::GetCreativeSlot(0, 0));
    m_input->AddSlotList(m_inventorySlots, Inventory::s_hotbarSize);
    m_input->AddSlotList(m_armorSlots, Inventory::s_totalSize);
    m_craftingSlots->SetContainerSlots(crafting, CraftingContainer::s_gridSlot);
    m_craftingOutputSlots->SetContainerSlots(crafting, CraftingContainer::s_resultSlot);
    m_input->AddSlotList(m_craftingSlots, Inventory::s_slotCount + CraftingContainer::s_gridSlot);
    m_input->AddSlotList(m_craftingOutputSlots, Inventory::s_slotCount + CraftingContainer::s_resultSlot);
    m_deleteSlot->SetEmptyIcons({"assets/texture/ui/scene/delete.png"});
    m_deleteSlot->SetVisible(m_creative);
    if (m_creative)
    {
        m_input->AddSlotList(m_deleteSlot, InventoryMenu::s_deleteSlot);
    }

    for (int i = 0; i < kTabCount; i++)
    {
        m_tabs[i] = new UIComponent_Image("assets/texture/ui/scene/creative_tab.png");
        m_tabX[i] = 0.0f;
        m_groupRoot->AddComponent(m_tabs[i]);
    }

    m_groupRoot->AddComponent(m_background);
    m_groupRoot->AddComponent(m_selectedFirstTab);
    m_groupRoot->AddComponent(m_selectedMiddleTab);
    m_groupRoot->AddComponent(m_selectedLastTab);
    m_groupRoot->AddComponent(m_scrollbar);
    m_groupRoot->AddComponent(m_pageSwitcher);
    m_groupRoot->AddComponent(m_searchInput);
    m_groupRoot->AddComponent(m_hotbarSlots);
    m_groupRoot->AddComponent(m_deleteSlot);
    m_groupRoot->AddComponent(m_itemSlots);
    m_groupRoot->AddComponent(m_inventorySlots);
    m_groupRoot->AddComponent(m_playerBackground);
    m_groupRoot->AddComponent(m_playerPreview);
    m_groupRoot->AddComponent(m_armorSlots);
    m_groupRoot->AddComponent(m_craftingSlots);
    m_groupRoot->AddComponent(m_craftingArrow);
    m_groupRoot->AddComponent(m_craftingOutputSlots);
    AddGroup(m_groupRoot);
    SelectTab(m_creative ? s_lastTabIndex : CreativeTabs::s_inventoryTab);
    UpdateLayout();
}

UIScene_Inventory::~UIScene_Inventory()
{
    delete m_input;
    delete m_itemContainer;
}

void UIScene_Inventory::Tick()
{
    if (!IsVisible())
    {
        return;
    }

    UIScene::Tick();
    InputManager *input = Everstead::GetInstance()->GetInputManager();
    if (input->IsUiTextInputFocused() && input->ConsumeUiTextCancelPress())
    {
        SetSearchFocused(false);
    }

    if (m_searchInput->GetText() != m_searchText)
    {
        m_searchText = m_searchInput->GetText();
        UpdateVisibleItems();
    }

    int scrollDelta = Everstead::GetInstance()->GetInputManager()->ConsumeUiScrollDelta();
    if (scrollDelta != 0 && m_selectedTabIndex != CreativeTabs::s_inventoryTab)
    {
        if (m_scrollbar->ScrollBy(-scrollDelta))
        {
            RefreshItems();
        }
    }

    float stickY = input->IsUsingController() ? input->GetControllerLookStickY() : 0.0f;
    if (stickY == 0.0f || m_selectedTabIndex == CreativeTabs::s_inventoryTab)
    {
        m_stickScroll = 0.0f;
    }
    else
    {
        m_stickScroll -= stickY * fabsf(stickY) * kStickScrollRowsPerSecond * (float) Time::GetDelta();
        int rows = (int) m_stickScroll;
        if (rows != 0)
        {
            m_stickScroll -= (float) rows;
            if (m_scrollbar->ScrollBy(rows))
            {
                RefreshItems();
            }
        }
    }

    if (input->IsControllerButtonPressed(eControllerButton_LeftShoulder))
    {
        SelectNextTab(-1);
    }
    else if (input->IsControllerButtonPressed(eControllerButton_RightShoulder))
    {
        SelectNextTab(1);
    }

    int mouseX     = 0;
    int mouseY     = 0;
    bool leftDown  = (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    bool leftPress = leftDown && !m_leftDown;
    m_leftDown     = leftDown;
    if (leftPress)
    {
        Everstead *everstead = Everstead::GetInstance();
        int actualWidth      = everstead->GetWidth();
        int actualHeight     = everstead->GetHeight();
        int tab              = GetTabAt(Screen::ToVirtualX((float) mouseX, actualWidth, actualHeight),
                                        Screen::ToVirtualY((float) mouseY, actualWidth, actualHeight));
        if (tab >= 0)
        {
            SelectTab(tab);
        }

        int direction = m_pageSwitcher->GetDirectionAt(Screen::ToVirtualX((float) mouseX, actualWidth, actualHeight),
                                                       Screen::ToVirtualY((float) mouseY, actualWidth, actualHeight));
        if (direction != 0)
        {
            everstead->GetUiController()->PlayUiSound("ui.press");
            ShowTabPage(m_tabPage + direction);
        }

        Screen::Rect searchRect = m_searchInput->GetActualRect(actualWidth, actualHeight);
        bool searchPressed      = m_searchInput->IsVisible() && (float) mouseX >= searchRect.x && (float) mouseX < searchRect.x + searchRect.width &&
                                  (float) mouseY >= searchRect.y && (float) mouseY < searchRect.y + searchRect.height;
        SetSearchFocused(searchPressed);
    }

    if (input->IsControllerButtonPressed(eControllerButton_South))
    {
        Everstead *everstead = Everstead::GetInstance();
        int tab              = GetTabAt(Screen::ToVirtualX(m_input->GetPointerX(), everstead->GetWidth(), everstead->GetHeight()),
                                        Screen::ToVirtualY(m_input->GetPointerY(), everstead->GetWidth(), everstead->GetHeight()));
        if (tab >= 0)
        {
            SelectTab(tab);
        }

        int direction = m_pageSwitcher->GetDirectionAt(Screen::ToVirtualX(m_input->GetPointerX(), everstead->GetWidth(), everstead->GetHeight()),
                                                       Screen::ToVirtualY(m_input->GetPointerY(), everstead->GetWidth(), everstead->GetHeight()));
        if (direction != 0)
        {
            everstead->GetUiController()->PlayUiSound("ui.press");
            ShowTabPage(m_tabPage + direction);
        }
    }

    if (m_selectedTabIndex != CreativeTabs::s_inventoryTab && input->IsControllerButtonPressed(eControllerButton_West))
    {
        m_input->ClearHotbar();
    }

    UpdateLayout();
    m_input->Tick();
    if (m_input->ConsumeWrapDirection() == 1 && m_selectedTabIndex != CreativeTabs::s_inventoryTab &&
        m_scrollbar->ScrollBy(-m_scrollbar->GetScrollOffset()))
    {
        RefreshItems();
    }

    m_playerPreview->SetLookTarget(m_input->GetPointerX(), m_input->GetPointerY());
}

void UIScene_Inventory::Render()
{
    if (!IsVisible())
    {
        return;
    }

    UIScene::Render();

    Everstead *everstead = Everstead::GetInstance();
    int actualWidth      = everstead->GetWidth();
    int actualHeight     = everstead->GetHeight();
    float hudScale       = Screen::SnapHudScale(everstead->options.GetHudScale(), actualWidth, actualHeight);
    float iconSize       = 16.0f;
    float bodyBottom     = m_tabHeight / hudScale - 3.0f;

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    for (int i = 0; i < kTabCount; i++)
    {
        const ItemInstance &icon = CreativeTabs::GetIcon(i);
        if (icon.IsEmpty() || !IsTabShown(i) || GetTabPosition(i) < 0)
        {
            continue;
        }

        float bodyTop = i == m_selectedTabIndex ? 0.0f : 2.0f;
        float iconY   = m_tabY + ((bodyTop + bodyBottom - iconSize) * 0.5f + 2.0f) * hudScale;
        float iconX   = m_tabX[i] + (m_tabWidth - iconSize * hudScale) * 0.5f;
        Screen::Rect rect =
                Screen::ToActualRect(Screen::SnapVirtualX(iconX, actualWidth, actualHeight), Screen::SnapVirtualY(iconY, actualWidth, actualHeight),
                                     iconSize * hudScale, iconSize * hudScale, actualWidth, actualHeight);
        UIItemRenderer::DrawItem(&icon, rect.x, rect.y, rect.width, rect.height);
    }

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();

    std::string nameKey      = CreativeTabs::GetNameKey(m_selectedTabIndex);
    UIPanelProperties *panel = m_background->GetProperties();
    Font *titleFont          = everstead->GetDefaultFont();
    float titleScale         = titleFont->SnapScale(Screen::ScaleUniform(actualWidth, actualHeight) * hudScale / 4.0f);
    if (!nameKey.empty() && m_selectedTabIndex != CreativeTabs::s_inventoryTab && m_selectedTabIndex != CreativeTabs::s_allItemsTab)
    {
        std::wstring title = everstead->GetString(nameKey);
        float titleWidth   = titleFont->GetWidth(title, titleScale) / Screen::ScaleUniform(actualWidth, actualHeight);
        float titleX       = panel->x + (panel->width - titleWidth) * 0.5f;
        float textX        = Screen::ToActualX(Screen::SnapVirtualX(titleX, actualWidth, actualHeight), actualWidth, actualHeight);
        float textY        = Screen::ToActualY(panel->y + 8.0f * hudScale, actualWidth, actualHeight);
        titleFont->SetNearest(true);
        titleFont->Draw(title, textX, textY + titleFont->GetAscent(titleScale), titleScale, 0xFF404040);
    }

    int hoveredTab = GetTabAt(Screen::ToVirtualX(m_input->GetPointerX(), actualWidth, actualHeight),
                              Screen::ToVirtualY(m_input->GetPointerY(), actualWidth, actualHeight));
    if (hoveredTab >= 0 && !CreativeTabs::GetNameKey(hoveredTab).empty())
    {
        m_tabTooltip.SetText(everstead->GetString(CreativeTabs::GetNameKey(hoveredTab)));
        m_input->PlaceTooltip(&m_tabTooltip, (int) m_input->GetPointerX(), (int) m_input->GetPointerY());
        m_tabTooltip.Render();
    }

    if (m_selectedTabIndex == CreativeTabs::s_inventoryTab)
    {
        UIPanelProperties *properties = m_background->GetProperties();
        Font *font                    = everstead->GetDefaultFont();
        float fontScale               = font->SnapScale(Screen::ScaleUniform(actualWidth, actualHeight) * hudScale / 3.0f);
        float inventoryX              = properties->x + (properties->width - 176.0f * hudScale) * 0.5f;
        float textX                   = Screen::ToActualX(inventoryX + 86.0f * hudScale, actualWidth, actualHeight);
        float textY                   = Screen::ToActualY(properties->y + 16.0f * hudScale, actualWidth, actualHeight);
        font->SetNearest(true);
        font->Draw(Everstead::GetInstance()->GetString("container.crafting"), textX, textY + font->GetAscent(fontScale), fontScale, 0xFF404040);
    }

    m_input->RenderOverlay();
}

void UIScene_Inventory::OnExit()
{
    SetSearchFocused(false);
    m_input->Close();
    UIScene::OnExit();
}

void UIScene_Inventory::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    bool controller = Everstead::GetInstance()->GetInputManager()->IsUsingController();
    if (action == eGameplayAction_Back || (action == eGameplayAction_ToggleInventory && !controller))
    {
        Everstead::GetInstance()->CloseInventory();
    }
}

void UIScene_Inventory::UpdateLayout()
{
    Everstead *everstead          = Everstead::GetInstance();
    int actualWidth               = everstead->GetWidth();
    int actualHeight              = everstead->GetHeight();
    float panelWidth              = 214.0f;
    float panelHeight             = 166.0f;
    float hudScale                = Screen::SnapHudScale(everstead->options.GetHudScale(), actualWidth, actualHeight);
    float width                   = panelWidth * hudScale;
    float height                  = panelHeight * hudScale;
    UIPanelProperties *properties = m_background->GetProperties();
    properties->x                 = Screen::SnapVirtualX(Screen::WIDTH * 0.5f - width * 0.5f, actualWidth, actualHeight);
    properties->y                 = Screen::SnapVirtualY(Screen::HEIGHT * 0.5f - height * 0.5f, actualWidth, actualHeight);
    properties->width             = width;
    properties->height            = height;
    properties->cornerScale       = hudScale / 3.0f;
    m_scrollbar->SetBounds(properties->x + 195.0f * hudScale, properties->y + 22.0f * hudScale, 12.0f * hudScale, 108.0f * hudScale, hudScale / 3.0f);
    m_hotbarSlots->SetCenterOffset(0.0f, 68.0f);
    m_deleteSlot->SetCenterOffset(92.0f, 68.0f);
    m_itemSlots->SetCenterOffset(-5.0f, -7.0f);
    m_inventorySlots->SetCenterOffset(0.0f, 28.0f);
    m_armorSlots->SetCenterOffset(-72.0f, -39.0f);
    m_craftingSlots->SetCenterOffset(17.0f, -40.0f);
    m_craftingOutputSlots->SetCenterOffset(65.0f, -38.0f);
    float inventoryX = properties->x + (width - 176.0f * hudScale) * 0.5f;
    m_playerBackground->SetBounds(inventoryX + 26.0f * hudScale, properties->y + 8.0f * hudScale, 54.0f * hudScale, 72.0f * hudScale);
    m_playerPreview->SetBounds(inventoryX + 26.0f * hudScale, properties->y + 8.0f * hudScale, 54.0f * hudScale, 72.0f * hudScale);
    m_searchInput->SetBounds(properties->x + (width - 90.0f * hudScale) * 0.5f, properties->y + 6.0f * hudScale, 90.0f * hudScale, 9.0f * hudScale);
    m_searchInput->SetTextScale(hudScale / 4.0f);
    m_searchInput->SetCaretScale(1.0f);
    m_craftingArrow->SetBounds(inventoryX + 126.0f * hudScale, properties->y + 38.0f * hudScale, 16.0f * hudScale, 13.0f * hudScale);
    m_input->SetPanelBounds(properties->x, properties->y, properties->width, properties->height);

    float tabOverlap = 3.0f * hudScale;
    m_tabWidth       = 83.0f / 3.0f * hudScale;
    m_tabHeight      = 78.0f / 3.0f * hudScale;
    m_tabY           = properties->y - m_tabHeight + tabOverlap;
    float tabSpacing = (width - m_tabWidth) / (float) (kTabPositions - 1);
    for (int i = 0; i < kTabCount; i++)
    {
        m_tabX[i] = Screen::SnapVirtualX(properties->x + tabSpacing * (float) std::max(GetTabPosition(i), 0), actualWidth, actualHeight);
        m_tabs[i]->SetBounds(m_tabX[i], m_tabY, m_tabWidth, m_tabHeight);
    }

    float buttonSize = 16.0f * hudScale;
    float switcherY  = m_tabY - buttonSize - 2.0f * hudScale;
    m_pageSwitcher->SetBounds(properties->x, switcherY, width, buttonSize, hudScale / 3.0f);
    m_input->SetExtraBounds(properties->x, switcherY, width, m_tabY + m_tabHeight - switcherY);

    m_selectedFirstTab->SetBounds(m_tabX[m_selectedTabIndex], m_tabY, m_tabWidth, m_tabHeight);
    m_selectedMiddleTab->SetBounds(m_tabX[m_selectedTabIndex], m_tabY, m_tabWidth, m_tabHeight);
    m_selectedLastTab->SetBounds(m_tabX[m_selectedTabIndex], m_tabY, m_tabWidth, m_tabHeight);
}

void UIScene_Inventory::SelectTab(int index)
{
    m_selectedTabIndex = index;
    s_lastTabIndex     = index;
    ShowTabPage(index < CreativeTabs::s_allItemsTab ? index / kTabsPerPage : m_tabPage);
    bool inventoryTab = m_selectedTabIndex == CreativeTabs::s_inventoryTab;
    m_inventorySlots->SetVisible(inventoryTab);
    m_armorSlots->SetVisible(inventoryTab);
    m_craftingSlots->SetVisible(inventoryTab);
    m_craftingOutputSlots->SetVisible(inventoryTab);
    m_playerBackground->SetVisible(inventoryTab);
    m_playerPreview->SetVisible(inventoryTab);
    m_craftingArrow->SetVisible(inventoryTab);
    m_input->SetMenuId(inventoryTab || !m_creative ? InventoryMenu::s_playerMenuId : InventoryMenu::s_creativeMenuId);
    m_searchInput->SetVisible(m_selectedTabIndex == CreativeTabs::s_allItemsTab);
    SetSearchFocused(false);
    m_scrollbar->ScrollBy(-m_scrollbar->GetScrollOffset());
    UpdateVisibleItems();
}

void UIScene_Inventory::UpdateVisibleItems()
{
    Everstead *everstead                   = Everstead::GetInstance();
    const std::vector<ItemInstance> &items = CreativeTabs::GetItems(m_selectedTabIndex);
    bool searchTab                         = m_selectedTabIndex == CreativeTabs::s_allItemsTab;
    std::wstring query                     = searchTab ? m_searchText : L"";
    std::transform(query.begin(), query.end(), query.begin(), towlower);

    m_visibleItems.clear();
    for (size_t i = 0; i < items.size(); i++)
    {
        std::wstring name = everstead->GetString(items[i].GetItem()->GetDescriptionId());
        std::transform(name.begin(), name.end(), name.begin(), towlower);
        if (query.empty() || name.find(query) != std::wstring::npos)
        {
            m_visibleItems.push_back((int) i);
        }
    }

    bool inventoryTab = m_selectedTabIndex == CreativeTabs::s_inventoryTab;
    m_itemSlots->SetVisible(!inventoryTab && (GetRowCount() > 0 || searchTab));
    m_scrollbar->SetRange(kVisibleRows, GetRowCount());
    m_scrollbar->SetVisible(m_itemSlots->IsVisible() && m_scrollbar->GetMaxScrollOffset() > 0);
    RefreshItems();
}

void UIScene_Inventory::SetSearchFocused(bool focused)
{
    m_searchInput->SetSelected(focused);
    Everstead::GetInstance()->GetInputManager()->SetUiTextInputFocused(focused);
}

void UIScene_Inventory::SelectNextTab(int direction)
{
    int tab = m_selectedTabIndex;
    do
    {
        tab = (tab + direction + kTabCount) % kTabCount;
    } while (!IsTabShown(tab));

    SelectTab(tab);
}

bool UIScene_Inventory::IsTabShown(int tab) const
{
    return m_creative || tab == CreativeTabs::s_inventoryTab || (tab == CreativeTabs::s_allItemsTab && !kStickFight);
}

int UIScene_Inventory::GetTabPosition(int tab) const
{
    // Category tabs are split into pages while the search and inventory tabs stay on every page
    if (tab == CreativeTabs::s_allItemsTab)
    {
        return kTabsPerPage + 1;
    }

    if (tab == CreativeTabs::s_inventoryTab)
    {
        return kTabsPerPage + 2;
    }

    return tab / kTabsPerPage == m_tabPage ? tab % kTabsPerPage : -1;
}

int UIScene_Inventory::GetTabPageCount() const
{
    return m_creative ? (CreativeTabs::s_allItemsTab + kTabsPerPage - 1) / kTabsPerPage : 1;
}

void UIScene_Inventory::ShowTabPage(int page)
{
    m_tabPage = page;
    for (int i = 0; i < kTabCount; i++)
    {
        m_tabs[i]->SetVisible(i != m_selectedTabIndex && IsTabShown(i) && GetTabPosition(i) >= 0);
    }

    int selectedPosition = GetTabPosition(m_selectedTabIndex);
    m_selectedFirstTab->SetVisible(selectedPosition == 0);
    m_selectedMiddleTab->SetVisible(selectedPosition > 0 && selectedPosition < kTabPositions - 1);
    m_selectedLastTab->SetVisible(selectedPosition == kTabPositions - 1);
    m_pageSwitcher->SetPage(m_tabPage, GetTabPageCount());
}

void UIScene_Inventory::RefreshItems()
{
    const std::vector<ItemInstance> &items = CreativeTabs::GetItems(m_selectedTabIndex);
    int firstIndex                         = m_scrollbar->GetScrollOffset() * kColumns;
    std::vector<int> menuSlots((size_t) m_itemContainer->GetSize());
    for (int i = 0; i < m_itemContainer->GetSize(); i++)
    {
        int visibleIndex = firstIndex + i;
        if (visibleIndex < (int) m_visibleItems.size())
        {
            int itemIndex = m_visibleItems[(size_t) visibleIndex];
            m_itemContainer->SetItem(i, items[(size_t) itemIndex]);
            menuSlots[(size_t) i] = InventoryMenu::GetCreativeSlot(m_selectedTabIndex, itemIndex);
        }
        else
        {
            m_itemContainer->ClearItem(i);
            menuSlots[(size_t) i] = InventoryMenu::GetCreativeSlot(m_selectedTabIndex, (int) items.size() + i);
        }
    }

    m_itemSlots->SetMenuSlots(menuSlots);
}

int UIScene_Inventory::GetRowCount() const
{
    return ((int) m_visibleItems.size() + kColumns - 1) / kColumns;
}

int UIScene_Inventory::GetTabAt(float virtualX, float virtualY) const
{
    if (virtualY < m_tabY || virtualY >= m_tabY + m_tabHeight)
    {
        return -1;
    }

    if (GetTabPosition(m_selectedTabIndex) >= 0 && virtualX >= m_tabX[m_selectedTabIndex] && virtualX < m_tabX[m_selectedTabIndex] + m_tabWidth)
    {
        return m_selectedTabIndex;
    }

    for (int i = 0; i < kTabCount; i++)
    {
        if (IsTabShown(i) && GetTabPosition(i) >= 0 && virtualX >= m_tabX[i] && virtualX < m_tabX[i] + m_tabWidth)
        {
            return i;
        }
    }

    return -1;
}

#endif
