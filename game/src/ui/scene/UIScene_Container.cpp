#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_Container.h"

#include <string>

#include "../../core/Everstead.h"
#include "../../entity/player/LocalPlayer.h"
#include "../../world/blockentity/ChestBlockEntity.h"
#include "../../world/inventory/InventoryMenu.h"
#include "../../world/item/Item.h"

UIScene_Container::UIScene_Container()
    : UIScene_AbstractContainer(eUIScene_Container, Everstead::GetInstance()->GetLocalPlayer()->GetChestMenuId(),
                                Everstead::GetInstance()->GetLocalPlayer()->GetOpenContainer())
{
    m_containerSlots = new UIControl_SlotList(Inventory::s_columns, 1);
    m_containerSlots->SetContainer(Everstead::GetInstance()->GetLocalPlayer()->GetOpenContainer());
    m_input->AddSlotList(m_containerSlots, 0);

    UpdateLayout();
    m_groupRoot->AddComponent(m_containerSlots);
}

void UIScene_Container::UpdateLayout()
{
    float rowsHeight = (float) m_containerSlots->GetRows() * 18.0f;
    float height     = rowsHeight + 114.0f;
    LayoutPanel(height);
    m_containerSlots->SetCenterOffset(0.0f, (rowsHeight - height) * 0.5f + 18.0f);
}

void UIScene_Container::RenderLabels()
{
    Everstead *everstead = Everstead::GetInstance();
    std::string title    = m_containerSlots->GetRows() > ChestBlockEntity::s_rows ? "container.chestDouble" : "container.chest";
    uint8_t menuId       = everstead->GetLocalPlayer()->GetChestMenuId();
    if (menuId == InventoryMenu::s_hopperMenuId)
    {
        title = "container.hopper";
    }
    else if (menuId == InventoryMenu::s_dispenserMenuId)
    {
        title = "container.dispenser";
    }
    else if (menuId == InventoryMenu::s_dropperMenuId)
    {
        title = "container.dropper";
    }

    DrawLabel(everstead->GetString(title), 8.0f, 6.0f, false);
}

#endif
