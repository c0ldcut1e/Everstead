#include "ModLoader.h"

#include <cstring>

#include "../core/GameInfo.h"
#include "../core/Logger.h"
#include "../utils/StringUtils.h"

#include <nlohmann/json.hpp>

thread_local ModLoader::ThreadState ModLoader::s_threadState;
thread_local bool ModLoader::s_threadClosed;

extern "C" void *ModHookResolve(ModHook *hook)
{
    return ModLoader::GetInstance()->Resolve(hook);
}

ModLoader *ModLoader::GetInstance()
{
    static ModLoader instance;
    return &instance;
}

ModLoader::ModLoader()
{
    m_symbols = nullptr;
    m_loading = false;
}

ModLoader::ThreadState::~ThreadState()
{
    s_threadClosed = true;
    if (state)
    {
        lua_close(state);
    }
}

void ModLoader::Load(const File &modsDirectory)
{
    modsDirectory.CreateDirectories();

    std::vector<Mod> mods;
    std::vector<File> files = modsDirectory.ListFiles();
    for (size_t i = 0; i < files.size(); i++)
    {
        Mod mod;
        if (!files[i].IsDirectory() || !ReadManifest(files[i], &mod))
        {
            continue;
        }

#ifdef EVERSTEAD_DEDICATED_SERVER
        if (mod.side == Side::Client)
        {
            Logger::LogInfo("Skipping %s because it only runs on clients", mod.id.c_str());
            continue;
        }
#endif

        bool duplicate = false;
        for (size_t j = 0; j < mods.size(); j++)
        {
            duplicate |= mods[j].id == mod.id;
        }

        if (duplicate)
        {
            Logger::LogError("Skipping %s because another mod already uses the id %s", files[i].GetPath().c_str(), mod.id.c_str());
            continue;
        }

        mods.push_back(mod);
    }

    if (mods.empty())
    {
        Logger::LogInfo("No mods found in %s", modsDirectory.GetPath().c_str());
        return;
    }

    std::vector<OrderState> states(mods.size(), OrderState::Unvisited);
    for (size_t i = 0; i < mods.size(); i++)
    {
        OrderMod(mods, i, &states);
    }

    std::string error;
    m_symbols = new ModSymbols();
    if (!m_symbols->Load(&error))
    {
        Logger::LogError("Mods are disabled because of an error: %s", error.c_str());
        m_mods.clear();
    }
    else
    {
        m_loading = true;
        BuildState(true);
        m_loading = false;
    }

    // The full symbol table is large, so after loading only the names mods asked for stay around
    delete m_symbols;
    m_symbols = nullptr;

    size_t loaded = 0;
    for (size_t i = 0; i < m_mods.size(); i++)
    {
        loaded += m_mods[i].loaded;
    }

    Logger::LogInfo("Loaded %zu of %zu mods with %zu hooks", loaded, mods.size(), m_hooks.size());
}

std::vector<File> ModLoader::GetModDirectories() const
{
    std::vector<File> directories;
    for (size_t i = 0; i < m_mods.size(); i++)
    {
        if (m_mods[i].loaded)
        {
            directories.push_back(m_mods[i].directory);
        }
    }

    return directories;
}

std::vector<ModLoader::SharedMod> ModLoader::GetSharedMods() const
{
    std::vector<SharedMod> mods;
    for (size_t i = 0; i < m_mods.size(); i++)
    {
        if (m_mods[i].loaded && m_mods[i].side == Side::Both)
        {
            mods.push_back({m_mods[i].id, m_mods[i].version});
        }
    }

    return mods;
}

void *ModLoader::Resolve(ModHook *hook)
{
    // Hooked functions run unmodified while this thread is building its Lua state or after it was torn down
    if (m_loading || s_threadClosed || s_threadState.building)
    {
        return hook->original;
    }

    if (!s_threadState.state)
    {
        BuildState(false);
    }

    if (hook->index >= s_threadState.callbacks.size() || !s_threadState.callbacks[hook->index])
    {
        return hook->original;
    }

    return s_threadState.callbacks[hook->index];
}

bool ModLoader::ReadManifest(const File &directory, Mod *mod)
{
    // Folders without a mod.json are skipped quietly because the editor definitions live in one
    File manifest = directory.GetChild("mod.json");
    if (!manifest.IsRegularFile())
    {
        return false;
    }

    if (!directory.GetChild("main.lua").IsRegularFile())
    {
        Logger::LogWarn("Skipping %s because it has no main.lua", directory.GetPath().c_str());
        return false;
    }

    std::vector<std::string> gameVersions;
    std::string side;
    try
    {
        nlohmann::json json = nlohmann::json::parse(manifest.ReadAllText());
        mod->id             = json.at("id").get<std::string>();
        mod->name           = json.value("name", mod->id);
        mod->version        = json.value("version", "0.0.0");
        mod->dependencies   = json.value("dependencies", std::vector<std::string>());
        gameVersions        = json.at("gameVersions").get<std::vector<std::string>>();
        side                = json.value("side", "both");
    }
    catch (const nlohmann::json::exception &exception)
    {
        Logger::LogError("Skipping %s because its mod.json is invalid: %s", directory.GetPath().c_str(), exception.what());
        return false;
    }

    if (mod->id.empty())
    {
        Logger::LogError("Skipping %s because its mod.json has an empty id", directory.GetPath().c_str());
        return false;
    }

    if (side == "both")
    {
        mod->side = Side::Both;
    }
    else if (side == "client")
    {
        mod->side = Side::Client;
    }
    else if (side == "server")
    {
        mod->side = Side::Server;
    }
    else
    {
        Logger::LogError("Skipping %s because its side %s is not both, client or server", mod->id.c_str(), side.c_str());
        return false;
    }

    // An entry like "0.2" matches every patch of that version, so mods only pin as tightly as they need
    std::string version = std::to_string(g_gameVersionMajor) + "." + std::to_string(g_gameVersionMinor) + "." + std::to_string(g_gameVersionPatch);
    bool compatible     = false;
    for (size_t i = 0; i < gameVersions.size(); i++)
    {
        compatible |= version == gameVersions[i] || version.starts_with(gameVersions[i] + ".");
    }

    if (!compatible)
    {
        Logger::LogError("Skipping %s because it does not support game version %s", mod->id.c_str(), version.c_str());
        return false;
    }

    mod->directory = directory;
    mod->loaded    = false;
    return true;
}

bool ModLoader::OrderMod(const std::vector<Mod> &mods, size_t index, std::vector<OrderState> *states)
{
    const Mod &mod = mods[index];
    switch ((*states)[index])
    {
        case OrderState::Ordered:
            return true;
        case OrderState::Failed:
            return false;
        case OrderState::Visiting:
            Logger::LogError("Mod %s is part of a dependency cycle", mod.id.c_str());
            (*states)[index] = OrderState::Failed;
            return false;
        case OrderState::Unvisited:
            break;
    }

    (*states)[index] = OrderState::Visiting;
    for (size_t i = 0; i < mod.dependencies.size(); i++)
    {
        size_t dependency = mods.size();
        for (size_t j = 0; j < mods.size(); j++)
        {
            if (mods[j].id == mod.dependencies[i])
            {
                dependency = j;
            }
        }

        if (dependency == mods.size())
        {
            Logger::LogError("Mod %s needs %s, which is not installed", mod.id.c_str(), mod.dependencies[i].c_str());
            (*states)[index] = OrderState::Failed;
            return false;
        }

        if (!OrderMod(mods, dependency, states))
        {
            Logger::LogError("Mod %s needs %s, which could not be loaded", mod.id.c_str(), mod.dependencies[i].c_str());
            (*states)[index] = OrderState::Failed;
            return false;
        }
    }

    m_mods.push_back(mod);
    (*states)[index] = OrderState::Ordered;
    return true;
}

void ModLoader::BuildState(bool primary)
{
    // Every thread that runs a hooked function gets its own state that reruns each main.lua, and only the main
    // thread's first run is allowed to patch new hooks
    constexpr char kBootstrap[] = R"lua(
local api = ...
local ffi = require("ffi")
local chains = {}
local loading = false

local function pointerType(signature)
    local open = signature:find("(", 1, true)
    if not open then
        error("A signature has to look like \"int(void*, float)\", got " .. signature, 3)
    end
    return ffi.typeof(signature:sub(1, open - 1) .. "(*)" .. signature:sub(open))
end

local function address(target)
    local value, message = api.find(target)
    if not value then
        error(message, 3)
    end
    return value
end

local function owned(pointer, free)
    return ffi.gc(ffi.cast("void*", pointer), function(value)
        free(tonumber(ffi.cast("uintptr_t", value)))
    end)
end

local function pointerNumber(pointer)
    return tonumber(ffi.cast("uintptr_t", pointer))
end

local std = {
    string = function(text)
        return owned(api.newString(text), api.freeString)
    end,
    wstring = function(text)
        return owned(api.newWideString(text), api.freeWideString)
    end,
    read = function(pointer)
        return api.readString(pointerNumber(pointer))
    end,
    readWide = function(pointer)
        return api.readWideString(pointerNumber(pointer))
    end,
}

local classes = {}
local classesByType = {}
local loadClass

local function abiType(param)
    if param.kind == "v" then
        return "void"
    elseif param.kind == "n" then
        return param.type
    elseif param.kind == "b" then
        return "bool"
    elseif param.kind == "c" then
        return "const char*"
    end
    return "void*"
end

local function destroyer(address)
    return ffi.cast("void(*)(void*)", address)
end

local function toLua(param, value)
    local kind = param.kind
    if kind == "n" then
        return tonumber(value)
    elseif kind == "v" or kind == "b" or value == nil then
        return value
    elseif kind == "c" then
        return ffi.string(value)
    elseif kind == "s" then
        return std.read(value)
    elseif kind == "w" then
        return std.readWide(value)
    elseif kind == "S" or kind == "W" then
        local text = kind == "S" and std.read(value) or std.readWide(value)
        destroyer(param.destroy)(value)
        return text
    end

    local class = param.type and loadClass(param.type)
    if class then
        value = ffi.cast(class.pointer, value)
    end
    if kind == "O" and param.destroy ~= 0 then
        value = ffi.gc(value, destroyer(param.destroy))
    end
    return value
end

-- Strings fit string parameters exactly, while classes built from a string are only a fallback
local function score(param, value)
    local kind = param.kind
    local valueType = type(value)
    if kind == "n" then
        return (valueType == "number" or valueType == "cdata") and 2 or nil
    elseif kind == "b" then
        return valueType == "boolean" and 2 or nil
    elseif valueType == "string" then
        if kind == "c" or kind == "s" or kind == "S" or kind == "w" or kind == "W" then
            return 2
        end
        return (kind == "o" or kind == "O") and param.type and 1 or nil
    end
    return (valueType == "cdata" or valueType == "nil") and kind ~= "c" and 2 or nil
end

local function choose(overloads, label, skipSelf, ...)
    local count = select("#", ...)
    local best, bestScore, bestSelf
    for i = 1, #overloads do
        local overload = overloads[i]
        local offset = (not skipSelf and not overload.static) and 1 or 0
        if overload.thunk ~= 0 and overload.count == count - offset then
            local total = 0
            for j = 1, overload.count do
                local value = score(overload.params[j], (select(j + offset, ...)))
                if not value then
                    total = nil
                    break
                end
                total = total + value
            end
            if total and (not bestScore or total > bestScore) then
                best, bestScore, bestSelf = overload, total, offset == 1
            end
        end
    end
    if not best then
        error("No overload of " .. label .. " takes " .. count .. " arguments of these types", 3)
    end
    return best, bestSelf
end

local construct

local function toC(param, value, keep)
    if type(value) ~= "string" then
        return value
    end

    local kind = param.kind
    if kind == "s" or kind == "S" then
        value = std.string(value)
    elseif kind == "w" or kind == "W" then
        value = std.wstring(value)
    elseif kind == "o" or kind == "O" then
        local class = loadClass(param.type)
        value = ffi.gc(construct(class, value), destroyer(class.data.destroy))
    end
    keep[#keep + 1] = value
    return value
end

local function invoke(overload, self, ...)
    if not overload.call then
        local types = {}
        if not overload.static then
            types[1] = "void*"
        end
        for i = 1, overload.count do
            types[#types + 1] = abiType(overload.params[i])
        end
        overload.call = ffi.cast(abiType(overload.result) .. "(*)(" .. (#types > 0 and table.concat(types, ", ") or "void") .. ")",
            overload.thunk)
    end

    -- Converted temporaries stay referenced in keep until the call returns so the game never sees freed strings
    local keep = {}
    local arguments = {}
    for i = 1, overload.count do
        arguments[i] = toC(overload.params[i], (select(i, ...)), keep)
    end
    local result
    if overload.static then
        result = overload.call(unpack(arguments, 1, overload.count))
    else
        result = overload.call(self, unpack(arguments, 1, overload.count))
    end
    keep = nil
    return toLua(overload.result, result)
end

construct = function(class, ...)
    local overloads = class.data.methods.new
    if not overloads then
        error(class.name .. " has no constructor", 3)
    end
    return invoke(choose(overloads, class.name .. " constructor", true, ...), nil, ...)
end

local function findMethod(class, name)
    local overloads = class.data.methods[name]
    if overloads or name == "new" then
        return overloads
    end
    for i = 1, #class.bases do
        local base = loadClass(class.bases[i])
        local found = base and findMethod(base, name)
        if found then
            return found
        end
    end
end

local function findField(class, name)
    local field = class.data.fields[name]
    if field then
        return field
    end
    for i = 1, #class.bases do
        local base = loadClass(class.bases[i])
        local found = base and findField(base, name)
        if found then
            return found
        end
    end
end

local function method(class, name, skipSelf)
    local cache = skipSelf and class.instanceMethods or class.staticMethods
    local dispatcher = cache[name]
    if dispatcher then
        return dispatcher
    end

    local overloads = findMethod(class, name)
    if not overloads then
        error(class.name .. " has no method or field named " .. tostring(name), 2)
    end

    local label = class.name .. "::" .. name
    if skipSelf then
        dispatcher = function(self, ...)
            return invoke(choose(overloads, label, true, ...), self, ...)
        end
    else
        dispatcher = function(...)
            local overload, hasSelf = choose(overloads, label, false, ...)
            if hasSelf then
                return invoke(overload, ...)
            end
            return invoke(overload, nil, ...)
        end
    end
    cache[name] = dispatcher
    return dispatcher
end

local function setField(class, name, self, value)
    local field = findField(class, name)
    if not field or field.set.static ~= (self == nil) then
        error(class.name .. " has no " .. (self and "" or "static ") .. "field named " .. tostring(name), 3)
    elseif field.set.thunk == 0 then
        error(class.name .. "::" .. name .. " can't be assigned", 3)
    end
    invoke(field.set, self, value)
end

loadClass = function(name)
    local class = classes[name]
    if class ~= nil then
        return class or nil
    end

    local data = api.class(name)
    if not data then
        classes[name] = false
        return nil
    end

    ffi.cdef("struct " .. name .. ";")
    class = { name = name, data = data, pointer = ffi.typeof("struct " .. name .. "*"), bases = {}, instanceMethods = {},
        staticMethods = {} }
    for base in data.bases:gmatch("[^,]+") do
        class.bases[#class.bases + 1] = base
    end
    ffi.metatype("struct " .. name, {
        __index = function(self, key)
            local field = findField(class, key)
            if field and not field.get.static then
                return invoke(field.get, self)
            end
            return method(class, key, true)
        end,
        __newindex = function(self, key, value)
            setField(class, key, self, value)
        end,
    })
    class.global = setmetatable({}, {
        __index = function(_, key)
            local field = findField(class, key)
            if field and field.get.static then
                return invoke(field.get, nil)
            end
            return method(class, key, false)
        end,
        __newindex = function(_, key, value)
            setField(class, key, nil, value)
        end,
    })
    classes[name] = class
    classesByType[tonumber(class.pointer)] = class
    return class
end

local function hookSignature(target)
    local className, methodName = target:match("^([%w_]+)::([%w_]+)$")
    local class = className and loadClass(className)
    local constructor = className == methodName
    local overloads = class and class.data.methods[constructor and "new" or methodName]
    if not overloads then
        error(target .. " is not a known method, so pass its signature", 3)
    elseif #overloads > 1 then
        error(target .. " has more than one overload, so pass its signature", 3)
    end

    local overload = overloads[1]
    local function rawType(param)
        if param.kind == "O" or param.kind == "S" or param.kind == "W" then
            error(target .. " passes a class by value, so pass its signature", 4)
        elseif param.reference and (param.kind == "n" or param.kind == "b") then
            return "void*"
        end
        return abiType(param)
    end

    local types = {}
    local params = {}
    if not overload.static or constructor then
        types[1] = "void*"
        params[1] = { kind = "o", type = className }
    end
    for i = 1, overload.count do
        types[#types + 1] = rawType(overload.params[i])
        params[#params + 1] = overload.params[i]
    end
    return (constructor and "void" or rawType(overload.result)) .. "(" .. (#types > 0 and table.concat(types, ", ") or "void") .. ")", params
end

local function typedArguments(params, ...)
    local arguments = { ... }
    for i = 1, #params do
        local class = params[i].kind == "o" and params[i].type and loadClass(params[i].type)
        if class and arguments[i] ~= nil then
            arguments[i] = ffi.cast(class.pointer, arguments[i])
        end
    end
    return unpack(arguments, 1, select("#", ...))
end

local function createEnvironment(id, name, version, directory)
    local environment = setmetatable({}, {
        __index = function(_, key)
            local value = _G[key]
            if value == nil and type(key) == "string" then
                local class = loadClass(key)
                return class and class.global
            end
            return value
        end,
    })
    local modules = {}

    local function format(...)
        local parts = {}
        for i = 1, select("#", ...) do
            parts[i] = tostring((select(i, ...)))
        end
        return "[" .. id .. "] " .. table.concat(parts, " ")
    end

    environment.ffi = ffi
    environment.std = std
    environment.mod = { id = id, name = name, version = version, directory = directory }
    environment.log = {
        info = function(...) api.log(0, format(...)) end,
        warn = function(...) api.log(1, format(...)) end,
        error = function(...) api.log(2, format(...)) end,
    }

    environment.hook = function(target, signature, handler)
        if not loading then
            error("Hooks can only be added while mods load", 2)
        end

        if handler == nil then
            local params
            local untyped = signature
            signature, params = hookSignature(target)
            handler = function(original, ...)
                return untyped(original, typedArguments(params, ...))
            end
        end

        local index, original = api.install(target)
        if not index then
            error(original, 2)
        end

        local chain = chains[index]
        if not chain then
            local pointer = pointerType(signature)
            chain = { target = target, signature = signature, original = ffi.cast(pointer, original) }
            chain.call = chain.original
            chain.callback = ffi.cast(pointer, function(...)
                local ok, result = pcall(chain.call, ...)
                if ok then
                    return result
                end
                api.log(2, "Hook on " .. chain.target .. " failed: " .. tostring(result))
                return chain.original(...)
            end)
            api.bind(index, pointerNumber(chain.callback))
            chains[index] = chain
        elseif chain.signature ~= signature then
            error(target .. " is already hooked with the signature " .. chain.signature, 2)
        end

        local inner = chain.call
        chain.call = function(...)
            return handler(inner, ...)
        end
    end

    environment.func = function(target, signature)
        return ffi.cast(pointerType(signature), address(target))
    end

    environment.symbol = function(target)
        return ffi.cast("void*", address(target))
    end

    environment.new = function(name, ...)
        local class = loadClass(name)
        if not class then
            error("There is no class named " .. tostring(name), 2)
        end
        return construct(class, ...)
    end

    environment.delete = function(object)
        local class = classesByType[tonumber(ffi.typeof(object))]
        if not class or class.data.destroy == 0 then
            error("Only objects made with new can be deleted", 2)
        end
        ffi.gc(object, nil)
        destroyer(class.data.destroy)(object)
    end

    environment.require = function(module)
        if modules[module] ~= nil then
            return modules[module]
        end

        local path = directory .. "/" .. module:gsub("%.", "/") .. ".lua"
        local source = api.readFile(path)
        if not source then
            return require(module)
        end

        local chunk = assert(loadstring(source, "@" .. path))
        setfenv(chunk, environment)
        local result = chunk(module)
        if result == nil then
            result = true
        end
        modules[module] = result
        return result
    end

    return environment
end

return function(id, name, version, directory)
    local environment = createEnvironment(id, name, version, directory)
    loading = true
    local ok, message = pcall(environment.require, "main")
    loading = false
    if not ok then
        error(message, 0)
    end
end
)lua";

    s_threadState.building = true;
    lua_State *state       = luaL_newstate();
    luaL_openlibs(state);
    // The JIT is off because nested FFI callbacks abort with "bad callback" from inside a trace
    luaJIT_setmode(state, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF);
    luaL_loadbuffer(state, kBootstrap, sizeof(kBootstrap) - 1, "=modloader");

    const luaL_Reg functions[] = {
            {"find", LuaFind},
            {"install", LuaInstall},
            {"bind", LuaBind},
            {"log", LuaLog},
            {"readFile", LuaReadFile},
            {"newString", LuaNewString},
            {"freeString", LuaFreeString},
            {"readString", LuaReadString},
            {"newWideString", LuaNewWideString},
            {"freeWideString", LuaFreeWideString},
            {"readWideString", LuaReadWideString},
            {"class", LuaClass},
            {nullptr, nullptr},
    };
    lua_newtable(state);
    luaL_register(state, nullptr, functions);
    lua_call(state, 1, 1);

    for (size_t i = 0; i < m_mods.size(); i++)
    {
        Mod &mod = m_mods[i];
        if (!primary && !mod.loaded)
        {
            continue;
        }

        lua_pushvalue(state, -1);
        lua_pushstring(state, mod.id.c_str());
        lua_pushstring(state, mod.name.c_str());
        lua_pushstring(state, mod.version.c_str());
        lua_pushstring(state, mod.directory.GetPath().c_str());
        if (lua_pcall(state, 4, 0, 0) != 0)
        {
            Logger::LogError("Mod %s failed to load: %s", mod.id.c_str(), lua_tostring(state, -1));
            lua_pop(state, 1);
        }
        else if (primary)
        {
            mod.loaded = true;
            Logger::LogInfo("Loaded mod %s %s", mod.id.c_str(), mod.version.c_str());
        }
    }

    lua_pop(state, 1);
    s_threadState.state    = state;
    s_threadState.building = false;
}

uintptr_t ModLoader::FindSymbol(const std::string &name, std::string *error)
{
    auto it = m_resolvedSymbols.find(name);
    if (it != m_resolvedSymbols.end())
    {
        return it->second;
    }

    if (!m_symbols)
    {
        *error = name + " was not looked up while mods loaded, so look it up at the top of the file instead";
        return 0;
    }

    uintptr_t address = m_symbols->Find(name, error);
    if (address)
    {
        m_resolvedSymbols[name] = address;
    }

    return address;
}

int ModLoader::LuaFind(lua_State *state)
{
    std::string error;
    uintptr_t address = GetInstance()->FindSymbol(luaL_checkstring(state, 1), &error);
    if (!address)
    {
        lua_pushnil(state);
        lua_pushstring(state, error.c_str());
        return 2;
    }

    lua_pushnumber(state, (lua_Number) address);
    return 1;
}

int ModLoader::LuaInstall(lua_State *state)
{
    ModLoader *loader = GetInstance();
    std::string error;
    uintptr_t address = loader->FindSymbol(luaL_checkstring(state, 1), &error);
    if (address)
    {
        auto it = loader->m_hookIndices.find(address);
        if (it != loader->m_hookIndices.end())
        {
            lua_pushnumber(state, (lua_Number) it->second);
            lua_pushnumber(state, (lua_Number) (uintptr_t) loader->m_hooks[it->second].original);
            return 2;
        }

        if (!loader->m_loading)
        {
            error = "Hooks can only be added while mods load";
        }
        else
        {
            loader->m_hooks.push_back({nullptr, loader->m_hooks.size()});
            if (loader->m_patcher.Patch(address, &loader->m_hooks.back(), &error))
            {
                loader->m_hookIndices[address] = loader->m_hooks.size() - 1;
                lua_pushnumber(state, (lua_Number) (loader->m_hooks.size() - 1));
                lua_pushnumber(state, (lua_Number) (uintptr_t) loader->m_hooks.back().original);
                return 2;
            }

            loader->m_hooks.pop_back();
        }
    }

    lua_pushnil(state);
    lua_pushstring(state, error.c_str());
    return 2;
}

int ModLoader::LuaBind(lua_State *state)
{
    size_t index                   = (size_t) luaL_checknumber(state, 1);
    std::vector<void *> &callbacks = s_threadState.callbacks;
    if (callbacks.size() <= index)
    {
        callbacks.resize(index + 1);
    }

    callbacks[index] = (void *) (uintptr_t) luaL_checknumber(state, 2);
    return 0;
}

int ModLoader::LuaLog(lua_State *state)
{
    // Other threads rerun every main.lua when they build their state, so we drop their load time messages
    if (s_threadState.building && !GetInstance()->m_loading)
    {
        return 0;
    }

    int level           = (int) luaL_checknumber(state, 1);
    const char *message = luaL_checkstring(state, 2);
    if (level == 0)
    {
        Logger::LogInfo("%s", message);
    }
    else if (level == 1)
    {
        Logger::LogWarn("%s", message);
    }
    else
    {
        Logger::LogError("%s", message);
    }

    return 0;
}

int ModLoader::LuaReadFile(lua_State *state)
{
    File file(luaL_checkstring(state, 1));
    if (!file.IsRegularFile())
    {
        return 0;
    }

    std::string text = file.ReadAllText();
    lua_pushlstring(state, text.data(), text.size());
    return 1;
}

int ModLoader::LuaNewString(lua_State *state)
{
    size_t length;
    const char *text = luaL_checklstring(state, 1, &length);
    lua_pushnumber(state, (lua_Number) (uintptr_t) new std::string(text, length));
    return 1;
}

int ModLoader::LuaFreeString(lua_State *state)
{
    delete (std::string *) (uintptr_t) luaL_checknumber(state, 1);
    return 0;
}

int ModLoader::LuaReadString(lua_State *state)
{
    const std::string *text = (const std::string *) (uintptr_t) luaL_checknumber(state, 1);
    lua_pushlstring(state, text->data(), text->size());
    return 1;
}

int ModLoader::LuaNewWideString(lua_State *state)
{
    size_t length;
    const char *text = luaL_checklstring(state, 1, &length);
    lua_pushnumber(state, (lua_Number) (uintptr_t) new std::wstring(StringUtils::ToUtf16(std::string(text, length))));
    return 1;
}

int ModLoader::LuaFreeWideString(lua_State *state)
{
    delete (std::wstring *) (uintptr_t) luaL_checknumber(state, 1);
    return 0;
}

int ModLoader::LuaReadWideString(lua_State *state)
{
    std::string text = StringUtils::ToUtf8(*(const std::wstring *) (uintptr_t) luaL_checknumber(state, 1));
    lua_pushlstring(state, text.data(), text.size());
    return 1;
}

int ModLoader::LuaClass(lua_State *state)
{
    const char *name = luaL_checkstring(state, 1);
    for (size_t i = 0; i < g_modClassCount; i++)
    {
        const ModClass &modClass = g_modClasses[i];
        if (strcmp(modClass.name, name) != 0)
        {
            continue;
        }

        lua_newtable(state);
        lua_pushstring(state, modClass.bases);
        lua_setfield(state, -2, "bases");
        lua_pushnumber(state, (lua_Number) (uintptr_t) modClass.destroy);
        lua_setfield(state, -2, "destroy");
        lua_newtable(state);
        for (size_t j = 0; j < modClass.count; j++)
        {
            const ModMethod &method = modClass.methods[j];
            lua_getfield(state, -1, method.name);
            if (lua_isnil(state, -1))
            {
                lua_pop(state, 1);
                lua_newtable(state);
                lua_pushvalue(state, -1);
                lua_setfield(state, -3, method.name);
            }

            PushMethod(state, method);
            lua_rawseti(state, -2, (int) lua_objlen(state, -2) + 1);
            lua_pop(state, 1);
        }

        lua_setfield(state, -2, "methods");
        lua_newtable(state);
        for (size_t j = 0; j < modClass.fieldCount; j++)
        {
            const ModField &field = modClass.fields[j];
            lua_newtable(state);
            PushMethod(state, field.get);
            lua_setfield(state, -2, "get");
            PushMethod(state, field.set);
            lua_setfield(state, -2, "set");
            lua_setfield(state, -2, field.name);
        }

        lua_setfield(state, -2, "fields");
        return 1;
    }

    return 0;
}

void ModLoader::PushMethod(lua_State *state, const ModMethod &method)
{
    lua_newtable(state);
    lua_pushnumber(state, (lua_Number) (uintptr_t) method.thunk);
    lua_setfield(state, -2, "thunk");
    lua_pushboolean(state, method.isStatic);
    lua_setfield(state, -2, "static");
    lua_pushnumber(state, (lua_Number) method.count);
    lua_setfield(state, -2, "count");
    PushParam(state, method.result);
    lua_setfield(state, -2, "result");
    lua_newtable(state);
    for (size_t i = 0; i < method.count; i++)
    {
        PushParam(state, method.params[i]);
        lua_rawseti(state, -2, (int) i + 1);
    }

    lua_setfield(state, -2, "params");
}

void ModLoader::PushParam(lua_State *state, const ModParam &param)
{
    lua_newtable(state);
    lua_pushlstring(state, &param.kind, 1);
    lua_setfield(state, -2, "kind");
    if (param.type)
    {
        lua_pushstring(state, param.type);
        lua_setfield(state, -2, "type");
    }

    lua_pushnumber(state, (lua_Number) (uintptr_t) param.destroy);
    lua_setfield(state, -2, "destroy");
    lua_pushboolean(state, param.reference);
    lua_setfield(state, -2, "reference");
}
