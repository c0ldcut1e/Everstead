#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../core/ResourceLocation.h"
#include "../../input/InputManager.h"
#include "../component/UIComponent_Notification.h"
#include "../component/UIComponent_Panorama.h"
#include "../component/UIComponent_VirtualKeyboard.h"
#include "../scene/UIScene.h"
#include "../scene/UIScene_ModalDialog.h"

class SoundEvent;

class UIController
{
public:
    UIController();
    ~UIController();

    void TickInput(InputManager *inputManager, bool inWorld);
    void Tick();
    void Render();
    void TickScenes();
    void RenderScenes();
    void UpdateSceneStackStates();

    UIScene *OpenScene(eUISCENE sceneId);
    UIScene *NavigateToScene(eUISCENE sceneId);
    void CloseScene();
    void ClearScenes();

    UIScene *GetTopScene() const;
    UIScene *FindScene(eUISCENE sceneId) const;
    bool IsSceneOpen(eUISCENE sceneId) const;
    bool IsInvitationMenuInputBlocking() const;

    void ShowModal(const UIScene_ModalDialog::Dialog &dialog);
    void CloseModal();
    bool IsModalActive() const;
    const UIScene_ModalDialog::Dialog &GetModalDialog() const;

    void ShowNotification(const std::wstring &title, const std::wstring &message, float duration);
    void OpenVirtualKeyboard(const std::wstring &text, bool masked);
    bool IsVirtualKeyboardOpen() const;
    void CloseVirtualKeyboard();
    const std::wstring &GetVirtualKeyboardText() const;

    void PlayUiSound(const ResourceLocation &location);

private:
    enum eSCENE_OPERATION_TYPE : uint8_t
    {
        eSceneOperationType_Open     = 0,
        eSceneOperationType_Navigate = 1,
        eSceneOperationType_Close    = 2,
        eSceneOperationType_Clear    = 3,
    };

    struct SceneEntry
    {
        UIScene *scene;
        bool restorePreviousVisibility;
    };

    struct PendingSceneOperation
    {
        eSCENE_OPERATION_TYPE type;
        eUISCENE sceneId;
    };

    UIScene *CreateScene(eUISCENE sceneId) const;
    void QueueSceneOperation(eSCENE_OPERATION_TYPE type, eUISCENE sceneId);
    void ApplyPendingSceneOperations();
    UIScene *OpenSceneImmediate(eUISCENE sceneId);
    UIScene *NavigateToSceneImmediate(eUISCENE sceneId);
    void CloseSceneImmediate();
    bool ShouldRenderMenuPanorama() const;
    bool IsSceneCloseQueued(eUISCENE sceneId) const;

    static const uint32_t s_uiSoundThrottleMs = 20;

    UIComponent_Panorama *m_menuPanorama;
    UIComponent_Notification *m_notification;
    UIComponent_VirtualKeyboard *m_virtualKeyboard;
    UIScene_ModalDialog::Dialog m_modalDialog;
    std::vector<SceneEntry> m_sceneStack;
    std::vector<PendingSceneOperation> m_pendingSceneOperations;
    int m_sceneStackLockDepth;
    std::unordered_map<SoundEvent *, uint32_t> m_uiSoundLastPlayedMs;
};
