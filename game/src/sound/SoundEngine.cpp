#ifndef EVERSTEAD_DEDICATED_SERVER

#include "SoundEngine.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <utility>

#define STB_VORBIS_HEADER_ONLY
#include "../stb_vorbis.c"
#undef STB_VORBIS_HEADER_ONLY

#include "../core/Everstead.h"
#include "../core/FrameProfiler.h"
#include "../core/Logger.h"

#define AL_ALEXT_PROTOTYPES
#include <AL/efx.h>
#include <glm/common.hpp>

const float SoundEngine::s_musicFadeInSeconds    = 2.0f;
const float SoundEngine::s_musicFadeOutSeconds   = 2.0f;
const int SoundEngine::s_streamBufferCount       = 8;
const int SoundEngine::s_streamBufferSampleCount = 16384;

SoundEngine::SoundEngine()
{
    m_options            = nullptr;
    m_device             = nullptr;
    m_context            = nullptr;
    m_initialized        = false;
    m_maxPlayingSounds   = 28;
    m_deviceConnected    = true;
    m_listenerPosition   = glm::dvec3(0.0);
    m_efxSupported       = false;
    m_acousticsActive    = false;
    m_acousticsTimer     = 0.0f;
    m_environment        = {};
    m_appliedEnvironment = {};
    m_reverbSlot         = 0;
    m_reverbEffect       = 0;
    m_echoSlot           = 0;
    m_echoEffect         = 0;
    m_filter             = 0;
}

SoundEngine::~SoundEngine()
{
    Destroy();
}

void SoundEngine::Init(Options *options)
{
    if (m_initialized)
    {
        Destroy();
    }

    m_options = options;
    m_device  = alcOpenDevice(nullptr);
    if (!m_device)
    {
        Logger::LogWarn("Failed to open OpenAL device");
        return;
    }

    // We ask for a big voice budget so constant rain never starves block and step sounds
    const ALCint attributes[] = {ALC_MONO_SOURCES, 247, ALC_STEREO_SOURCES, 8, ALC_MAX_AUXILIARY_SENDS, 2, 0};
    m_context                 = alcCreateContext(m_device, attributes);
    if (!m_context)
    {
        Logger::LogWarn("Failed to create OpenAL context");
        alcCloseDevice(m_device);
        m_device = nullptr;
        return;
    }

    if (!alcMakeContextCurrent(m_context))
    {
        Logger::LogWarn("Failed to make OpenAL context current");
        alcDestroyContext(m_context);
        alcCloseDevice(m_device);
        m_context = nullptr;
        m_device  = nullptr;
        return;
    }

    ALCint monoSources = 0;
    alcGetIntegerv(m_device, ALC_MONO_SOURCES, 1, &monoSources);
    m_maxPlayingSounds = (uint32_t) std::max(monoSources, 28);

    InitEfx();
    alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
    SetListenerPosition(glm::dvec3(0.0));
    SetListenerOrientation(glm::dvec3(0.0, 0.0, -1.0), glm::dvec3(0.0, 1.0, 0.0));
    m_dimensionTransitionSound.Init();

    m_initialized = true;
    Logger::LogInfo("SoundEngine initialized voices=%u efx=%d", m_maxPlayingSounds, (int) m_efxSupported);
}

void SoundEngine::Destroy()
{
    StopAllSounds();
    if (m_initialized)
    {
        m_dimensionTransitionSound.Destroy();
    }

    for (const auto &entry : m_buffers)
    {
        alDeleteBuffers(1, &entry.second);
    }

    m_buffers.clear();

    if (m_efxSupported)
    {
        alDeleteAuxiliaryEffectSlots(1, &m_reverbSlot);
        alDeleteAuxiliaryEffectSlots(1, &m_echoSlot);
        alDeleteEffects(1, &m_reverbEffect);
        alDeleteEffects(1, &m_echoEffect);
        alDeleteFilters(1, &m_filter);
        m_efxSupported    = false;
        m_acousticsActive = false;
    }

    if (m_context)
    {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(m_context);
        m_context = nullptr;
    }

    if (m_device)
    {
        alcCloseDevice(m_device);
        m_device = nullptr;
    }

    m_initialized = false;
    m_options     = nullptr;
}

void SoundEngine::Tick(float deltaSeconds)
{
    if (!m_initialized)
    {
        return;
    }

    RemoveFinishedSounds();
    UpdateVolumes(deltaSeconds);
    UpdateAcoustics(deltaSeconds);
    m_dimensionTransitionSound.Tick(GetCategoryVolume(eSoundCategory_Ambient));
    CheckDeviceConnected();
}

void SoundEngine::InitEfx()
{
    ALCint sends = 0;
    alcGetIntegerv(m_device, ALC_MAX_AUXILIARY_SENDS, 1, &sends);
    if (!alcIsExtensionPresent(m_device, "ALC_EXT_EFX") || sends < 2)
    {
        Logger::LogWarn("OpenAL EFX unavailable sends=%d, raytraced sound is disabled", sends);
        return;
    }

    alGenAuxiliaryEffectSlots(1, &m_reverbSlot);
    alGenAuxiliaryEffectSlots(1, &m_echoSlot);
    alGenEffects(1, &m_reverbEffect);
    alGenEffects(1, &m_echoEffect);
    alGenFilters(1, &m_filter);
    alEffecti(m_reverbEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
    alEffecti(m_echoEffect, AL_EFFECT_TYPE, AL_EFFECT_ECHO);
    alEffectf(m_echoEffect, AL_ECHO_DAMPING, 0.5f);
    alEffectf(m_echoEffect, AL_ECHO_FEEDBACK, 0.2f);
    alFilteri(m_filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
    alAuxiliaryEffectSlotf(m_reverbSlot, AL_EFFECTSLOT_GAIN, 0.0f);
    alAuxiliaryEffectSlotf(m_echoSlot, AL_EFFECTSLOT_GAIN, 0.0f);
    m_efxSupported = alGetError() == AL_NO_ERROR;
}

void SoundEngine::UpdateAcoustics(float deltaSeconds)
{
    const float traceInterval = 0.1f;

    Level *level = Everstead::GetInstance()->GetLevel();
    if (!m_efxSupported || !m_options->soundRaytracing || !level)
    {
        if (m_acousticsActive)
        {
            ClearAcoustics();
        }

        return;
    }

    m_acousticsActive = true;
    m_acousticsTimer -= deltaSeconds;
    if (m_acousticsTimer <= 0.0f)
    {
        FrameProfilerScope scope("SoundAcoustics");
        m_acousticsTimer               = traceInterval;
        AcousticEnvironment target     = m_acoustics.TraceEnvironment(level, m_listenerPosition);
        m_environment.enclosure        = glm::mix(m_environment.enclosure, target.enclosure, 0.35f);
        m_environment.reverbGain       = glm::mix(m_environment.reverbGain, target.reverbGain, 0.35f);
        m_environment.decayTime        = glm::mix(m_environment.decayTime, target.decayTime, 0.35f);
        m_environment.reflectionsDelay = glm::mix(m_environment.reflectionsDelay, target.reflectionsDelay, 0.35f);
        m_environment.lateDelay        = glm::mix(m_environment.lateDelay, target.lateDelay, 0.35f);
        m_environment.echoGain         = glm::mix(m_environment.echoGain, target.echoGain, 0.35f);
        m_environment.echoDelay        = glm::mix(m_environment.echoDelay, target.echoDelay, 0.35f);

        // Attaching an effect again resets its delay lines and clicks, so we only do it when the room really changed
        if (fabsf(m_environment.decayTime - m_appliedEnvironment.decayTime) > m_appliedEnvironment.decayTime * 0.2f ||
            fabsf(m_environment.reflectionsDelay - m_appliedEnvironment.reflectionsDelay) > 0.01f ||
            fabsf(m_environment.lateDelay - m_appliedEnvironment.lateDelay) > 0.01f)
        {
            m_appliedEnvironment.decayTime        = m_environment.decayTime;
            m_appliedEnvironment.reflectionsDelay = m_environment.reflectionsDelay;
            m_appliedEnvironment.lateDelay        = m_environment.lateDelay;
            alEffectf(m_reverbEffect, AL_REVERB_DECAY_TIME, std::max(m_environment.decayTime, 0.1f));
            alEffectf(m_reverbEffect, AL_REVERB_REFLECTIONS_DELAY, m_environment.reflectionsDelay);
            alEffectf(m_reverbEffect, AL_REVERB_LATE_REVERB_DELAY, m_environment.lateDelay);
            alAuxiliaryEffectSloti(m_reverbSlot, AL_EFFECTSLOT_EFFECT, (ALint) m_reverbEffect);
        }

        if (fabsf(m_environment.echoDelay - m_appliedEnvironment.echoDelay) > 0.02f)
        {
            m_appliedEnvironment.echoDelay = m_environment.echoDelay;
            alEffectf(m_echoEffect, AL_ECHO_DELAY, m_environment.echoDelay);
            alEffectf(m_echoEffect, AL_ECHO_LRDELAY, m_environment.echoDelay);
            alAuxiliaryEffectSloti(m_echoSlot, AL_EFFECTSLOT_EFFECT, (ALint) m_echoEffect);
        }

        alAuxiliaryEffectSlotf(m_reverbSlot, AL_EFFECTSLOT_GAIN, std::min(m_environment.reverbGain * m_options->soundReverb / 100.0f, 1.0f));
        alAuxiliaryEffectSlotf(m_echoSlot, AL_EFFECTSLOT_GAIN, std::min(m_environment.echoGain * m_options->soundEcho / 100.0f, 1.0f));

        for (size_t i = 0; i < m_activeSounds.size(); i++)
        {
            if (m_activeSounds[i].positional)
            {
                m_activeSounds[i].targetOcclusion =
                        m_acoustics.TraceOcclusion(level, m_activeSounds[i].position, m_listenerPosition) * m_options->soundMuffling / 100.0f;
            }
        }
    }

    float blend = std::min(deltaSeconds * 8.0f, 1.0f);
    for (size_t i = 0; i < m_activeSounds.size(); i++)
    {
        ActiveSound *activeSound = &m_activeSounds[i];
        if (activeSound->positional)
        {
            activeSound->occlusion = glm::mix(activeSound->occlusion, activeSound->targetOcclusion, blend);
            if (fabsf(activeSound->occlusion - activeSound->appliedOcclusion) > 0.02f)
            {
                activeSound->appliedOcclusion = activeSound->occlusion;
                ApplyAcoustics(activeSound->source, activeSound->occlusion);
            }
        }
    }
}

void SoundEngine::ApplyAcoustics(ALuint source, float occlusion)
{
    alFilterf(m_filter, AL_LOWPASS_GAIN, expf(-0.3f * occlusion));
    alFilterf(m_filter, AL_LOWPASS_GAINHF, std::max(expf(-1.2f * occlusion), 0.02f));
    alSourcei(source, AL_DIRECT_FILTER, (ALint) m_filter);

    // Reflected sound reaches the listener around walls, so sends lose less than the direct path
    alFilterf(m_filter, AL_LOWPASS_GAIN, expf(-0.15f * occlusion));
    alFilterf(m_filter, AL_LOWPASS_GAINHF, std::max(expf(-0.6f * occlusion), 0.05f));
    alSource3i(source, AL_AUXILIARY_SEND_FILTER, (ALint) m_reverbSlot, 0, (ALint) m_filter);
    alSource3i(source, AL_AUXILIARY_SEND_FILTER, (ALint) m_echoSlot, 1, (ALint) m_filter);
}

void SoundEngine::ClearAcoustics()
{
    for (size_t i = 0; i < m_activeSounds.size(); i++)
    {
        if (m_activeSounds[i].positional)
        {
            m_activeSounds[i].appliedOcclusion = -1.0f;
            alSourcei(m_activeSounds[i].source, AL_DIRECT_FILTER, AL_FILTER_NULL);
            alSource3i(m_activeSounds[i].source, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
            alSource3i(m_activeSounds[i].source, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 1, AL_FILTER_NULL);
        }
    }

    alAuxiliaryEffectSlotf(m_reverbSlot, AL_EFFECTSLOT_GAIN, 0.0f);
    alAuxiliaryEffectSlotf(m_echoSlot, AL_EFFECTSLOT_GAIN, 0.0f);
    m_acousticsActive = false;
}

bool SoundEngine::IsAcousticsActive() const
{
    return m_acousticsActive;
}

const AcousticEnvironment &SoundEngine::GetAcousticEnvironment() const
{
    return m_environment;
}

const SoundAcoustics &SoundEngine::GetAcoustics() const
{
    return m_acoustics;
}

DimensionTransitionSound *SoundEngine::GetDimensionTransitionSound()
{
    return &m_dimensionTransitionSound;
}

void SoundEngine::CheckDeviceConnected()
{
    if (!m_deviceConnected || !alcIsExtensionPresent(m_device, "ALC_EXT_disconnect"))
    {
        return;
    }

    ALCint connected = 1;
    alcGetIntegerv(m_device, alcGetEnumValue(m_device, "ALC_CONNECTED"), 1, &connected);
    if (!connected)
    {
        Logger::LogWarn("Audio device disconnected, every sound is now silent");
        m_deviceConnected = false;
    }
}

bool SoundEngine::IsPlaying(SoundEvent *sound) const
{
    for (size_t i = 0; i < m_activeSounds.size(); i++)
    {
        if (m_activeSounds[i].sound == sound && !m_activeSounds[i].stopping)
        {
            return true;
        }
    }

    return false;
}

// This advances music fades and reapplies the option volumes so moving a slider changes sounds that are already playing
void SoundEngine::UpdateVolumes(float deltaSeconds)
{
    for (size_t i = m_activeSounds.size(); i > 0; i--)
    {
        ActiveSound *activeSound = &m_activeSounds[i - 1];
        activeSound->fade        = std::clamp(activeSound->fade + activeSound->fadeSpeed * deltaSeconds, 0.0f, 1.0f);
        activeSound->idleSeconds += deltaSeconds;
        if (activeSound->looping && !activeSound->stopping && activeSound->idleSeconds > 3.0f)
        {
            FadeOut(activeSound);
        }

        if (activeSound->stopping && activeSound->fade <= 0.0f)
        {
            RemoveActiveSound(i - 1, "faded out");
            continue;
        }

        alSourcef(activeSound->source, AL_GAIN, activeSound->volume * activeSound->fade * GetOptionVolume(activeSound));
    }
}

void SoundEngine::FadeOut(ActiveSound *activeSound)
{
    activeSound->stopping  = true;
    activeSound->fadeSpeed = -1.0f / s_musicFadeOutSeconds;
}

void SoundEngine::PlaySound(SoundEvent *sound, float volume, float pitch)
{
    PlaySource(sound, volume, pitch, nullptr, false, false, false, false);
}

void SoundEngine::PlayUISound(SoundEvent *sound, float volume, float pitch)
{
    PlaySource(sound, volume, pitch, nullptr, false, false, true, false);
}

void SoundEngine::PlaySound3D(SoundEvent *sound, const glm::dvec3 &position, float volume, float pitch)
{
    PlaySource(sound, volume, pitch, &position, false, false, false, false);
}

// Callers keep a loop alive by calling this again, it moves to the closest reported position and fades out once reports stop
void SoundEngine::PlayLoop3D(SoundEvent *sound, const glm::dvec3 &position, float volume)
{
    for (size_t i = 0; i < m_activeSounds.size(); i++)
    {
        ActiveSound *activeSound = &m_activeSounds[i];
        if (activeSound->sound != sound || activeSound->stopping)
        {
            continue;
        }

        if (activeSound->idleSeconds > 0.5f || glm::distance(position, m_listenerPosition) < glm::distance(activeSound->position, m_listenerPosition))
        {
            activeSound->position = position;
            alSource3f(activeSound->source, AL_POSITION, (ALfloat) position.x, (ALfloat) position.y, (ALfloat) position.z);
        }

        activeSound->idleSeconds = 0.0f;
        return;
    }

    PlaySource(sound, volume, 1.0f, &position, false, false, false, true);
}

void SoundEngine::PlayStreamingSound(SoundEvent *sound, float volume, float pitch)
{
    PlaySource(sound, volume, pitch, nullptr, true, false, false, false);
}

void SoundEngine::PlayMusic(SoundEvent *sound, float volume, float pitch)
{
    StopMusic();
    PlaySource(sound, volume, pitch, nullptr, true, true, false, false);
}

void SoundEngine::StopAllSounds()
{
    for (size_t i = m_activeSounds.size(); i > 0; i--)
    {
        RemoveActiveSound(i - 1, "stop all");
    }
}

// This stops everything a world played but leaves UI and music running, so a button click survives leaving a world
void SoundEngine::StopWorldSounds()
{
    for (size_t i = m_activeSounds.size(); i > 0; i--)
    {
        if (!m_activeSounds[i - 1].ui && !m_activeSounds[i - 1].music)
        {
            RemoveActiveSound(i - 1, "world sounds stopped");
        }
    }
}

// Sounds have no category, so music is its own source and any other source means world sounds
void SoundEngine::StopSounds(const std::string &source, const std::string &soundId)
{
    bool anySource = source.empty() || source == "*" || source == "master";
    ResourceLocation location(soundId);
    for (size_t i = m_activeSounds.size(); i > 0; i--)
    {
        ActiveSound &activeSound = m_activeSounds[i - 1];
        if (activeSound.ui)
        {
            continue;
        }

        if (!anySource && activeSound.music != (source == "music"))
        {
            continue;
        }

        if (!soundId.empty())
        {
            const ResourceLocation &name = activeSound.sound->GetResourceLocation();
            const std::string &path      = name.GetPath();
            size_t end                   = path.size();
            while (end > 0 && path[end - 1] >= '0' && path[end - 1] <= '9')
            {
                end--;
            }

            if (name.GetNamespace() != location.GetNamespace() || (path != location.GetPath() && path.substr(0, end) != location.GetPath()))
            {
                continue;
            }
        }

        if (activeSound.music)
        {
            LogMusic(&activeSound, "Music fading out", "stopsound");
            FadeOut(&activeSound);
            continue;
        }

        RemoveActiveSound(i - 1, "stopsound");
    }
}

void SoundEngine::StopMusic()
{
    for (size_t i = m_activeSounds.size(); i > 0; i--)
    {
        if (m_activeSounds[i - 1].music)
        {
            LogMusic(&m_activeSounds[i - 1], "Music fading out", "stop music");
            FadeOut(&m_activeSounds[i - 1]);
        }
    }
}

void SoundEngine::SetListenerPosition(const glm::dvec3 &position)
{
    m_listenerPosition = position;
    if (!m_initialized)
    {
        return;
    }

    alListener3f(AL_POSITION, (ALfloat) position.x, (ALfloat) position.y, (ALfloat) position.z);
}

void SoundEngine::SetListenerOrientation(const glm::dvec3 &forward, const glm::dvec3 &up)
{
    if (!m_initialized)
    {
        return;
    }

    ALfloat orientation[] = {(ALfloat) forward.x, (ALfloat) forward.y, (ALfloat) forward.z, (ALfloat) up.x, (ALfloat) up.y, (ALfloat) up.z};
    alListenerfv(AL_ORIENTATION, orientation);
}

uint32_t SoundEngine::GetMaxPlayingSounds() const
{
    return m_maxPlayingSounds;
}

void SoundEngine::SetMaxPlayingSounds(uint32_t maxPlayingSounds)
{
    m_maxPlayingSounds = maxPlayingSounds;
    while (m_activeSounds.size() > (size_t) m_maxPlayingSounds)
    {
        RemoveActiveSound(0, "source limit");
    }
}

ALuint SoundEngine::LoadBuffer(const SoundEvent *sound)
{
    auto inserted = m_buffers.try_emplace(sound, 0);
    if (!inserted.second || sound->IsStreaming())
    {
        return inserted.first->second;
    }

    const File &file           = sound->GetFile();
    std::vector<uint8_t> bytes = file.ReadAllBytes();
    if (bytes.empty() || bytes.size() > INT_MAX)
    {
        Logger::LogWarn("Failed to read sound file path=%s", file.GetPath().c_str());
        return 0;
    }

    int channels    = 0;
    int sampleRate  = 0;
    short *samples  = nullptr;
    int sampleCount = stb_vorbis_decode_memory(bytes.data(), (int) bytes.size(), &channels, &sampleRate, &samples);
    if (sampleCount <= 0 || !samples)
    {
        Logger::LogWarn("Failed to decode sound path=%s", file.GetPath().c_str());
        free(samples);
        return 0;
    }

    // OpenAL only attenuates mono buffers, so stereo world sounds would play at full volume at any distance
    if (channels == 2)
    {
        for (int i = 0; i < sampleCount; i++)
        {
            samples[i] = (short) (((int) samples[i * 2] + (int) samples[i * 2 + 1]) / 2);
        }

        channels = 1;
    }

    if (channels != 1)
    {
        Logger::LogWarn("Unsupported sound channel count path=%s channels=%d", file.GetPath().c_str(), channels);
        free(samples);
        return 0;
    }

    int fadeFrames = std::min(sampleRate / 1000, sampleCount / 2);
    for (int i = 0; i < fadeFrames; i++)
    {
        float gain                   = (float) i / (float) fadeFrames;
        samples[i]                   = (short) (samples[i] * gain);
        samples[sampleCount - 1 - i] = (short) (samples[sampleCount - 1 - i] * gain);
    }

    ALuint buffer = 0;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, AL_FORMAT_MONO16, samples, sampleCount * (int) sizeof(short), sampleRate);
    free(samples);

    ALenum error = alGetError();
    if (error != AL_NO_ERROR)
    {
        Logger::LogWarn("OpenAL buffer upload failed path=%s error=%d", file.GetPath().c_str(), (int) error);
        alDeleteBuffers(1, &buffer);
        return 0;
    }

    Logger::LogInfo("Sound loaded name=%s path=%s", sound->GetResourceLocation().ToString().c_str(), file.GetPath().c_str());
    inserted.first->second = buffer;
    return buffer;
}

void SoundEngine::PlaySource(SoundEvent *sound, float volume, float pitch, const glm::dvec3 *position, bool forceStreaming, bool music, bool ui,
                             bool looping)
{
    if (!m_initialized || !sound)
    {
        return;
    }

    bool streaming = forceStreaming || sound->IsStreaming();
    ALuint buffer  = streaming ? 0 : LoadBuffer(sound);
    if (!streaming && !buffer)
    {
        return;
    }

    float maxDistance = std::max(16.0f * volume, 16.0f);
    if (position && glm::distance(*position, m_listenerPosition) > (double) maxDistance)
    {
        return;
    }

    EnsureSourceCapacity();
    if (m_activeSounds.size() >= (size_t) m_maxPlayingSounds)
    {
        return;
    }

    ActiveSound activeSound;
    activeSound.source           = 0;
    activeSound.sound            = sound;
    activeSound.volume           = std::clamp(volume, 0.0f, 1.0f);
    activeSound.stopping         = false;
    activeSound.streaming        = streaming;
    activeSound.music            = music || sound->IsMusic();
    activeSound.ui               = ui;
    activeSound.fade             = activeSound.music || looping ? 0.0f : 1.0f;
    activeSound.fadeSpeed        = activeSound.music || looping ? 1.0f / s_musicFadeInSeconds : 0.0f;
    activeSound.finished         = false;
    activeSound.positional       = position != nullptr;
    activeSound.looping          = looping;
    activeSound.idleSeconds      = 0.0f;
    activeSound.position         = position ? *position : glm::dvec3(0.0);
    activeSound.occlusion        = 0.0f;
    activeSound.targetOcclusion  = 0.0f;
    activeSound.appliedOcclusion = -1.0f;
    activeSound.decoder          = nullptr;
    activeSound.streamChannels   = 0;
    activeSound.streamSampleRate = 0;

    alGenSources(1, &activeSound.source);
    if (!activeSound.source)
    {
        return;
    }

    alSourcef(activeSound.source, AL_GAIN, activeSound.volume * activeSound.fade * GetOptionVolume(&activeSound));
    alSourcef(activeSound.source, AL_PITCH, std::min(std::max(pitch, 0.5f), 2.0f));

    if (position)
    {
        alSourcei(activeSound.source, AL_SOURCE_RELATIVE, AL_FALSE);
        alSource3f(activeSound.source, AL_POSITION, (ALfloat) position->x, (ALfloat) position->y, (ALfloat) position->z);
        // Sounds that reach past 64 blocks stay at full volume until the last 64 blocks of their range
        alSourcef(activeSound.source, AL_REFERENCE_DISTANCE, std::max(maxDistance - 64.0f, 1.0f));
        alSourcef(activeSound.source, AL_MAX_DISTANCE, maxDistance);
        alSourcef(activeSound.source, AL_ROLLOFF_FACTOR, 1.0f);

        // Effect sends ignore distance unless room rolloff is set, so far sounds would play at full volume
        alSourcef(activeSound.source, AL_ROOM_ROLLOFF_FACTOR, 1.0f);

        // We trace new sounds right away since short ones would finish before the next acoustics pass
        Level *level = Everstead::GetInstance()->GetLevel();
        if (m_acousticsActive && level)
        {
            FrameProfilerScope scope("SoundAcoustics");
            activeSound.occlusion        = m_acoustics.TraceOcclusion(level, *position, m_listenerPosition) * m_options->soundMuffling / 100.0f;
            activeSound.targetOcclusion  = activeSound.occlusion;
            activeSound.appliedOcclusion = activeSound.occlusion;
            ApplyAcoustics(activeSound.source, activeSound.occlusion);
        }
    }
    else
    {
        alSourcei(activeSound.source, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(activeSound.source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    }

    if (!streaming)
    {
        alSourcei(activeSound.source, AL_BUFFER, (ALint) buffer);
        alSourcei(activeSound.source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
        alSourcePlay(activeSound.source);
        AddActiveSound(activeSound);
        return;
    }

    activeSound.streamBytes = sound->GetFile().ReadAllBytes();
    if (activeSound.streamBytes.empty() || activeSound.streamBytes.size() > INT_MAX)
    {
        alDeleteSources(1, &activeSound.source);
        return;
    }

    int error           = 0;
    activeSound.decoder = stb_vorbis_open_memory(activeSound.streamBytes.data(), (int) activeSound.streamBytes.size(), &error, nullptr);
    if (!activeSound.decoder)
    {
        Logger::LogWarn("Failed to open stream path=%s error=%d", sound->GetFile().GetPath().c_str(), error);
        alDeleteSources(1, &activeSound.source);
        return;
    }

    stb_vorbis_info info         = stb_vorbis_get_info(activeSound.decoder);
    activeSound.streamChannels   = info.channels;
    activeSound.streamSampleRate = (int) info.sample_rate;
    activeSound.streamSamples.resize((size_t) s_streamBufferSampleCount * (size_t) activeSound.streamChannels);
    activeSound.streamBuffers.resize((size_t) s_streamBufferCount);
    alGenBuffers(s_streamBufferCount, activeSound.streamBuffers.data());

    int queued = 0;
    for (size_t i = 0; i < activeSound.streamBuffers.size(); i++)
    {
        if (FillStreamBuffer(&activeSound, activeSound.streamBuffers[i]))
        {
            queued++;
        }
    }

    if (queued <= 0)
    {
        stb_vorbis_close(activeSound.decoder);
        alDeleteBuffers((ALsizei) activeSound.streamBuffers.size(), activeSound.streamBuffers.data());
        alDeleteSources(1, &activeSound.source);
        return;
    }

    alSourceQueueBuffers(activeSound.source, queued, activeSound.streamBuffers.data());
    alSourcePlay(activeSound.source);
    if (activeSound.music)
    {
        LogMusic(&activeSound, "Music started", "music manager");
    }

    AddActiveSound(activeSound);
}

void SoundEngine::LogMusic(ActiveSound *activeSound, const char *event, const char *reason) const
{
    float position = 0.0f;
    float length   = 0.0f;
    if (activeSound->decoder && activeSound->streamSampleRate > 0)
    {
        position = (float) stb_vorbis_get_sample_offset(activeSound->decoder) / (float) activeSound->streamSampleRate;
        length   = (float) stb_vorbis_stream_length_in_samples(activeSound->decoder) / (float) activeSound->streamSampleRate;
    }

    ALint state = 0;
    alGetSourcei(activeSound->source, AL_SOURCE_STATE, &state);
    Logger::LogInfo("%s name=%s reason=%s decoded=%.1fs length=%.1fs fade=%.2f stopping=%d state=0x%x sounds=%d", event,
                    activeSound->sound->GetResourceLocation().ToString().c_str(), reason, position, length, activeSound->fade,
                    (int) activeSound->stopping, (int) state, (int) m_activeSounds.size());
}

void SoundEngine::AddActiveSound(ActiveSound &activeSound)
{
    m_activeSounds.push_back(std::move(activeSound));
}

void SoundEngine::RemoveActiveSound(size_t index, const char *reason)
{
    ActiveSound *activeSound = &m_activeSounds[index];
    if (activeSound->music)
    {
        LogMusic(activeSound, "Music removed", reason);
    }

    if (activeSound->source)
    {
        alSourceStop(activeSound->source);

        if (activeSound->streaming)
        {
            ALint queued = 0;
            alGetSourcei(activeSound->source, AL_BUFFERS_QUEUED, &queued);
            while (queued > 0)
            {
                ALuint buffer = 0;
                alSourceUnqueueBuffers(activeSound->source, 1, &buffer);
                queued--;
            }
        }
        else
        {
            alSourcei(activeSound->source, AL_BUFFER, 0);
        }

        alDeleteSources(1, &activeSound->source);
        activeSound->source = 0;
    }

    if (activeSound->decoder)
    {
        stb_vorbis_close(activeSound->decoder);
        activeSound->decoder = nullptr;
    }

    if (!activeSound->streamBuffers.empty())
    {
        alDeleteBuffers((ALsizei) activeSound->streamBuffers.size(), activeSound->streamBuffers.data());
        activeSound->streamBuffers.clear();
    }

    m_activeSounds.erase(m_activeSounds.begin() + (ptrdiff_t) index);
}

void SoundEngine::RemoveFinishedSounds()
{
    for (size_t i = m_activeSounds.size(); i > 0; i--)
    {
        ActiveSound *activeSound = &m_activeSounds[i - 1];
        if (activeSound->streaming)
        {
            UpdateStreamingSound(activeSound);
        }

        ALint state = AL_STOPPED;
        alGetSourcei(activeSound->source, AL_SOURCE_STATE, &state);
        if (!activeSound->streaming && state == AL_STOPPED)
        {
            RemoveActiveSound(i - 1, "finished");
        }
        else if (activeSound->streaming && activeSound->finished && state == AL_STOPPED)
        {
            RemoveActiveSound(i - 1, "stream ended");
        }
    }
}

void SoundEngine::EnsureSourceCapacity()
{
    // New sounds are dropped when every voice is busy because stealing the oldest one cuts off rain loops
    RemoveFinishedSounds();
}

bool SoundEngine::FillStreamBuffer(ActiveSound *activeSound, ALuint buffer)
{
    ALenum format = GetFormatForChannels(activeSound->streamChannels);
    if (format == 0)
    {
        return false;
    }

    int samples = stb_vorbis_get_samples_short_interleaved(activeSound->decoder, activeSound->streamChannels, activeSound->streamSamples.data(),
                                                           (int) activeSound->streamSamples.size());
    if (samples <= 0)
    {
        return false;
    }

    // We flush errors left by earlier AL calls so they aren't blamed on this upload and end the stream early
    alGetError();
    alBufferData(buffer, format, activeSound->streamSamples.data(), samples * activeSound->streamChannels * (int) sizeof(short),
                 activeSound->streamSampleRate);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR)
    {
        Logger::LogWarn("Stream buffer upload failed path=%s error=%d", activeSound->sound->GetFile().GetPath().c_str(), (int) error);
        return false;
    }

    return true;
}

void SoundEngine::UpdateStreamingSound(ActiveSound *activeSound)
{
    ALint processed = 0;
    alGetSourcei(activeSound->source, AL_BUFFERS_PROCESSED, &processed);

    while (processed > 0)
    {
        ALuint buffer = 0;
        alSourceUnqueueBuffers(activeSound->source, 1, &buffer);
        if (FillStreamBuffer(activeSound, buffer))
        {
            alSourceQueueBuffers(activeSound->source, 1, &buffer);
        }
        else if (!activeSound->finished)
        {
            activeSound->finished = true;
            if (activeSound->music)
            {
                LogMusic(activeSound, "Music stream reached its end", "decoder");
            }
        }

        processed--;
    }

    if (!activeSound->finished)
    {
        ALint queued = 0;
        ALint state  = AL_STOPPED;
        alGetSourcei(activeSound->source, AL_BUFFERS_QUEUED, &queued);
        alGetSourcei(activeSound->source, AL_SOURCE_STATE, &state);
        if (queued > 0 && state != AL_PLAYING)
        {
            if (activeSound->music)
            {
                LogMusic(activeSound, "Music stream underrun, restarting", "stalled");
            }

            alSourcePlay(activeSound->source);
        }
    }
}

float SoundEngine::GetOptionVolume(const ActiveSound *activeSound) const
{
    if (!m_options)
    {
        return 1.0f;
    }

    eSOUND_CATEGORY category = activeSound->ui ? eSoundCategory_Ui : activeSound->sound->GetCategory();
    // The void ambience plays through the music manager but follows the ambient slider
    if (activeSound->music && activeSound->sound->IsMusic())
    {
        category = eSoundCategory_Music;
    }

    return GetCategoryVolume(category);
}

float SoundEngine::GetCategoryVolume(eSOUND_CATEGORY category) const
{
    float master = std::clamp(m_options->soundVolume / 100.0f, 0.0f, 1.0f);
    return master * std::clamp(m_options->GetCategoryVolume(category) / 100.0f, 0.0f, 1.0f);
}

ALenum SoundEngine::GetFormatForChannels(int channels) const
{
    if (channels == 1)
    {
        return AL_FORMAT_MONO16;
    }

    if (channels == 2)
    {
        return AL_FORMAT_STEREO16;
    }

    return 0;
}

#endif
