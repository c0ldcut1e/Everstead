#pragma once

#include "../utils/Random.h"

class SoundEngine;
class SoundEvent;

enum eMUSIC_TYPE
{
    eMusicType_Menu        = 0,
    eMusicType_Game        = 1,
    eMusicType_Cave        = 2,
    eMusicType_IceCaves    = 3,
    eMusicType_DesertCaves = 4,
    eMusicType_Void        = 5,
};

class MusicManager
{
public:
    MusicManager();

    void Update(SoundEngine *soundEngine, eMUSIC_TYPE type, float deltaSeconds);

private:
    struct Music
    {
        const char *folder;
        const char *sharedFolders[2];
        int minDelay;
        int maxDelay;
    };

    void Tick(SoundEngine *soundEngine, eMUSIC_TYPE type);
    void StartPlaying(SoundEngine *soundEngine, const Music &music);

    static const Music s_music[];
    static const int s_switchTicks;

    SoundEvent *m_currentMusic;
    eMUSIC_TYPE m_currentType;
    eMUSIC_TYPE m_pendingType;
    int m_pendingTicks;
    bool m_playingBeforeVoid;
    int m_nextSongDelay;
    float m_tickTime;
    Random m_random;
};
