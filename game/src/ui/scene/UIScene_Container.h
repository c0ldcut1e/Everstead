#pragma once

#include "UIScene_AbstractContainer.h"

class UIScene_Container : public UIScene_AbstractContainer
{
public:
    UIScene_Container();

private:
    void UpdateLayout() override;
    void RenderLabels() override;

    UIControl_SlotList *m_containerSlots;
};
