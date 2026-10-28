#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

struct ModHook
{
    void *original;
    size_t index;
};

class ModPatcher
{
public:
    ModPatcher();

    bool Patch(uintptr_t target, ModHook *hook, std::string *error);

private:
    bool AllocatePool(uintptr_t target, std::string *error);
    void ProtectPool(bool writable);

    static constexpr size_t kPoolSize = 64 * 1024;

    uint8_t *m_pool;
    size_t m_poolUsed;
};
