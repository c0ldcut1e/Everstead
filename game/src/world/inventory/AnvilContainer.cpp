#include "AnvilContainer.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "../../entity/player/Player.h"
#include "../../ui/ChatMessageParser.h"
#include "../fracture/FractureTraits.h"
#include "../item/Item.h"

#include <nlohmann/json.hpp>

AnvilContainer::AnvilContainer(Player *player)
{
    m_player              = player;
    m_cost                = 0;
    m_repairItemCountCost = 0;
    m_onlyRenaming        = false;
    m_taken               = false;
}

int AnvilContainer::GetColumns() const
{
    return s_slotCount;
}

int AnvilContainer::GetRows() const
{
    return 1;
}

int AnvilContainer::GetSize() const
{
    return s_slotCount;
}

ItemInstance *AnvilContainer::GetItem(int index)
{
    if (index < 0 || index >= s_slotCount)
    {
        return nullptr;
    }

    return &m_items[index];
}

const ItemInstance *AnvilContainer::GetItem(int index) const
{
    if (index < 0 || index >= s_slotCount)
    {
        return nullptr;
    }

    return &m_items[index];
}

void AnvilContainer::SetItem(int index, const ItemInstance &item)
{
    if (index < 0 || index >= s_slotCount)
    {
        return;
    }

    m_items[index] = item;
}

void AnvilContainer::ClearItem(int index)
{
    SetItem(index, ItemInstance());
}

bool AnvilContainer::IsEmpty() const
{
    for (int i = 0; i < s_slotCount; i++)
    {
        if (!m_items[i].IsEmpty())
        {
            return false;
        }
    }

    return true;
}

void AnvilContainer::SetItemName(const std::string &itemName)
{
    m_itemName = itemName;
}

void AnvilContainer::UpdateResult()
{
    const ItemInstance &input    = m_items[s_inputSlot];
    const ItemInstance &addition = m_items[s_additionSlot];
    bool creative                = m_player->GetGameType() == eGameType_Creative;
    m_onlyRenaming               = false;
    m_repairItemCountCost        = 0;
    m_cost                       = 0;
    m_items[s_resultSlot]        = ItemInstance();
    if (input.IsEmpty())
    {
        return;
    }

    Item *type          = input.GetItem();
    Item *fracturedBook = Item::ByName("fractured_book");
    ItemInstance result = input;
    int64_t tax         = (int64_t) input.GetRepairCost() + addition.GetRepairCost();
    int price           = 0;
    int namingCost      = 0;
    if (!addition.IsEmpty())
    {
        bool usingBook = addition.GetItem() == fracturedBook;
        if (type->IsDamageable() && type->IsValidRepairItem(addition.GetItem()))
        {
            int repairAmount = std::min(result.GetAuxValue(), type->GetMaxDamage() / 4);
            if (repairAmount <= 0)
            {
                return;
            }

            int count = 0;
            for (; repairAmount > 0 && count < addition.GetCount(); count++)
            {
                result.SetAuxValue(result.GetAuxValue() - repairAmount);
                price++;
                repairAmount = std::min(result.GetAuxValue(), type->GetMaxDamage() / 4);
            }

            m_repairItemCountCost = count;
        }
        else
        {
            if (!usingBook && (addition.GetItem() != type || !type->IsDamageable()))
            {
                return;
            }

            if (type->IsDamageable() && !usingBook)
            {
                int remaining    = type->GetMaxDamage() * 2 - input.GetAuxValue() - addition.GetAuxValue() + type->GetMaxDamage() * 12 / 100;
                int resultDamage = std::max(0, type->GetMaxDamage() - remaining);
                if (resultDamage < result.GetAuxValue())
                {
                    result.SetAuxValue(resultDamage);
                    price += 2;
                }
            }

            bool anyCompatible                     = false;
            bool anyIncompatible                   = false;
            std::vector<FractureTrait *> fractures = FractureTraits::Get(addition);
            for (size_t i = 0; i < fractures.size(); i++)
            {
                std::vector<FractureTrait *> current = FractureTraits::Get(result);
                bool present    = std::find(current.begin(), current.end(), fractures[i]) != current.end();
                bool compatible = (fractures[i]->GetGearTypes() & FractureTraits::GetGearType(type)) || creative || type == fracturedBook;
                if (!present && FractureTraits::GetCount(result) >= FractureTraits::kMaxFractures)
                {
                    compatible = false;
                    price++;
                }

                if (!compatible)
                {
                    anyIncompatible = true;
                    continue;
                }

                anyCompatible = true;
                if (!present)
                {
                    FractureTraits::Apply(&result, fractures[i]);
                }

                price += usingBook ? 1 : 2;
                if (input.GetCount() > 1)
                {
                    price = s_tooExpensive;
                }
            }

            if (anyIncompatible && !anyCompatible)
            {
                return;
            }
        }
    }

    if (m_itemName.find_first_not_of(" \t\r\n") != std::string::npos)
    {
        if (!input.HasCustomName() ||
            ChatMessageParser().ToPlainText(nlohmann::json::parse(input.GetCustomName(), nullptr, false)) != m_itemName)
        {
            namingCost = 1;
            price++;
            result.SetCustomName(nlohmann::json {{"text", m_itemName}}.dump());
        }
    }
    else if (input.HasCustomName())
    {
        namingCost = 1;
        price++;
        result.SetCustomName("");
    }

    m_cost = (int) std::clamp(tax + price, (int64_t) 0, (int64_t) INT32_MAX);
    if (price <= 0)
    {
        return;
    }

    if (namingCost == price)
    {
        m_cost         = std::min(m_cost, s_tooExpensive - 1);
        m_onlyRenaming = true;
    }

    if (m_cost >= s_tooExpensive && !creative)
    {
        return;
    }

    int repairCost = std::max(result.GetRepairCost(), addition.GetRepairCost());
    if (!m_onlyRenaming)
    {
        repairCost = repairCost * 2 + 1;
    }

    result.SetRepairCost(repairCost);
    m_items[s_resultSlot] = result;
}

bool AnvilContainer::MayPickup() const
{
    return (m_player->GetGameType() == eGameType_Creative || m_player->GetExperienceLevel() >= m_cost) && m_cost > 0;
}

void AnvilContainer::OnTake()
{
    ItemInstance &addition = m_items[s_additionSlot];
    if (m_repairItemCountCost > 0)
    {
        addition.SetCount(addition.GetCount() - m_repairItemCountCost);
        if (addition.GetCount() <= 0)
        {
            addition = ItemInstance();
        }
    }
    else if (!m_onlyRenaming)
    {
        addition = ItemInstance();
    }

    m_items[s_inputSlot] = ItemInstance();
    m_taken              = true;
    UpdateResult();
}

bool AnvilContainer::PopTaken()
{
    bool taken = m_taken;
    m_taken    = false;
    return taken;
}

int AnvilContainer::GetCost() const
{
    return m_cost;
}

void AnvilContainer::SetCost(int cost)
{
    m_cost = cost;
}
