#include "SignBlockEntity.h"

#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

#include "../../tag/CompoundTag.h"
#include "../../tag/ListTag.h"
#include "../../tag/StringTag.h"
#include "../../utils/math/Mth.h"
#include "../Level.h"
#include "../block/SignBlock.h"

constexpr const char *kColorNames[] = {"white",      "orange", "magenta", "light_blue", "yellow", "lime",  "pink", "gray",
                                       "light_gray", "cyan",   "purple",  "blue",       "brown",  "green", "red",  "black"};

SignBlockEntity::SignBlockEntity(Level *level, const BlockPos &pos) : BlockEntity(level, pos)
{
    m_frontText.color   = 15;
    m_frontText.glowing = false;
    m_backText.color    = 15;
    m_backText.glowing  = false;
    m_waxed             = false;
    m_editorId          = -1;
}

const char *SignBlockEntity::GetId() const
{
    return "sign";
}

void SignBlockEntity::Save(CompoundTag *tag) const
{
    BlockEntity::Save(tag);
    const char *sideNames[] = {"front_text", "back_text"};
    const SignText *texts[] = {&m_frontText, &m_backText};
    for (size_t i = 0; i < std::size(texts); i++)
    {
        std::unique_ptr<ListTag> messages(new ListTag());
        for (const std::string &message : texts[i]->messages)
        {
            messages->Add(std::make_unique<StringTag>(message));
        }

        std::unique_ptr<CompoundTag> side(new CompoundTag());
        side->Put("messages", std::move(messages));
        side->PutString("color", kColorNames[texts[i]->color]);
        side->PutBoolean("has_glowing_text", texts[i]->glowing);
        tag->Put(sideNames[i], std::move(side));
    }

    tag->PutBoolean("is_waxed", m_waxed);
}

void SignBlockEntity::Load(const CompoundTag *tag)
{
    const char *sideNames[] = {"front_text", "back_text"};
    SignText *texts[]       = {&m_frontText, &m_backText};
    for (size_t i = 0; i < std::size(texts); i++)
    {
        const CompoundTag *side = tag->GetCompound(sideNames[i]);
        if (!side)
        {
            continue;
        }

        ListTag *messages = side->GetList("messages");
        for (size_t j = 0; messages && j < messages->Size() && j < 4; j++)
        {
            texts[i]->messages[j] = ((const StringTag *) messages->Get(j))->GetValue();
        }

        std::string color = side->GetString("color");
        for (size_t j = 0; j < std::size(kColorNames); j++)
        {
            if (color == kColorNames[j])
            {
                texts[i]->color = (int) j;
            }
        }

        texts[i]->glowing = side->GetBoolean("has_glowing_text");
    }

    m_waxed = tag->GetBoolean("is_waxed");
}

SignBlockEntity::SignText *SignBlockEntity::GetText(bool front)
{
    return front ? &m_frontText : &m_backText;
}

bool SignBlockEntity::IsWaxed() const
{
    return m_waxed;
}

void SignBlockEntity::SetWaxed(bool waxed)
{
    m_waxed = waxed;
}

int SignBlockEntity::GetEditorId() const
{
    return m_editorId;
}

void SignBlockEntity::SetEditorId(int editorId)
{
    m_editorId = editorId;
}

bool SignBlockEntity::IsFacingFrontText(const glm::dvec3 &position) const
{
    SignBlock *block = (SignBlock *) Block::ById(m_level->GetBlockId(m_pos));
    uint16_t data    = m_level->GetBlockData(m_pos);
    std::vector<AABB> boxes;
    block->GetBoxes(data, &boxes);
    glm::dvec3 center = glm::dvec3(m_pos.x, m_pos.y, m_pos.z) + (boxes[0].GetMin() + boxes[0].GetMax()) * 0.5;
    float yaw         = (float) (atan2(position.z - center.z, position.x - center.x) * 180.0 / std::numbers::pi) - 90.0f;
    return fabsf(Mth::WrapDegrees(block->GetYRot(data) - yaw)) <= 90.0f;
}

uint32_t SignBlockEntity::GetTextColor(int color)
{
    constexpr uint32_t kTextColors[] = {0xFFFFFF, 0xFF681F, 0xFF00FF, 0x9AC0CD, 0xFFFF00, 0xBFFF00, 0xFF69B4, 0x808080,
                                        0xD3D3D3, 0x00FFFF, 0xA020F0, 0x0000FF, 0x8B4513, 0x00FF00, 0xFF0000, 0x000000};
    return kTextColors[color];
}
