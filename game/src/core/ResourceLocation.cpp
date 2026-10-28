#include "ResourceLocation.h"

#include <functional>

ResourceLocation::ResourceLocation()
{
    m_namespace = kDefaultNamespace;
    m_path      = "";
}

ResourceLocation::ResourceLocation(const std::string &namespaceId, const std::string &path)
{
    m_namespace = namespaceId;
    m_path      = path;
}

ResourceLocation::ResourceLocation(const std::string &combined)
{
    size_t separator = combined.find(':');
    if (separator == std::string::npos)
    {
        m_namespace = kDefaultNamespace;
        m_path      = combined;
        return;
    }

    m_namespace = combined.substr(0, separator);
    m_path      = combined.substr(separator + 1);
}

ResourceLocation::ResourceLocation(const char *combined) : ResourceLocation(std::string(combined))
{}

bool ResourceLocation::TryParse(const std::string &text, ResourceLocation *location)
{
    ResourceLocation parsed(text);
    if (parsed.m_namespace.empty() || parsed.m_path.empty())
    {
        return false;
    }

    for (size_t i = 0; i < parsed.m_namespace.size(); i++)
    {
        char c = parsed.m_namespace[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-'))
        {
            return false;
        }
    }

    for (size_t i = 0; i < parsed.m_path.size(); i++)
    {
        char c = parsed.m_path[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-' || c == '/'))
        {
            return false;
        }
    }

    *location = parsed;
    return true;
}

std::string ResourceLocation::ToString() const
{
    return m_namespace + ":" + m_path;
}

std::string ResourceLocation::ToLanguageKey(const std::string &type) const
{
    return type + "." + m_namespace + "." + m_path;
}

bool ResourceLocation::operator==(const ResourceLocation &other) const
{
    return m_namespace == other.m_namespace && m_path == other.m_path;
}

bool ResourceLocation::operator!=(const ResourceLocation &other) const
{
    return !(*this == other);
}

size_t ResourceLocationHash::operator()(const ResourceLocation &location) const
{
    size_t namespaceHash = std::hash<std::string>()(location.GetNamespace());
    size_t pathHash      = std::hash<std::string>()(location.GetPath());
    return namespaceHash ^ (pathHash + 0x9e3779b9 + (namespaceHash << 6) + (namespaceHash >> 2));
}
