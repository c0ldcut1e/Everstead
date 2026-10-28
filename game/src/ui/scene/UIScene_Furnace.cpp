#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_Furnace.h"

#include "../../core/Everstead.h"
#include "../../entity/player/LocalPlayer.h"
#include "../../world/inventory/FurnaceContainer.h"
#include "../Screen.h"

UIScene_Furnace::UIScene_Furnace()
    : UIScene_AbstractContainer(eUIScene_Furnace, InventoryMenu::s_furnaceMenuId, Everstead::GetInstance()->GetLocalPlayer()->GetFurnaceContainer())
{
    m_inputSlots  = new UIControl_SlotList(1, 1);
    m_fuelSlots   = new UIControl_SlotList(1, 1);
    m_resultSlots = new UIControl_SlotList(1, 1);
    m_resultSlots->SetSlotSize(26);
    m_flame       = new UIComponent_Image("assets/texture/ui/scene/furnace_flame.png");
    m_litFlame    = new UIComponent_Image("assets/texture/ui/scene/furnace_flame_lit.png");
    m_arrow       = new UIComponent_Image("assets/texture/ui/scene/arrow.png");
    m_filledArrow = new UIComponent_Image("assets/texture/ui/scene/arrow_filled.png");

    FurnaceContainer *container = Everstead::GetInstance()->GetLocalPlayer()->GetFurnaceContainer();
    m_inputSlots->SetContainerSlots(container, FurnaceContainer::s_inputSlot);
    m_fuelSlots->SetContainerSlots(container, FurnaceContainer::s_fuelSlot);
    m_resultSlots->SetContainerSlots(container, FurnaceContainer::s_resultSlot);
    m_input->AddSlotList(m_inputSlots, FurnaceContainer::s_inputSlot);
    m_input->AddSlotList(m_fuelSlots, FurnaceContainer::s_fuelSlot);
    m_input->AddSlotList(m_resultSlots, FurnaceContainer::s_resultSlot);

    UpdateLayout();
    m_groupRoot->AddComponent(m_inputSlots);
    m_groupRoot->AddComponent(m_fuelSlots);
    m_groupRoot->AddComponent(m_flame);
    m_groupRoot->AddComponent(m_litFlame);
    m_groupRoot->AddComponent(m_arrow);
    m_groupRoot->AddComponent(m_filledArrow);
    m_groupRoot->AddComponent(m_resultSlots);
}

void UIScene_Furnace::Render()
{
    FurnaceContainer *furnace = Everstead::GetInstance()->GetLocalPlayer()->GetFurnaceContainer();
    int litDuration           = furnace->GetLitDuration() == 0 ? FurnaceContainer::s_burnInterval : furnace->GetLitDuration();
    m_litFlame->SetVisible(furnace->IsLit());
    m_litFlame->SetCrop(1.0f, (float) ((furnace->GetLitTime() * 13 + litDuration - 1) / litDuration) / 14.0f);
    m_filledArrow->SetCrop((float) (furnace->GetCookTime() * 16 / (FurnaceContainer::s_burnInterval - 1)) / 16.0f, 1.0f);

    UIScene_AbstractContainer::Render();
}

void UIScene_Furnace::UpdateLayout()
{
    LayoutPanel(166.0f);

    Everstead *everstead          = Everstead::GetInstance();
    float hudScale                = Screen::SnapHudScale(everstead->options.GetHudScale(), everstead->GetWidth(), everstead->GetHeight());
    UIPanelProperties *properties = m_background->GetProperties();
    m_inputSlots->SetCenterOffset(-24.0f, -58.0f);
    m_fuelSlots->SetCenterOffset(-24.0f, -22.0f);
    m_resultSlots->SetCenterOffset(37.0f, -40.0f);
    m_flame->SetBounds(properties->x + 56.5f * hudScale, properties->y + 35.5f * hudScale, 14.0f * hudScale, 14.0f * hudScale);
    m_litFlame->SetBounds(properties->x + 56.5f * hudScale, properties->y + 35.5f * hudScale, 14.0f * hudScale, 14.0f * hudScale);
    m_arrow->SetBounds(properties->x + 84.0f * hudScale, properties->y + 32.0f * hudScale, 24.0f * hudScale, 19.5f * hudScale);
    m_filledArrow->SetBounds(properties->x + 84.0f * hudScale, properties->y + 32.0f * hudScale, 24.0f * hudScale, 19.5f * hudScale);
}

void UIScene_Furnace::RenderLabels()
{
    DrawLabel(Everstead::GetInstance()->GetString("container.furnace"), (float) s_panelWidth * 0.5f, 6.0f, true);
}

#endif
