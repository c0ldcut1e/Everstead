#pragma once

#include <atomic>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include "../io/File.h"
#include "ModBindings.h"
#include "ModPatcher.h"
#include "ModSymbols.h"

#include <lua.hpp>

class ModLoader
{
public:
    struct SharedMod
    {
        std::string id;
        std::string version;
    };

    static ModLoader *GetInstance();

    void Load(const File &modsDirectory);
    std::vector<File> GetModDirectories() const;
    std::vector<SharedMod> GetSharedMods() const;
    void *Resolve(ModHook *hook);

private:
    enum class Side
    {
        Both,
        Client,
        Server
    };

    struct Mod
    {
        std::string id;
        std::string name;
        std::string version;
        std::vector<std::string> dependencies;
        Side side;
        File directory;
        bool loaded;
    };

    struct ThreadState
    {
        lua_State *state;
        std::vector<void *> callbacks;
        bool building;

        ~ThreadState();
    };

    enum class OrderState
    {
        Unvisited,
        Visiting,
        Ordered,
        Failed
    };

    ModLoader();

    bool ReadManifest(const File &directory, Mod *mod);
    bool OrderMod(const std::vector<Mod> &mods, size_t index, std::vector<OrderState> *states);
    void BuildState(bool primary);
    uintptr_t FindSymbol(const std::string &name, std::string *error);

    static int LuaFind(lua_State *state);
    static int LuaInstall(lua_State *state);
    static int LuaBind(lua_State *state);
    static int LuaLog(lua_State *state);
    static int LuaReadFile(lua_State *state);
    static int LuaNewString(lua_State *state);
    static int LuaFreeString(lua_State *state);
    static int LuaReadString(lua_State *state);
    static int LuaNewWideString(lua_State *state);
    static int LuaFreeWideString(lua_State *state);
    static int LuaReadWideString(lua_State *state);
    static int LuaClass(lua_State *state);
    static void PushMethod(lua_State *state, const ModMethod &method);
    static void PushParam(lua_State *state, const ModParam &param);

    std::vector<Mod> m_mods;
    std::deque<ModHook> m_hooks;
    std::unordered_map<uintptr_t, size_t> m_hookIndices;
    std::unordered_map<std::string, uintptr_t> m_resolvedSymbols;
    ModSymbols *m_symbols;
    ModPatcher m_patcher;
    std::atomic<bool> m_loading;

    static thread_local ThreadState s_threadState;
    static thread_local bool s_threadClosed;
};
