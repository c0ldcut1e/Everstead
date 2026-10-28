#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum eCHAT_CLICK_ACTION
{
    eChatClickAction_None,
    eChatClickAction_CopyToClipboard,
    eChatClickAction_OpenFile,
};

struct ChatTextStyle
{
    uint32_t color                 = 0xFFFFFFFF;
    bool bold                      = false;
    bool italic                    = false;
    bool underlined                = false;
    bool strikethrough             = false;
    bool obfuscated                = false;
    eCHAT_CLICK_ACTION clickAction = eChatClickAction_None;
    std::string clickValue;
};

struct ChatTextSegment
{
    std::wstring text;
    ChatTextStyle style;
};
