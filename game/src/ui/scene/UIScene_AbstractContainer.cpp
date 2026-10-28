#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_AbstractContainer.h"

#include <memory>

#include <SDL2/SDL.h>

#include "../../core/Everstead.h"
#include "../../entity/player/LocalPlayer.h"
#include "../../input/InputManager.h"
#include "../../rendering/Font.h"
#include "../Screen.h"
#include "../UIPanelProperties.h"
#include "../control/UIController.h"

class UIPanelProperties_AbstractContainer : public UIPanelProperties
{
public:
    UIPanelProperties_AbstractContainer(float x, float y, float width, float height) : UIPanelProperties(x, y, width, height, 1.0f)
    {}

    ~UIPanelProperties_AbstractContainer() override
    {}

    const char *GetTexturePath() const override
    {
        return "assets/texture/ui/control/panel9grid.png";
    }
};

UIScene_AbstractContainer::UIScene_AbstractContainer(eUISCENE id, uint8_t menuId, AbstractContainer *container) : UIScene(id)
{
    m_groupRoot      = new UIGroup("Group_Container");
    m_background     = new UIComponent_Panel(std::make_unique<UIPanelProperties_AbstractContainer>(0.0f, 0.0f, 0.0f, 0.0f));
    m_inventorySlots = new UIControl_SlotList(Inventory::s_columns, Inventory::s_inventorySize / Inventory::s_columns);
    m_hotbarSlots    = new UIControl_SlotList(Inventory::s_columns, 1);

    Inventory *inventory = Everstead::GetInstance()->GetLocalPlayer()->GetInventory();
    m_inventorySlots->SetInventory(inventory, Inventory::s_hotbarSize);
    m_hotbarSlots->SetInventory(inventory, 0);

    InventoryMenu menu(inventory, container, menuId);
    m_menuId         = menuId;
    m_playerSlotBase = menu.GetPlayerSlotBase();
    m_storage        = menu.IsStorage();
    m_leftDown       = true;
    for (int row = 0; row < 2; row++)
    {
        for (int i = 0; i < 3; i++)
        {
            m_sortButtons[row][i] = new UIControl_Button(0.0f, 0.0f, 0.0f, 0.0f, L"");
        }
    }

    m_sortIcons[InventoryMenu::s_sortMode]        = new UIComponent_Image("assets/texture/ui/control/sort_icon.png");
    m_sortIcons[InventoryMenu::s_sortRowsMode]    = new UIComponent_Image("assets/texture/ui/control/sort_rows_icon.png");
    m_sortIcons[InventoryMenu::s_sortColumnsMode] = new UIComponent_Image("assets/texture/ui/control/sort_columns_icon.png");
    m_input                                       = new UIContainerInput(menuId, container);
    m_input->AddSlotList(m_hotbarSlots, m_playerSlotBase);
    m_input->AddSlotList(m_inventorySlots, m_playerSlotBase + Inventory::s_hotbarSize);

    m_groupRoot->AddComponent(m_background);
    m_groupRoot->AddComponent(m_inventorySlots);
    m_groupRoot->AddComponent(m_hotbarSlots);
    AddGroup(m_groupRoot);
}

UIScene_AbstractContainer::~UIScene_AbstractContainer()
{
    delete m_input;
    for (int i = 0; i < 3; i++)
    {
        delete m_sortButtons[0][i];
        delete m_sortButtons[1][i];
        delete m_sortIcons[i];
    }
}

void UIScene_AbstractContainer::Tick()
{
    if (!IsVisible())
    {
        return;
    }

    UIScene::Tick();
    UpdateLayout();
    m_input->Tick();

    bool leftDown = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    bool pressed  = (leftDown && !m_leftDown) || Everstead::GetInstance()->GetInputManager()->IsControllerButtonPressed(eControllerButton_South);
    m_leftDown    = leftDown;
    int firstSlot = 0;
    int mode      = 0;
    if (pressed && GetHoveredSortButton(&firstSlot, &mode))
    {
        Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
        m_input->Apply(firstSlot, mode, eClickType_Sort);
    }
}

void UIScene_AbstractContainer::Render()
{
    if (!IsVisible())
    {
        return;
    }

    UIScene::Render();
    DrawLabel(Everstead::GetInstance()->GetString("container.inventory"), 8.0f, m_panelHeight - 94.0f, false);
    RenderLabels();
    RenderSortButtons();
    m_input->RenderOverlay();
}

void UIScene_AbstractContainer::OnExit()
{
    m_input->Close();
    UIScene::OnExit();
}

void UIScene_AbstractContainer::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action == eGameplayAction_Back)
    {
        Close();
    }
}

void UIScene_AbstractContainer::Close()
{
    Everstead::GetInstance()->GetLocalPlayer()->CloseContainer();
}

void UIScene_AbstractContainer::RenderLabels()
{}

void UIScene_AbstractContainer::LayoutPanel(float height)
{
    Everstead *everstead          = Everstead::GetInstance();
    int actualWidth               = everstead->GetWidth();
    int actualHeight              = everstead->GetHeight();
    float hudScale                = Screen::SnapHudScale(everstead->options.GetHudScale(), actualWidth, actualHeight);
    UIPanelProperties *properties = m_background->GetProperties();
    properties->width             = (float) s_panelWidth * hudScale;
    properties->height            = height * hudScale;
    m_panelHeight                 = height;
    properties->x                 = Screen::SnapVirtualX(Screen::WIDTH * 0.5f - properties->width * 0.5f, actualWidth, actualHeight);
    properties->y                 = Screen::SnapVirtualY(Screen::HEIGHT * 0.5f - properties->height * 0.5f, actualWidth, actualHeight);
    properties->cornerScale       = hudScale / 3.0f;
    m_input->SetPanelBounds(properties->x, properties->y, properties->width, properties->height);

    m_inventorySlots->SetCenterOffset(0.0f, height * 0.5f - 55.0f);
    m_hotbarSlots->SetCenterOffset(0.0f, height * 0.5f - 15.0f);
}

void UIScene_AbstractContainer::DrawLabel(const std::wstring &text, float x, float y, bool centered)
{
    Everstead *everstead          = Everstead::GetInstance();
    UIPanelProperties *properties = m_background->GetProperties();
    int actualWidth               = everstead->GetWidth();
    int actualHeight              = everstead->GetHeight();
    float hudScale                = Screen::SnapHudScale(everstead->options.GetHudScale(), actualWidth, actualHeight);
    Font *font                    = everstead->GetDefaultFont();
    float fontScale               = font->GetPixelPerfectScale(Screen::ToActualLength(hudScale, actualWidth, actualHeight));
    float textX                   = Screen::ToActualX(properties->x + x * hudScale, actualWidth, actualHeight);
    float textY                   = Screen::ToActualY(properties->y + y * hudScale, actualWidth, actualHeight);
    if (centered)
    {
        textX -= font->GetWidth(text, fontScale) * 0.5f;
    }

    font->SetNearest(true);
    font->Draw(text, textX, textY + font->GetAscent(fontScale), fontScale, 0xFF404040);
}

int UIScene_AbstractContainer::GetSortRows(float *rowY, int *firstSlot) const
{
    if (!m_storage || m_menuId == InventoryMenu::s_recipeViewMenuId)
    {
        return 0;
    }

    rowY[0]      = 6.0f;
    firstSlot[0] = 0;
    return 1;
}

bool UIScene_AbstractContainer::GetHoveredSortButton(int *firstSlot, int *mode) const
{
    Everstead *everstead                = Everstead::GetInstance();
    int actualWidth                     = everstead->GetWidth();
    int actualHeight                    = everstead->GetHeight();
    float hudScale                      = Screen::SnapHudScale(everstead->options.GetHudScale(), actualWidth, actualHeight);
    const UIPanelProperties *properties = m_background->GetProperties();
    float pointerX                      = m_input->GetPointerX();
    float pointerY                      = m_input->GetPointerY();
    float rowY[2];
    int rowSlots[2];
    int rows = GetSortRows(rowY, rowSlots);
    for (int row = 0; row < rows; row++)
    {
        for (int i = 0; i < 3; i++)
        {
            float x = properties->x + (float) (s_panelWidth - 7 - s_sortButtonSize - (2 - i) * s_sortButtonSpacing) * hudScale;
            float y = properties->y + (rowY[row] - 1.0f) * hudScale;
            Screen::Rect rect =
                    Screen::ToActualRect(x, y, (float) s_sortButtonSize * hudScale, (float) s_sortButtonSize * hudScale, actualWidth, actualHeight);
            if (pointerX >= rect.x && pointerX < rect.x + rect.width && pointerY >= rect.y && pointerY < rect.y + rect.height)
            {
                *firstSlot = rowSlots[row];
                *mode      = i;
                return true;
            }
        }
    }

    return false;
}

void UIScene_AbstractContainer::RenderSortButtons()
{
    Everstead *everstead          = Everstead::GetInstance();
    float hudScale                = Screen::SnapHudScale(everstead->options.GetHudScale(), everstead->GetWidth(), everstead->GetHeight());
    UIPanelProperties *properties = m_background->GetProperties();
    int hoveredSlot               = 0;
    int hoveredMode               = 0;
    bool hovered                  = GetHoveredSortButton(&hoveredSlot, &hoveredMode);
    float rowY[2];
    int rowSlots[2];
    int rows = GetSortRows(rowY, rowSlots);
    for (int row = 0; row < rows; row++)
    {
        for (int i = 0; i < 3; i++)
        {
            float x                  = properties->x + (float) (s_panelWidth - 7 - s_sortButtonSize - (2 - i) * s_sortButtonSpacing) * hudScale;
            float y                  = properties->y + (rowY[row] - 1.0f) * hudScale;
            bool buttonHovered       = hovered && hoveredSlot == rowSlots[row] && hoveredMode == i;
            UIControl_Button *button = m_sortButtons[row][i];
            button->SetBounds(x, y, (float) s_sortButtonSize * hudScale, (float) s_sortButtonSize * hudScale);
            button->SetPreferredWidth((float) s_sortButtonSize * hudScale);
            button->SetTexelLength(hudScale);
            button->SetSelected(buttonHovered);
            button->Render();
            m_sortIcons[i]->SetBounds(x + 3.0f * hudScale, y + 3.0f * hudScale, (float) s_sortIconSize * hudScale, (float) s_sortIconSize * hudScale);
            m_sortIcons[i]->Render();
        }
    }

    if (hovered)
    {
        const char *keys[] = {"container.sort", "container.sort_rows", "container.sort_columns"};
        m_sortTooltip.SetText(everstead->GetString(keys[hoveredMode]));
        m_input->PlaceTooltip(&m_sortTooltip, (int) m_input->GetPointerX(), (int) m_input->GetPointerY());
        m_sortTooltip.Render();
    }
}

#endif
