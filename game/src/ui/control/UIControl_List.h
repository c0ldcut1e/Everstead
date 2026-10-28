#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../component/UIComponent_Panel.h"
#include "../scene/UIScene.h"
#include "UIControl.h"

class UIControl_List : public UIControl
{
public:
    enum eUI_CONTROL_TYPE : uint8_t
    {
        eUIControlType_Generic     = 0,
        eUIControlType_Button      = 1,
        eUIControlType_Checkbox    = 2,
        eUIControlType_Slider      = 3,
        eUIControlType_TextInput   = 4,
        eUIControlType_Label       = 5,
        eUIControlType_ProgressBar = 6,
        eUIControlType_Spacer      = 7,
    };

    UIControl_List(float x, float y, float width, float height, float transparency = 1.0f);
    ~UIControl_List() override;

    void Tick() override;
    void Render() override;

    virtual void Init();
    void SetupControl(UIScene *scene);

    void AddControl(UIControl *control);
    void RemoveControl(UIControl *control);
    virtual UIControl *AddElement(eUI_CONTROL_TYPE type, uint32_t id);
    virtual void ClearElements();
    void ClearList();

    UIControl *GetControlById(uint32_t id) const;
    UIControl *GetTopItem() const;
    int GetItemCount() const;
    int GetListIndex(const UIControl *control) const;

    virtual void SetCurrentSelection(int index);
    virtual int GetCurrentSelection() const;

    UIScene *GetScene() const;

    void SetPanelVisible(bool visible);
    bool IsPanelVisible() const;
    void SetPanelBoundsOffset(float x, float y, float width, float height);
    void ClearPanelBoundsOffset();

    void ResizeVertical(float padding = 24.0f) override;
    void ResizeHorizontal(float padding = 24.0f) override;

protected:
    void SyncPanel();
    Screen::Rect GetPanelActualRect(int actualWidth, int actualHeight) const;
    virtual bool ShouldProcessControl(const UIControl *control) const;

    std::vector<UIControl *> m_controls;
    std::unique_ptr<UIComponent_Panel> m_panel;
    float m_transparency;
    float m_contentPaddingX;
    float m_contentPaddingY;
    UIScene *m_scene;
    int m_currentSelection;
    bool m_hasPanelBoundsOffset;
    float m_panelOffsetX;
    float m_panelOffsetY;
    float m_panelOffsetWidth;
    float m_panelOffsetHeight;
};
