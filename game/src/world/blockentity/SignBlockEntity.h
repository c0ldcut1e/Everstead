#pragma once

#include <string>

#include "BlockEntity.h"

#include <glm/vec3.hpp>

class SignBlockEntity : public BlockEntity
{
public:
    SignBlockEntity(Level *level, const BlockPos &pos);

    struct SignText
    {
        std::string messages[4];
        int color;
        bool glowing;
    };

    const char *GetId() const override;
    void Save(CompoundTag *tag) const override;
    void Load(const CompoundTag *tag) override;

    SignText *GetText(bool front);
    bool IsWaxed() const;
    void SetWaxed(bool waxed);
    int GetEditorId() const;
    void SetEditorId(int editorId);
    bool IsFacingFrontText(const glm::dvec3 &position) const;

    static uint32_t GetTextColor(int color);

private:
    SignText m_frontText;
    SignText m_backText;
    bool m_waxed;
    int m_editorId;
};
