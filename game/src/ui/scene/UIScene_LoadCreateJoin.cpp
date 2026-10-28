#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_LoadCreateJoin.h"
#include "../../utils/Time.h"

#include <cmath>

#include <SDL2/SDL.h>

#include "../../core/Everstead.h"
#include "../../core/GameInfo.h"
#include "../../input/InputManager.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/TextureRepository.h"
#include "../../rendering/Textures.h"
#include "../../utils/StringUtils.h"
#include "../Screen.h"

const float UIScene_LoadCreateJoin::s_createLoadJoinMenuWidth        = 760.0f;
const float UIScene_LoadCreateJoin::s_createLoadJoinMenuHeight       = 646.0f;
const float UIScene_LoadCreateJoin::s_createLoadJoinTabOffsetY       = -70.0f;
const float UIScene_LoadCreateJoin::s_createLoadJoinTabTextScale     = 0.9f;
const float UIScene_LoadCreateJoin::s_createLoadJoinTabTextRaise     = 6.0f;
const float UIScene_LoadCreateJoin::s_createLoadJoinTabTextY         = -40.0f;
const float UIScene_LoadCreateJoin::s_createLoadJoinTabTextHeight    = 30.0f;
const float UIScene_LoadCreateJoin::s_createLoadJoinListX            = 30.0f;
const float UIScene_LoadCreateJoin::s_createLoadJoinListY            = 40.0f;
const float UIScene_LoadCreateJoin::s_createLoadJoinListButtonWidth  = 660.0f;
const float UIScene_LoadCreateJoin::s_createLoadJoinListButtonHeight = 80.0f;
static const char *s_createLoadJoinIconTexture                       = "assets/texture/ui/scene/worldmenu_icon.png";

UIScene_LoadCreateJoin::UIScene_LoadCreateJoin() : UIScene(eUIScene_LoadCreateJoin)
{
    m_groupRoot                  = new UIGroup("Group_LoadCreateJoin");
    m_selectedTab                = 0;
    m_worldPanelIndex            = 0;
    m_lastJoinSessionRefreshMs   = 0;
    m_lastSessionsVersion        = 0;
    m_previousMousePrimaryDown   = true;
    m_previousMouseSecondaryDown = true;
    m_wasVisible                 = false;
    m_backgroundRegion           = UITextureAtlas::GetRegion("assets/texture/ui/scene/worldmenu.png");
    m_tabRegions[0]              = UITextureAtlas::GetRegion("assets/texture/ui/scene/worldmenu_tab1.png");
    m_tabRegions[1]              = UITextureAtlas::GetRegion("assets/texture/ui/scene/worldmenu_tab2.png");
    m_tabRegions[2]              = UITextureAtlas::GetRegion("assets/texture/ui/scene/worldmenu_tab3.png");

    Everstead *everstead     = Everstead::GetInstance();
    std::wstring tabNames[3] = {everstead->GetString("world.tab.load"), everstead->GetString("world.tab.create"),
                                everstead->GetString("world.tab.join")};
    for (int i = 0; i < 3; i++)
    {
        m_tabLabels[i] = new UIControl_Label(0.0f, 0.0f, 100.0f, s_createLoadJoinTabTextHeight, tabNames[i], s_createLoadJoinTabTextColor, false);
        m_tabLabels[i]->SetTextScale(s_createLoadJoinTabTextScale);
        m_tabLabels[i]->SetHorizontalAlign(UIControl_Label::eHorizontalAlign_Center);
        m_groupRoot->AddComponent(m_tabLabels[i]);
    }

    m_list = new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f);
    m_list->SetPanelVisible(false);
    m_list->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_list->SetButtonRowWidth(s_createLoadJoinListButtonWidth);
    m_list->SetButtonRowHeight(s_createLoadJoinListButtonHeight);
    m_list->SetRowSpacing(0.0f);
    m_list->SetMaxVisibleElements(s_createLoadJoinListVisibleRows);
    m_list->SetButtonPressedCallback(HandleListButtonPressed, this);
    m_list->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);
    m_groupRoot->AddComponent(m_list);

    m_worldPanel = new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f);
    m_worldPanel->SetPanelVisible(true);
    m_worldPanel->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_worldPanel->SetButtonRowWidth(260.0f);
    m_worldPanel->SetButtonRowHeight(42.0f);
    m_worldPanel->SetTextInputRowWidth(260.0f);
    m_worldPanel->SetTextInputRowHeight(42.0f);
    m_worldPanel->SetRowSpacing(6.0f);
    m_worldPanel->SetButtonPressedCallback(HandleWorldPanelButtonPressed, this);
    m_worldPanel->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);
    m_worldPanel->AddNewTextInput(s_createLoadJoinWorldPanelRenameInputId)->SetMaxLength(64);
    m_worldPanel->AddNewButton(s_createLoadJoinWorldPanelRenameId, everstead->GetString("world.panel.rename"));
    m_worldPanel->AddNewButton(s_createLoadJoinWorldPanelDuplicateId, everstead->GetString("world.panel.duplicate"));
    m_worldPanel->AddNewButton(s_createLoadJoinWorldPanelDeleteId, everstead->GetString("world.panel.delete"));
    m_worldPanel->AddNewButton(s_createLoadJoinWorldPanelBackId, everstead->GetString("button.back"));
    m_worldPanel->ResizeHorizontal();
    m_worldPanel->ResizeVertical();
    m_worldPanel->SetVisible(false);
    m_groupRoot->AddComponent(m_worldPanel);

    AddGroup(m_groupRoot);

    BuildTabContent();
    LayoutControls();
}

UIScene_LoadCreateJoin::~UIScene_LoadCreateJoin()
{}

void UIScene_LoadCreateJoin::OnEnter()
{
    BuildTabContent();
    m_wasVisible          = true;
    m_lastSessionsVersion = Everstead::GetInstance()->GetMultiplayerSession()->GetFriendSessionsVersion();
    if (m_selectedTab == 2)
    {
        RequestSessionRefresh();
    }
}

void UIScene_LoadCreateJoin::TickInput()
{
    InputManager *input = Everstead::GetInstance()->GetInputManager();
    if (!input->IsUiTextInputFocused())
    {
        if (input->ConsumeUiNavigateLeftPress())
        {
            SetSelectedTab(m_selectedTab - 1);
        }
        else if (input->ConsumeUiNavigateRightPress())
        {
            SetSelectedTab(m_selectedTab + 1);
        }
    }

    int tabDelta = input->ConsumeUiTabDelta();
    if (tabDelta != 0)
    {
        SetSelectedTab(m_selectedTab + tabDelta);
    }

    HandleTabMouseInput();
    HandleWorldPanelMouseInput();
    UIScene::TickInput();
}

void UIScene_LoadCreateJoin::Tick()
{
    if (!IsVisible())
    {
        m_wasVisible = false;
        return;
    }

    if (!m_wasVisible)
    {
        BuildTabContent();
        m_wasVisible = true;
        if (m_selectedTab == 2)
        {
            RequestSessionRefresh();
        }
    }

    MultiplayerSession *multiplayerSession = Everstead::GetInstance()->GetMultiplayerSession();
    if (m_selectedTab == 2 && multiplayerSession->GetFriendSessionsVersion() != m_lastSessionsVersion)
    {
        m_lastSessionsVersion = multiplayerSession->GetFriendSessionsVersion();
        BuildTabContent();
    }

    if (m_selectedTab == 2 && Time::GetMilliseconds() - m_lastJoinSessionRefreshMs >= s_createLoadJoinSessionRefreshIntervalMs)
    {
        RequestSessionRefresh();
    }

    if (m_worldPanel->IsVisible())
    {
        m_worldPanel->Tick();
        return;
    }

    UIScene::Tick();
}

void UIScene_LoadCreateJoin::Render()
{
    if (!IsVisible())
    {
        return;
    }

    DrawMenuBackground();
    UIScene::Render();
}

void UIScene_LoadCreateJoin::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    Everstead::GetInstance()->GetUiController()->CloseScene();
}

void UIScene_LoadCreateJoin::HandleListButtonPressed(void *context, uint32_t id)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    UIScene_LoadCreateJoin *scene = (UIScene_LoadCreateJoin *) context;
    if (scene->m_worldPanel->IsVisible())
    {
        return;
    }

    if (scene->m_selectedTab == 0)
    {
        if (id == 0 || id > scene->m_worlds.size())
        {
            return;
        }

        Everstead::GetInstance()->OpenLoadWorldMenu(scene->m_worlds[(size_t) id - 1].name);
        scene->HideWorldPanel();
        return;
    }

    if (scene->m_selectedTab == 1 && id == s_createLoadJoinCreateId)
    {
        if (kStickFight)
        {
            Everstead *everstead                           = Everstead::GetInstance();
            everstead->GetNewWorldGameRules()              = GameRules();
            everstead->GetNewWorldGenSettings()            = WorldGenSettings();
            everstead->GetNewWorldGenSettings().worldType  = eWorldType_Flat;
            everstead->GetNewWorldGenSettings().flatLayers = {{"air", 1}};
            everstead->GetNewWorldGameRules().Set(eGameRule_DoImmediateRespawn, 1);
            everstead->GetNewWorldGameRules().Set(eGameRule_KeepInventory, 1);
            everstead->GetNewWorldGameRules().Set(eGameRule_DoMobSpawning, 0);
            everstead->GetNewWorldGameRules().Set(eGameRule_DoDaylightCycle, 0);
            everstead->GetNewWorldGameRules().Set(eGameRule_DoWeatherCycle, 0);
            everstead->StartNewWorld("Stick Fight", "", 0, eGameType_Survival);
            scene->HideWorldPanel();
            return;
        }

        Everstead::GetInstance()->GetUiController()->NavigateToScene(eUIScene_CreateWorldMenu);
        scene->HideWorldPanel();
        return;
    }

    if (scene->m_selectedTab == 2 && id >= s_createLoadJoinSessionBaseId)
    {
        size_t index = (size_t) (id - s_createLoadJoinSessionBaseId);
        if (index >= scene->m_sessions.size())
        {
            return;
        }

        Everstead::GetInstance()->GetMultiplayerSession()->SetSelectedFriendSession(scene->m_sessions[index]);
        Everstead::GetInstance()->GetUiController()->NavigateToScene(eUIScene_JoinSessionMenu);
        scene->HideWorldPanel();
    }
}

void UIScene_LoadCreateJoin::HandleWorldPanelButtonPressed(void *context, uint32_t id)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    UIScene_LoadCreateJoin *scene = (UIScene_LoadCreateJoin *) context;

    switch (id)
    {
        case s_createLoadJoinWorldPanelRenameId: {
            scene->RenamePanelWorld();
            break;
        }

        case s_createLoadJoinWorldPanelDuplicateId: {
            scene->DuplicatePanelWorld();
            break;
        }

        case s_createLoadJoinWorldPanelDeleteId: {
            scene->DeletePanelWorld();
            break;
        }

        case s_createLoadJoinWorldPanelBackId: {
            scene->HideWorldPanel();
            break;
        }

        default: {
            break;
        }
    }
}

void UIScene_LoadCreateJoin::BuildTabContent()
{
    m_list->ClearEntries();
    HideWorldPanel();
    if (m_selectedTab == 0)
    {
        m_sessions.clear();
        m_worlds.clear();
        if (!kStickFight)
        {
            m_worlds = WorldSave::ListWorlds();
        }

        uint32_t buttonId = 1;
        for (auto it = m_worlds.begin(); it != m_worlds.end(); it++)
        {
            std::wstring name(it->name.begin(), it->name.end());
            TextureRepository::GetInstance()->Invalidate(it->iconPath);
            m_list->AddNewIconButton(buttonId, name, it->iconPath.empty() ? s_createLoadJoinIconTexture : it->iconPath, s_createLoadJoinIconTexture);
            buttonId++;
        }
    }
    else if (m_selectedTab == 1)
    {
        m_worlds.clear();
        m_sessions.clear();
        m_list->AddNewIconButton(s_createLoadJoinCreateId,
                                 Everstead::GetInstance()->GetString(kStickFight ? "menu.main.stick_fight" : "world.create.create_new_world"),
                                 s_createLoadJoinIconTexture);
    }
    else
    {
        RefreshJoinSessions();
    }

    m_list->ResizeHorizontal();
    m_list->ResizeVertical();
    m_list->ResetPointerInputState();
    LayoutControls();
}

void UIScene_LoadCreateJoin::RequestSessionRefresh()
{
    Everstead::GetInstance()->GetMultiplayerSession()->RequestRefreshFriendSessions();
    m_lastJoinSessionRefreshMs = Time::GetMilliseconds();
}

void UIScene_LoadCreateJoin::RefreshJoinSessions()
{
    m_worlds.clear();
    m_sessions = Everstead::GetInstance()->GetMultiplayerSession()->GetFriendSessions();
    for (size_t i = 0; i < m_sessions.size(); i++)
    {
        std::wstring label = m_sessions[i].hostUsername.empty() ? Everstead::GetInstance()->GetString("world.join.friend")
                                                                : StringUtils::ToUtf16(m_sessions[i].hostUsername);
        m_list->AddNewIconButton(s_createLoadJoinSessionBaseId + (uint32_t) i, label, s_createLoadJoinIconTexture);
    }
}

void UIScene_LoadCreateJoin::SetSelectedTab(int selectedTab)
{
    int wrapped = (selectedTab + 3) % 3;
    if (wrapped == m_selectedTab)
    {
        return;
    }

    m_selectedTab = wrapped;
    BuildTabContent();
    if (m_selectedTab == 2)
    {
        RequestSessionRefresh();
    }
}

void UIScene_LoadCreateJoin::LayoutControls()
{
    float menuX   = (Screen::WIDTH - s_createLoadJoinMenuWidth) * 0.5f;
    float menuY   = (Screen::HEIGHT - s_createLoadJoinMenuHeight) * 0.5f;
    float tabPart = s_createLoadJoinMenuWidth / 3.0f;

    for (int i = 0; i < 3; i++)
    {
        float raise = i == m_selectedTab ? s_createLoadJoinTabTextRaise : 0.0f;
        m_tabLabels[i]->SetBounds(menuX + (float) i * tabPart, menuY + s_createLoadJoinTabTextY - raise, tabPart, s_createLoadJoinTabTextHeight);
    }

    m_list->SetPosition(menuX + s_createLoadJoinListX, menuY + s_createLoadJoinListY);
}

void UIScene_LoadCreateJoin::HandleTabMouseInput()
{
    int mouseX            = 0;
    int mouseY            = 0;
    uint32_t mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
    bool mousePrimaryDown = (mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    if (!mousePrimaryDown || m_previousMousePrimaryDown)
    {
        m_previousMousePrimaryDown = mousePrimaryDown;
        return;
    }

    Everstead *everstead = Everstead::GetInstance();
    float menuX          = (Screen::WIDTH - s_createLoadJoinMenuWidth) * 0.5f;
    float menuY          = (Screen::HEIGHT - s_createLoadJoinMenuHeight) * 0.5f;
    float tabHeight = s_createLoadJoinMenuWidth * ((float) m_tabRegions[m_selectedTab].pixelHeight / (float) m_tabRegions[m_selectedTab].pixelWidth);
    Screen::Rect tabRect = Screen::ToActualRect(menuX, menuY + s_createLoadJoinTabOffsetY, s_createLoadJoinMenuWidth, tabHeight,
                                                everstead->GetWidth(), everstead->GetHeight());
    if ((float) mouseX < tabRect.x || (float) mouseX > tabRect.x + tabRect.width || (float) mouseY < tabRect.y ||
        (float) mouseY > tabRect.y + tabRect.height)
    {
        m_previousMousePrimaryDown = mousePrimaryDown;
        return;
    }

    SetSelectedTab((int) (((float) mouseX - tabRect.x) / (tabRect.width / 3.0f)));
    m_previousMousePrimaryDown = mousePrimaryDown;
}

void UIScene_LoadCreateJoin::HandleWorldPanelMouseInput()
{
    int mouseX              = 0;
    int mouseY              = 0;
    uint32_t mouseButtons   = SDL_GetMouseState(&mouseX, &mouseY);
    bool mouseSecondaryDown = (mouseButtons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    if (!mouseSecondaryDown || m_previousMouseSecondaryDown)
    {
        m_previousMouseSecondaryDown = mouseSecondaryDown;
        return;
    }

    m_previousMouseSecondaryDown = mouseSecondaryDown;
    if (m_selectedTab != 0)
    {
        HideWorldPanel();
        return;
    }

    Everstead *everstead = Everstead::GetInstance();
    float menuX          = (Screen::WIDTH - s_createLoadJoinMenuWidth) * 0.5f;
    float menuY          = (Screen::HEIGHT - s_createLoadJoinMenuHeight) * 0.5f;
    float rowX           = menuX + s_createLoadJoinListX;
    float rowY           = menuY + s_createLoadJoinListY;

    for (size_t i = 0; i < m_worlds.size() && i < (size_t) s_createLoadJoinListVisibleRows; i++)
    {
        Screen::Rect rowRect = Screen::ToActualRect(rowX, rowY + (float) i * s_createLoadJoinListButtonHeight, s_createLoadJoinListButtonWidth,
                                                    s_createLoadJoinListButtonHeight, everstead->GetWidth(), everstead->GetHeight());
        if ((float) mouseX >= rowRect.x && (float) mouseX <= rowRect.x + rowRect.width && (float) mouseY >= rowRect.y &&
            (float) mouseY <= rowRect.y + rowRect.height)
        {
            ShowWorldPanel(i);
            return;
        }
    }

    HideWorldPanel();
}

void UIScene_LoadCreateJoin::ShowWorldPanel(size_t worldIndex)
{
    if (worldIndex >= m_worlds.size())
    {
        HideWorldPanel();
        return;
    }

    m_worldPanelIndex     = worldIndex;
    std::string worldName = m_worlds[worldIndex].name;
    float x               = (Screen::WIDTH - m_worldPanel->GetWidth()) * 0.5f;
    float y               = (Screen::HEIGHT - m_worldPanel->GetHeight()) * 0.5f;
    m_worldPanel->SetTextInputText(s_createLoadJoinWorldPanelRenameInputId, std::wstring(worldName.begin(), worldName.end()));
    m_worldPanel->SetPosition(x, y);
    m_worldPanel->SetVisible(true);
    m_worldPanel->ResetPointerInputState();
}

void UIScene_LoadCreateJoin::HideWorldPanel()
{
    m_worldPanel->SetVisible(false);
}

void UIScene_LoadCreateJoin::RenamePanelWorld()
{
    if (!m_worldPanel->IsVisible() || m_worldPanelIndex >= m_worlds.size())
    {
        return;
    }

    std::string oldName = m_worlds[m_worldPanelIndex].name;
    std::string newName = StringUtils::ToUtf8(m_worldPanel->GetTextInputText(s_createLoadJoinWorldPanelRenameInputId));
    if (WorldSave::RenameWorld(oldName, newName))
    {
        BuildTabContent();
    }
}

void UIScene_LoadCreateJoin::DuplicatePanelWorld()
{
    if (!m_worldPanel->IsVisible() || m_worldPanelIndex >= m_worlds.size())
    {
        return;
    }

    if (WorldSave::DuplicateWorld(m_worlds[m_worldPanelIndex].name))
    {
        BuildTabContent();
    }
}

void UIScene_LoadCreateJoin::DeletePanelWorld()
{
    if (!m_worldPanel->IsVisible() || m_worldPanelIndex >= m_worlds.size())
    {
        return;
    }

    if (WorldSave::DeleteWorld(m_worlds[m_worldPanelIndex].name))
    {
        BuildTabContent();
    }
}

void UIScene_LoadCreateJoin::DrawMenuBackground()
{
    Everstead *everstead = Everstead::GetInstance();
    int actualWidth      = everstead->GetWidth();
    int actualHeight     = everstead->GetHeight();
    float menuX          = (Screen::WIDTH - s_createLoadJoinMenuWidth) * 0.5f;
    float menuY          = (Screen::HEIGHT - s_createLoadJoinMenuHeight) * 0.5f;

    Screen::Rect backgroundRect =
            Screen::ToActualRect(menuX, menuY, s_createLoadJoinMenuWidth, s_createLoadJoinMenuHeight, actualWidth, actualHeight);
    float tabHeight = s_createLoadJoinMenuWidth * ((float) m_tabRegions[m_selectedTab].pixelHeight / (float) m_tabRegions[m_selectedTab].pixelWidth);
    Screen::Rect tabRect =
            Screen::ToActualRect(menuX, menuY + s_createLoadJoinTabOffsetY, s_createLoadJoinMenuWidth, tabHeight, actualWidth, actualHeight);

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());

    DrawRegion(m_backgroundRegion, round(backgroundRect.x), round(backgroundRect.y), round(backgroundRect.x + backgroundRect.width),
               round(backgroundRect.y + backgroundRect.height));
    DrawRegion(m_tabRegions[m_selectedTab], round(tabRect.x), round(tabRect.y), round(tabRect.x + tabRect.width), round(tabRect.y + tabRect.height));

    builder->End();
    Textures::Unbind();

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();
}

void UIScene_LoadCreateJoin::DrawRegion(const UITextureAtlas::Region &region, float x0, float y0, float x1, float y1)
{
    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Color(1.0f, 1.0f, 1.0f, 1.0f);
    builder->VertexUV(x0, y0, 0.0f, region.u0, region.v0);
    builder->VertexUV(x1, y0, 0.0f, region.u1, region.v0);
    builder->VertexUV(x1, y1, 0.0f, region.u1, region.v1);
    builder->VertexUV(x0, y0, 0.0f, region.u0, region.v0);
    builder->VertexUV(x1, y1, 0.0f, region.u1, region.v1);
    builder->VertexUV(x0, y1, 0.0f, region.u0, region.v1);
}

#endif
