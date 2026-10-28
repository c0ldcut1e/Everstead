#pragma once

#include <cstdint>
#include <future>
#include <string>
#include <vector>

#include "../../network/NetworkResult.h"
#include "../../network/NetworkTypes.h"
#include "../component/UIComponent_Fill.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_ServersMenu : public UIScene
{
public:
    UIScene_ServersMenu();
    ~UIScene_ServersMenu() override;

    void OnEnter() override;
    void Tick() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static const uint32_t s_serversMenuTitleId        = 1;
    static const uint32_t s_serversMenuNameLabelId    = 2;
    static const uint32_t s_serversMenuNameInputId    = 3;
    static const uint32_t s_serversMenuAddId          = 4;
    static const uint32_t s_serversMenuBackId         = 5;
    static const uint32_t s_serversMenuEmptyLabelId   = 6;
    static const uint32_t s_serversMenuStatusLabelId  = 7;
    static const uint32_t s_serversMenuPlayersLabelId = 8;
    static const uint32_t s_serversMenuJoinId         = 9;
    static const uint32_t s_serversMenuRefreshId      = 10;
    static const uint32_t s_serversMenuRemoveId       = 11;
    static const uint32_t s_serversMenuServerBase     = 1000;

    struct ServerStatus
    {
        NetworkResult result;
        bool online        = false;
        bool joinable      = false;
        Sid sid            = 0;
        int maxPlayers     = 0;
        int currentPlayers = 0;
    };

    static void HandleButtonPressed(void *context, uint32_t id);

    void RefreshView();
    void AddServer();
    void OpenServer(uint32_t index);
    void RequestStatus();
    void JoinServer();
    void RemoveServer();
    void GoBack();
    void LoadServers();
    void SaveServers() const;
    void ResizeMenu();

    UIGroup *m_groupRoot;
    UIComponent_Fill *m_backgroundFill;
    UIControl_MultiList *m_multiList;
    std::vector<std::string> m_servers;
    std::wstring m_nameText;
    bool m_detailsOpen;
    uint32_t m_selectedIndex;
    std::future<ServerStatus> m_statusFuture;
    ServerStatus m_status;
    bool m_statusLoaded;
};
