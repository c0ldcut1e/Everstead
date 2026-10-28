#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIContainerInput.h"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <memory>
#include <string>
#include <vector>

#include "../core/Everstead.h"
#include "../entity/player/MultiPlayerLocalPlayer.h"
#include "../input/InputManager.h"
#include "../network/listener/ClientConnection.h"
#include "../network/packet/c2s/ContainerClickC2SPacket.h"
#include "../rendering/GlStateManager.h"
#include "../rendering/Textures.h"
#include "../tag/CompoundTag.h"
#include "../tag/StringTag.h"
#include "../utils/Time.h"
#include "../world/fracture/FractureTraits.h"
#include "../world/item/ArmorItem.h"
#include "../world/item/Item.h"
#include "ChatMessageParser.h"
#include "Screen.h"
#include "UIItemRenderer.h"

#include <SDL2/SDL.h>

UIContainerInput::UIContainerInput(uint8_t menuId, AbstractContainer *container)
{
    m_menuId              = menuId;
    m_container           = container;
    m_leftDown            = true;
    m_rightDown           = true;
    m_middleDown          = true;
    m_dropDown            = true;
    m_viewRecipeDown      = true;
    m_dragging            = false;
    m_dragButton          = InventoryMenu::s_leftButton;
    m_lastClickSlot       = -1;
    m_lastClickMs         = 0;
    m_closed              = false;
    m_hasPreview          = false;
    m_controllerPointer   = false;
    m_pointerX            = 0.0f;
    m_pointerY            = 0.0f;
    m_stickTicks          = 0.0f;
    m_hoveredSlot         = -1;
    m_navigationSlotCount = 0;
    m_panelX              = 0.0f;
    m_panelY              = 0.0f;
    m_panelWidth          = 0.0f;
    m_panelHeight         = 0.0f;
    m_extraX              = 0.0f;
    m_extraY              = 0.0f;
    m_extraWidth          = 0.0f;
    m_extraHeight         = 0.0f;
    m_creativeReplace     = false;
    m_dpadRepeatTimers.fill(0.0f);
    m_dpadWasDown.fill(false);
    m_wrapDirection = 0;
}

void UIContainerInput::AddSlotList(UIControl_SlotList *slotList, int menuSlotBase)
{
    slotList->SetMenuSlotBase(menuSlotBase);
    slotList->SetHighlightedMenuSlots(&m_placedSlots);
    slotList->SetPreviewItems(&m_previewItems);
    m_slotLists.push_back(slotList);
}

void UIContainerInput::Tick()
{
    if (m_closed)
    {
        return;
    }

    uint32_t buttons  = SDL_GetMouseState(nullptr, nullptr);
    bool leftDown     = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    bool rightDown    = (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    bool middleDown   = (buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
    bool leftPress    = leftDown && !m_leftDown;
    bool rightPress   = rightDown && !m_rightDown;
    bool middlePress  = middleDown && !m_middleDown;
    bool leftRelease  = !leftDown && m_leftDown;
    bool rightRelease = !rightDown && m_rightDown;
    m_leftDown        = leftDown;
    m_rightDown       = rightDown;
    m_middleDown      = middleDown;

    UpdatePointer();
    int hovered           = m_hoveredSlot < s_navigationSlotBase ? m_hoveredSlot : -1;
    ItemInstance &carried = Everstead::GetInstance()->GetLocalPlayer()->GetInventory()->GetCarried();
    InputManager *input   = Everstead::GetInstance()->GetInputManager();
    if (carried.IsEmpty())
    {
        m_creativeReplace = false;
    }

    if (m_controllerPointer && !m_dragging && hovered >= 0)
    {
        bool creativeMenu = m_menuId == InventoryMenu::s_creativeMenuId;
        bool southPress   = input->IsControllerButtonPressed(eControllerButton_South);
        bool northPress   = input->IsControllerButtonPressed(eControllerButton_North);
        if (hovered >= InventoryMenu::s_creativeSlotBase)
        {
            if (creativeMenu && (southPress || northPress))
            {
                TakeCreativeItem(hovered, northPress);
            }
        }
        else if (southPress && creativeMenu && m_creativeReplace && hovered < Inventory::s_hotbarSize)
        {
            // This replaces the hotbar item instead of swapping it onto the cursor
            Apply(hovered, InventoryMenu::s_leftButton, eClickType_QuickMove);
            Apply(hovered, InventoryMenu::s_leftButton, eClickType_Pickup);
            m_creativeReplace = false;
        }
        else if (southPress)
        {
            Apply(hovered, InventoryMenu::s_leftButton, eClickType_Pickup);
        }
        else if (northPress && !creativeMenu)
        {
            Apply(hovered, InventoryMenu::s_leftButton, eClickType_QuickMove);
        }
        else if (!creativeMenu && input->IsControllerButtonPressed(eControllerButton_West))
        {
            Apply(hovered, InventoryMenu::s_rightButton, eClickType_Pickup);
        }
    }

    int dropKey             = input->GetBinding(eKeyBinding_Drop);
    const uint8_t *keyboard = SDL_GetKeyboardState(nullptr);
    bool dropDown           = keyboard && dropKey >= 0 && dropKey < SDL_NUM_SCANCODES && keyboard[dropKey];
    bool dropPress          = dropDown && !m_dropDown;
    m_dropDown              = dropDown;
    bool lockedSlot         = hovered >= InventoryMenu::s_creativeSlotBase && m_menuId != InventoryMenu::s_creativeMenuId;
    if (dropPress && !m_dragging && hovered >= 0 && !lockedSlot && carried.IsEmpty())
    {
        bool ctrl = (SDL_GetModState() & KMOD_CTRL) != 0;
        Apply(hovered, ctrl ? InventoryMenu::s_rightButton : InventoryMenu::s_leftButton, eClickType_Throw);
    }

    int viewRecipeKey    = input->GetBinding(eKeyBinding_ViewRecipe);
    bool viewRecipeDown  = keyboard && viewRecipeKey >= 0 && viewRecipeKey < SDL_NUM_SCANCODES && keyboard[viewRecipeKey];
    bool viewRecipePress = viewRecipeDown && !m_viewRecipeDown;
    m_viewRecipeDown     = viewRecipeDown;
    if (viewRecipePress && !m_dragging && hovered >= 0 && hovered != InventoryMenu::s_deleteSlot && carried.IsEmpty())
    {
        MultiPlayerLocalPlayer *player = Everstead::GetInstance()->GetLocalPlayer();
        const ItemInstance *item       = InventoryMenu(player->GetInventory(), m_container, m_menuId).GetDisplayItem(hovered);
        if (item && !item->IsEmpty())
        {
            player->ViewRecipe(item->GetItem());
            return;
        }
    }

    bool throwAll = leftPress || (m_controllerPointer && input->IsControllerButtonPressed(eControllerButton_South));
    bool throwOne = rightPress || (m_controllerPointer && input->IsControllerButtonPressed(eControllerButton_West));
    if (!m_dragging && hovered < 0 && !carried.IsEmpty() && (throwAll || throwOne) && IsOutsidePanel())
    {
        Apply(InventoryMenu::s_outsideSlot, throwAll ? InventoryMenu::s_leftButton : InventoryMenu::s_rightButton, eClickType_Throw);
        return;
    }

    if (lockedSlot && !m_dragging)
    {
        return;
    }

    if (hovered == InventoryMenu::s_deleteSlot && !m_dragging)
    {
        if (leftPress)
        {
            Apply(hovered, InventoryMenu::s_leftButton, (SDL_GetModState() & KMOD_SHIFT) != 0 ? eClickType_QuickMove : eClickType_Pickup);
        }

        return;
    }

    if (!m_dragging && hovered >= InventoryMenu::s_creativeSlotBase)
    {
        if (leftPress || rightPress)
        {
            bool shift = (SDL_GetModState() & KMOD_SHIFT) != 0;
            Apply(hovered, leftPress ? InventoryMenu::s_leftButton : InventoryMenu::s_rightButton, shift ? eClickType_QuickMove : eClickType_Pickup);
        }
        else if (middlePress)
        {
            Apply(hovered, InventoryMenu::s_middleButton, eClickType_Clone);
        }

        int hotbarKey = input->ConsumeHotbarSlot();
        if (hotbarKey >= 0)
        {
            Apply(hovered, hotbarKey, eClickType_Swap);
        }

        return;
    }

    if (m_dragging)
    {
        if ((m_dragButton == InventoryMenu::s_leftButton && rightPress) || (m_dragButton == InventoryMenu::s_rightButton && leftPress))
        {
            CancelDrag();
            return;
        }

        if (hovered >= 0 && hovered != InventoryMenu::s_deleteSlot && hovered < InventoryMenu::s_creativeSlotBase &&
            std::find(m_dragSlots.begin(), m_dragSlots.end(), hovered) == m_dragSlots.end())
        {
            m_dragSlots.push_back(hovered);
            if (m_dragButton == InventoryMenu::s_rightButton && CanPlaceCarried(hovered))
            {
                Apply(hovered, InventoryMenu::s_rightButton, eClickType_Pickup);
                m_placedSlots.push_back(hovered);
            }
        }

        if (m_dragButton == InventoryMenu::s_rightButton && rightRelease)
        {
            CancelDrag();
        }
        else if (m_dragButton == InventoryMenu::s_leftButton && leftRelease)
        {
            ApplyDrag();
        }

        UpdateDragPreview();
        return;
    }

    if (middlePress && hovered >= 0 && GameType::HasInfiniteItems(Everstead::GetInstance()->GetLocalPlayer()->GetGameType()))
    {
        Apply(hovered, InventoryMenu::s_middleButton, eClickType_Clone);
    }

    int button = leftPress ? InventoryMenu::s_leftButton : (rightPress ? InventoryMenu::s_rightButton : -1);
    if (button >= 0 && hovered >= 0)
    {
        uint32_t nowMs   = Time::GetMilliseconds();
        bool doubleClick = button == InventoryMenu::s_leftButton && hovered == m_lastClickSlot && nowMs - m_lastClickMs <= s_doubleClickMs;
        m_lastClickSlot  = button == InventoryMenu::s_leftButton ? hovered : -1;
        m_lastClickMs    = nowMs;

        if ((SDL_GetModState() & KMOD_SHIFT) != 0)
        {
            Apply(hovered, button, eClickType_QuickMove);
        }
        else if (doubleClick && !carried.IsEmpty())
        {
            m_lastClickSlot = -1;
            Apply(hovered, button, eClickType_PickupAll);
        }
        else if (!carried.IsEmpty() && button == InventoryMenu::s_rightButton)
        {
            bool canPlace = CanPlaceCarried(hovered);
            Apply(hovered, button, eClickType_Pickup);
            if (canPlace)
            {
                m_dragging   = true;
                m_dragButton = button;
                m_dragSlots.assign(1, hovered);
                m_placedSlots.assign(1, hovered);
            }
        }
        else if (!carried.IsEmpty())
        {
            m_dragging   = true;
            m_dragButton = button;
            m_dragSlots.assign(1, hovered);
            UpdateDragPreview();
        }
        else
        {
            Apply(hovered, button, eClickType_Pickup);
        }
    }

    int hotbarKey = input->ConsumeHotbarSlot();
    if (hotbarKey >= 0 && hovered >= 0)
    {
        Apply(hovered, hotbarKey, eClickType_Swap);
    }
}

void UIContainerInput::RenderOverlay()
{
    Everstead *everstead  = Everstead::GetInstance();
    Inventory *inventory  = everstead->GetLocalPlayer()->GetInventory();
    ItemInstance &carried = inventory->GetCarried();
    if (carried.IsEmpty())
    {
        bool menuSlot            = m_hoveredSlot >= 0 && m_hoveredSlot < s_navigationSlotBase;
        const ItemInstance *item = menuSlot ? InventoryMenu(inventory, m_container, m_menuId).GetDisplayItem(m_hoveredSlot) : nullptr;
        std::vector<std::vector<ChatTextSegment>> lines;
        if (item && !item->IsEmpty())
        {
            Item *type = item->GetItem();
            std::vector<ChatTextSegment> nameLine;
            ChatTextStyle customNameStyle;
            customNameStyle.italic = true;
            if (!item->HasCustomName() || !ChatMessageParser().Parse(item->GetCustomName(), customNameStyle, &nameLine))
            {
                nameLine = {{everstead->GetString(type->GetDescriptionId()), ChatTextStyle()}};
            }

            if (everstead->options.advancedTooltips)
            {
                wchar_t idText[16];
                swprintf(idText, std::size(idText), L" (#%04u)", type->GetId());
                nameLine.push_back({idText, ChatTextStyle()});
            }

            lines.push_back(nameLine);
            CompoundTag *tag = item->GetTag();
            if (tag && tag->Contains("display", Tag::eTagType_Compound) && tag->GetCompound("display")->Contains("Lore", Tag::eTagType_List))
            {
                ListTag *lore = tag->GetCompound("display")->GetList("Lore");
                ChatTextStyle loreStyle;
                loreStyle.color  = 0xFFAA00AA;
                loreStyle.italic = true;
                ChatMessageParser parser;
                for (size_t i = 0; i < lore->Size(); i++)
                {
                    std::vector<ChatTextSegment> segments;
                    if (lore->Get(i)->GetType() == Tag::eTagType_String &&
                        parser.Parse(((StringTag *) lore->Get(i))->GetValue(), loreStyle, &segments))
                    {
                        lines.push_back(segments);
                    }
                }
            }

            ChatTextStyle fractureStyle;
            fractureStyle.color = 0xFF7FE8E8;
            ChatTextStyle fractureDescriptionStyle;
            fractureDescriptionStyle.color         = 0xFF808080;
            std::vector<FractureTrait *> fractures = FractureTraits::Get(*item);
            for (size_t i = 0; i < fractures.size(); i++)
            {
                lines.push_back({{everstead->GetString(fractures[i]->GetId().ToLanguageKey("fracture")), fractureStyle}});
                if (everstead->options.advancedTooltips)
                {
                    lines.push_back(
                            {{L"  " + everstead->GetString(fractures[i]->GetId().ToLanguageKey("fracture") + ".desc"), fractureDescriptionStyle}});
                }
            }

            ChatTextStyle attributeStyle;
            attributeStyle.color = 0xFF5555FF;
            ChatTextStyle durabilityStyle;
            durabilityStyle.color = 0xFFAAAAAA;
            if (type->GetAttackDamage() > 0)
            {
                lines.push_back({});
                lines.push_back(
                        {{L"+" + std::to_wstring(type->GetAttackDamage()) + L" " + everstead->GetString("attribute.name.generic.attackDamage"),
                          attributeStyle}});
            }

            if (type->GetArmorSlot() >= 0)
            {
                lines.push_back({});
                lines.push_back(
                        {{L"+" + std::to_wstring(((ArmorItem *) type)->GetDefense()) + L" " + everstead->GetString("attribute.name.generic.armor"),
                          attributeStyle}});
            }

            if (everstead->options.advancedTooltips && type->IsDamageable() && item->GetAuxValue() > 0)
            {
                lines.push_back({{everstead->GetString("item.durability") + L": " + std::to_wstring(type->GetMaxDamage() - item->GetAuxValue()) +
                                          L" / " + std::to_wstring(type->GetMaxDamage()),
                                  durabilityStyle}});
            }
        }

        m_tooltip.SetLines(lines);
        PlaceTooltip(&m_tooltip, (int) m_pointerX, (int) m_pointerY);
        m_tooltip.Render();
    }

    const ItemInstance &shown = m_hasPreview ? m_previewCarried : carried;
    if (!shown.IsEmpty())
    {
        float size = (float) s_itemSize * Screen::GetGuiPixelSize(everstead->options.GetHudScale(), everstead->GetWidth(), everstead->GetHeight());
        float x    = roundf(m_pointerX - size * 0.5f);
        float y    = roundf(m_pointerY - size * 0.5f);

        GlStateManager::DisableDepthTest();
        GlStateManager::DisableCull();
        GlStateManager::EnableBlend();
        UIItemRenderer::DrawItem(&shown, x, y, size, size);
        Textures::Unbind();
        UIItemRenderer::DrawItemCount(&shown, x, y, size);
        GlStateManager::DisableBlend();
        GlStateManager::EnableCull();
        GlStateManager::EnableDepthTest();
    }

    m_cursor.SetVisible(m_controllerPointer);
    m_cursor.SetPosition(m_pointerX, m_pointerY);
    m_cursor.Render();
}

void UIContainerInput::PlaceTooltip(UIComponent_Tooltip *tooltip, int pointerX, int pointerY) const
{
    Everstead *everstead = Everstead::GetInstance();
    float guiPixel       = Screen::GetGuiPixelSize(everstead->options.GetHudScale(), everstead->GetWidth(), everstead->GetHeight());
    float width          = tooltip->GetWidth();
    float height         = tooltip->GetHeight();
    if (!everstead->options.javaTooltips)
    {
        float texel = std::max(1.0f, roundf(guiPixel * kLegacyTooltipTexelScale));
        tooltip->SetPosition((int) ((float) pointerX + (float) s_legacyTooltipMouseOffset * texel), (int) ((float) pointerY - height));
        return;
    }

    float x = (float) pointerX + (float) (s_javaTooltipMouseOffset - s_javaTooltipPadding) * guiPixel;
    float y = (float) pointerY - (float) (s_javaTooltipMouseOffset + s_javaTooltipPadding) * guiPixel;
    if (x + width - (float) s_javaTooltipPadding * guiPixel > (float) everstead->GetWidth())
    {
        x -= (float) (s_javaTooltipScreenFlip - s_javaTooltipPadding * 2) * guiPixel + width;
    }

    if (y + height + (float) (s_javaTooltipScreenMargin - s_javaTooltipPadding) * guiPixel > (float) everstead->GetHeight())
    {
        y = (float) everstead->GetHeight() - height - (float) (s_javaTooltipScreenMargin - s_javaTooltipPadding) * guiPixel;
    }

    tooltip->SetPosition((int) x, (int) y);
}

float UIContainerInput::GetPointerX() const
{
    return m_pointerX;
}

float UIContainerInput::GetPointerY() const
{
    return m_pointerY;
}

void UIContainerInput::Close()
{
    if (m_closed)
    {
        return;
    }

    m_closed = true;
    CancelDrag();
    Apply(0, 0, eClickType_Close);
}

void UIContainerInput::AddNavigationSlotList(UIControl_SlotList *slotList)
{
    slotList->SetMenuSlotBase(s_navigationSlotBase + m_navigationSlotCount);
    m_navigationSlotCount += slotList->GetColumns() * slotList->GetRows();
    m_slotLists.push_back(slotList);
}

void UIContainerInput::SetMenuId(uint8_t menuId)
{
    m_menuId = menuId;
}

void UIContainerInput::SetPanelBounds(float x, float y, float width, float height)
{
    m_panelX      = x;
    m_panelY      = y;
    m_panelWidth  = width;
    m_panelHeight = height;
}

void UIContainerInput::SetExtraBounds(float x, float y, float width, float height)
{
    m_extraX      = x;
    m_extraY      = y;
    m_extraWidth  = width;
    m_extraHeight = height;
}

void UIContainerInput::UpdatePointer()
{
    m_cursor.SetSize(m_slotLists[0]->GetSlotRect(0).width * kReferenceCursorSize / kReferenceSlotSize);
    bool controller = Everstead::GetInstance()->GetInputManager()->IsUsingController();
    if (controller)
    {
        if (!m_controllerPointer)
        {
            int slot = FindMenuSlotAt(m_pointerX, m_pointerY);
            SnapPointerToSlot(slot >= 0 ? slot : m_slotLists[0]->GetMenuSlotBase());
            m_stickTicks = 0.0f;
        }

        m_controllerPointer = true;
        MoveControllerPointer();
    }
    else
    {
        int mouseX = 0;
        int mouseY = 0;
        SDL_GetMouseState(&mouseX, &mouseY);
        m_controllerPointer = false;
        m_pointerX          = (float) mouseX;
        m_pointerY          = (float) mouseY;
    }

    m_hoveredSlot = FindMenuSlotAt(m_pointerX, m_pointerY);
    for (size_t i = 0; i < m_slotLists.size(); i++)
    {
        m_slotLists[i]->SetHoveredMenuSlot(m_hoveredSlot);
    }
}

void UIContainerInput::MoveControllerPointer()
{
    Everstead *everstead = Everstead::GetInstance();
    InputManager *input  = everstead->GetInputManager();
    bool up              = UpdateDpadRepeat(0, input->IsControllerButtonDown(eControllerButton_DpadUp));
    bool down            = UpdateDpadRepeat(1, input->IsControllerButtonDown(eControllerButton_DpadDown));
    bool left            = UpdateDpadRepeat(2, input->IsControllerButtonDown(eControllerButton_DpadLeft));
    bool right           = UpdateDpadRepeat(3, input->IsControllerButtonDown(eControllerButton_DpadRight));
    int directionX       = (right ? 1 : 0) - (left ? 1 : 0);
    int directionY       = (down ? 1 : 0) - (up ? 1 : 0);
    if (directionX != 0 || directionY != 0)
    {
        JumpToSlot(directionX, directionY);
        m_stickTicks = 0.0f;
        return;
    }

    float stickX = 0.0f;
    float stickY = 0.0f;
    input->GetControllerStick(&stickX, &stickY);
    if (fabsf(stickX) < kStickDeadzone && fabsf(stickY) < kStickDeadzone)
    {
        m_stickTicks = 0.0f;
        int slot     = FindMenuSlotAt(m_pointerX, m_pointerY);
        if (slot >= 0)
        {
            SnapPointerToSlot(slot);
        }

        return;
    }

    float slotSize = m_slotLists[0]->GetSlotRect(0).width;
    float updates  = (float) Time::GetDelta() * kPointerUpdatesPerSecond;
    m_stickTicks   = std::min(m_stickTicks + updates, kPointerRampTicks);
    float speed    = slotSize * kPointerSpeed / kReferenceSlotSize * updates * m_stickTicks / kPointerRampTicks *
                     everstead->options.menuCursorSensitivity / 100.0f;
    m_pointerX += stickX * fabsf(stickX) * speed;
    m_pointerY -= stickY * fabsf(stickY) * speed;

    float cursorSize = m_cursor.GetSize();
    float minX       = Screen::ToActualX(m_panelX, everstead->GetWidth(), everstead->GetHeight()) - cursorSize;
    float minY       = Screen::ToActualY(m_panelY, everstead->GetWidth(), everstead->GetHeight()) - cursorSize;
    float maxX       = Screen::ToActualX(m_panelX + m_panelWidth, everstead->GetWidth(), everstead->GetHeight()) + cursorSize;
    float maxY       = Screen::ToActualY(m_panelY + m_panelHeight, everstead->GetWidth(), everstead->GetHeight()) + cursorSize * 0.5f;
    m_pointerX       = std::clamp(m_pointerX, minX, maxX);
    m_pointerY       = std::clamp(m_pointerY, minY, maxY);
}

void UIContainerInput::JumpToSlot(int directionX, int directionY)
{
    int bestSlot    = -1;
    float bestScore = 0.0f;
    for (size_t list = 0; list < m_slotLists.size(); list++)
    {
        if (!m_slotLists[list]->IsVisible())
        {
            continue;
        }

        for (size_t i = 0; i < m_slotLists[list]->GetSlots().size(); i++)
        {
            Screen::Rect rect = m_slotLists[list]->GetSlotRect((int) i);
            float offsetX     = rect.x + rect.width * 0.5f - m_pointerX;
            float offsetY     = rect.y + rect.height * 0.5f - m_pointerY;
            float along       = directionX != 0 ? offsetX * (float) directionX : offsetY * (float) directionY;
            float across      = directionX != 0 ? fabsf(offsetY) : fabsf(offsetX);
            if (along < rect.width * 0.5f)
            {
                continue;
            }

            float score = along + across * 2.0f;
            if (bestSlot < 0 || score < bestScore)
            {
                bestSlot  = m_slotLists[list]->GetMenuSlot((int) i);
                bestScore = score;
            }
        }
    }

    if (bestSlot < 0)
    {
        // We wrap to the farthest slot on the opposite side and keep the closest row or column
        for (size_t list = 0; list < m_slotLists.size(); list++)
        {
            if (!m_slotLists[list]->IsVisible())
            {
                continue;
            }

            for (size_t i = 0; i < m_slotLists[list]->GetSlots().size(); i++)
            {
                Screen::Rect rect = m_slotLists[list]->GetSlotRect((int) i);
                float offsetX     = rect.x + rect.width * 0.5f - m_pointerX;
                float offsetY     = rect.y + rect.height * 0.5f - m_pointerY;
                float along       = directionX != 0 ? offsetX * (float) directionX : offsetY * (float) directionY;
                float across      = directionX != 0 ? fabsf(offsetY) : fabsf(offsetX);
                float score       = along + across * 2.0f;
                if (bestSlot < 0 || score < bestScore)
                {
                    bestSlot  = m_slotLists[list]->GetMenuSlot((int) i);
                    bestScore = score;
                }
            }
        }

        if (bestSlot >= 0)
        {
            m_wrapDirection = directionX != 0 ? directionX * 2 : directionY;
        }
    }

    if (bestSlot >= 0)
    {
        SnapPointerToSlot(bestSlot);
    }
}

bool UIContainerInput::UpdateDpadRepeat(int index, bool down)
{
    bool wasDown         = m_dpadWasDown[index];
    m_dpadWasDown[index] = down;
    if (!down)
    {
        m_dpadRepeatTimers[index] = 0.0f;
        return false;
    }

    if (!wasDown)
    {
        m_dpadRepeatTimers[index] = kDpadRepeatDelay;
        return true;
    }

    m_dpadRepeatTimers[index] -= (float) Time::GetDelta();
    if (m_dpadRepeatTimers[index] > 0.0f)
    {
        return false;
    }

    m_dpadRepeatTimers[index] = std::max(m_dpadRepeatTimers[index] + kDpadRepeatRate, 0.0f);
    return true;
}

int UIContainerInput::ConsumeWrapDirection()
{
    int direction   = m_wrapDirection;
    m_wrapDirection = 0;
    return direction;
}

void UIContainerInput::SnapPointerToSlot(int menuSlot)
{
    Screen::Rect rect;
    if (GetMenuSlotRect(menuSlot, &rect))
    {
        m_pointerX = rect.x + rect.width * 0.5f;
        m_pointerY = rect.y + rect.height * 0.5f;
    }
}

int UIContainerInput::FindMenuSlotAt(float actualX, float actualY) const
{
    for (size_t i = 0; i < m_slotLists.size(); i++)
    {
        int slot = m_slotLists[i]->GetMenuSlotAt(actualX, actualY);
        if (slot >= 0 && m_slotLists[i]->IsVisible())
        {
            return slot;
        }
    }

    return -1;
}

bool UIContainerInput::GetMenuSlotRect(int menuSlot, Screen::Rect *rect) const
{
    for (size_t i = 0; i < m_slotLists.size(); i++)
    {
        int index = m_slotLists[i]->GetSlotIndex(menuSlot);
        if (index >= 0 && index < (int) m_slotLists[i]->GetSlots().size())
        {
            *rect = m_slotLists[i]->GetSlotRect(index);
            return true;
        }
    }

    return false;
}

void UIContainerInput::Apply(int slot, int button, eCLICK_TYPE type)
{
    if (m_menuId == InventoryMenu::s_recipeViewMenuId)
    {
        return;
    }

    MultiPlayerLocalPlayer *player = Everstead::GetInstance()->GetLocalPlayer();
    InventoryMenu(player->GetInventory(), m_container, m_menuId).Click(slot, button, type);
    player->GetClientConnection()->Send(std::shared_ptr<Packet>(new ContainerClickC2SPacket(m_menuId, (int16_t) slot, (uint8_t) button, type)));
}

void UIContainerInput::TakeCreativeItem(int slot, bool stack)
{
    Inventory *inventory = Everstead::GetInstance()->GetLocalPlayer()->GetInventory();
    if (!inventory->GetCarried().IsEmpty())
    {
        Apply(slot, InventoryMenu::s_leftButton, stack ? eClickType_QuickMove : eClickType_Pickup);
        return;
    }

    const ItemInstance *source = InventoryMenu(inventory, m_container, m_menuId).GetDisplayItem(slot);
    for (int i = 0; source && i < Inventory::s_hotbarSize; i++)
    {
        ItemInstance *target = inventory->GetItem(i);
        if (target->IsEmpty() || !target->IsSameItem(*source) || target->GetCount() >= target->GetMaxStackSize())
        {
            continue;
        }

        if (stack)
        {
            Apply(slot, i, eClickType_Swap);
        }
        else
        {
            Apply(slot, InventoryMenu::s_leftButton, eClickType_Pickup);
            Apply(i, InventoryMenu::s_leftButton, eClickType_Pickup);
        }

        return;
    }

    for (int i = 0; i < Inventory::s_hotbarSize; i++)
    {
        if (inventory->GetItem(i)->IsEmpty())
        {
            Apply(slot, i, stack ? eClickType_Swap : eClickType_PickBlock);
            return;
        }
    }

    // The hotbar is full, so we carry the item and move the pointer to let a hotbar slot be replaced
    Apply(slot, InventoryMenu::s_leftButton, stack ? eClickType_QuickMove : eClickType_Pickup);
    m_creativeReplace = true;
    SnapPointerToSlot(inventory->GetSelectedSlot());
}

void UIContainerInput::ClearHotbar()
{
    if (m_menuId != InventoryMenu::s_creativeMenuId)
    {
        return;
    }

    for (int i = 0; i < Inventory::s_hotbarSize; i++)
    {
        Apply(i, InventoryMenu::s_leftButton, eClickType_QuickMove);
    }
}

bool UIContainerInput::IsOutsidePanel() const
{
    bool insidePanel = IsInsideVirtualRect(m_panelX, m_panelY, m_panelWidth, m_panelHeight);
    bool insideExtra = m_extraWidth > 0.0f && IsInsideVirtualRect(m_extraX, m_extraY, m_extraWidth, m_extraHeight);
    return !insidePanel && !insideExtra;
}

bool UIContainerInput::IsInsideVirtualRect(float x, float y, float width, float height) const
{
    Everstead *everstead = Everstead::GetInstance();
    int actualWidth      = everstead->GetWidth();
    int actualHeight     = everstead->GetHeight();
    float minX           = Screen::ToActualX(x, actualWidth, actualHeight);
    float minY           = Screen::ToActualY(y, actualWidth, actualHeight);
    float maxX           = Screen::ToActualX(x + width, actualWidth, actualHeight);
    float maxY           = Screen::ToActualY(y + height, actualWidth, actualHeight);
    return m_pointerX >= minX && m_pointerX < maxX && m_pointerY >= minY && m_pointerY < maxY;
}

void UIContainerInput::ApplyDrag()
{
    m_dragging = false;
    m_placedSlots.clear();
    UpdateDragPreview();
    if (m_dragSlots.size() == 1)
    {
        int slot = m_dragSlots[0];
        m_dragSlots.clear();
        Apply(slot, m_dragButton, eClickType_Pickup);
        return;
    }

    MultiPlayerLocalPlayer *player = Everstead::GetInstance()->GetLocalPlayer();
    InventoryMenu(player->GetInventory(), m_container, m_menuId).Drag(m_dragSlots, m_dragButton);
    player->GetClientConnection()->Send(std::shared_ptr<Packet>(new ContainerClickC2SPacket(m_menuId, m_dragSlots, (uint8_t) m_dragButton)));
    m_dragSlots.clear();
}

bool UIContainerInput::CanPlaceCarried(int slot)
{
    Inventory *inventory        = Everstead::GetInstance()->GetLocalPlayer()->GetInventory();
    const ItemInstance &carried = inventory->GetCarried();
    ItemInstance *target        = InventoryMenu(inventory, m_container, m_menuId).GetSlot(slot);
    if (carried.IsEmpty())
    {
        return false;
    }

    return target->IsEmpty() || (target->IsSameItem(carried) && target->GetCount() < target->GetMaxStackSize());
}

void UIContainerInput::CancelDrag()
{
    m_dragging = false;
    m_dragSlots.clear();
    m_placedSlots.clear();
    UpdateDragPreview();
}

void UIContainerInput::UpdateDragPreview()
{
    m_previewItems.clear();
    m_hasPreview = false;
    if (!m_dragging || m_dragButton != InventoryMenu::s_leftButton)
    {
        return;
    }

    Inventory *inventory = Everstead::GetInstance()->GetLocalPlayer()->GetInventory();
    m_hasPreview = InventoryMenu(inventory, m_container, m_menuId).ComputeDrag(m_dragSlots, m_dragButton, &m_previewItems, &m_previewCarried) > 0;
    m_placedSlots.clear();
    for (size_t i = 0; i < m_previewItems.size(); i++)
    {
        m_placedSlots.push_back(m_previewItems[i].first);
    }
}

#endif
