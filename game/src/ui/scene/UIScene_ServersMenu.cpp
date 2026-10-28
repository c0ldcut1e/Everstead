#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_ServersMenu.h"

#include <string>
#include <vector>

#include "../../core/Everstead.h"
#include "../../io/FileSystem.h"
#include "../../network/AccountService.h"
#include "../../network/ApiClient.h"
#include "../../utils/StringUtils.h"
#include "../Screen.h"

#include <nlohmann/json.hpp>

UIScene_ServersMenu::UIScene_ServersMenu() : UIScene(eUIScene_ServersMenu)
{
    m_groupRoot      = new UIGroup("Group_ServersMenu");
    m_backgroundFill = new UIComponent_Fill(0x70000000);
    m_multiList      = new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f);
    m_detailsOpen    = false;
    m_selectedIndex  = 0;
    m_statusLoaded   = false;
    m_multiList->SetPanelVisible(true);
    m_multiList->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_multiList->SetButtonRowWidth(680.0f);
    m_multiList->SetButtonRowHeight(50.0f);
    m_multiList->SetTextInputRowWidth(680.0f);
    m_multiList->SetTextInputRowHeight(50.0f);
    m_multiList->SetLabelRowWidth(680.0f);
    m_multiList->SetLabelRowHeight(28.0f);
    m_multiList->SetButtonPressedCallback(HandleButtonPressed, this);
    m_multiList->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);
    m_multiList->SetRowSpacing(10.0f);
    m_multiList->SetMaxVisibleElements(13);

    m_backgroundFill->SetVisible(false);

    m_groupRoot->AddComponent(m_backgroundFill);
    m_groupRoot->AddComponent(m_multiList);
    AddGroup(m_groupRoot);
}

UIScene_ServersMenu::~UIScene_ServersMenu()
{}

void UIScene_ServersMenu::OnEnter()
{
    m_backgroundFill->SetVisible(Everstead::GetInstance()->GetLevel() != nullptr);
    m_detailsOpen   = false;
    m_selectedIndex = 0;
    LoadServers();
    RefreshView();
    m_multiList->ResetPointerInputState();
}

void UIScene_ServersMenu::Tick()
{
    UIScene::Tick();

    if (!m_statusFuture.valid() || m_statusFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
    {
        return;
    }

    m_status       = m_statusFuture.get();
    m_statusLoaded = true;
    if (m_detailsOpen)
    {
        RefreshView();
    }
}

void UIScene_ServersMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    GoBack();
}

void UIScene_ServersMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    UIScene_ServersMenu *_this = (UIScene_ServersMenu *) context;

    if (id >= s_serversMenuServerBase)
    {
        _this->OpenServer(id - s_serversMenuServerBase);
        return;
    }

    switch (id)
    {
        case s_serversMenuAddId: {
            _this->AddServer();
            break;
        }

        case s_serversMenuJoinId: {
            _this->JoinServer();
            break;
        }

        case s_serversMenuRefreshId: {
            _this->RequestStatus();
            _this->RefreshView();
            break;
        }

        case s_serversMenuRemoveId: {
            _this->RemoveServer();
            break;
        }

        case s_serversMenuBackId: {
            _this->GoBack();
            break;
        }

        default: {
            break;
        }
    }
}

void UIScene_ServersMenu::RefreshView()
{
    Everstead *everstead = Everstead::GetInstance();
    if (!m_detailsOpen)
    {
        m_nameText = m_multiList->GetTextInputText(s_serversMenuNameInputId);
    }

    m_multiList->ClearEntries();

    if (!m_detailsOpen)
    {
        m_multiList->AddNewLabel(s_serversMenuTitleId, everstead->GetString("servers.title"));
        m_multiList->AddNewLabel(s_serversMenuNameLabelId, everstead->GetString("servers.name") + L":");
        m_multiList->AddNewTextInput(s_serversMenuNameInputId);
        m_multiList->SetTextInputText(s_serversMenuNameInputId, m_nameText);
        m_multiList->AddNewButton(s_serversMenuAddId, everstead->GetString("servers.add"));
        m_multiList->AddNewSpacer(0, 20.0f);
        if (m_servers.empty())
        {
            m_multiList->AddNewLabel(s_serversMenuEmptyLabelId, everstead->GetString("servers.empty"));
        }

        for (size_t i = 0; i < m_servers.size(); i++)
        {
            m_multiList->AddNewButton(s_serversMenuServerBase + (uint32_t) i, StringUtils::ToUtf16(m_servers[i]));
        }

        m_multiList->AddNewSpacer(0, 20.0f);
        m_multiList->AddNewButton(s_serversMenuBackId, everstead->GetString("button.back"));
        ResizeMenu();
        return;
    }

    m_multiList->AddNewLabel(s_serversMenuTitleId, StringUtils::ToUtf16(m_servers[m_selectedIndex]));
    if (!m_statusLoaded)
    {
        m_multiList->AddNewLabel(s_serversMenuStatusLabelId, everstead->GetString("servers.checking"));
    }
    else if (!m_status.result.IsSuccess())
    {
        m_multiList->AddNewLabel(s_serversMenuStatusLabelId, m_status.result.GetMessage());
    }
    else if (!m_status.online)
    {
        m_multiList->AddNewLabel(s_serversMenuStatusLabelId, everstead->GetString("servers.offline"));
    }
    else
    {
        m_multiList->AddNewLabel(s_serversMenuStatusLabelId, everstead->GetString("servers.online"));
        m_multiList->AddNewLabel(s_serversMenuPlayersLabelId, everstead->GetString("servers.players") + L": " +
                                                                      std::to_wstring(m_status.currentPlayers) + L"/" +
                                                                      std::to_wstring(m_status.maxPlayers));
        if (m_status.joinable)
        {
            m_multiList->AddNewButton(s_serversMenuJoinId, everstead->GetString("servers.join"));
        }
    }

    m_multiList->AddNewButton(s_serversMenuRefreshId, everstead->GetString("servers.refresh"));
    m_multiList->AddNewButton(s_serversMenuRemoveId, everstead->GetString("servers.remove"));
    m_multiList->AddNewSpacer(0, 20.0f);
    m_multiList->AddNewButton(s_serversMenuBackId, everstead->GetString("button.back"));
    ResizeMenu();
}

void UIScene_ServersMenu::AddServer()
{
    std::string name = StringUtils::Trim(StringUtils::ToUtf8(m_multiList->GetTextInputText(s_serversMenuNameInputId)));
    m_multiList->SetTextInputText(s_serversMenuNameInputId, L"");
    if (name.empty())
    {
        RefreshView();
        return;
    }

    for (size_t i = 0; i < m_servers.size(); i++)
    {
        if (StringUtils::EqualsIgnoreCase(m_servers[i], name))
        {
            RefreshView();
            return;
        }
    }

    m_servers.push_back(name);
    SaveServers();
    RefreshView();
}

void UIScene_ServersMenu::OpenServer(uint32_t index)
{
    m_nameText      = m_multiList->GetTextInputText(s_serversMenuNameInputId);
    m_detailsOpen   = true;
    m_selectedIndex = index;
    RequestStatus();
    RefreshView();
}

void UIScene_ServersMenu::RequestStatus()
{
    if (m_statusFuture.valid())
    {
        m_statusFuture.wait();
    }

    m_statusLoaded       = false;
    ApiClient *apiClient = Everstead::GetInstance()->GetAccountService()->GetApiClient();
    std::string name     = m_servers[m_selectedIndex];
    m_statusFuture       = std::async(std::launch::async,
                                      [apiClient, name]()
                                      {
                                    ServerStatus status;
                                    nlohmann::json document;
                                    status.result = apiClient->Get("/servers/name/" + name, &document);
                                    if (!status.result.IsSuccess())
                                    {
                                        return status;
                                    }

                                    status.online   = document.value("online", false);
                                    status.joinable = document.value("joinable", false);
                                    if (status.online)
                                    {
                                        status.sid            = std::stoull(document.value("sid", std::string("0")));
                                        status.maxPlayers     = document.value("maxPlayers", 0);
                                        status.currentPlayers = document.value("currentPlayers", 0);
                                    }

                                    return status;
                                      });
}

void UIScene_ServersMenu::JoinServer()
{
    Everstead::GetInstance()->StartJoinedWorld(m_status.sid, m_servers[m_selectedIndex]);
}

void UIScene_ServersMenu::RemoveServer()
{
    m_servers.erase(m_servers.begin() + m_selectedIndex);
    SaveServers();
    m_detailsOpen = false;
    RefreshView();
}

void UIScene_ServersMenu::GoBack()
{
    if (m_detailsOpen)
    {
        m_detailsOpen = false;
        RefreshView();
        return;
    }

    Everstead::GetInstance()->GetUiController()->CloseScene();
}

void UIScene_ServersMenu::LoadServers()
{
    m_servers.clear();
    File file = FileSystem::GetUserDirectory().GetChild("servers.json");
    if (!file.Exists())
    {
        return;
    }

    nlohmann::json document = nlohmann::json::parse(file.ReadAllText(), nullptr, false);
    if (!document.is_object() || !document["servers"].is_array())
    {
        return;
    }

    for (size_t i = 0; i < document["servers"].size(); i++)
    {
        if (document["servers"][i].is_string())
        {
            m_servers.push_back(document["servers"][i].get<std::string>());
        }
    }
}

void UIScene_ServersMenu::SaveServers() const
{
    nlohmann::json document;
    document["servers"] = m_servers;
    std::string text    = document.dump(4);
    FileSystem::GetUserDirectory().GetChild("servers.json").WriteAllBytes(std::vector<uint8_t>(text.begin(), text.end()));
}

void UIScene_ServersMenu::ResizeMenu()
{
    m_multiList->SetSize(0.0f, 0.0f);
    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);
}

#endif
