#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIController.h"

#include "../../core/Everstead.h"
#include "../../entity/player/LocalPlayer.h"
#include "../../sound/SoundEngine.h"
#include "../../sound/SoundRegistry.h"
#include "../../utils/Time.h"
#include "../scene/UIScene_Anvil.h"
#include "../scene/UIScene_Container.h"
#include "../scene/UIScene_ControllerMenu.h"
#include "../scene/UIScene_ControlsMenu.h"
#include "../scene/UIScene_CraftingTable.h"
#include "../scene/UIScene_CreateWorldMenu.h"
#include "../scene/UIScene_DeathMenu.h"
#include "../scene/UIScene_DebugOverlay.h"
#include "../scene/UIScene_FriendsMenu.h"
#include "../scene/UIScene_Furnace.h"
#include "../scene/UIScene_HUD.h"
#include "../scene/UIScene_HelpAndOptionsMenu.h"
#include "../scene/UIScene_Inventory.h"
#include "../scene/UIScene_InvitationMenu.h"
#include "../scene/UIScene_JoinSessionMenu.h"
#include "../scene/UIScene_KeyboardMouseMenu.h"
#include "../scene/UIScene_LanguagesMenu.h"
#include "../scene/UIScene_LoadCreateJoin.h"
#include "../scene/UIScene_LoadWorldMenu.h"
#include "../scene/UIScene_MainMenu.h"
#include "../scene/UIScene_ModalDialog.h"
#include "../scene/UIScene_MoreWorldOptions.h"
#include "../scene/UIScene_NetworkMenu.h"
#include "../scene/UIScene_PauseMenu.h"
#include "../scene/UIScene_RecipeView.h"
#include "../scene/UIScene_ResonantSocket.h"
#include "../scene/UIScene_Resonator.h"
#include "../scene/UIScene_ServersMenu.h"
#include "../scene/UIScene_SettingsMenu.h"
#include "../scene/UIScene_SignEdit.h"
#include "../scene/UIScene_SleepMenu.h"
#include "../scene/UIScene_Test.h"
#include "../scene/UIScene_WorldGenMenu.h"
#include "../scene/UIScene_WorldLoading.h"
#include "../scene/UIScene_WorldOptionsMenu.h"
#include "../scene/UIScene_WorldSaving.h"

#include <SDL2/SDL.h>

UIController::UIController()
{
    m_menuPanorama        = new UIComponent_Panorama("assets/texture/ui/panorama");
    m_notification        = new UIComponent_Notification();
    m_virtualKeyboard     = new UIComponent_VirtualKeyboard();
    m_sceneStackLockDepth = 0;
}

UIController::~UIController()
{
    ClearScenes();

    delete m_menuPanorama;
    m_menuPanorama = nullptr;

    delete m_notification;
    m_notification = nullptr;

    delete m_virtualKeyboard;
    m_virtualKeyboard = nullptr;
}

void UIController::TickInput(InputManager *inputManager, bool inWorld)
{
    if (!inputManager)
    {
        return;
    }

    bool controller       = inputManager->IsUsingController();
    LocalPlayer *player   = Everstead::GetInstance()->GetLocalPlayer();
    eGAME_TYPE gameType   = player ? player->GetGameType() : eGameType_Survival;
    UIScene *inventoryTop = GetTopScene();
    bool inventoryInHud   = inventoryTop && (inventoryTop->GetId() == eUIScene_HUD || inventoryTop->GetId() == eUIScene_DebugOverlay);
    if (inputManager->ConsumeActionPress(eGameplayAction_ToggleInventory) && inWorld)
    {
        // On controller the inventory button acts on slots, so only cancel closes the menus
        if (inventoryTop && !controller && inventoryTop->GetId() == eUIScene_Inventory)
        {
            Everstead::GetInstance()->CloseInventory();
        }
        else if (inventoryTop && !controller &&
                 (inventoryTop->GetId() == eUIScene_Container || inventoryTop->GetId() == eUIScene_CraftingTable ||
                  inventoryTop->GetId() == eUIScene_RecipeView || inventoryTop->GetId() == eUIScene_Furnace ||
                  inventoryTop->GetId() == eUIScene_ResonantSocket || inventoryTop->GetId() == eUIScene_Resonator ||
                  inventoryTop->GetId() == eUIScene_Anvil))
        {
            player->CloseContainer();
        }
        else if (inventoryInHud && !GameType::IsSpectator(gameType))
        {
            Everstead::GetInstance()->OpenInventory(eUIScene_Inventory);
        }
    }

    bool backPressed      = inputManager->ConsumeActionPress(eGameplayAction_Back);
    UIScene *cancelScene  = GetTopScene();
    bool cancelInGameplay = !cancelScene || cancelScene->GetId() == eUIScene_HUD || cancelScene->GetId() == eUIScene_DebugOverlay;
    if (inputManager->ConsumeUiCancelPress() && !cancelInGameplay)
    {
        backPressed = true;
    }

    if (!inWorld && inputManager->ConsumeUiTextCancelPress())
    {
        backPressed = true;
    }

    if (cancelInGameplay)
    {
        inputManager->ConsumeUiNavigateUpPress();
        inputManager->ConsumeUiNavigateDownPress();
        inputManager->ConsumeUiNavigateLeftPress();
        inputManager->ConsumeUiNavigateRightPress();
        inputManager->ConsumeUiActivatePress();
    }

    if (backPressed)
    {
        UIScene *topScene = GetTopScene();
        if (topScene && topScene->GetId() != eUIScene_HUD && topScene->GetId() != eUIScene_DebugOverlay && topScene->GetId() != eUIScene_MainMenu)
        {
            PlayUiSound("ui.back");
        }

        if (inWorld)
        {
            if (topScene)
            {
                if (Everstead::GetInstance()->GetGui()->IsChatOpen())
                {
                    Everstead::GetInstance()->GetGui()->CloseChat();
                }
                else if (topScene->GetId() == eUIScene_HUD || topScene->GetId() == eUIScene_DebugOverlay)
                {
                    Everstead::GetInstance()->OpenPauseMenu();
                }
                else if (topScene->IsVisible())
                {
                    topScene->OnGameplayAction(eGameplayAction_Back);
                }
            }
        }
        else if (topScene && topScene->IsVisible())
        {
            topScene->OnGameplayAction(eGameplayAction_Back);
        }
    }

    UpdateSceneStackStates();

    m_sceneStackLockDepth++;
    for (size_t i = 0; i < m_sceneStack.size(); i++)
    {
        UIScene *scene = m_sceneStack[i].scene;
        if (scene)
        {
            scene->TickInput();
        }
    }

    m_sceneStackLockDepth--;
    ApplyPendingSceneOperations();
}

void UIController::Tick()
{
    UpdateSceneStackStates();
    TickScenes();
    m_virtualKeyboard->Tick();
}

void UIController::OpenVirtualKeyboard(const std::wstring &text, bool masked)
{
    m_virtualKeyboard->Open(text, masked);
}

bool UIController::IsVirtualKeyboardOpen() const
{
    return m_virtualKeyboard->IsOpen();
}

void UIController::CloseVirtualKeyboard()
{
    m_virtualKeyboard->Close();
}

const std::wstring &UIController::GetVirtualKeyboardText() const
{
    return m_virtualKeyboard->GetText();
}

void UIController::Render()
{
    UpdateSceneStackStates();
    RenderScenes();
}

void UIController::TickScenes()
{
    m_sceneStackLockDepth++;
    for (size_t i = 0; i < m_sceneStack.size(); i++)
    {
        UIScene *scene = m_sceneStack[i].scene;
        if (scene)
        {
            scene->Tick();
        }
    }

    m_sceneStackLockDepth--;
    ApplyPendingSceneOperations();
}

void UIController::RenderScenes()
{
    m_sceneStackLockDepth++;

    if (ShouldRenderMenuPanorama())
    {
        m_menuPanorama->Render();
    }

    for (size_t i = 0; i < m_sceneStack.size(); i++)
    {
        UIScene *scene = m_sceneStack[i].scene;
        if (!scene)
        {
            continue;
        }

        scene->Render();
        UIScene *nextScene = i + 1 < m_sceneStack.size() ? m_sceneStack[i + 1].scene : nullptr;
        // The transition covers the HUD and debug overlay but stays under every menu opened on top of them
        if ((scene->GetId() == eUIScene_HUD || scene->GetId() == eUIScene_DebugOverlay) &&
            !(nextScene && (nextScene->GetId() == eUIScene_HUD || nextScene->GetId() == eUIScene_DebugOverlay)))
        {
            Everstead *everstead = Everstead::GetInstance();
            everstead->GetGameRenderer()->GetDimensionTransition()->Render(everstead->GetWidth(), everstead->GetHeight());
        }
    }

    m_notification->Render();
    m_virtualKeyboard->Render();

    m_sceneStackLockDepth--;
    ApplyPendingSceneOperations();
}

void UIController::UpdateSceneStackStates()
{
    int topSceneStackIndex = (int) m_sceneStack.size() - 1;
    for (size_t i = 0; i < m_sceneStack.size(); i++)
    {
        UIScene *scene = m_sceneStack[i].scene;
        if (scene)
        {
            scene->UpdateStackStates((int) i, topSceneStackIndex);
        }
    }
}

UIScene *UIController::OpenScene(eUISCENE sceneId)
{
    if (m_sceneStackLockDepth > 0)
    {
        QueueSceneOperation(eSceneOperationType_Open, sceneId);
        return nullptr;
    }

    return OpenSceneImmediate(sceneId);
}

UIScene *UIController::NavigateToScene(eUISCENE sceneId)
{
    if (m_sceneStackLockDepth > 0)
    {
        QueueSceneOperation(eSceneOperationType_Navigate, sceneId);
        return nullptr;
    }

    return NavigateToSceneImmediate(sceneId);
}

void UIController::CloseScene()
{
    if (m_sceneStack.empty())
    {
        return;
    }

    if (m_sceneStackLockDepth > 0)
    {
        QueueSceneOperation(eSceneOperationType_Close, eUIScene_None);
        return;
    }

    CloseSceneImmediate();
}

void UIController::ClearScenes()
{
    if (m_sceneStackLockDepth > 0)
    {
        QueueSceneOperation(eSceneOperationType_Clear, eUIScene_None);
        return;
    }

    m_pendingSceneOperations.clear();
    while (!m_sceneStack.empty())
    {
        UIScene *scene = m_sceneStack.back().scene;
        m_sceneStack.pop_back();
        if (scene)
        {
            scene->OnExit();
            delete scene;
        }
    }
}

UIScene *UIController::GetTopScene() const
{
    if (m_sceneStack.empty())
    {
        return nullptr;
    }

    return m_sceneStack.back().scene;
}

UIScene *UIController::FindScene(eUISCENE sceneId) const
{
    for (const SceneEntry &entry : m_sceneStack)
    {
        UIScene *scene = entry.scene;
        if (scene && scene->GetId() == sceneId)
        {
            return scene;
        }
    }

    return nullptr;
}

bool UIController::IsSceneOpen(eUISCENE sceneId) const
{
    return FindScene(sceneId) != nullptr;
}

bool UIController::IsInvitationMenuInputBlocking() const
{
    return IsSceneOpen(eUIScene_InvitationMenu);
}

bool UIController::IsSceneCloseQueued(eUISCENE sceneId) const
{
    UIScene *topScene = GetTopScene();
    if (!topScene || topScene->GetId() != sceneId)
    {
        return false;
    }

    for (size_t i = 0; i < m_pendingSceneOperations.size(); i++)
    {
        if (m_pendingSceneOperations[i].type == eSceneOperationType_Close || m_pendingSceneOperations[i].type == eSceneOperationType_Clear)
        {
            return true;
        }
    }

    return false;
}

void UIController::ShowModal(const UIScene_ModalDialog::Dialog &dialog)
{
    m_modalDialog = dialog;

    UIScene_ModalDialog *modalScene = (UIScene_ModalDialog *) FindScene(eUIScene_ModalDialog);
    if (modalScene && !IsSceneCloseQueued(eUIScene_ModalDialog))
    {
        modalScene->RefreshDialog();
        return;
    }

    OpenScene(eUIScene_ModalDialog);
}

void UIController::CloseModal()
{
    UIScene *topScene = GetTopScene();
    if (topScene && topScene->GetId() == eUIScene_ModalDialog)
    {
        CloseScene();
    }
}

bool UIController::IsModalActive() const
{
    return IsSceneOpen(eUIScene_ModalDialog);
}

const UIScene_ModalDialog::Dialog &UIController::GetModalDialog() const
{
    return m_modalDialog;
}

void UIController::ShowNotification(const std::wstring &title, const std::wstring &message, float duration)
{
    m_notification->Show(title, message, duration);
}

void UIController::PlayUiSound(const ResourceLocation &location)
{
    SoundEngine *soundEngine = Everstead::GetInstance()->GetSoundEngine();
    if (!soundEngine)
    {
        return;
    }

    SoundEvent *sound = SoundRegistry::ByName(location);
    if (!sound)
    {
        return;
    }

    uint32_t now = Time::GetMilliseconds();
    auto it      = m_uiSoundLastPlayedMs.find(sound);
    if (it != m_uiSoundLastPlayedMs.end() && now - it->second < s_uiSoundThrottleMs)
    {
        return;
    }

    m_uiSoundLastPlayedMs[sound] = now;
    soundEngine->PlayUISound(sound, 1.0f, 1.0f);
}

UIScene *UIController::CreateScene(eUISCENE sceneId) const
{
    switch (sceneId)
    {
        case eUIScene_MainMenu: {
            return new UIScene_MainMenu();
        }

        case eUIScene_HelpAndOptionsMenu: {
            return new UIScene_HelpAndOptionsMenu();
        }

        case eUIScene_SettingsMenu: {
            return new UIScene_SettingsMenu();
        }

        case eUIScene_ControlsMenu: {
            return new UIScene_ControlsMenu();
        }

        case eUIScene_KeyboardMouseMenu: {
            return new UIScene_KeyboardMouseMenu();
        }

        case eUIScene_ControllerMenu: {
            return new UIScene_ControllerMenu();
        }

        case eUIScene_HUD: {
            return new UIScene_HUD();
        }

        case eUIScene_DebugOverlay: {
            return new UIScene_DebugOverlay();
        }

        case eUIScene_WorldLoading: {
            return new UIScene_WorldLoading();
        }

        case eUIScene_WorldSaving: {
            return new UIScene_WorldSaving();
        }

        case eUIScene_LoadCreateJoin: {
            return new UIScene_LoadCreateJoin();
        }

        case eUIScene_LoadWorldMenu: {
            return new UIScene_LoadWorldMenu();
        }

        case eUIScene_JoinSessionMenu: {
            return new UIScene_JoinSessionMenu();
        }

        case eUIScene_ModalDialog: {
            return new UIScene_ModalDialog();
        }

        case eUIScene_InvitationMenu: {
            return new UIScene_InvitationMenu();
        }

        case eUIScene_CreateWorldMenu: {
            return new UIScene_CreateWorldMenu();
        }

        case eUIScene_PauseMenu: {
            return new UIScene_PauseMenu();
        }

        case eUIScene_DeathMenu: {
            return new UIScene_DeathMenu();
        }

        case eUIScene_SleepMenu: {
            return new UIScene_SleepMenu();
        }

        case eUIScene_SignEdit: {
            return new UIScene_SignEdit();
        }

        case eUIScene_NetworkMenu: {
            return new UIScene_NetworkMenu();
        }

        case eUIScene_FriendsMenu: {
            return new UIScene_FriendsMenu();
        }

        case eUIScene_Test: {
            return new UIScene_Test();
        }

        case eUIScene_LanguagesMenu: {
            return new UIScene_LanguagesMenu();
        }

        case eUIScene_MoreWorldOptions: {
            return new UIScene_MoreWorldOptions();
        }

        case eUIScene_WorldGenMenu: {
            return new UIScene_WorldGenMenu();
        }

        case eUIScene_ServersMenu: {
            return new UIScene_ServersMenu();
        }

        case eUIScene_WorldOptionsMenu: {
            return new UIScene_WorldOptionsMenu();
        }

        case eUIScene_Inventory: {
            return new UIScene_Inventory();
        }

        case eUIScene_Container: {
            return new UIScene_Container();
        }

        case eUIScene_CraftingTable: {
            return new UIScene_CraftingTable();
        }
        case eUIScene_Furnace: {
            return new UIScene_Furnace();
        }
        case eUIScene_ResonantSocket: {
            return new UIScene_ResonantSocket();
        }
        case eUIScene_Resonator: {
            return new UIScene_Resonator();
        }
        case eUIScene_Anvil: {
            return new UIScene_Anvil();
        }

        case eUIScene_RecipeView: {
            return new UIScene_RecipeView();
        }

        default: {
            return nullptr;
        }
    }
}

void UIController::QueueSceneOperation(eSCENE_OPERATION_TYPE type, eUISCENE sceneId)
{
    PendingSceneOperation operation;
    operation.type    = type;
    operation.sceneId = sceneId;
    m_pendingSceneOperations.push_back(operation);
}

void UIController::ApplyPendingSceneOperations()
{
    if (m_sceneStackLockDepth > 0 || m_pendingSceneOperations.empty())
    {
        return;
    }

    std::vector<PendingSceneOperation> operations = m_pendingSceneOperations;
    m_pendingSceneOperations.clear();

    for (const PendingSceneOperation &operation : operations)
    {
        if (operation.type == eSceneOperationType_Open)
        {
            OpenSceneImmediate(operation.sceneId);
        }
        else if (operation.type == eSceneOperationType_Navigate)
        {
            NavigateToSceneImmediate(operation.sceneId);
        }
        else if (operation.type == eSceneOperationType_Close)
        {
            CloseSceneImmediate();
        }
        else if (operation.type == eSceneOperationType_Clear)
        {
            ClearScenes();
        }
    }
}

UIScene *UIController::OpenSceneImmediate(eUISCENE sceneId)
{
    UIScene *scene = CreateScene(sceneId);
    if (!scene)
    {
        return nullptr;
    }

    scene->SetVisible(true);
    scene->OnEnter();
    SceneEntry entry;
    entry.scene                     = scene;
    entry.restorePreviousVisibility = false;
    m_sceneStack.push_back(entry);
    UpdateSceneStackStates();
    return scene;
}

UIScene *UIController::NavigateToSceneImmediate(eUISCENE sceneId)
{
    UIScene *topScene    = GetTopScene();
    bool previousVisible = topScene && topScene->IsVisible();
    if (topScene)
    {
        topScene->SetVisible(false);
        Everstead::GetInstance()->GetInputManager()->SetUiTextInputFocused(false);
    }

    UIScene *scene = CreateScene(sceneId);
    if (!scene)
    {
        if (topScene && previousVisible)
        {
            topScene->SetVisible(true);
        }

        return nullptr;
    }

    scene->SetVisible(true);
    scene->OnEnter();

    SceneEntry entry;
    entry.scene                     = scene;
    entry.restorePreviousVisibility = previousVisible;
    m_sceneStack.push_back(entry);
    UpdateSceneStackStates();
    return scene;
}

void UIController::CloseSceneImmediate()
{
    if (m_sceneStack.empty())
    {
        return;
    }

    SceneEntry entry = m_sceneStack.back();
    m_sceneStack.pop_back();
    UIScene *scene = entry.scene;
    if (scene)
    {
        scene->OnExit();
        delete scene;
    }

    UIScene *topScene = GetTopScene();
    if (topScene && entry.restorePreviousVisibility)
    {
        topScene->SetVisible(true);
    }

    UpdateSceneStackStates();
}

bool UIController::ShouldRenderMenuPanorama() const
{
    if (m_sceneStack.empty())
    {
        return false;
    }

    for (const SceneEntry &entry : m_sceneStack)
    {
        UIScene *scene = entry.scene;
        if (scene && (scene->GetId() == eUIScene_MainMenu || scene->GetId() == eUIScene_WorldLoading || scene->GetId() == eUIScene_WorldSaving ||
                      scene->GetId() == eUIScene_LoadCreateJoin || scene->GetId() == eUIScene_LoadWorldMenu ||
                      scene->GetId() == eUIScene_JoinSessionMenu || scene->GetId() == eUIScene_CreateWorldMenu))
        {
            return true;
        }
    }

    return false;
}

#endif
