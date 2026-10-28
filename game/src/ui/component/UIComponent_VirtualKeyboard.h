#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../../input/ControllerGlyphs.h"
#include "../UITextureAtlas.h"
#include "UIComponent.h"
#include "UIComponent_Panel.h"

class BufferBuilder;
class Font;

class UIComponent_VirtualKeyboard : public UIComponent
{
public:
    UIComponent_VirtualKeyboard();
    ~UIComponent_VirtualKeyboard() override;

    void Open(const std::wstring &text, bool masked);
    void Close();
    bool IsOpen() const;
    const std::wstring &GetText() const;

    void Tick() override;
    void Render() override;

private:
    enum eKEY_ACTION : uint8_t
    {
        eKeyAction_Character = 0,
        eKeyAction_Symbols   = 1,
        eKeyAction_Caps      = 2,
        eKeyAction_Backspace = 3,
        eKeyAction_Space     = 4,
        eKeyAction_Up        = 5,
        eKeyAction_Down      = 6,
        eKeyAction_Left      = 7,
        eKeyAction_Right     = 8,
        eKeyAction_Enter     = 9,
    };

    enum eCAPS_STATE : uint8_t
    {
        eCapsState_Off    = 0,
        eCapsState_Once   = 1,
        eCapsState_Locked = 2,
    };

    struct Key
    {
        eKEY_ACTION action;
        wchar_t character;
        wchar_t shifted;
        wchar_t symbol;
        int units;
        eCONTROLLER_BUTTON shortcut;
    };

    static constexpr float kKeyUnitWidth        = 60.0f;
    static constexpr float kKeyHeight           = 44.0f;
    static constexpr float kKeyGap              = 6.0f;
    static constexpr float kPanelPadding        = 16.0f;
    static constexpr float kTextFieldHeight     = 40.0f;
    static constexpr float kSectionGap          = 12.0f;
    static constexpr float kHintHeight          = 20.0f;
    static constexpr float kBottomMargin        = 24.0f;
    static constexpr float kKeyTextScale        = 0.8f;
    static constexpr float kHintTextScale       = 0.55f;
    static constexpr float kKeyIconSize         = 20.0f;
    static constexpr float kHintIconSize        = 20.0f;
    static constexpr float kButtonCornerPixels  = 4.0f;
    static constexpr float kFieldBorder         = 2.0f;
    static constexpr int kRowUnits              = 10;
    static constexpr size_t kMaxTextLength      = 256;
    static constexpr uint32_t kDoubleTapMs      = 400;
    static constexpr uint32_t kCaretBlinkMs     = 500;
    static constexpr uint32_t kTextColor        = 0xFFE0E0E0;
    static constexpr uint32_t kSelectedColor    = 0xFFFFFFA0;
    static constexpr uint32_t kHintColor        = 0xFFA0A0A0;
    static constexpr uint32_t kFieldColor       = 0xFF000000;
    static constexpr uint32_t kFieldBorderColor = 0xFFA0A0A0;

    void BuildRows();
    void MoveSelection(int horizontal, int vertical);
    void PressKey(const Key &key);
    void PressCaps();
    void InsertText(const std::wstring &text);
    float GetKeyCenterUnits(int row, int column) const;
    std::wstring GetKeyLabel(const Key &key) const;
    float GetPanelWidth() const;
    float GetPanelHeight() const;

    void RenderTextField(Font *font, float x, float y, float width, float height, float uiScale);
    void RenderKey(Font *font, const Key &key, bool selected, float x, float y, float width, float height, float uiScale);
    void RenderArrow(eKEY_ACTION action, float centerX, float centerY, float size, uint32_t color);
    void DrawRect(float x, float y, float width, float height, uint32_t color);
    void DrawNineSlice(const UITextureAtlas::Region &region, float x, float y, float width, float height, float corner);
    void EmitQuad(BufferBuilder *builder, const UITextureAtlas::Region &region, float x0, float y0, float x1, float y1, float u0, float v0, float u1,
                  float v1);

    bool m_open;
    bool m_masked;
    std::wstring m_text;
    size_t m_cursor;
    bool m_symbols;
    eCAPS_STATE m_caps;
    uint32_t m_lastCapsPressMs;
    uint32_t m_openedAtMs;
    int m_selectedRow;
    int m_selectedColumn;
    std::vector<std::vector<Key>> m_rows;
    std::unique_ptr<UIComponent_Panel> m_panel;
    UITextureAtlas::Region m_buttonRegion;
    UITextureAtlas::Region m_buttonHighlightRegion;
};
