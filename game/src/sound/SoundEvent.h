#pragma once

#include <cstdint>
#include <string>

#include "../core/ResourceLocation.h"
#include "../io/File.h"

enum eSOUND_EVENT_TYPE : uint8_t
{
    eSoundEventType_Sound          = 0,
    eSoundEventType_Music          = 1,
    eSoundEventType_StreamingSound = 2,
};

enum eSOUND_CATEGORY : uint8_t
{
    eSoundCategory_Music   = 0,
    eSoundCategory_Weather = 1,
    eSoundCategory_Blocks  = 2,
    eSoundCategory_Ambient = 3,
    eSoundCategory_Players = 4,
    eSoundCategory_Ui      = 5,
    eSoundCategory_Count   = 6,
};

class SoundEvent
{
public:
    SoundEvent(const ResourceLocation &location, const File &file, eSOUND_EVENT_TYPE type);

    static SoundEvent *ByName(const ResourceLocation &location);
    static SoundEvent *ById(uint32_t id);

    const ResourceLocation &GetResourceLocation() const;
    const File &GetFile() const;
    eSOUND_EVENT_TYPE GetType() const;
    eSOUND_CATEGORY GetCategory() const;
    bool IsMusic() const;
    bool IsStreaming() const;

private:
    static eSOUND_CATEGORY CategoryForName(const std::string &name, eSOUND_EVENT_TYPE type);

    ResourceLocation m_location;
    File m_file;
    eSOUND_EVENT_TYPE m_type;
    eSOUND_CATEGORY m_category;
};
