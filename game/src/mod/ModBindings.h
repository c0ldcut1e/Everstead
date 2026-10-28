#pragma once

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

struct ModParam
{
    char kind;
    const char *type;
    void (*destroy)(void *);
    bool reference;
};

struct ModMethod
{
    const char *name;
    void *thunk;
    bool isStatic;
    ModParam result;
    const ModParam *params;
    size_t count;
};

struct ModClass
{
    const char *name;
    const char *bases;
    const ModMethod *methods;
    size_t count;
    void (*destroy)(void *);
};

extern const ModClass g_modClasses[];
extern const size_t g_modClassCount;

template<typename T>
constexpr const char *kModName = nullptr;

template<typename T>
using ModBare = std::remove_cv_t<std::remove_reference_t<T>>;

template<typename T>
using ModPointee = std::remove_cv_t<std::remove_pointer_t<ModBare<T>>>;

template<typename T>
constexpr bool kModNumber = std::is_arithmetic_v<ModBare<T>> || std::is_enum_v<ModBare<T>>;

template<typename T>
constexpr bool kModOutNumber = kModNumber<T> && std::is_lvalue_reference_v<T> && !std::is_const_v<std::remove_reference_t<T>>;

template<typename T>
constexpr bool kModText = std::is_same_v<ModBare<T>, const char *>;

template<typename T>
void ModDestroy(void *object)
{
    delete (T *) object;
}

template<typename T>
constexpr void (*ModDestroyOf())(void *)
{
    if constexpr (std::is_destructible_v<T> && !std::is_abstract_v<T>)
    {
        return ModDestroy<T>;
    }
    else
    {
        return nullptr;
    }
}

template<typename T>
struct ModUnderlying
{
    using Type = T;
};

template<typename T>
    requires std::is_enum_v<T>
struct ModUnderlying<T>
{
    using Type = std::underlying_type_t<T>;
};

template<typename T>
constexpr const char *ModNumberName()
{
    using Number = typename ModUnderlying<ModBare<T>>::Type;
    if constexpr (std::is_same_v<Number, bool>)
    {
        return "bool";
    }
    else if constexpr (std::is_floating_point_v<Number>)
    {
        return sizeof(Number) == 4 ? "float" : "double";
    }
    else
    {
        constexpr const char *kSigned[]   = {"int8_t", "int16_t", "", "int32_t", "", "", "", "int64_t"};
        constexpr const char *kUnsigned[] = {"uint8_t", "uint16_t", "", "uint32_t", "", "", "", "uint64_t"};
        return std::is_signed_v<Number> ? kSigned[sizeof(Number) - 1] : kUnsigned[sizeof(Number) - 1];
    }
}

// We hand every value to Lua as a number, a C string or a plain pointer so the thunks can be called through
// the FFI without it knowing any class layout
template<typename T>
using ModAbi = std::conditional_t<std::is_void_v<T>, void,
                                  std::conditional_t<kModNumber<T> && !kModOutNumber<T>, typename ModUnderlying<ModBare<T>>::Type,
                                                     std::conditional_t<kModText<T>, const char *, void *>>>;

template<typename T>
struct ModValue
{
    using Result = std::conditional_t<kModNumber<T> && !kModOutNumber<T>, ModBare<T>, T>;

    static constexpr ModParam Describe()
    {
        ModParam param  = Classify();
        param.reference = std::is_reference_v<T>;
        return param;
    }

    static constexpr ModParam Classify()
    {
        using Bare = ModBare<T>;
        if constexpr (kModOutNumber<T>)
        {
            return {'p', nullptr, nullptr, false};
        }
        else if constexpr (std::is_same_v<Bare, bool>)
        {
            return {'b', nullptr, nullptr, false};
        }
        else if constexpr (kModNumber<T>)
        {
            return {'n', ModNumberName<T>(), nullptr, false};
        }
        else if constexpr (kModText<T>)
        {
            return {'c', nullptr, nullptr, false};
        }
        else if constexpr (std::is_pointer_v<Bare>)
        {
            if constexpr (std::is_same_v<ModPointee<T>, std::string>)
            {
                return {'s', nullptr, nullptr, false};
            }
            else if constexpr (std::is_same_v<ModPointee<T>, std::wstring>)
            {
                return {'w', nullptr, nullptr, false};
            }
            else
            {
                return {kModName<ModPointee<T>> ? 'o' : 'p', kModName<ModPointee<T>>, nullptr, false};
            }
        }
        else if constexpr (std::is_same_v<Bare, std::string>)
        {
            return {std::is_reference_v<T> ? 's' : 'S', nullptr, nullptr, false};
        }
        else if constexpr (std::is_same_v<Bare, std::wstring>)
        {
            return {std::is_reference_v<T> ? 'w' : 'W', nullptr, nullptr, false};
        }
        else
        {
            return {std::is_reference_v<T> ? 'o' : 'O', kModName<Bare>, nullptr, false};
        }
    }

    static Result From(ModAbi<T> value)
    {
        using Bare = ModBare<T>;
        if constexpr (kModOutNumber<T>)
        {
            return *(Bare *) value;
        }
        else if constexpr (kModNumber<T>)
        {
            return (Bare) value;
        }
        else if constexpr (std::is_pointer_v<T> || kModText<T>)
        {
            return (T) value;
        }
        else if constexpr (std::is_lvalue_reference_v<T>)
        {
            return *(Bare *) value;
        }
        else if constexpr (std::is_rvalue_reference_v<T> || !std::is_copy_constructible_v<Bare>)
        {
            return std::move(*(Bare *) value);
        }
        else
        {
            return *(Bare *) value;
        }
    }
};

template<typename R>
struct ModResult
{
    static constexpr ModParam Describe()
    {
        using Bare = ModBare<R>;
        if constexpr (std::is_void_v<R>)
        {
            return {'v', nullptr, nullptr, false};
        }
        else if constexpr (std::is_reference_v<R> || std::is_pointer_v<R> || kModNumber<R> || kModText<R>)
        {
            return ModValue<R>::Describe();
        }
        else if constexpr (std::is_same_v<Bare, std::string>)
        {
            return {'S', nullptr, ModDestroyOf<Bare>(), false};
        }
        else if constexpr (std::is_same_v<Bare, std::wstring>)
        {
            return {'W', nullptr, ModDestroyOf<Bare>(), false};
        }
        else
        {
            return {'O', kModName<Bare>, ModDestroyOf<Bare>(), false};
        }
    }

    template<typename V>
    static ModAbi<R> To(V &&value)
    {
        using Bare = ModBare<R>;
        if constexpr (kModOutNumber<R>)
        {
            return (void *) &value;
        }
        else if constexpr (kModNumber<R>)
        {
            return (ModAbi<R>) value;
        }
        else if constexpr (kModText<R>)
        {
            return value;
        }
        else if constexpr (std::is_pointer_v<R>)
        {
            return (void *) value;
        }
        else if constexpr (std::is_reference_v<R>)
        {
            return (void *) &value;
        }
        else
        {
            return new Bare(std::move(value));
        }
    }
};

template<typename R, typename... A>
struct ModSignature
{
    static constexpr ModParam kParams[] = {ModValue<A>::Describe()..., {}};

    static ModMethod Describe(const char *name, void *thunk, bool isStatic)
    {
        return {name, thunk, isStatic, ModResult<R>::Describe(), kParams, sizeof...(A)};
    }
};

template<auto F, typename R, typename C, typename... A>
struct ModMemberBind
{
    static ModAbi<R> Call(void *self, ModAbi<A>... args)
    {
        if constexpr (std::is_void_v<R>)
        {
            (((C *) self)->*F)(ModValue<A>::From(args)...);
        }
        else
        {
            return ModResult<R>::To((((C *) self)->*F)(ModValue<A>::From(args)...));
        }
    }

    static ModMethod Describe(const char *name)
    {
        return ModSignature<R, A...>::Describe(name, (void *) Call, false);
    }
};

template<auto F, typename R, typename... A>
struct ModStaticBind
{
    static ModAbi<R> Call(ModAbi<A>... args)
    {
        if constexpr (std::is_void_v<R>)
        {
            F(ModValue<A>::From(args)...);
        }
        else
        {
            return ModResult<R>::To(F(ModValue<A>::From(args)...));
        }
    }

    static ModMethod Describe(const char *name)
    {
        return ModSignature<R, A...>::Describe(name, (void *) Call, true);
    }
};

template<auto F, typename T = decltype(F)>
struct ModBind;

template<auto F, typename R, typename C, typename... A>
struct ModBind<F, R (C::*)(A...)> : ModMemberBind<F, R, C, A...>
{
};

template<auto F, typename R, typename C, typename... A>
struct ModBind<F, R (C::*)(A...) noexcept> : ModMemberBind<F, R, C, A...>
{
};

template<auto F, typename R, typename C, typename... A>
struct ModBind<F, R (C::*)(A...) const> : ModMemberBind<F, R, C, A...>
{
};

template<auto F, typename R, typename C, typename... A>
struct ModBind<F, R (C::*)(A...) const noexcept> : ModMemberBind<F, R, C, A...>
{
};

template<auto F, typename R, typename... A>
struct ModBind<F, R (*)(A...)> : ModStaticBind<F, R, A...>
{
};

template<auto F, typename R, typename... A>
struct ModBind<F, R (*)(A...) noexcept> : ModStaticBind<F, R, A...>
{
};

template<typename... A>
struct ModPick
{
    template<typename R, typename C>
    static constexpr auto Member(R (C::*function)(A...))
    {
        return function;
    }

    template<typename R, typename C>
    static constexpr auto Const(R (C::*function)(A...) const)
    {
        return function;
    }

    template<typename R>
    static constexpr auto Static(R (*function)(A...))
    {
        return function;
    }
};

template<typename C, typename... A>
struct ModConstruct
{
    static void *Call(ModAbi<A>... args)
    {
        return new C(ModValue<A>::From(args)...);
    }

    static ModMethod Describe()
    {
        void *thunk = nullptr;
        if constexpr (std::is_constructible_v<C, A...> && !std::is_abstract_v<C>)
        {
            thunk = (void *) Call;
        }

        ModMethod method      = ModSignature<C *, A...>::Describe("new", thunk, true);
        method.result.destroy = ModDestroyOf<C>();
        return method;
    }
};
