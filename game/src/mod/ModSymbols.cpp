#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ModSymbols.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cxxabi.h>
#include <fstream>
#include <iterator>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <filesystem>
#else
#include <elf.h>
#include <link.h>
#endif

#ifndef _WIN32
static int FindExecutableBase(dl_phdr_info *info, size_t size, void *data)
{
    (void) size;
    *(uintptr_t *) data = info->dlpi_addr;
    return 1;
}
#endif

bool ModSymbols::Load(std::string *error)
{
#ifdef _WIN32
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::ifstream stream(std::filesystem::path(path), std::ios::binary);
#else
    std::ifstream stream("/proc/self/exe", std::ios::binary);
#endif
    std::vector<uint8_t> image((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    if (image.empty())
    {
        *error = "Could not read the game executable";
        return false;
    }

#ifdef _WIN32
    IMAGE_DOS_HEADER dosHeader;
    memcpy(&dosHeader, image.data(), sizeof(dosHeader));
    IMAGE_FILE_HEADER fileHeader;
    memcpy(&fileHeader, image.data() + dosHeader.e_lfanew + sizeof(DWORD), sizeof(fileHeader));
    if (fileHeader.PointerToSymbolTable == 0)
    {
        *error = "The game executable has no symbol table";
        return false;
    }

    std::vector<IMAGE_SECTION_HEADER> sections(fileHeader.NumberOfSections);
    memcpy(sections.data(), image.data() + dosHeader.e_lfanew + sizeof(DWORD) + sizeof(fileHeader) + fileHeader.SizeOfOptionalHeader,
           sections.size() * sizeof(IMAGE_SECTION_HEADER));
    const uint8_t *symbols  = image.data() + fileHeader.PointerToSymbolTable;
    const char *stringTable = (const char *) symbols + fileHeader.NumberOfSymbols * IMAGE_SIZEOF_SYMBOL;
    uintptr_t base          = (uintptr_t) GetModuleHandleW(nullptr);
    for (DWORD i = 0; i < fileHeader.NumberOfSymbols; i++)
    {
        IMAGE_SYMBOL symbol;
        memcpy(&symbol, symbols + i * IMAGE_SIZEOF_SYMBOL, IMAGE_SIZEOF_SYMBOL);
        i += symbol.NumberOfAuxSymbols;
        if (symbol.SectionNumber <= 0 || symbol.SectionNumber > fileHeader.NumberOfSections)
        {
            continue;
        }

        if (symbol.StorageClass != IMAGE_SYM_CLASS_EXTERNAL && symbol.StorageClass != IMAGE_SYM_CLASS_STATIC)
        {
            continue;
        }

        std::string name = symbol.N.Name.Short ? std::string((const char *) symbol.N.ShortName, strnlen((const char *) symbol.N.ShortName, 8))
                                               : std::string(stringTable + symbol.N.Name.Long);
        if (name.empty() || name[0] == '.')
        {
            continue;
        }

        Add(name, base + sections[symbol.SectionNumber - 1].VirtualAddress + symbol.Value);
    }
#else
    uintptr_t base = 0;
    dl_iterate_phdr(FindExecutableBase, &base);
    Elf64_Ehdr header;
    memcpy(&header, image.data(), sizeof(header));
    bool foundTable = false;
    for (int i = 0; i < header.e_shnum; i++)
    {
        Elf64_Shdr section;
        memcpy(&section, image.data() + header.e_shoff + i * header.e_shentsize, sizeof(section));
        if (section.sh_type != SHT_SYMTAB)
        {
            continue;
        }

        Elf64_Shdr strings;
        memcpy(&strings, image.data() + header.e_shoff + section.sh_link * header.e_shentsize, sizeof(strings));
        foundTable = true;
        for (size_t j = 0; j < section.sh_size / sizeof(Elf64_Sym); j++)
        {
            Elf64_Sym symbol;
            memcpy(&symbol, image.data() + section.sh_offset + j * sizeof(Elf64_Sym), sizeof(symbol));
            int type = ELF64_ST_TYPE(symbol.st_info);
            if ((type != STT_FUNC && type != STT_OBJECT) || symbol.st_shndx == SHN_UNDEF || symbol.st_value == 0)
            {
                continue;
            }

            Add((const char *) image.data() + strings.sh_offset + symbol.st_name, base + symbol.st_value);
        }
    }

    if (!foundTable)
    {
        *error = "The game executable has no symbol table";
        return false;
    }
#endif

    return true;
}

uintptr_t ModSymbols::Find(const std::string &name, std::string *error) const
{
    auto it = m_addresses.find(Normalize(name));
    if (it == m_addresses.end())
    {
        *error = "No function or variable named " + name;
        return 0;
    }

    if (it->second == 0)
    {
        *error = name + " matches more than one function, add its parameter list";
    }

    return it->second;
}

void ModSymbols::Add(const std::string &mangledName, uintptr_t address)
{
    AddName(mangledName, address);

    int status;
    char *demangled = abi::__cxa_demangle(mangledName.c_str(), nullptr, nullptr, &status);
    if (!demangled)
    {
        return;
    }

    std::string name = Normalize(demangled);
    free(demangled);
    if (name.find("[clone") != std::string::npos)
    {
        return;
    }

    AddName(name, address);

    // We also register the name without its parameter list so mods can skip it when there is only one overload
    size_t end = name.rfind(')');
    if (end == std::string::npos || name.find("::", end) != std::string::npos)
    {
        return;
    }

    int depth = 0;
    for (size_t i = end + 1; i-- > 0;)
    {
        if (name[i] == ')')
        {
            depth++;
        }
        else if (name[i] == '(' && --depth == 0)
        {
            AddName(name.substr(0, i), address);
            return;
        }
    }
}

void ModSymbols::AddName(const std::string &name, uintptr_t address)
{
    auto result = m_addresses.emplace(name, address);
    if (!result.second && result.first->second != address)
    {
        result.first->second = 0;
    }
}

std::string ModSymbols::Normalize(const std::string &name) const
{
    // Spaces only matter between two identifier characters, like in "unsigned int"
    std::string result;
    for (size_t i = 0; i < name.size(); i++)
    {
        if (!isspace((unsigned char) name[i]))
        {
            result += name[i];
        }
        else if (!result.empty() && (isalnum((unsigned char) result.back()) || result.back() == '_') && i + 1 < name.size() &&
                 (isalnum((unsigned char) name[i + 1]) || name[i + 1] == '_'))
        {
            result += ' ';
        }
    }

    // We spell strings the way source code does since nobody can type the demangled form
    constexpr const char *kAliases[][2] = {
            {"std::__cxx11::basic_string<char,std::char_traits<char>,std::allocator<char>>", "std::string"},
            {"std::__cxx11::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t>>", "std::wstring"},
    };
    for (size_t i = 0; i < std::size(kAliases); i++)
    {
        size_t length = strlen(kAliases[i][0]);
        for (size_t position = result.find(kAliases[i][0]); position != std::string::npos; position = result.find(kAliases[i][0], position))
        {
            std::string alias = kAliases[i][1];
            if (position + length < result.size() && (isalnum((unsigned char) result[position + length]) || result[position + length] == '_'))
            {
                alias += ' ';
            }

            result.replace(position, length, alias);
        }
    }

    return result;
}

#endif
