#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../core/ResourceLocation.h"

template<typename T>
class MappedRegistry
{
public:
    uint32_t Register(const ResourceLocation &key, T value)
    {
        uint32_t id        = (uint32_t) m_values.size();
        m_keyToId[key]     = id;
        m_valueToId[value] = id;
        m_values.push_back(value);
        m_keys.push_back(key);
        return id;
    }

    T ById(uint32_t id) const
    {
        if (id >= (uint32_t) m_values.size())
        {
            return (T) nullptr;
        }

        return m_values[id];
    }

    T Get(const ResourceLocation &key) const
    {
        auto it = m_keyToId.find(key);
        if (it == m_keyToId.end())
        {
            return (T) nullptr;
        }

        return m_values[it->second];
    }

    uint32_t IdOf(T value) const
    {
        auto it = m_valueToId.find(value);
        if (it != m_valueToId.end())
        {
            return it->second;
        }

        return 0;
    }

    const ResourceLocation &GetKey(uint32_t id) const
    {
        return m_keys[id];
    }

    bool Has(const ResourceLocation &key) const
    {
        return m_keyToId.find(key) != m_keyToId.end();
    }

    uint32_t Size() const
    {
        return (uint32_t) m_values.size();
    }

private:
    std::unordered_map<ResourceLocation, uint32_t, ResourceLocationHash> m_keyToId;
    std::unordered_map<T, uint32_t> m_valueToId;
    std::vector<T> m_values;
    std::vector<ResourceLocation> m_keys;
};
