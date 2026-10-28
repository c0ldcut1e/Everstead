#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

class ModSymbols
{
public:
    bool Load(std::string *error);
    uintptr_t Find(const std::string &name, std::string *error) const;

private:
    void Add(const std::string &mangledName, uintptr_t address);
    void AddName(const std::string &name, uintptr_t address);
    std::string Normalize(const std::string &name) const;

    // Names that resolve to more than one address map to 0
    std::unordered_map<std::string, uintptr_t> m_addresses;
};
