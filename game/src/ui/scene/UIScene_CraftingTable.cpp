#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_CraftingTable.h"

#include "../../core/Everstead.h"
#include "../../entity/player/LocalPlayer.h"
#include "../Screen.h"

UIScene_CraftingTable::UIScene_CraftingTable()
    : UIScene_AbstractContainer(eUIScene_CraftingTable, InventoryMenu::s_craftingMenuId,
                                Everstead::GetInstance()->GetLocalPlayer()->GetCraftingContainer())
{
    m_gridSlots   = new UIControl_SlotList(3, 3);
    m_arrow       = new UIComponent_Image("assets/texture/ui/scene/arrow.png");
    m_resultSlots = new UIControl_SlotList(1, 1);
    m_resultSlots->SetSlotSize(26);

    CraftingContainer *container = Everstead::GetInstance()->GetLocalPlayer()->GetCraftingContainer();
    m_gridSlots->SetContainerSlots(container, CraftingContainer::s_gridSlot);
    m_resultSlots->SetContainerSlots(container, CraftingContainer::s_resultSlot);
    m_input->AddSlotList(m_gridSlots, CraftingContainer::s_gridSlot);
    m_input->AddSlotList(m_resultSlots, CraftingContainer::s_resultSlot);

    UpdateLayout();
    m_groupRoot->AddComponent(m_gridSlots);
    m_groupRoot->AddComponent(m_arrow);
    m_groupRoot->AddComponent(m_resultSlots);
}

void UIScene_CraftingTable::UpdateLayout()
{
    LayoutPanel(166.0f);

    Everstead *everstead          = Everstead::GetInstance();
    float hudScale                = Screen::SnapHudScale(everstead->options.GetHudScale(), everstead->GetWidth(), everstead->GetHeight());
    UIPanelProperties *properties = m_background->GetProperties();
    m_gridSlots->SetCenterOffset(-31.0f, -39.0f);
    m_arrow->SetBounds(properties->x + 90.0f * hudScale, properties->y + 34.0f * hudScale, 24.0f * hudScale, 19.5f * hudScale);
    m_resultSlots->SetCenterOffset(45.0f, -39.0f);
}

void UIScene_CraftingTable::RenderLabels()
{
    DrawLabel(Everstead::GetInstance()->GetString("container.crafting"), (float) s_panelWidth * 0.5f, 6.0f, true);
}

#endif
