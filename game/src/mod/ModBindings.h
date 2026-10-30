#pragma once

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct ModContainer;

struct ModParam
{
    char kind;
    const char *type;
    void (*destroy)(void *);
    bool reference;
    const ModContainer *container;
};

// Pairs leave size, insert and erase null and always hold two elements
struct ModContainer
{
    ModParam first;
    ModParam second;
    void *(*create)();
    void (*destroy)(void *);
    size_t (*size)(void *);
    void *(*at)(void *, size_t);
    void (*assign)(void *, size_t, void *);
    void (*insert)(void *, size_t, void *);
    void (*erase)(void *, size_t);
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

struct ModField
{
    const char *name;
    ModMethod get;
    ModMethod set;
};

struct ModClass
{
    const char *name;
    const char *bases;
    const ModMethod *methods;
    size_t count;
    const ModField *fields;
    size_t fieldCount;
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
constexpr bool kModContainer = false;

// We skip vector<bool> since it packs its bits and has no element address to hand out
template<typename T, typename A>
constexpr bool kModContainer<std::vector<T, A>> = !std::is_same_v<T, bool>;

template<typename F, typename S>
constexpr bool kModContainer<std::pair<F, S>> = true;

// We check elements ourselves since is_copy_constructible says yes to containers of move-only types
template<typename T>
constexpr bool kModCopyable = std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>;

template<typename T, typename A>
constexpr bool kModCopyable<std::vector<T, A>> = kModCopyable<T>;

template<typename F, typename S>
constexpr bool kModCopyable<std::pair<F, S>> = kModCopyable<F> && kModCopyable<S>;

template<typename T>
struct ModContainerOf;

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
            return {'p', nullptr, nullptr, false, nullptr};
        }
        else if constexpr (std::is_same_v<Bare, bool>)
        {
            return {'b', nullptr, nullptr, false, nullptr};
        }
        else if constexpr (kModNumber<T>)
        {
            return {'n', ModNumberName<T>(), nullptr, false, nullptr};
        }
        else if constexpr (kModText<T>)
        {
            return {'c', nullptr, nullptr, false, nullptr};
        }
        else if constexpr (std::is_pointer_v<Bare>)
        {
            if constexpr (kModContainer<ModPointee<T>>)
            {
                return {'l', nullptr, nullptr, false, &ModContainerOf<ModPointee<T>>::kContainer};
            }
            else if constexpr (std::is_same_v<ModPointee<T>, std::string>)
            {
                return {'s', nullptr, nullptr, false, nullptr};
            }
            else if constexpr (std::is_same_v<ModPointee<T>, std::wstring>)
            {
                return {'w', nullptr, nullptr, false, nullptr};
            }
            else
            {
                return {kModName<ModPointee<T>> ? 'o' : 'p', kModName<ModPointee<T>>, nullptr, false, nullptr};
            }
        }
        else if constexpr (std::is_same_v<Bare, std::string>)
        {
            return {std::is_reference_v<T> ? 's' : 'S', nullptr, nullptr, false, nullptr};
        }
        else if constexpr (std::is_same_v<Bare, std::wstring>)
        {
            return {std::is_reference_v<T> ? 'w' : 'W', nullptr, nullptr, false, nullptr};
        }
        else if constexpr (kModContainer<Bare>)
        {
            return {std::is_reference_v<T> ? 'l' : 'L', nullptr, nullptr, false, &ModContainerOf<Bare>::kContainer};
        }
        else
        {
            return {std::is_reference_v<T> ? 'o' : 'O', kModName<Bare>, nullptr, false, nullptr};
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
            return {'v', nullptr, nullptr, false, nullptr};
        }
        else if constexpr (std::is_reference_v<R> || std::is_pointer_v<R> || kModNumber<R> || kModText<R>)
        {
            return ModValue<R>::Describe();
        }
        else if constexpr (std::is_same_v<Bare, std::string>)
        {
            return {'S', nullptr, ModDestroyOf<Bare>(), false, nullptr};
        }
        else if constexpr (std::is_same_v<Bare, std::wstring>)
        {
            return {'W', nullptr, ModDestroyOf<Bare>(), false, nullptr};
        }
        else if constexpr (kModContainer<Bare>)
        {
            return {'L', nullptr, ModDestroyOf<Bare>(), false, &ModContainerOf<Bare>::kContainer};
        }
        else
        {
            return {'O', kModName<Bare>, ModDestroyOf<Bare>(), false, nullptr};
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

// Elements always move through pointers to them, and Lua reads or fills those based on the element's kind
template<typename T, typename A>
struct ModContainerOf<std::vector<T, A>>
{
    static void *Create()
    {
        return new std::vector<T, A>();
    }

    static size_t Size(void *self)
    {
        return ((std::vector<T, A> *) self)->size();
    }

    static void *At(void *self, size_t index)
    {
        return (void *) &(*(std::vector<T, A> *) self)[index];
    }

    static void Assign(void *self, size_t index, void *value)
    {
        (*(std::vector<T, A> *) self)[index] = *(const T *) value;
    }

    static void Insert(void *self, size_t index, void *value)
    {
        std::vector<T, A> *list = (std::vector<T, A> *) self;
        list->insert(list->begin() + (ptrdiff_t) index, *(const T *) value);
    }

    static void Erase(void *self, size_t index)
    {
        std::vector<T, A> *list = (std::vector<T, A> *) self;
        list->erase(list->begin() + (ptrdiff_t) index);
    }

    static constexpr void (*AssignOf())(void *, size_t, void *)
    {
        if constexpr (kModCopyable<T>)
        {
            return Assign;
        }
        else
        {
            return nullptr;
        }
    }

    static constexpr void (*InsertOf())(void *, size_t, void *)
    {
        if constexpr (kModCopyable<T>)
        {
            return Insert;
        }
        else
        {
            return nullptr;
        }
    }

    static constexpr void (*EraseOf())(void *, size_t)
    {
        if constexpr (std::is_move_assignable_v<T>)
        {
            return Erase;
        }
        else
        {
            return nullptr;
        }
    }

    static constexpr ModContainer kContainer = {
            ModValue<T>::Describe(), {}, Create, ModDestroyOf<std::vector<T, A>>(), Size, At, AssignOf(), InsertOf(), EraseOf()};
};

template<typename F, typename S>
struct ModContainerOf<std::pair<F, S>>
{
    static void *Create()
    {
        return new std::pair<F, S>();
    }

    static void *At(void *self, size_t index)
    {
        std::pair<F, S> *pair = (std::pair<F, S> *) self;
        return index == 0 ? (void *) &pair->first : (void *) &pair->second;
    }

    static void Assign(void *self, size_t index, void *value)
    {
        std::pair<F, S> *pair = (std::pair<F, S> *) self;
        if (index == 0)
        {
            pair->first = *(const F *) value;
        }
        else
        {
            pair->second = *(const S *) value;
        }
    }

    static constexpr void *(*CreateOf())()
    {
        if constexpr (std::is_default_constructible_v<std::pair<F, S>>)
        {
            return Create;
        }
        else
        {
            return nullptr;
        }
    }

    static constexpr void (*AssignOf())(void *, size_t, void *)
    {
        if constexpr (kModCopyable<std::pair<F, S>>)
        {
            return Assign;
        }
        else
        {
            return nullptr;
        }
    }

    static constexpr ModContainer kContainer = {
            ModValue<F>::Describe(), ModValue<S>::Describe(), CreateOf(), ModDestroyOf<std::pair<F, S>>(), nullptr, At, AssignOf(), nullptr, nullptr};
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

// Numbers and pointers are read by value, everything else is handed out by reference so mods edit the field itself
template<typename T>
using ModFieldValue = std::conditional_t<kModNumber<T> || std::is_pointer_v<T>, T, T &>;

// We only assign numbers, pointers and strings since is_copy_assignable says yes to containers of move-only types
template<typename T>
constexpr bool kModAssignable =
        std::is_copy_assignable_v<T> && (kModNumber<T> || std::is_pointer_v<T> || std::is_same_v<T, std::string> || std::is_same_v<T, std::wstring>);

template<auto M, typename T = decltype(M)>
struct ModFieldBind;

template<auto M, typename T, typename C>
struct ModFieldBind<M, T C::*>
{
    static ModAbi<ModFieldValue<T>> Get(void *self)
    {
        return ModResult<ModFieldValue<T>>::To(((C *) self)->*M);
    }

    static void Set(void *self, ModAbi<T> value)
    {
        ((C *) self)->*M = ModValue<T>::From(value);
    }

    static ModField Describe(const char *name)
    {
        ModMethod setter = ModSignature<void>::Describe(name, nullptr, false);
        if constexpr (kModAssignable<T>)
        {
            setter = ModSignature<void, T>::Describe(name, (void *) Set, false);
        }

        return {name, ModSignature<ModFieldValue<T>>::Describe(name, (void *) Get, false), setter};
    }
};

template<auto M, typename T>
struct ModFieldBind<M, T *>
{
    static ModAbi<ModFieldValue<T>> Get()
    {
        return ModResult<ModFieldValue<T>>::To(*M);
    }

    static void Set(ModAbi<T> value)
    {
        *M = ModValue<T>::From(value);
    }

    static ModField Describe(const char *name)
    {
        ModMethod setter = ModSignature<void>::Describe(name, nullptr, true);
        if constexpr (kModAssignable<T>)
        {
            setter = ModSignature<void, T>::Describe(name, (void *) Set, true);
        }

        return {name, ModSignature<ModFieldValue<T>>::Describe(name, (void *) Get, true), setter};
    }
};
