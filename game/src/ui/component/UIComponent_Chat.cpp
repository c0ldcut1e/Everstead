#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_Chat.h"

#include "../../core/Everstead.h"
#include "../Gui.h"

UIComponent_Chat::UIComponent_Chat() : UIComponent()
{}

UIComponent_Chat::~UIComponent_Chat()
{}

void UIComponent_Chat::Tick()
{
    Everstead::GetInstance()->GetGui()->Tick();
}

void UIComponent_Chat::Render()
{
    Everstead *everstead = Everstead::GetInstance();
    everstead->GetGui()->RenderChat(everstead->GetWidth(), everstead->GetHeight());
}

#endif
