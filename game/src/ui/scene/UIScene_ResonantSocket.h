#pragma once

#include "UIScene_AbstractContainer.h"

class UIScene_ResonantSocket : public UIScene_AbstractContainer
{
public:
    UIScene_ResonantSocket();

private:
    void UpdateLayout() override;
    void RenderLabels() override;

    UIControl_SlotList *m_socketSlots;
};
