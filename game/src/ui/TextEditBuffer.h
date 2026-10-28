#pragma once

#include <cstddef>
#include <string>

#include "../input/InputManager.h"

class TextEditBuffer
{
public:
    TextEditBuffer();

    void SetText(const std::wstring &text);
    const std::wstring &GetText() const;
    void SetMaxLength(size_t maxLength);
    void SetMaxUtf8Bytes(size_t maxUtf8Bytes);

    size_t GetCursor() const;
    size_t GetSelectionStart() const;
    size_t GetSelectionEnd() const;
    void MoveCursor(size_t position, bool extendSelection);

    void HandleEditKey(const InputManager::TextEditKey &editKey);
    void Insert(const std::wstring &text);
    void Replace(size_t start, size_t end, const std::wstring &text);
    void DeleteRange(size_t start, size_t end);

private:
    size_t FindWordBoundary(int direction) const;
    bool Fits(const std::wstring &text) const;

    std::wstring m_text;
    size_t m_cursor;
    size_t m_selectionAnchor;
    size_t m_maxLength;
    size_t m_maxUtf8Bytes;
};
