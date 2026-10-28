#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../core/ResourceLocation.h"
#include "../../rendering/Icon.h"

#include <glm/vec3.hpp>

class Block;
class Level;
class Direction;
struct BlockPos;

enum eUSE_ANIM : uint8_t
{
    eUseAnim_None  = 0,
    eUseAnim_Eat   = 1,
    eUseAnim_Block = 2,
    eUseAnim_Drink = 3,
    eUseAnim_Bow   = 4,
};

class Item
{
public:
    Item();
    virtual ~Item() = default;

    virtual Block *GetBlock() const;
    virtual Block *GetLightBlock() const;

    virtual bool CanUseOn(Level *level, const BlockPos &pos, Direction *face) const;
    virtual void UseOn(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &look) const;
    virtual void SpawnUseEffects(Level *level, const BlockPos &pos) const;
    virtual eUSE_ANIM GetUseAnimation() const;
    virtual int GetUseDuration() const;
    virtual float GetDestroySpeed(Block *block) const;
    virtual bool CanHarvest(Block *block) const;
    virtual int GetMineDamage(Block *block) const;
    virtual int GetHitDamage() const;
    virtual int GetArmorSlot() const;

    static Item *ById(uint32_t id);
    static Item *ByName(const ResourceLocation &location);

    uint32_t GetId() const;
    void SetId(uint32_t id);

    virtual std::string GetDescriptionId() const;
    const ResourceLocation &GetResourceLocation() const;

    void SetMaxStackSize(int maxStackSize);
    int GetMaxStackSize() const;

    void SetMaxDamage(int maxDamage);
    int GetMaxDamage() const;
    bool IsDamageable() const;
    void SetRepairItems(const std::vector<ResourceLocation> &repairItems);
    bool IsValidRepairItem(const Item *item) const;

    void SetAttackDamage(int attackDamage);
    int GetAttackDamage() const;

    void SetHandEquipped(bool handEquipped);
    bool IsHandEquipped() const;

    void SetIconPath(const std::string &iconPath);
    const std::string &GetIconPath() const;
    void SetIcon(const Icon &icon);
    const Icon &GetIcon() const;

protected:
    uint32_t m_id;
    int m_maxStackSize;
    int m_maxDamage;
    int m_attackDamage;
    std::vector<ResourceLocation> m_repairItems;
    bool m_handEquipped;
    std::string m_iconPath;
    Icon m_icon;
};
