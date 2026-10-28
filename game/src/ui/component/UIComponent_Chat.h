#pragma once

#include "UIComponent.h"

class UIComponent_Chat : public UIComponent
{
public:
    UIComponent_Chat();
    ~UIComponent_Chat() override;

    void Tick() override;
    void Render() override;
};
