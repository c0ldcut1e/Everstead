#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_VirtualKeyboard.h"

#include <algorithm>
#include <cmath>

#include "../../core/Everstead.h"
#include "../../input/InputManager.h"
#include "../../rendering/Font.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../../utils/Time.h"
#include "../Screen.h"
#include "../UIControllerGlyphRenderer.h"

class UIPanelProperties_VirtualKeyboard : public UIPanelProperties
{
public:
    UIPanelProperties_VirtualKeyboard() : UIPanelProperties(0.0f, 0.0f, 0.0f, 0.0f)
    {}

    const char *GetTexturePath() const override
    {
        return "assets/texture/ui/control/panel9grid.png";
    }
};

UIComponent_VirtualKeyboard::UIComponent_VirtualKeyboard() : UIComponent()
{
    m_open                  = false;
    m_masked                = false;
    m_text                  = L"";
    m_cursor                = 0;
    m_symbols               = false;
    m_caps                  = eCapsState_Off;
    m_lastCapsPressMs       = 0;
    m_openedAtMs            = 0;
    m_selectedRow           = 1;
    m_selectedColumn        = 0;
    m_rows                  = {};
    m_panel                 = std::make_unique<UIComponent_Panel>(std::make_unique<UIPanelProperties_VirtualKeyboard>());
    m_buttonRegion          = UITextureAtlas::GetRegion("assets/texture/ui/control/mainmenubutton.png");
    m_buttonHighlightRegion = UITextureAtlas::GetRegion("assets/texture/ui/control/mainmenubutton_highlight.png");
    BuildRows();
}

UIComponent_VirtualKeyboard::~UIComponent_VirtualKeyboard()
{}

void UIComponent_VirtualKeyboard::BuildRows()
{
    static const wchar_t *characters[] = {L"1234567890", L"qwertyuiop", L"asdfghjkl'", L"zxcvbnm,.?"};
    static const wchar_t *shifted[]    = {L"1234567890", L"QWERTYUIOP", L"ASDFGHJKL\"", L"ZXCVBNM;:!"};
    static const wchar_t *symbols[]    = {L"1234567890", L"!@#$%^&*()", L"-_=+[]{}\\|", L";:\"<>/`~,."};
    for (int row = 0; row < 4; row++)
    {
        std::vector<Key> keys;
        for (int column = 0; column < kRowUnits; column++)
        {
            keys.push_back({eKeyAction_Character, characters[row][column], shifted[row][column], symbols[row][column], 1, eControllerButton_Count});
        }

        m_rows.push_back(keys);
    }

    m_rows.push_back({{eKeyAction_Symbols, 0, 0, 0, 2, eControllerButton_Count},
                      {eKeyAction_Caps, 0, 0, 0, 2, eControllerButton_RightTrigger},
                      {eKeyAction_Backspace, 0, 0, 0, 3, eControllerButton_West},
                      {eKeyAction_Space, 0, 0, 0, 3, eControllerButton_North}});
    m_rows.push_back({{eKeyAction_Up, 0, 0, 0, 1, eControllerButton_Count},
                      {eKeyAction_Down, 0, 0, 0, 1, eControllerButton_Count},
                      {eKeyAction_Left, 0, 0, 0, 1, eControllerButton_Count},
                      {eKeyAction_Right, 0, 0, 0, 1, eControllerButton_Count},
                      {eKeyAction_Enter, 0, 0, 0, 6, eControllerButton_Start}});
}

void UIComponent_VirtualKeyboard::Open(const std::wstring &text, bool masked)
{
    if (m_open)
    {
        return;
    }

    m_open           = true;
    m_masked         = masked;
    m_text           = text;
    m_cursor         = text.size();
    m_symbols        = false;
    m_caps           = eCapsState_Off;
    m_selectedRow    = 1;
    m_selectedColumn = 0;
    m_openedAtMs     = Time::GetMilliseconds();
    Everstead::GetInstance()->GetInputManager()->SetVirtualKeyboardActive(true);
}

void UIComponent_VirtualKeyboard::Close()
{
    if (!m_open)
    {
        return;
    }

    m_open = false;
    Everstead::GetInstance()->GetInputManager()->SetVirtualKeyboardActive(false);
}

bool UIComponent_VirtualKeyboard::IsOpen() const
{
    return m_open;
}

const std::wstring &UIComponent_VirtualKeyboard::GetText() const
{
    return m_text;
}

void UIComponent_VirtualKeyboard::Tick()
{
    if (!m_open)
    {
        return;
    }

    Everstead *everstead       = Everstead::GetInstance();
    InputManager *inputManager = everstead->GetInputManager();
    if (!inputManager->IsUiTextInputFocused())
    {
        Close();
        return;
    }

    int horizontal = 0;
    int vertical   = 0;
    inputManager->ConsumeVirtualKeyboardNavigation(&horizontal, &vertical);
    MoveSelection(horizontal, vertical);

    if (inputManager->ConsumeControllerButtonPress(eControllerButton_South))
    {
        PressKey(m_rows[(size_t) m_selectedRow][(size_t) m_selectedColumn]);
    }

    for (size_t row = 0; row < m_rows.size(); row++)
    {
        for (size_t column = 0; column < m_rows[row].size(); column++)
        {
            const Key &key = m_rows[row][column];
            if (key.shortcut != eControllerButton_Count && inputManager->ConsumeControllerButtonPress(key.shortcut))
            {
                PressKey(key);
            }
        }
    }

    if (m_open && inputManager->ConsumeControllerButtonPress(eControllerButton_East))
    {
        if (everstead->GetGui()->IsChatOpen())
        {
            inputManager->InjectUiTextCancel();
        }

        Close();
    }
}

void UIComponent_VirtualKeyboard::MoveSelection(int horizontal, int vertical)
{
    if (vertical != 0)
    {
        float center       = GetKeyCenterUnits(m_selectedRow, m_selectedColumn);
        m_selectedRow      = std::clamp(m_selectedRow + vertical, 0, (int) m_rows.size() - 1);
        m_selectedColumn   = 0;
        float bestDistance = 1000.0f;
        for (int column = 0; column < (int) m_rows[(size_t) m_selectedRow].size(); column++)
        {
            float distance = fabsf(GetKeyCenterUnits(m_selectedRow, column) - center);
            if (distance < bestDistance)
            {
                bestDistance     = distance;
                m_selectedColumn = column;
            }
        }
    }

    int columnCount  = (int) m_rows[(size_t) m_selectedRow].size();
    m_selectedColumn = ((m_selectedColumn + horizontal) % columnCount + columnCount) % columnCount;
}

float UIComponent_VirtualKeyboard::GetKeyCenterUnits(int row, int column) const
{
    int start = 0;
    for (int i = 0; i < column; i++)
    {
        start += m_rows[(size_t) row][(size_t) i].units;
    }

    return (float) start + (float) m_rows[(size_t) row][(size_t) column].units * 0.5f;
}

void UIComponent_VirtualKeyboard::PressKey(const Key &key)
{
    Everstead *everstead       = Everstead::GetInstance();
    InputManager *inputManager = everstead->GetInputManager();
    switch (key.action)
    {
        case eKeyAction_Character:
            InsertText(GetKeyLabel(key));
            if (m_caps == eCapsState_Once && !m_symbols)
            {
                m_caps = eCapsState_Off;
            }

            break;
        case eKeyAction_Symbols:
            m_symbols = !m_symbols;
            break;
        case eKeyAction_Caps:
            PressCaps();
            break;
        case eKeyAction_Backspace:
            if (m_cursor > 0)
            {
                m_text.erase(m_cursor - 1, 1);
                m_cursor--;
            }

            break;
        case eKeyAction_Space:
            InsertText(L" ");
            break;
        case eKeyAction_Up:
            m_cursor = 0;
            break;
        case eKeyAction_Down:
            m_cursor = m_text.size();
            break;
        case eKeyAction_Left:
            m_cursor = m_cursor > 0 ? m_cursor - 1 : 0;
            break;
        case eKeyAction_Right:
            m_cursor = std::min(m_cursor + 1, m_text.size());
            break;
        case eKeyAction_Enter:
            inputManager->SetUiTextReplacement(m_text);
            if (everstead->GetGui()->IsChatOpen())
            {
                inputManager->InjectUiTextSubmit();
            }

            Close();
            break;
    }
}

void UIComponent_VirtualKeyboard::InsertText(const std::wstring &text)
{
    if (m_text.size() + text.size() > kMaxTextLength)
    {
        return;
    }

    m_text.insert(m_cursor, text);
    m_cursor += text.size();
}

void UIComponent_VirtualKeyboard::PressCaps()
{
    uint32_t now = Time::GetMilliseconds();
    if (m_caps == eCapsState_Off)
    {
        m_caps = eCapsState_Once;
    }
    else if (m_caps == eCapsState_Once && now - m_lastCapsPressMs <= kDoubleTapMs)
    {
        m_caps = eCapsState_Locked;
    }
    else
    {
        m_caps = eCapsState_Off;
    }

    m_lastCapsPressMs = now;
}

std::wstring UIComponent_VirtualKeyboard::GetKeyLabel(const Key &key) const
{
    switch (key.action)
    {
        case eKeyAction_Character:
            if (m_symbols)
            {
                return std::wstring(1, key.symbol);
            }

            return std::wstring(1, m_caps != eCapsState_Off ? key.shifted : key.character);
        case eKeyAction_Symbols:
            return m_symbols ? L"abc" : L"@#:";
        case eKeyAction_Caps:
            return m_caps != eCapsState_Off ? L"A/a" : L"a/A";
        case eKeyAction_Backspace:
            return Everstead::GetInstance()->GetString("keyboard.backspace");
        case eKeyAction_Space:
            return Everstead::GetInstance()->GetString("keyboard.space");
        case eKeyAction_Enter:
            return Everstead::GetInstance()->GetString("keyboard.enter");
        default:
            return L"";
    }
}

float UIComponent_VirtualKeyboard::GetPanelWidth() const
{
    return kKeyUnitWidth * kRowUnits + kKeyGap * (kRowUnits - 1) + kPanelPadding * 2.0f;
}

float UIComponent_VirtualKeyboard::GetPanelHeight() const
{
    float rows = (float) m_rows.size();
    return kPanelPadding * 2.0f + kTextFieldHeight + kSectionGap + rows * kKeyHeight + (rows - 1.0f) * kKeyGap + kSectionGap + kHintHeight;
}

void UIComponent_VirtualKeyboard::Render()
{
    if (!m_open)
    {
        return;
    }

    Everstead *everstead = Everstead::GetInstance();
    Font *font           = everstead->GetDefaultFont();
    int actualWidth      = everstead->GetWidth();
    int actualHeight     = everstead->GetHeight();
    float uiScale        = Screen::ScaleUniform(actualWidth, actualHeight);

    float panelWidth  = GetPanelWidth();
    float panelHeight = GetPanelHeight();
    float panelX      = (Screen::WIDTH - panelWidth) * 0.5f;
    float panelY      = Screen::HEIGHT - panelHeight - kBottomMargin;

    UIPanelProperties *properties = m_panel->GetProperties();
    properties->x                 = panelX;
    properties->y                 = panelY;
    properties->width             = panelWidth;
    properties->height            = panelHeight;
    m_panel->Render();

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    font->SetNearest(true);

    float contentX = panelX + kPanelPadding;
    float contentY = panelY + kPanelPadding;
    RenderTextField(font, Screen::ToActualX(contentX, actualWidth, actualHeight), Screen::ToActualY(contentY, actualWidth, actualHeight),
                    Screen::ToActualLength(panelWidth - kPanelPadding * 2.0f, actualWidth, actualHeight),
                    Screen::ToActualLength(kTextFieldHeight, actualWidth, actualHeight), uiScale);

    float rowY = contentY + kTextFieldHeight + kSectionGap;
    for (size_t row = 0; row < m_rows.size(); row++)
    {
        float keyX = contentX;
        for (size_t column = 0; column < m_rows[row].size(); column++)
        {
            const Key &key = m_rows[row][column];
            float width    = kKeyUnitWidth * (float) key.units + kKeyGap * (float) (key.units - 1);
            bool selected  = (int) row == m_selectedRow && (int) column == m_selectedColumn;
            RenderKey(font, key, selected, Screen::ToActualX(keyX, actualWidth, actualHeight), Screen::ToActualY(rowY, actualWidth, actualHeight),
                      Screen::ToActualLength(width, actualWidth, actualHeight), Screen::ToActualLength(kKeyHeight, actualWidth, actualHeight),
                      uiScale);
            keyX += width + kKeyGap;
        }

        rowY += kKeyHeight + kKeyGap;
    }

    float hintScale         = font->SnapScale(uiScale * kHintTextScale);
    float iconSize          = Screen::ToActualLength(kHintIconSize, actualWidth, actualHeight);
    float spacing           = roundf(6.0f * uiScale);
    float hintCenterY       = Screen::ToActualY(rowY - kKeyGap + kSectionGap + kHintHeight * 0.5f, actualWidth, actualHeight);
    std::wstring selectText = Everstead::GetInstance()->GetString("keyboard.select");
    std::wstring closeText  = Everstead::GetInstance()->GetString("keyboard.close");
    float hintWidth         = iconSize * 2.0f + spacing * 4.0f + font->GetWidth(selectText, hintScale) + font->GetWidth(closeText, hintScale);
    float hintX             = floorf(Screen::ToActualX(Screen::WIDTH * 0.5f, actualWidth, actualHeight) - hintWidth * 0.5f);
    float baseline          = floorf(hintCenterY + font->GetAscent(hintScale) * 0.5f);
    hintX += UIControllerGlyphRenderer::Render(eControllerButton_South, hintX, floorf(hintCenterY - iconSize * 0.5f), iconSize) + spacing;
    font->DrawShadow(selectText, hintX, baseline, hintScale, kHintColor);
    hintX += font->GetWidth(selectText, hintScale) + spacing * 2.0f;
    hintX += UIControllerGlyphRenderer::Render(eControllerButton_East, hintX, floorf(hintCenterY - iconSize * 0.5f), iconSize) + spacing;
    font->DrawShadow(closeText, hintX, baseline, hintScale, kHintColor);
}

void UIComponent_VirtualKeyboard::RenderTextField(Font *font, float x, float y, float width, float height, float uiScale)
{
    float border = std::max(1.0f, roundf(kFieldBorder * uiScale));
    DrawRect(x, y, width, height, kFieldBorderColor);
    DrawRect(x + border, y + border, width - border * 2.0f, height - border * 2.0f, kFieldColor);

    std::wstring text = m_masked ? std::wstring(m_text.size(), L'*') : m_text;
    size_t cursor     = m_cursor;
    float scale       = font->SnapScale(uiScale * kKeyTextScale);
    float padding     = roundf(8.0f * uiScale);
    float available   = width - padding * 2.0f;

    size_t start = 0;
    while (start < cursor && font->GetWidth(std::wstring_view(text).substr(start, cursor - start), scale) > available)
    {
        start++;
    }

    size_t end = cursor;
    while (end < text.size() && font->GetWidth(std::wstring_view(text).substr(start, end + 1 - start), scale) <= available)
    {
        end++;
    }

    float baseline = floorf(y + height * 0.5f + font->GetAscent(scale) * 0.5f);
    font->DrawShadow(std::wstring_view(text).substr(start, end - start), floorf(x + padding), baseline, scale, kTextColor);

    if (((Time::GetMilliseconds() - m_openedAtMs) / kCaretBlinkMs) % 2 == 0)
    {
        float caretX = floorf(x + padding + font->GetWidth(std::wstring_view(text).substr(start, cursor - start), scale));
        font->DrawShadow(cursor >= text.size() ? L"_" : L"|", caretX, baseline, scale, kTextColor);
    }
}

void UIComponent_VirtualKeyboard::RenderKey(Font *font, const Key &key, bool selected, float x, float y, float width, float height, float uiScale)
{
    DrawNineSlice(selected ? m_buttonHighlightRegion : m_buttonRegion, x, y, width, height, roundf(kButtonCornerPixels * uiScale));

    uint32_t color = selected ? kSelectedColor : kTextColor;
    if (key.action == eKeyAction_Caps && m_caps != eCapsState_Off)
    {
        color = kSelectedColor;
    }

    if (key.action == eKeyAction_Symbols && m_symbols)
    {
        color = kSelectedColor;
    }

    float centerX = x + width * 0.5f;
    float centerY = y + height * 0.5f;
    if (key.action >= eKeyAction_Up && key.action <= eKeyAction_Right)
    {
        RenderArrow(key.action, centerX, centerY, height * 0.18f, color);
    }
    else
    {
        std::wstring label = GetKeyLabel(key);
        float scale        = font->SnapScale(uiScale * kKeyTextScale);
        float iconSize     = key.shortcut != eControllerButton_Count ? roundf(kKeyIconSize * uiScale) : 0.0f;
        float iconSpacing  = key.shortcut != eControllerButton_Count ? roundf(6.0f * uiScale) : 0.0f;
        float labelX       = floorf(centerX - (iconSize + iconSpacing + font->GetWidth(label, scale)) * 0.5f);
        if (key.shortcut != eControllerButton_Count)
        {
            labelX += UIControllerGlyphRenderer::Render(key.shortcut, labelX, floorf(centerY - iconSize * 0.5f), iconSize) + iconSpacing;
        }

        font->DrawShadow(label, labelX, floorf(centerY + font->GetAscent(scale) * 0.5f), scale, color);
    }

    if (key.action == eKeyAction_Caps && m_caps == eCapsState_Locked)
    {
        float indicatorWidth = width * 0.3f;
        float indicator      = std::max(1.0f, roundf(2.0f * uiScale));
        DrawRect(floorf(centerX - indicatorWidth * 0.5f), floorf(y + height - indicator * 4.0f), floorf(indicatorWidth), indicator, kSelectedColor);
    }
}

void UIComponent_VirtualKeyboard::RenderArrow(eKEY_ACTION action, float centerX, float centerY, float size, uint32_t color)
{
    GlStateManager::DisableDepthTest();
    float directions[4][2] = {{0.0f, -1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f}, {1.0f, 0.0f}};
    float dx               = directions[action - eKeyAction_Up][0];
    float dy               = directions[action - eKeyAction_Up][1];

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    for (int pass = 0; pass < 2; pass++)
    {
        float offset = pass == 0 ? std::max(1.0f, roundf(size * 0.2f)) : 0.0f;
        builder->Begin(ePrimitiveType_Triangles);
        builder->Color(pass == 0 ? 0xFF3F3F3F : color);
        builder->Vertex(centerX + dx * size + offset, centerY + dy * size + offset, 0.0f);
        builder->Vertex(centerX - dx * size * 0.6f + dy * size + offset, centerY - dy * size * 0.6f - dx * size + offset, 0.0f);
        builder->Vertex(centerX - dx * size * 0.6f - dy * size + offset, centerY - dy * size * 0.6f + dx * size + offset, 0.0f);
        builder->End();
    }
}

void UIComponent_VirtualKeyboard::DrawRect(float x, float y, float width, float height, uint32_t color)
{
    GlStateManager::DisableDepthTest();
    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    builder->Color(color);
    builder->Vertex(x, y + height, 0.0f);
    builder->Vertex(x + width, y + height, 0.0f);
    builder->Vertex(x + width, y, 0.0f);
    builder->Vertex(x, y + height, 0.0f);
    builder->Vertex(x + width, y, 0.0f);
    builder->Vertex(x, y, 0.0f);
    builder->End();
}

void UIComponent_VirtualKeyboard::DrawNineSlice(const UITextureAtlas::Region &region, float x, float y, float width, float height, float corner)
{
    GlStateManager::DisableDepthTest();
    float cornerU = kButtonCornerPixels / (float) region.pixelWidth;
    float cornerV = kButtonCornerPixels / (float) region.pixelHeight;
    float xs[4]   = {x, x + corner, x + width - corner, x + width};
    float ys[4]   = {y, y + corner, y + height - corner, y + height};
    float us[4]   = {0.0f, cornerU, 1.0f - cornerU, 1.0f};
    float vs[4]   = {0.0f, cornerV, 1.0f - cornerV, 1.0f};

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    for (int row = 0; row < 3; row++)
    {
        for (int column = 0; column < 3; column++)
        {
            EmitQuad(builder, region, xs[column], ys[row], xs[column + 1], ys[row + 1], us[column], vs[row], us[column + 1], vs[row + 1]);
        }
    }

    builder->End();
    Textures::Unbind();
}

void UIComponent_VirtualKeyboard::EmitQuad(BufferBuilder *builder, const UITextureAtlas::Region &region, float x0, float y0, float x1, float y1,
                                           float u0, float v0, float u1, float v1)
{
    builder->Color(0xFFFFFFFF);
    builder->VertexUV(x0, y0, 0.0f, UITextureAtlas::GetU(region, u0), UITextureAtlas::GetV(region, v0));
    builder->VertexUV(x1, y0, 0.0f, UITextureAtlas::GetU(region, u1), UITextureAtlas::GetV(region, v0));
    builder->VertexUV(x1, y1, 0.0f, UITextureAtlas::GetU(region, u1), UITextureAtlas::GetV(region, v1));
    builder->VertexUV(x0, y0, 0.0f, UITextureAtlas::GetU(region, u0), UITextureAtlas::GetV(region, v0));
    builder->VertexUV(x1, y1, 0.0f, UITextureAtlas::GetU(region, u1), UITextureAtlas::GetV(region, v1));
    builder->VertexUV(x0, y1, 0.0f, UITextureAtlas::GetU(region, u0), UITextureAtlas::GetV(region, v1));
}

#endif
