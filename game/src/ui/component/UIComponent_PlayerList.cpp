#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_PlayerList.h"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <string>
#include <vector>

#include "../../core/Everstead.h"
#include "../../entity/player/MultiPlayerLocalPlayer.h"
#include "../../input/GameplayAction.h"
#include "../../input/InputManager.h"
#include "../../network/listener/ClientConnection.h"
#include "../../rendering/BufferBuilder.h"
#include "../../rendering/Font.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../utils/StringUtils.h"
#include "../../world/MultiPlayerLevel.h"
#include "../ChatMessageParser.h"
#include "../Gui.h"
#include "../Screen.h"

UIComponent_PlayerList::UIComponent_PlayerList() : UIComponent()
{}

void UIComponent_PlayerList::Render()
{
    Everstead *everstead = Everstead::GetInstance();
    if (!everstead->GetInputManager()->IsActionDown(eGameplayAction_PlayerList) || everstead->IsWorldMenuOpen() || everstead->GetGui()->IsChatOpen())
    {
        return;
    }

    ClientConnection *clientConnection         = everstead->GetLocalPlayer()->GetClientConnection();
    std::vector<ClientPlayerListEntry> entries = clientConnection->GetPlayerList();
    Scoreboard &scoreboard                     = everstead->GetLevel()->GetScoreboard();
    std::sort(entries.begin(), entries.end(),
              [&scoreboard](const ClientPlayerListEntry &left, const ClientPlayerListEntry &right)
              {
                  PlayerTeam *leftTeam  = scoreboard.GetMemberTeam(StringUtils::ToUtf8(left.name));
                  PlayerTeam *rightTeam = scoreboard.GetMemberTeam(StringUtils::ToUtf8(right.name));
                  std::string leftName  = leftTeam ? leftTeam->name : "";
                  std::string rightName = rightTeam ? rightTeam->name : "";
                  return leftName != rightName ? leftName < rightName : StringUtils::ToLower(left.name) < StringUtils::ToLower(right.name);
              });

    float screenWidth  = (float) everstead->GetWidth();
    float screenHeight = (float) everstead->GetHeight();
    float uiScale      = Screen::ScaleUniform(everstead->GetWidth(), everstead->GetHeight());
    Font *font         = everstead->GetDefaultFont();

    float margin    = floorf(16.0f * uiScale);
    float padding   = floorf(10.0f * uiScale);
    float lineGap   = floorf(4.0f * uiScale);
    float spacing   = std::max(1.0f, floorf(2.0f * uiScale));
    float cellPadX  = floorf(4.0f * uiScale);
    float cellPadY  = std::max(1.0f, floorf(1.0f * uiScale));
    float pingGap   = floorf(10.0f * uiScale);
    float maxPanelW = screenWidth - margin * 2.0f;
    float maxPanelH = screenHeight - margin * 2.0f;

    float titleScale = font->SnapScale(0.8f * uiScale);
    float infoScale  = font->SnapScale(0.55f * uiScale);

    std::wstring title     = StringUtils::ToUtf16(clientConnection->GetWorldName());
    std::wstring worldLine = L"";
    if (!clientConnection->GetServerName().empty())
    {
        worldLine = title;
        title     = StringUtils::ToUtf16(clientConnection->GetServerName());
    }

    int localPing = -1;
    for (size_t i = 0; i < entries.size(); i++)
    {
        if (entries[i].playerIndex == clientConnection->GetPlayerIndex())
        {
            localPing = entries[i].ping;
        }
    }

    wchar_t buffer[64];
    swprintf(buffer, 64, L"%d/%d", (int) entries.size(), clientConnection->GetMaxPlayers());
    std::wstring info = everstead->GetString("player_list.players") + L" " + buffer;
    swprintf(buffer, 64, L"%.1f", clientConnection->GetTps());
    info += L"    " + everstead->GetString("player_list.tps") + L" " + buffer;
    swprintf(buffer, 64, L"%.1f", clientConnection->GetMspt());
    info += L"    " + everstead->GetString("player_list.mspt") + L" " + buffer;
    swprintf(buffer, 64, localPing < 0 ? L"-" : L"%d ms", localPing);
    info += L"    " + everstead->GetString("player_list.ping") + L" " + buffer;

    float infoLineHeight = floorf(font->GetLineHeight(infoScale));
    float headerHeight   = floorf(font->GetLineHeight(titleScale)) + lineGap + infoLineHeight;
    if (!worldLine.empty())
    {
        headerHeight += infoLineHeight + lineGap;
    }
    float gridAvailW = maxPanelW - padding * 2.0f;
    float gridAvailH = maxPanelH - padding * 4.0f - headerHeight - 1.0f;

    Gui *gui = everstead->GetGui();
    ChatMessageParser parser;
    std::vector<std::vector<ChatTextSegment>> names(entries.size());
    std::vector<float> nameWidths(entries.size());
    for (size_t i = 0; i < entries.size(); i++)
    {
        nlohmann::json name = entries[i].name.empty() ? nlohmann::json{{"translate", "friends.unknown_player"}}
                                                      : nlohmann::json{{"text", StringUtils::ToUtf8(entries[i].name)}};
        PlayerTeam *team    = scoreboard.GetMemberTeam(StringUtils::ToUtf8(entries[i].name));
        nlohmann::json line = {{"text", ""}, {"extra", {team ? team->FormatName(name) : name}}};
        if (entries[i].playerIndex == 0)
        {
            line["extra"].push_back(" (" + StringUtils::ToUtf8(everstead->GetString("world.join.host")) + ")");
        }

        parser.Parse(line.dump(), ChatTextStyle(), &names[i]);
    }

    // We shrink the text until every column fits, so large servers stay on screen without scrolling
    constexpr float kCellScales[] = {0.55f, 0.45f, 0.38f, 0.32f, 0.27f};
    int count                     = (int) entries.size();
    float scale                   = 0.0f;
    float cellWidth               = 0.0f;
    float cellHeight              = 0.0f;
    float pingWidth               = 0.0f;
    int rows                      = 1;
    int columns                   = 1;
    for (size_t s = 0; s < std::size(kCellScales); s++)
    {
        scale           = font->SnapScale(kCellScales[s] * uiScale);
        pingWidth       = font->GetWidth(L"9999 ms", scale);
        float nameWidth = 0.0f;
        for (size_t i = 0; i < names.size(); i++)
        {
            nameWidths[i] = 0.0f;
            for (size_t j = 0; j < names[i].size(); j++)
            {
                nameWidths[i] += gui->GetTextWidth(font, names[i][j], scale);
            }

            nameWidth = std::max(nameWidth, nameWidths[i]);
        }

        cellWidth  = cellPadX * 2.0f + nameWidth + pingGap + pingWidth;
        cellHeight = floorf(font->GetLineHeight(scale)) + cellPadY * 2.0f;
        rows       = std::max(1, (int) ((gridAvailH + spacing) / (cellHeight + spacing)));
        columns    = (count + rows - 1) / rows;
        if ((float) columns * cellWidth + (float) (columns - 1) * spacing <= gridAvailW)
        {
            break;
        }
    }

    rows      = (count + columns - 1) / columns;
    cellWidth = std::min(cellWidth, floorf((gridAvailW - (float) (columns - 1) * spacing) / (float) columns));

    float nameAvail     = cellWidth - cellPadX * 2.0f - pingGap - pingWidth;
    float ellipsisWidth = font->GetWidth(L"...", scale);
    for (size_t i = 0; i < names.size(); i++)
    {
        if (nameWidths[i] <= nameAvail)
        {
            continue;
        }

        std::vector<ChatTextSegment> &segments = names[i];
        float width                            = nameWidths[i];
        while (!segments.empty() && width + ellipsisWidth > nameAvail)
        {
            width -= gui->GetTextWidth(font, segments.back(), scale);
            if (segments.back().text.size() <= 1)
            {
                segments.pop_back();
                continue;
            }

            segments.back().text.pop_back();
            width += gui->GetTextWidth(font, segments.back(), scale);
        }

        segments.push_back({L"...", segments.empty() ? ChatTextStyle() : segments.back().style});
    }

    float gridWidth  = (float) columns * cellWidth + (float) (columns - 1) * spacing;
    float gridHeight = (float) rows * cellHeight + (float) (rows - 1) * spacing;
    float contentW   = std::max({gridWidth, font->GetWidth(title, titleScale), font->GetWidth(info, infoScale), font->GetWidth(worldLine, infoScale),
                                 floorf(260.0f * uiScale)});
    float panelW     = std::min(contentW + padding * 2.0f, maxPanelW);
    float panelH     = padding * 4.0f + headerHeight + 1.0f + gridHeight;
    float panelX     = floorf((screenWidth - panelW) * 0.5f);
    float panelY     = margin;
    float separatorY = panelY + padding * 2.0f + headerHeight;
    float gridX      = floorf(panelX + (panelW - gridWidth) * 0.5f);
    float gridY      = separatorY + 1.0f + padding;

    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::DisableTexture();

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    DrawRect(builder, panelX, panelY, panelW, panelH, 0x90000000);
    DrawRect(builder, panelX + padding, separatorY, panelW - padding * 2.0f, 1.0f, 0x60FFFFFF);
    for (int i = 0; i < count; i++)
    {
        float cellX = gridX + (float) (i / rows) * (cellWidth + spacing);
        float cellY = gridY + (float) (i % rows) * (cellHeight + spacing);
        DrawRect(builder, cellX, cellY, cellWidth, cellHeight,
                 entries[(size_t) i].playerIndex == clientConnection->GetPlayerIndex() ? 0x40FFFFFF : 0x20FFFFFF);
    }

    builder->End();

    builder->Begin(ePrimitiveType_LineLoop);
    builder->Color(0x50FFFFFF);
    builder->Vertex(panelX, panelY, 0.0f);
    builder->Vertex(panelX + panelW, panelY, 0.0f);
    builder->Vertex(panelX + panelW, panelY + panelH, 0.0f);
    builder->Vertex(panelX, panelY + panelH, 0.0f);
    builder->End();

    GlStateManager::EnableTexture();
    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();

    float titleY = panelY + padding;
    font->DrawShadow(title, floorf(panelX + (panelW - font->GetWidth(title, titleScale)) * 0.5f), titleY + font->GetAscent(titleScale), titleScale,
                     0xFFFFFFFF);
    float infoY = titleY + floorf(font->GetLineHeight(titleScale)) + lineGap;
    if (!worldLine.empty())
    {
        font->DrawShadow(worldLine, floorf(panelX + (panelW - font->GetWidth(worldLine, infoScale)) * 0.5f), infoY + font->GetAscent(infoScale),
                         infoScale, 0xFFA0A0A0);
        infoY += infoLineHeight + lineGap;
    }

    font->DrawShadow(info, floorf(panelX + (panelW - font->GetWidth(info, infoScale)) * 0.5f), infoY + font->GetAscent(infoScale), infoScale,
                     0xFFC8C8C8);

    for (int i = 0; i < count; i++)
    {
        float cellX = gridX + (float) (i / rows) * (cellWidth + spacing);
        float textY = gridY + (float) (i % rows) * (cellHeight + spacing) + cellPadY + font->GetAscent(scale);
        float nameX = cellX + cellPadX;
        for (size_t j = 0; j < names[(size_t) i].size(); j++)
        {
            gui->DrawTextSegment(font, names[(size_t) i][j], nameX, textY, scale, 1.0f, true);
            nameX += gui->GetTextWidth(font, names[(size_t) i][j], scale);
        }

        int ping           = entries[(size_t) i].ping;
        uint32_t pingColor = ping < 0 ? 0xFF808080 : ping <= 100 ? 0xFF55FF55 : ping <= 250 ? 0xFFFFFF55 : 0xFFFF5555;
        swprintf(buffer, 64, ping < 0 ? L"-" : L"%d ms", ping);
        font->DrawShadow(buffer, cellX + cellWidth - cellPadX - font->GetWidth(buffer, scale), textY, scale, pingColor);
    }
}

void UIComponent_PlayerList::DrawRect(BufferBuilder *builder, float x, float y, float width, float height, uint32_t argb)
{
    builder->Color(argb);
    builder->Vertex(x, y, 0.0f);
    builder->Vertex(x + width, y, 0.0f);
    builder->Vertex(x, y + height, 0.0f);
    builder->Vertex(x + width, y, 0.0f);
    builder->Vertex(x + width, y + height, 0.0f);
    builder->Vertex(x, y + height, 0.0f);
}

#endif
