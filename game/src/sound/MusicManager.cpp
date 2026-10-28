#ifndef EVERSTEAD_DEDICATED_SERVER

#include "MusicManager.h"

#include <algorithm>
#include <climits>
#include <vector>

#include "../core/Logger.h"
#include "SoundEngine.h"
#include "SoundRegistry.h"

// These delays are in ticks
const MusicManager::Music MusicManager::s_music[] = {
        {"menu", {nullptr, nullptr}, 20, 600},
        {"game", {nullptr, nullptr}, 12000, 24000},
        {"cave", {"ice_caves", "desert_caves"}, 200, 2400},
        {"ice_caves", {nullptr, nullptr}, 200, 2400},
        {"desert_caves", {nullptr, nullptr}, 200, 2400},
        {"void", {nullptr, nullptr}, 0, 0},
};

// This specifies how long a new music type has to hold before switching
const int MusicManager::s_switchTicks = 60;

MusicManager::MusicManager()
{
    m_currentMusic      = nullptr;
    m_currentType       = eMusicType_Menu;
    m_pendingType       = eMusicType_Menu;
    m_pendingTicks      = 0;
    m_playingBeforeVoid = false;
    m_nextSongDelay     = 100;
    m_tickTime          = 0.0f;
}

void MusicManager::Update(SoundEngine *soundEngine, eMUSIC_TYPE type, float deltaSeconds)
{
    if (!soundEngine)
    {
        return;
    }

    m_tickTime += deltaSeconds;
    while (m_tickTime >= 0.05f)
    {
        m_tickTime -= 0.05f;
        Tick(soundEngine, type);
    }
}

void MusicManager::Tick(SoundEngine *soundEngine, eMUSIC_TYPE type)
{
    if (type != m_pendingType)
    {
        m_pendingType  = type;
        m_pendingTicks = 0;
    }

    bool voidChange = m_pendingType != m_currentType && (m_pendingType == eMusicType_Void || m_currentType == eMusicType_Void);
    if (voidChange)
    {
        bool wasPlaying = m_currentMusic != nullptr;
        if (m_currentMusic)
        {
            soundEngine->StopMusic();
            m_currentMusic = nullptr;
        }

        // We resume with a fresh song after the void only if something was playing before we fell in
        bool resume         = m_currentType == eMusicType_Void && m_playingBeforeVoid;
        m_playingBeforeVoid = wasPlaying;
        m_currentType       = m_pendingType;
        m_nextSongDelay     = resume ? 0 : m_random.NextInt(s_music[m_currentType].minDelay, s_music[m_currentType].maxDelay);
    }
    else if (m_pendingType != m_currentType && ++m_pendingTicks >= s_switchTicks)
    {
        m_currentType   = m_pendingType;
        m_nextSongDelay = m_random.NextInt(s_music[m_currentType].minDelay, s_music[m_currentType].maxDelay);
        if (m_currentMusic)
        {
            soundEngine->StopMusic();
            m_currentMusic  = nullptr;
            m_nextSongDelay = 0;
        }
    }

    const Music &music = s_music[m_currentType];
    if (m_currentMusic)
    {
        if (!soundEngine->IsPlaying(m_currentMusic))
        {
            m_currentMusic  = nullptr;
            m_nextSongDelay = std::min(m_nextSongDelay, m_random.NextInt(music.minDelay, music.maxDelay));
        }
    }

    m_nextSongDelay = std::min(m_nextSongDelay, music.maxDelay);
    if (!m_currentMusic && m_nextSongDelay-- <= 0)
    {
        StartPlaying(soundEngine, music);
    }
}

void MusicManager::StartPlaying(SoundEngine *soundEngine, const Music &music)
{
    std::vector<SoundEvent *> tracks = SoundRegistry::GetMusic(music.folder);
    for (int i = 0; i < 2 && music.sharedFolders[i]; i++)
    {
        std::vector<SoundEvent *> shared = SoundRegistry::GetMusic(music.sharedFolders[i]);
        tracks.insert(tracks.end(), shared.begin(), shared.end());
    }

    if (!tracks.empty())
    {
        SoundEvent *sound = tracks[(size_t) m_random.NextInt(0, (int) tracks.size() - 1)];
        soundEngine->PlayMusic(sound, 1.0f, 1.0f);
        m_currentMusic = sound;
        Logger::LogInfo("Playing music %s", sound->GetResourceLocation().ToString().c_str());
    }

    m_nextSongDelay = INT_MAX;
}

#endif
