#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

class ThreadStorage
{
public:
    struct Storage
    {
        std::unordered_map<std::string, uintptr_t> values;
    };

    static Storage *CreateNewThreadStorage();
    static Storage *UseDefaultThreadStorage();
    static void ReleaseThreadStorage();

    static Storage *GetStorage();

    static void SetValue(const std::string &key, uintptr_t value);
    static uintptr_t GetValue(const std::string &key, uintptr_t defaultValue = 0);
};
