#include "ModPatcher.h"

#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

// Every thunk jumps here with its ModHook in r11 or x17, and we tail call whatever ModHookResolve picks with the
// argument registers untouched
#if defined(__aarch64__)
asm(R"(
    .text
    .globl ModHookStub
    .type ModHookStub, %function
ModHookStub:
    stp x29, x30, [sp, #-224]!
    mov x29, sp
    stp x0, x1, [sp, #16]
    stp x2, x3, [sp, #32]
    stp x4, x5, [sp, #48]
    stp x6, x7, [sp, #64]
    str x8, [sp, #80]
    stp q0, q1, [sp, #96]
    stp q2, q3, [sp, #128]
    stp q4, q5, [sp, #160]
    stp q6, q7, [sp, #192]
    mov x0, x17
    bl ModHookResolve
    mov x16, x0
    ldp x0, x1, [sp, #16]
    ldp x2, x3, [sp, #32]
    ldp x4, x5, [sp, #48]
    ldp x6, x7, [sp, #64]
    ldr x8, [sp, #80]
    ldp q0, q1, [sp, #96]
    ldp q2, q3, [sp, #128]
    ldp q4, q5, [sp, #160]
    ldp q6, q7, [sp, #192]
    ldp x29, x30, [sp], #224
    br x16
)");
#elif defined(_WIN32)
asm(R"(
    .text
    .globl ModHookStub
ModHookStub:
    pushq %rbp
    movq %rsp, %rbp
    movq %rcx, 16(%rbp)
    movq %rdx, 24(%rbp)
    movq %r8, 32(%rbp)
    movq %r9, 40(%rbp)
    subq $96, %rsp
    movdqa %xmm0, 32(%rsp)
    movdqa %xmm1, 48(%rsp)
    movdqa %xmm2, 64(%rsp)
    movdqa %xmm3, 80(%rsp)
    movq %r11, %rcx
    call ModHookResolve
    movq %rax, %r11
    movdqa 32(%rsp), %xmm0
    movdqa 48(%rsp), %xmm1
    movdqa 64(%rsp), %xmm2
    movdqa 80(%rsp), %xmm3
    movq 16(%rbp), %rcx
    movq 24(%rbp), %rdx
    movq 32(%rbp), %r8
    movq 40(%rbp), %r9
    movq %rbp, %rsp
    popq %rbp
    jmp *%r11
)");
#else
asm(R"(
    .text
    .globl ModHookStub
    .type ModHookStub, @function
ModHookStub:
    pushq %rbp
    movq %rsp, %rbp
    subq $192, %rsp
    movq %rdi, 0(%rsp)
    movq %rsi, 8(%rsp)
    movq %rdx, 16(%rsp)
    movq %rcx, 24(%rsp)
    movq %r8, 32(%rsp)
    movq %r9, 40(%rsp)
    movq %rax, 48(%rsp)
    movdqa %xmm0, 64(%rsp)
    movdqa %xmm1, 80(%rsp)
    movdqa %xmm2, 96(%rsp)
    movdqa %xmm3, 112(%rsp)
    movdqa %xmm4, 128(%rsp)
    movdqa %xmm5, 144(%rsp)
    movdqa %xmm6, 160(%rsp)
    movdqa %xmm7, 176(%rsp)
    movq %r11, %rdi
    call ModHookResolve
    movq %rax, %r11
    movq 0(%rsp), %rdi
    movq 8(%rsp), %rsi
    movq 16(%rsp), %rdx
    movq 24(%rsp), %rcx
    movq 32(%rsp), %r8
    movq 40(%rsp), %r9
    movq 48(%rsp), %rax
    movdqa 64(%rsp), %xmm0
    movdqa 80(%rsp), %xmm1
    movdqa 96(%rsp), %xmm2
    movdqa 112(%rsp), %xmm3
    movdqa 128(%rsp), %xmm4
    movdqa 144(%rsp), %xmm5
    movdqa 160(%rsp), %xmm6
    movdqa 176(%rsp), %xmm7
    movq %rbp, %rsp
    popq %rbp
    jmp *%r11
)");
#endif

extern "C" void ModHookStub();

ModPatcher::ModPatcher()
{
    m_pool     = nullptr;
    m_poolUsed = 0;
}

bool ModPatcher::Patch(uintptr_t target, ModHook *hook, std::string *error)
{
    constexpr size_t kThunkSize = 32;
    if (!m_pool && !AllocatePool(target, error))
    {
        return false;
    }

    if (m_poolUsed + kThunkSize > kPoolSize)
    {
        *error = "Too many hooks";
        return false;
    }

    uint8_t *thunk = m_pool + m_poolUsed;
    uintptr_t stub = (uintptr_t) ModHookStub;
#if defined(__aarch64__)
    constexpr size_t kPadSize = 4;
    uint32_t pad;
    memcpy(&pad, (void *) target, sizeof(pad));
    if (pad != 0xD503201F)
    {
        *error = "The function has no patch pad, it was probably inlined or is not part of the game";
        return false;
    }

    intptr_t distance = (intptr_t) thunk - (intptr_t) target;
    if (distance < -(128 << 20) || distance >= (128 << 20))
    {
        *error = "The function is too far away from the hook memory";
        return false;
    }

    // ldr x17, #16, ldr x16, #20, br x16 then the hook and stub addresses as literals
    uint32_t code[4] = {0x58000000 | (4 << 5) | 17, 0x58000000 | (5 << 5) | 16, 0xD61F0200, 0xD503201F};
    uint8_t jump[kPadSize];
    uint32_t branch = 0x14000000 | (uint32_t) ((distance >> 2) & 0x3FFFFFF);
    memcpy(jump, &branch, sizeof(branch));
#else
    constexpr size_t kPadSize = 5;
    const uint8_t *pad        = (const uint8_t *) target;
    bool singleNop            = pad[0] == 0x0F && pad[1] == 0x1F && pad[2] == 0x44 && pad[3] == 0x00;
    bool byteNops             = pad[0] == 0x90 && pad[1] == 0x90 && pad[2] == 0x90 && pad[3] == 0x90 && pad[4] == 0x90;
    if (!singleNop && !byteNops)
    {
        *error = "The function has no patch pad, it was probably inlined or is not part of the game";
        return false;
    }

    intptr_t relative = (intptr_t) thunk - (intptr_t) (target + kPadSize);
    if (relative < INT32_MIN || relative > INT32_MAX)
    {
        *error = "The function is too far away from the hook memory";
        return false;
    }

    // movabs r11, hook then jmp [rip], with the stub address right after the jump
    uint8_t code[16] = {0x49, 0xBB, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0x25, 0, 0, 0, 0};
    memcpy(code + 2, &hook, sizeof(hook));
    uint8_t jump[kPadSize] = {0xE9};
    int32_t relative32     = (int32_t) relative;
    memcpy(jump + 1, &relative32, sizeof(relative32));
#endif

    hook->original = (void *) (target + kPadSize);

    ProtectPool(true);
    memcpy(thunk, code, sizeof(code));
#if defined(__aarch64__)
    memcpy(thunk + 16, &hook, sizeof(hook));
    memcpy(thunk + 24, &stub, sizeof(stub));
#else
    memcpy(thunk + 16, &stub, sizeof(stub));
#endif
    ProtectPool(false);
    m_poolUsed += kThunkSize;

#ifdef _WIN32
    DWORD oldProtection;
    VirtualProtect((void *) target, kPadSize, PAGE_EXECUTE_READWRITE, &oldProtection);
    memcpy((void *) target, jump, kPadSize);
    VirtualProtect((void *) target, kPadSize, oldProtection, &oldProtection);
    FlushInstructionCache(GetCurrentProcess(), thunk, kThunkSize);
    FlushInstructionCache(GetCurrentProcess(), (void *) target, kPadSize);
#else
    uintptr_t pageSize = (uintptr_t) sysconf(_SC_PAGESIZE);
    uintptr_t page     = target & ~(pageSize - 1);
    size_t length      = target + kPadSize - page;
    if (mprotect((void *) page, length, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
    {
        *error = "Could not make the game code writable";
        return false;
    }

    memcpy((void *) target, jump, kPadSize);
    mprotect((void *) page, length, PROT_READ | PROT_EXEC);
    __builtin___clear_cache((char *) thunk, (char *) thunk + kThunkSize);
    __builtin___clear_cache((char *) target, (char *) target + kPadSize);
#endif

    return true;
}

bool ModPatcher::AllocatePool(uintptr_t target, std::string *error)
{
    // Thunks have to sit within reach of a direct branch from the game code, so we probe outwards from the first target
#if defined(__aarch64__)
    constexpr uintptr_t kRange = 64 << 20;
#else
    constexpr uintptr_t kRange = 1u << 30;
#endif
    uintptr_t origin = target & ~(uintptr_t) (kPoolSize - 1);
    for (uintptr_t offset = kPoolSize; offset < kRange; offset += kPoolSize)
    {
        uintptr_t candidates[2] = {origin - offset, origin + offset};
        for (int i = 0; i < 2; i++)
        {
#ifdef _WIN32
            m_pool = (uint8_t *) VirtualAlloc((void *) candidates[i], kPoolSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (m_pool)
            {
                return true;
            }
#else
            void *pool = mmap((void *) candidates[i], kPoolSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
            if (pool == (void *) candidates[i])
            {
                m_pool = (uint8_t *) pool;
                return true;
            }

            // Kernels without MAP_FIXED_NOREPLACE treat the address as a hint and may map somewhere else
            if (pool != MAP_FAILED)
            {
                munmap(pool, kPoolSize);
            }
#endif
        }
    }

    *error = "Could not allocate hook memory near the game code";
    return false;
}

void ModPatcher::ProtectPool(bool writable)
{
#ifdef _WIN32
    DWORD oldProtection;
    VirtualProtect(m_pool, kPoolSize, writable ? PAGE_READWRITE : PAGE_EXECUTE_READ, &oldProtection);
#else
    mprotect(m_pool, kPoolSize, writable ? PROT_READ | PROT_WRITE : PROT_READ | PROT_EXEC);
#endif
}
