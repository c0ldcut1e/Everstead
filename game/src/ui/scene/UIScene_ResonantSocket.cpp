#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_ResonantSocket.h"

#include "../../core/Everstead.h"
#include "../../entity/player/LocalPlayer.h"

UIScene_ResonantSocket::UIScene_ResonantSocket()
    : UIScene_AbstractContainer(eUIScene_ResonantSocket, InventoryMenu::s_resonantSocketMenuId,
                                Everstead::GetInstance()->GetLocalPlayer()->GetResonantSocketContainer())
{
    m_socketSlots = new UIControl_SlotList(1, 1);
    m_socketSlots->SetContainerSlots(Everstead::GetInstance()->GetLocalPlayer()->GetResonantSocketContainer(), 0);
    m_input->AddSlotList(m_socketSlots, 0);

    UpdateLayout();
    m_groupRoot->AddComponent(m_socketSlots);
}

void UIScene_ResonantSocket::UpdateLayout()
{
    LayoutPanel(166.0f);
    m_socketSlots->SetCenterOffset(0.0f, -40.0f);
}

void UIScene_ResonantSocket::RenderLabels()
{
    DrawLabel(Everstead::GetInstance()->GetString("container.resonant_socket"), (float) s_panelWidth * 0.5f, 6.0f, true);
}

#endif
