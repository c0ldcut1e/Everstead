#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../core/ResourceLocation.h"
#include "../io/File.h"
#include "SoundEvent.h"

class SoundRegistry
{
public:
    static SoundRegistry *GetInstance();
    static void Init();
    static void Clear();

    static SoundEvent *AddSound(const ResourceLocation &location, File *file);
    static SoundEvent *AddMusic(const ResourceLocation &location, File *file);
    static SoundEvent *AddStreamingSound(const ResourceLocation &location, File *file);

    static const std::vector<SoundEvent *> &GetMusic(const std::string &folder);
    static SoundEvent *ByName(const ResourceLocation &location);
    static SoundEvent *ById(uint32_t id);
    static bool Has(const ResourceLocation &location);
    static uint32_t Size();

private:
    SoundRegistry();

    SoundEvent *RegisterEvent(const ResourceLocation &location, File *file, eSOUND_EVENT_TYPE type);
    void ClearRegistry();

    std::unordered_map<ResourceLocation, uint32_t, ResourceLocationHash> m_locationToId;
    std::vector<SoundEvent *> m_events;
    std::vector<std::unique_ptr<SoundEvent>> m_ownedEvents;
    std::unordered_map<std::string, std::vector<SoundEvent *>> m_music;
};
