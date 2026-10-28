#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_Anvil.h"

#include <memory>

#include <SDL2/SDL.h>

#include "../../core/Everstead.h"
#include "../../entity/player/GameType.h"
#include "../../entity/player/MultiPlayerLocalPlayer.h"
#include "../../input/InputManager.h"
#include "../../network/listener/ClientConnection.h"
#include "../../network/packet/c2s/RenameItemC2SPacket.h"
#include "../../rendering/Font.h"
#include "../../rendering/GlStateManager.h"
#include "../../utils/StringUtils.h"
#include "../../world/inventory/AnvilContainer.h"
#include "../ChatMessageParser.h"
#include "../Screen.h"
#include "../UIItemRenderer.h"
#include "../UIPanelProperties.h"

#include <nlohmann/json.hpp>

UIScene_Anvil::UIScene_Anvil()
    : UIScene_AbstractContainer(eUIScene_Anvil, InventoryMenu::s_anvilMenuId, Everstead::GetInstance()->GetLocalPlayer()->GetAnvilContainer())
{
    m_inputSlots      = new UIControl_SlotList(1, 1);
    m_additionSlots   = new UIControl_SlotList(1, 1);
    m_resultSlots     = new UIControl_SlotList(1, 1);
    m_hammer          = new UIComponent_Image("assets/texture/ui/scene/hammer.png");
    m_nameAvailable   = new UIComponent_Image("assets/texture/ui/scene/anvil_input_available.png");
    m_nameUnavailable = new UIComponent_Image("assets/texture/ui/scene/anvil_input_unavailable.png");
    m_plus            = new UIComponent_Image("assets/texture/ui/scene/plus.png");
    m_arrow           = new UIComponent_Image("assets/texture/ui/scene/arrow.png");
    m_xArrow          = new UIComponent_Image("assets/texture/ui/scene/x_arrow.png");
    m_nameInput       = new UIControl_TextInput(0.0f, 0.0f, 0.0f, 0.0f);
    m_nameInput->SetMaxLength(AnvilContainer::s_maxNameLength);
    m_nameInput->SetBackgroundVisible(false);
    m_leftDown = false;

    AnvilContainer *container = Everstead::GetInstance()->GetLocalPlayer()->GetAnvilContainer();
    m_inputSlots->SetContainerSlots(container, AnvilContainer::s_inputSlot);
    m_additionSlots->SetContainerSlots(container, AnvilContainer::s_additionSlot);
    m_resultSlots->SetContainerSlots(container, AnvilContainer::s_resultSlot);
    m_input->AddSlotList(m_inputSlots, AnvilContainer::s_inputSlot);
    m_input->AddSlotList(m_additionSlots, AnvilContainer::s_additionSlot);
    m_input->AddSlotList(m_resultSlots, AnvilContainer::s_resultSlot);

    UpdateLayout();
    m_groupRoot->AddComponent(m_hammer);
    m_groupRoot->AddComponent(m_nameAvailable);
    m_groupRoot->AddComponent(m_nameUnavailable);
    m_groupRoot->AddComponent(m_nameInput);
    m_groupRoot->AddComponent(m_inputSlots);
    m_groupRoot->AddComponent(m_plus);
    m_groupRoot->AddComponent(m_additionSlots);
    m_groupRoot->AddComponent(m_arrow);
    m_groupRoot->AddComponent(m_xArrow);
    m_groupRoot->AddComponent(m_resultSlots);
}

void UIScene_Anvil::Tick()
{
    if (!IsVisible())
    {
        return;
    }

    Everstead *everstead      = Everstead::GetInstance();
    InputManager *input       = everstead->GetInputManager();
    AnvilContainer *container = everstead->GetLocalPlayer()->GetAnvilContainer();
    const ItemInstance &item  = *container->GetItem(AnvilContainer::s_inputSlot);
    if (input->IsUiTextInputFocused() && input->ConsumeUiTextCancelPress())
    {
        SetNameFocused(false);
    }

    bool inputChanged = item.GetItem() != m_lastInput.GetItem() || item.GetAuxValue() != m_lastInput.GetAuxValue() ||
                        item.GetCustomName() != m_lastInput.GetCustomName();
    if (inputChanged)
    {
        m_lastInput = item;
        m_nameInput->SetText(item.HasCustomName() ? StringUtils::ToUtf16(ChatMessageParser().ToPlainText(
                                                            nlohmann::json::parse(item.GetCustomName(), nullptr, false)))
                                                  : L"");
        if (item.IsEmpty())
        {
            SetNameFocused(false);
        }
    }

    if (inputChanged || m_nameInput->GetText() != m_nameText)
    {
        m_nameText = m_nameInput->GetText();
        container->SetItemName(StringUtils::ToUtf8(m_nameText));
        everstead->GetLocalPlayer()->GetClientConnection()->Send(std::shared_ptr<Packet>(new RenameItemC2SPacket(StringUtils::ToUtf8(m_nameText))));
    }

    int mouseX     = 0;
    int mouseY     = 0;
    bool leftDown  = (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    bool leftPress = leftDown && !m_leftDown;
    m_leftDown     = leftDown;
    if (leftPress)
    {
        Screen::Rect nameRect = m_nameInput->GetActualRect(everstead->GetWidth(), everstead->GetHeight());
        SetNameFocused(!item.IsEmpty() && (float) mouseX >= nameRect.x && (float) mouseX < nameRect.x + nameRect.width &&
                       (float) mouseY >= nameRect.y && (float) mouseY < nameRect.y + nameRect.height);
    }

    UIScene_AbstractContainer::Tick();
}

void UIScene_Anvil::Render()
{
    AnvilContainer *container = Everstead::GetInstance()->GetLocalPlayer()->GetAnvilContainer();
    bool hasInput             = !container->GetItem(AnvilContainer::s_inputSlot)->IsEmpty();
    bool blocked              = (hasInput || !container->GetItem(AnvilContainer::s_additionSlot)->IsEmpty()) &&
                                container->GetItem(AnvilContainer::s_resultSlot)->IsEmpty();
    m_nameAvailable->SetVisible(hasInput);
    m_nameUnavailable->SetVisible(!hasInput);
    m_arrow->SetVisible(!blocked);
    m_xArrow->SetVisible(blocked);

    UIScene_AbstractContainer::Render();
}

void UIScene_Anvil::OnExit()
{
    SetNameFocused(false);
    UIScene_AbstractContainer::OnExit();
}

void UIScene_Anvil::UpdateLayout()
{
    LayoutPanel(166.0f);

    Everstead *everstead          = Everstead::GetInstance();
    float hudScale                = Screen::SnapHudScale(everstead->options.GetHudScale(), everstead->GetWidth(), everstead->GetHeight());
    UIPanelProperties *properties = m_background->GetProperties();
    m_inputSlots->SetCenterOffset(-53.0f, -28.0f);
    m_additionSlots->SetCenterOffset(-4.0f, -28.0f);
    m_resultSlots->SetCenterOffset(54.0f, -28.0f);
    m_hammer->SetBounds(properties->x + 16.0f * hudScale, properties->y + 8.0f * hudScale, 30.0f * hudScale, 30.0f * hudScale);
    m_nameAvailable->SetBounds(properties->x + 59.0f * hudScale, properties->y + 20.0f * hudScale, 110.0f * hudScale, 16.0f * hudScale);
    m_nameUnavailable->SetBounds(properties->x + 59.0f * hudScale, properties->y + 20.0f * hudScale, 110.0f * hudScale, 16.0f * hudScale);
    m_nameInput->SetBounds(properties->x + 61.0f * hudScale, properties->y + 22.0f * hudScale, 106.0f * hudScale, 12.0f * hudScale);
    m_nameInput->SetTextScale(hudScale / 3.0f);
    m_nameInput->SetCaretScale(1.0f);
    m_plus->SetBounds(properties->x + 53.0f * hudScale, properties->y + 48.0f * hudScale, 13.0f * hudScale, 13.0f * hudScale);
    m_arrow->SetBounds(properties->x + 98.0f * hudScale, properties->y + 45.0f * hudScale, 24.0f * hudScale, 19.5f * hudScale);
    m_xArrow->SetBounds(properties->x + 99.0f * hudScale, properties->y + 47.0f * hudScale, 22.0f * hudScale, 15.0f * hudScale);
}

void UIScene_Anvil::RenderLabels()
{
    Everstead *everstead      = Everstead::GetInstance();
    LocalPlayer *player       = everstead->GetLocalPlayer();
    AnvilContainer *container = player->GetAnvilContainer();
    DrawLabel(everstead->GetString("container.repair"), 60.0f, 6.0f, false);
    int cost = container->GetCost();
    if (cost <= 0)
    {
        return;
    }

    std::wstring text;
    uint32_t color = 0xFFFF6060;
    if (cost >= AnvilContainer::s_tooExpensive && player->GetGameType() != eGameType_Creative)
    {
        text = everstead->GetString("container.repair.expensive");
    }
    else if (!container->GetItem(AnvilContainer::s_resultSlot)->IsEmpty())
    {
        text = everstead->GetString("container.repair.cost");
        text.replace(text.find(L"%s"), 2, std::to_wstring(cost));
        if (container->MayPickup())
        {
            color = 0xFF80FF20;
        }
    }
    else
    {
        return;
    }

    int actualWidth               = everstead->GetWidth();
    int actualHeight              = everstead->GetHeight();
    float hudScale                = Screen::SnapHudScale(everstead->options.GetHudScale(), actualWidth, actualHeight);
    UIPanelProperties *properties = m_background->GetProperties();
    Font *font                    = everstead->GetDefaultFont();
    float fontScale               = font->GetPixelPerfectScale(Screen::ToActualLength(hudScale, actualWidth, actualHeight));
    float pixel                   = Screen::ToActualLength(hudScale, actualWidth, actualHeight);
    float right                   = Screen::ToActualX(properties->x + ((float) s_panelWidth - 8.0f) * hudScale, actualWidth, actualHeight);
    float top                     = Screen::ToActualY(properties->y + 67.0f * hudScale, actualWidth, actualHeight);
    float textX                   = right - font->GetWidth(text, fontScale) - 2.0f * pixel;
    GlStateManager::DisableDepthTest();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    UIItemRenderer::DrawRect(textX - 2.0f * pixel, top, right - textX + 2.0f * pixel, 12.0f * pixel, 0x4F000000);
    GlStateManager::DisableBlend();
    font->SetNearest(true);
    font->DrawShadow(text, textX, top + 2.0f * pixel + font->GetAscent(fontScale), fontScale, color);
}

void UIScene_Anvil::SetNameFocused(bool focused)
{
    m_nameInput->SetSelected(focused);
    Everstead::GetInstance()->GetInputManager()->SetUiTextInputFocused(focused);
}

#endif
