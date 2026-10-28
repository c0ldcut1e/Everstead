#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_SignEdit.h"

#include <string>

#include "../../core/Everstead.h"
#include "../../entity/player/MultiPlayerLocalPlayer.h"
#include "../../network/listener/ClientConnection.h"
#include "../../network/packet/c2s/SignUpdateC2SPacket.h"
#include "../../utils/StringUtils.h"
#include "../../world/Level.h"
#include "../../world/blockentity/SignBlockEntity.h"
#include "../Screen.h"

UIScene_SignEdit::UIScene_SignEdit() : UIScene(eUIScene_SignEdit)
{
    Everstead *everstead = Everstead::GetInstance();
    m_pos                = BlockPos(0, 0, 0);
    m_front              = true;
    m_groupRoot          = new UIGroup("Group_SignEdit");
    m_panel              = new UIControl_MultiList(0.0f, 0.0f, 430.0f, 0.0f);
    m_panel->SetContentPadding(18.0f, 16.0f);
    m_panel->SetLabelRowHeight(34.0f);
    m_panel->SetLabelRowWidth(394.0f);
    m_panel->SetTextInputRowWidth(394.0f);
    m_panel->SetTextInputRowHeight(46.0f);
    m_panel->SetButtonRowWidth(394.0f);
    m_panel->SetButtonRowHeight(46.0f);
    m_panel->SetRowSpacing(8.0f);
    m_panel->SetButtonPressedCallback(HandleButtonPressed, this);
    m_panel->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);
    m_panel->AddNewLabel(0, everstead->GetString("sign.edit"));
    for (uint32_t i = 0; i < 4; i++)
    {
        m_panel->AddNewTextInput(1 + i)->SetMaxLength(15);
    }

    m_panel->AddNewSpacer(0, 8.0f);
    m_panel->AddNewButton(5, everstead->GetString("gui.done"));
    m_panel->ResizeHorizontal();
    m_panel->ResizeVertical();
    m_panel->SetPosition((Screen::WIDTH - m_panel->GetWidth()) * 0.5f, (Screen::HEIGHT - m_panel->GetHeight()) * 0.5f);

    m_groupRoot->AddComponent(m_panel);
    AddGroup(m_groupRoot);
}

UIScene_SignEdit::~UIScene_SignEdit()
{}

void UIScene_SignEdit::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action == eGameplayAction_Back)
    {
        Finish();
    }
}

void UIScene_SignEdit::SetSign(const BlockPos &pos, bool front)
{
    m_pos                 = pos;
    m_front               = front;
    SignBlockEntity *sign = dynamic_cast<SignBlockEntity *>(Everstead::GetInstance()->GetLevel()->GetBlockEntity(pos));
    if (!sign)
    {
        return;
    }

    for (uint32_t i = 0; i < 4; i++)
    {
        m_panel->SetTextInputText(1 + i, StringUtils::ToUtf16(sign->GetText(front)->messages[i]));
    }
}

void UIScene_SignEdit::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    (void) id;
    ((UIScene_SignEdit *) context)->Finish();
}

void UIScene_SignEdit::Finish()
{
    Everstead *everstead = Everstead::GetInstance();
    std::string lines[4];
    for (uint32_t i = 0; i < 4; i++)
    {
        lines[i] = StringUtils::ToUtf8(m_panel->GetTextInputText(1 + i));
    }

    everstead->GetLocalPlayer()->GetClientConnection()->Send(std::shared_ptr<Packet>(new SignUpdateC2SPacket(m_pos, m_front, lines)));
    everstead->CloseInventory();
}

#endif
