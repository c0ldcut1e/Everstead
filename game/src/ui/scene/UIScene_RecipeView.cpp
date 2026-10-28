#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_RecipeView.h"

#include <SDL2/SDL.h>

#include "../../core/Everstead.h"
#include "../../input/InputManager.h"
#include "../../rendering/Font.h"
#include "../Screen.h"

UIScene_RecipeView::UIScene_RecipeView()
    : UIScene_AbstractContainer(eUIScene_RecipeView, InventoryMenu::s_recipeViewMenuId, Everstead::GetInstance()->GetLocalPlayer()->GetRecipeView())
{
    LocalPlayer *player          = Everstead::GetInstance()->GetLocalPlayer();
    CraftingContainer *container = player->GetRecipeView();
    m_type                       = player->GetRecipeViewType();
    bool furnace                 = m_type == eRecipeViewType_Furnace;
    bool secondInput             = furnace || m_type == eRecipeViewType_Resonator;
    m_gridSlots =
            m_type != eRecipeViewType_Crafting ? new UIControl_SlotList(1, 1) : new UIControl_SlotList(container->GetColumns(), container->GetRows());
    m_arrow = new UIComponent_Image("assets/texture/ui/scene/arrow.png");
    m_flame = new UIComponent_Image("assets/texture/ui/scene/furnace_flame_lit.png");
    m_arrow->SetVisible(m_type != eRecipeViewType_Break);
    m_flame->SetVisible(furnace);
    m_resultSlots = new UIControl_SlotList(1, 1);
    if (m_type != eRecipeViewType_Break)
    {
        m_resultSlots->SetSlotSize(26);
    }

    m_fuelSlots = new UIControl_SlotList(1, 1);
    m_fuelSlots->SetContainerSlots(container, CraftingContainer::s_gridSlot + 1);
    m_fuelSlots->SetVisible(secondInput);
    m_pageSwitcher = new UIComponent_PageSwitcher();
    m_pageSwitcher->SetPage(player->GetRecipeViewPage(), player->GetRecipeViewPageCount());
    m_leftDown     = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    m_actionLabelX = 0.0f;
    m_toGetLabelX  = 0.0f;

    m_gridSlots->SetContainerSlots(container, CraftingContainer::s_gridSlot);
    m_resultSlots->SetContainerSlots(container, CraftingContainer::s_resultSlot);
    m_input->AddSlotList(m_gridSlots, CraftingContainer::s_gridSlot);
    m_input->AddSlotList(m_resultSlots, CraftingContainer::s_resultSlot);
    if (secondInput)
    {
        m_input->AddSlotList(m_fuelSlots, CraftingContainer::s_gridSlot + 1);
    }

    UpdateLayout();
    m_groupRoot->AddComponent(m_gridSlots);
    m_groupRoot->AddComponent(m_arrow);
    m_groupRoot->AddComponent(m_flame);
    m_groupRoot->AddComponent(m_resultSlots);
    m_groupRoot->AddComponent(m_fuelSlots);
    m_groupRoot->AddComponent(m_pageSwitcher);
}

void UIScene_RecipeView::Tick()
{
    Everstead *everstead = Everstead::GetInstance();
    LocalPlayer *player  = everstead->GetLocalPlayer();
    player->FillRecipeView();
    UIScene_AbstractContainer::Tick();
    if (!IsVisible())
    {
        return;
    }

    InputManager *input = everstead->GetInputManager();
    int actualWidth     = everstead->GetWidth();
    int actualHeight    = everstead->GetHeight();
    int mouseX          = 0;
    int mouseY          = 0;
    bool leftDown       = (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    bool leftPress      = leftDown && !m_leftDown;
    m_leftDown          = leftDown;
    int direction       = 0;
    if (leftPress)
    {
        direction = m_pageSwitcher->GetDirectionAt(Screen::ToVirtualX((float) mouseX, actualWidth, actualHeight),
                                                   Screen::ToVirtualY((float) mouseY, actualWidth, actualHeight));
    }
    else if (input->IsControllerButtonPressed(eControllerButton_South))
    {
        direction = m_pageSwitcher->GetDirectionAt(Screen::ToVirtualX(m_input->GetPointerX(), actualWidth, actualHeight),
                                                   Screen::ToVirtualY(m_input->GetPointerY(), actualWidth, actualHeight));
    }
    else if (input->IsControllerButtonPressed(eControllerButton_LeftShoulder))
    {
        direction = -1;
    }
    else if (input->IsControllerButtonPressed(eControllerButton_RightShoulder))
    {
        direction = 1;
    }

    int page = player->GetRecipeViewPage() + direction;
    if (direction != 0 && page >= 0 && page < player->GetRecipeViewPageCount())
    {
        everstead->GetUiController()->PlayUiSound("ui.press");
        player->SetRecipeViewPage(page);
    }
}

void UIScene_RecipeView::UpdateLayout()
{
    LayoutPanel(166.0f);

    Everstead *everstead          = Everstead::GetInstance();
    float hudScale                = Screen::SnapHudScale(everstead->options.GetHudScale(), everstead->GetWidth(), everstead->GetHeight());
    UIPanelProperties *properties = m_background->GetProperties();
    float buttonSize              = 16.0f * hudScale;
    float switcherY               = properties->y - buttonSize - 2.0f * hudScale;
    m_pageSwitcher->SetBounds(properties->x, switcherY, properties->width, buttonSize, hudScale / 3.0f);
    m_input->SetExtraBounds(properties->x, switcherY, properties->width, properties->y - switcherY);
    if (m_type == eRecipeViewType_Break)
    {
        bool kill         = everstead->GetLocalPlayer()->IsKillRecipeView();
        Font *font        = everstead->GetDefaultFont();
        float unit        = Screen::ToActualLength(hudScale, everstead->GetWidth(), everstead->GetHeight());
        float fontScale   = font->GetPixelPerfectScale(unit);
        float actionWidth = font->GetWidth(everstead->GetString(kill ? "container.block_drop.kill" : "container.block_drop.break"), fontScale) / unit;
        float toGetWidth  = font->GetWidth(everstead->GetString("container.block_drop.to_get"), fontScale) / unit;
        float gap         = 6.0f;
        float slotSize    = 18.0f;
        m_actionLabelX    = ((float) s_panelWidth - (actionWidth + toGetWidth + 2.0f * slotSize + 3.0f * gap)) * 0.5f;
        float gridX       = m_actionLabelX + actionWidth + gap;
        m_toGetLabelX     = gridX + slotSize + gap;
        float resultX     = m_toGetLabelX + toGetWidth + gap;
        m_gridSlots->SetCenterOffset(gridX + slotSize * 0.5f - (float) s_panelWidth * 0.5f, -39.0f);
        m_resultSlots->SetCenterOffset(resultX + slotSize * 0.5f - (float) s_panelWidth * 0.5f, -39.0f);
        return;
    }

    if (m_type == eRecipeViewType_Resonator)
    {
        m_gridSlots->SetCenterOffset(-52.0f, -40.0f);
        m_fuelSlots->SetCenterOffset(-24.0f, -40.0f);
        m_arrow->SetBounds(properties->x + 84.0f * hudScale, properties->y + 32.0f * hudScale, 24.0f * hudScale, 19.5f * hudScale);
        m_resultSlots->SetCenterOffset(37.0f, -40.0f);
        return;
    }

    if (m_type == eRecipeViewType_Furnace)
    {
        m_gridSlots->SetCenterOffset(-24.0f, -58.0f);
        m_fuelSlots->SetCenterOffset(-24.0f, -22.0f);
        m_flame->SetBounds(properties->x + 56.5f * hudScale, properties->y + 35.5f * hudScale, 14.0f * hudScale, 14.0f * hudScale);
        m_arrow->SetBounds(properties->x + 84.0f * hudScale, properties->y + 32.0f * hudScale, 24.0f * hudScale, 19.5f * hudScale);
        m_resultSlots->SetCenterOffset(37.0f, -40.0f);
        return;
    }

    m_gridSlots->SetCenterOffset(-31.0f, -39.0f);
    m_arrow->SetBounds(properties->x + 90.0f * hudScale, properties->y + 34.0f * hudScale, 24.0f * hudScale, 19.5f * hudScale);
    m_resultSlots->SetCenterOffset(45.0f, -39.0f);
}

void UIScene_RecipeView::RenderLabels()
{
    Everstead *everstead = Everstead::GetInstance();
    if (m_type == eRecipeViewType_Break)
    {
        bool kill = everstead->GetLocalPlayer()->IsKillRecipeView();
        DrawLabel(everstead->GetString(kill ? "container.mob_drop" : "container.block_drop"), (float) s_panelWidth * 0.5f, 6.0f, true);
        DrawLabel(everstead->GetString(kill ? "container.block_drop.kill" : "container.block_drop.break"), m_actionLabelX, 40.0f, false);
        DrawLabel(everstead->GetString("container.block_drop.to_get"), m_toGetLabelX, 40.0f, false);
        return;
    }

    const char *title = "container.crafting";
    if (m_type == eRecipeViewType_Furnace)
    {
        title = "container.furnace";
    }
    else if (m_type == eRecipeViewType_Resonator)
    {
        title = "container.resonator";
    }

    DrawLabel(everstead->GetString(title), (float) s_panelWidth * 0.5f, 6.0f, true);
}

#endif
