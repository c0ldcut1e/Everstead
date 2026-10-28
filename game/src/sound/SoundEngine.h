#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../core/Options.h"
#include "DimensionTransitionSound.h"
#include "SoundAcoustics.h"
#include "SoundEvent.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <glm/vec3.hpp>

extern "C" {
typedef struct stb_vorbis stb_vorbis;
}

class SoundEngine
{
public:
    SoundEngine();
    ~SoundEngine();

    void Init(Options *options);
    void Destroy();
    void Tick(float deltaSeconds);

    ALuint LoadBuffer(const SoundEvent *sound);
    void PlaySound(SoundEvent *sound, float volume, float pitch);
    void PlayUISound(SoundEvent *sound, float volume, float pitch);
    void PlaySound3D(SoundEvent *sound, const glm::dvec3 &position, float volume, float pitch);
    void PlayLoop3D(SoundEvent *sound, const glm::dvec3 &position, float volume);
    void PlayStreamingSound(SoundEvent *sound, float volume, float pitch);
    void PlayMusic(SoundEvent *sound, float volume, float pitch);

    void StopAllSounds();
    void StopWorldSounds();
    void StopMusic();
    bool IsPlaying(SoundEvent *sound) const;
    void StopSounds(const std::string &source, const std::string &soundId);

    void SetListenerPosition(const glm::dvec3 &position);
    void SetListenerOrientation(const glm::dvec3 &forward, const glm::dvec3 &up);

    uint32_t GetMaxPlayingSounds() const;
    void SetMaxPlayingSounds(uint32_t maxPlayingSounds);

    bool IsAcousticsActive() const;
    const AcousticEnvironment &GetAcousticEnvironment() const;
    const SoundAcoustics &GetAcoustics() const;
    DimensionTransitionSound *GetDimensionTransitionSound();

private:
    struct ActiveSound
    {
        ALuint source;
        SoundEvent *sound;
        float volume;
        float fade;
        float fadeSpeed;
        bool stopping;
        bool streaming;
        bool music;
        bool ui;
        bool finished;
        bool positional;
        bool looping;
        float idleSeconds;
        glm::dvec3 position;
        float occlusion;
        float targetOcclusion;
        float appliedOcclusion;
        stb_vorbis *decoder;
        std::vector<uint8_t> streamBytes;
        std::vector<ALuint> streamBuffers;
        std::vector<short> streamSamples;
        int streamChannels;
        int streamSampleRate;
    };

    void PlaySource(SoundEvent *sound, float volume, float pitch, const glm::dvec3 *position, bool forceStreaming, bool music, bool ui, bool looping);
    void AddActiveSound(ActiveSound &activeSound);
    void RemoveActiveSound(size_t index, const char *reason);
    void LogMusic(ActiveSound *activeSound, const char *event, const char *reason) const;
    void CheckDeviceConnected();
    void RemoveFinishedSounds();
    void UpdateVolumes(float deltaSeconds);
    void FadeOut(ActiveSound *activeSound);
    void EnsureSourceCapacity();
    bool FillStreamBuffer(ActiveSound *activeSound, ALuint buffer);
    void UpdateStreamingSound(ActiveSound *activeSound);
    float GetOptionVolume(const ActiveSound *activeSound) const;
    float GetCategoryVolume(eSOUND_CATEGORY category) const;
    ALenum GetFormatForChannels(int channels) const;
    void InitEfx();
    void UpdateAcoustics(float deltaSeconds);
    void ApplyAcoustics(ALuint source, float occlusion);
    void ClearAcoustics();

    static const int s_streamBufferCount;
    static const float s_musicFadeInSeconds;
    static const float s_musicFadeOutSeconds;
    static const int s_streamBufferSampleCount;

    Options *m_options;
    ALCdevice *m_device;
    bool m_deviceConnected;
    ALCcontext *m_context;
    bool m_initialized;
    uint32_t m_maxPlayingSounds;
    std::vector<ActiveSound> m_activeSounds;
    glm::dvec3 m_listenerPosition;
    bool m_efxSupported;
    bool m_acousticsActive;
    float m_acousticsTimer;
    SoundAcoustics m_acoustics;
    AcousticEnvironment m_environment;
    AcousticEnvironment m_appliedEnvironment;
    ALuint m_reverbSlot;
    ALuint m_reverbEffect;
    ALuint m_echoSlot;
    ALuint m_echoEffect;
    ALuint m_filter;
    std::unordered_map<const SoundEvent *, ALuint> m_buffers;
    DimensionTransitionSound m_dimensionTransitionSound;
};
