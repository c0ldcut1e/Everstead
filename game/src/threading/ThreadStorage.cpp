#include "ThreadStorage.h"

#include <memory>

static thread_local std::unique_ptr<ThreadStorage::Storage> s_threadStorage;

ThreadStorage::Storage *ThreadStorage::CreateNewThreadStorage()
{
    s_threadStorage = std::make_unique<Storage>();
    return s_threadStorage.get();
}

ThreadStorage::Storage *ThreadStorage::UseDefaultThreadStorage()
{
    if (!s_threadStorage)
    {
        s_threadStorage = std::make_unique<Storage>();
    }

    return s_threadStorage.get();
}

void ThreadStorage::ReleaseThreadStorage()
{
    s_threadStorage.reset();
}

ThreadStorage::Storage *ThreadStorage::GetStorage()
{
    return s_threadStorage.get();
}

void ThreadStorage::SetValue(const std::string &key, uintptr_t value)
{
    Storage *storage     = UseDefaultThreadStorage();
    storage->values[key] = value;
}

uintptr_t ThreadStorage::GetValue(const std::string &key, uintptr_t defaultValue)
{
    Storage *storage = UseDefaultThreadStorage();
    auto it          = storage->values.find(key);
    if (it == storage->values.end())
    {
        return defaultValue;
    }

    return it->second;
}
