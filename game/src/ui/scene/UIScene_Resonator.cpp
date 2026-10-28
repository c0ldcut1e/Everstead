#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_Resonator.h"

#include "../../core/Everstead.h"
#include "../../entity/player/LocalPlayer.h"
#include "../../world/inventory/ResonatorContainer.h"
#include "../Screen.h"

UIScene_Resonator::UIScene_Resonator()
    : UIScene_AbstractContainer(eUIScene_Resonator, InventoryMenu::s_resonatorMenuId,
                                Everstead::GetInstance()->GetLocalPlayer()->GetResonatorContainer())
{
    m_gearSlots     = new UIControl_SlotList(1, 1);
    m_materialSlots = new UIControl_SlotList(1, 1);
    m_resultSlots   = new UIControl_SlotList(1, 1);
    m_resultSlots->SetSlotSize(26);
    m_arrow       = new UIComponent_Image("assets/texture/ui/scene/arrow.png");
    m_filledArrow = new UIComponent_Image("assets/texture/ui/scene/arrow_filled.png");

    ResonatorContainer *container = Everstead::GetInstance()->GetLocalPlayer()->GetResonatorContainer();
    m_gearSlots->SetContainerSlots(container, ResonatorContainer::s_gearSlot);
    m_materialSlots->SetContainerSlots(container, ResonatorContainer::s_materialSlot);
    m_resultSlots->SetContainerSlots(container, ResonatorContainer::s_resultSlot);
    m_input->AddSlotList(m_gearSlots, ResonatorContainer::s_gearSlot);
    m_input->AddSlotList(m_materialSlots, ResonatorContainer::s_materialSlot);
    m_input->AddSlotList(m_resultSlots, ResonatorContainer::s_resultSlot);

    UpdateLayout();
    m_groupRoot->AddComponent(m_gearSlots);
    m_groupRoot->AddComponent(m_materialSlots);
    m_groupRoot->AddComponent(m_arrow);
    m_groupRoot->AddComponent(m_filledArrow);
    m_groupRoot->AddComponent(m_resultSlots);
}

void UIScene_Resonator::Render()
{
    int progress = Everstead::GetInstance()->GetLocalPlayer()->GetResonatorContainer()->GetProgress();
    m_filledArrow->SetCrop((float) (progress * 16 / ResonatorContainer::s_workDuration) / 16.0f, 1.0f);

    UIScene_AbstractContainer::Render();
}

void UIScene_Resonator::UpdateLayout()
{
    LayoutPanel(166.0f);

    Everstead *everstead          = Everstead::GetInstance();
    float hudScale                = Screen::SnapHudScale(everstead->options.GetHudScale(), everstead->GetWidth(), everstead->GetHeight());
    UIPanelProperties *properties = m_background->GetProperties();
    m_gearSlots->SetCenterOffset(-52.0f, -40.0f);
    m_materialSlots->SetCenterOffset(-24.0f, -40.0f);
    m_resultSlots->SetCenterOffset(37.0f, -40.0f);
    m_arrow->SetBounds(properties->x + 84.0f * hudScale, properties->y + 32.0f * hudScale, 24.0f * hudScale, 19.5f * hudScale);
    m_filledArrow->SetBounds(properties->x + 84.0f * hudScale, properties->y + 32.0f * hudScale, 24.0f * hudScale, 19.5f * hudScale);
}

void UIScene_Resonator::RenderLabels()
{
    DrawLabel(Everstead::GetInstance()->GetString("container.resonator"), (float) s_panelWidth * 0.5f, 6.0f, true);
}

#endif
