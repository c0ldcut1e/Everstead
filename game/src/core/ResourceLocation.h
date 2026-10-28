#pragma once

#include <cstddef>
#include <string>

class ResourceLocation
{
public:
    ResourceLocation();
    ResourceLocation(const std::string &namespaceId, const std::string &path);
    ResourceLocation(const std::string &combined);
    ResourceLocation(const char *combined);

    static bool TryParse(const std::string &text, ResourceLocation *location);

    const std::string &GetNamespace() const
    {
        return m_namespace;
    }

    const std::string &GetPath() const
    {
        return m_path;
    }

    std::string ToString() const;
    std::string ToLanguageKey(const std::string &type) const;

    bool operator==(const ResourceLocation &other) const;
    bool operator!=(const ResourceLocation &other) const;

    static constexpr const char *kDefaultNamespace = "everstead";

private:
    std::string m_namespace;
    std::string m_path;
};

struct ResourceLocationHash
{
    size_t operator()(const ResourceLocation &location) const;
};
