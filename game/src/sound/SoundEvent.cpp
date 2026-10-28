#include "SoundEvent.h"

#include "SoundRegistry.h"

SoundEvent::SoundEvent(const ResourceLocation &location, const File &file, eSOUND_EVENT_TYPE type)
{
    m_location     = location;
    m_file         = file;
    m_type         = type;
    m_category     = CategoryForName(location.GetPath(), type);
}

SoundEvent *SoundEvent::ByName(const ResourceLocation &location)
{
    return SoundRegistry::ByName(location);
}

SoundEvent *SoundEvent::ById(uint32_t id)
{
    return SoundRegistry::ById(id);
}

const ResourceLocation &SoundEvent::GetResourceLocation() const
{
    return m_location;
}

const File &SoundEvent::GetFile() const
{
    return m_file;
}

eSOUND_EVENT_TYPE SoundEvent::GetType() const
{
    return m_type;
}

eSOUND_CATEGORY SoundEvent::GetCategory() const
{
    return m_category;
}

eSOUND_CATEGORY SoundEvent::CategoryForName(const std::string &name, eSOUND_EVENT_TYPE type)
{
    if (type == eSoundEventType_Music || name.rfind("music", 0) == 0)
    {
        return eSoundCategory_Music;
    }

    if (name.rfind("ambient.weather", 0) == 0 || name.rfind("weather", 0) == 0)
    {
        return eSoundCategory_Weather;
    }

    if (name.rfind("ambient", 0) == 0 || name.rfind("liquid", 0) == 0 || name.rfind("fire", 0) == 0)
    {
        return eSoundCategory_Ambient;
    }

    if (name.rfind("ui.", 0) == 0)
    {
        return eSoundCategory_Ui;
    }

    if (name.rfind("entity.player", 0) == 0 || name.rfind("damage.", 0) == 0 || name == "random.splash" || name == "random.pop" ||
        name == "random.orb" || name == "random.levelup")
    {
        return eSoundCategory_Players;
    }

    return eSoundCategory_Blocks;
}

bool SoundEvent::IsMusic() const
{
    return m_type == eSoundEventType_Music;
}

bool SoundEvent::IsStreaming() const
{
    return m_type == eSoundEventType_StreamingSound || m_type == eSoundEventType_Music;
}
