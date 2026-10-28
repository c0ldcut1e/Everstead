---@meta

---@class ModInfo
---@field id string
---@field name string
---@field version string
---@field directory string

---@type ModInfo
mod = {}

log = {}

---@param ... any
function log.info(...) end

---@param ... any
function log.warn(...) end

---@param ... any
function log.error(...) end

std = {}

---Allocates a std::string the game can read, freed when Lua collects it
---@param text string
---@return ffi.cdata*
function std.string(text) end

---Allocates a std::wstring the game can read, freed when Lua collects it
---@param text string
---@return ffi.cdata*
function std.wstring(text) end

---Reads a std::string the game passed in
---@param pointer ffi.cdata*
---@return string
function std.read(pointer) end

---Reads a std::wstring the game passed in
---@param pointer ffi.cdata*
---@return string
function std.readWide(pointer) end

ffi = require("ffi")

---Replaces a game function, only allowed while main.lua runs. The signature can be left out when the
---method has one overload and takes no class by value
---@param target string Name like "Class::Function" or "Class::Function(int, float)"
---@param signature string|fun(original: function, ...: any): any C signature like "bool(void*, uint8_t, int, void*)"
---@param handler? fun(original: function, ...: any): any
function hook(target, signature, handler) end

---Returns a callable pointer to a game function
---@param target string
---@param signature string
---@return function
function func(target, signature) end

---Returns the address of a game function or global variable
---@param target string
---@return ffi.cdata*
function symbol(target) end

---Creates a game object with the constructor matching the arguments. Lua strings convert to std::string or to
---classes built from a string. Game classes are also globals, like ItemRegistry.GetInstance()
---@generic T
---@param name `T` Class name like "Item"
---@param ... any Constructor arguments
---@return T
function new(name, ...) end

---Runs the destructor of an object made with new and frees it
---@param object any
function delete(object) end
