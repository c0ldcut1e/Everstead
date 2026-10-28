#ifndef EVERSTEAD_DEDICATED_SERVER

#include "SynthSound.h"

#include <algorithm>
#include <cmath>
#include <numbers>

SynthSound::SynthSound(int channels)
{
    m_channels = channels;
    m_source   = 0;
    m_playing  = false;
    m_noise    = 0x9E3779B9;
    std::fill(m_buffers, m_buffers + kBufferCount, 0);
}

void SynthSound::Init()
{
    alGenSources(1, &m_source);
    alGenBuffers(kBufferCount, m_buffers);
    if (m_channels == 2)
    {
        alSourcei(m_source, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(m_source, AL_POSITION, 0.0f, 0.0f, 0.0f);
        alSourcef(m_source, AL_ROLLOFF_FACTOR, 0.0f);
    }
    else
    {
        alSourcei(m_source, AL_SOURCE_RELATIVE, AL_FALSE);
        alSourcef(m_source, AL_REFERENCE_DISTANCE, 2.0f);
        alSourcef(m_source, AL_MAX_DISTANCE, 20.0f);
        alSourcef(m_source, AL_ROLLOFF_FACTOR, 1.0f);
    }

    m_samples.resize(kFrameCount * m_channels);
}

void SynthSound::Destroy()
{
    Stop();
    alDeleteSources(1, &m_source);
    alDeleteBuffers(kBufferCount, m_buffers);
    m_source = 0;
}

void SynthSound::Tick(float volume)
{
    if (!m_playing)
    {
        return;
    }

    alSourcef(m_source, AL_GAIN, volume);
    ALint processed = 0;
    alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processed);
    for (int i = 0; i < processed; i++)
    {
        ALuint buffer = 0;
        alSourceUnqueueBuffers(m_source, 1, &buffer);
        if (HasMoreSamples())
        {
            FillBuffer(buffer);
            alSourceQueueBuffers(m_source, 1, &buffer);
        }
    }

    ALint queued = 0;
    alGetSourcei(m_source, AL_BUFFERS_QUEUED, &queued);
    if (queued == 0)
    {
        Stop();
        return;
    }

    // A hitch longer than the queue starves the source, which stops it for good until we play it again
    ALint state = 0;
    alGetSourcei(m_source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING)
    {
        alSourcePlay(m_source);
    }
}

void SynthSound::Start()
{
    // There is no source when the audio device failed to open
    if (!m_source)
    {
        return;
    }

    OnStart();
    if (m_playing)
    {
        return;
    }

    m_playing = true;
    for (int i = 0; i < kBufferCount; i++)
    {
        FillBuffer(m_buffers[i]);
    }

    alSourceQueueBuffers(m_source, kBufferCount, m_buffers);
    alSourcePlay(m_source);
}

void SynthSound::Stop()
{
    if (!m_playing)
    {
        return;
    }

    alSourceStop(m_source);
    alSourcei(m_source, AL_BUFFER, 0);
    m_playing = false;
}

bool SynthSound::IsPlaying() const
{
    return m_playing;
}

ALuint SynthSound::GetSource() const
{
    return m_source;
}

float SynthSound::Filter(BandPass *filter, float input, float cutoff, float q)
{
    float g     = tanf((float) std::numbers::pi * cutoff / kSampleRate);
    float k     = 1.0f / q;
    float a1    = 1.0f / (1.0f + g * (g + k));
    float a2    = g * a1;
    float a3    = g * a2;
    float v3    = input - filter->ic2;
    float v1    = a1 * filter->ic1 + a2 * v3;
    float v2    = filter->ic2 + a2 * filter->ic1 + a3 * v3;
    filter->ic1 = 2.0f * v1 - filter->ic1;
    filter->ic2 = 2.0f * v2 - filter->ic2;
    return v1 * k;
}

float SynthSound::Noise()
{
    m_noise ^= m_noise << 13;
    m_noise ^= m_noise >> 17;
    m_noise ^= m_noise << 5;
    return (float) (int) m_noise / 2147483648.0f;
}

void SynthSound::OnStart()
{}

bool SynthSound::HasMoreSamples() const
{
    return true;
}

void SynthSound::FillBuffer(ALuint buffer)
{
    FillSamples(m_samples.data());
    alBufferData(buffer, m_channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, m_samples.data(), (ALsizei) (m_samples.size() * sizeof(short)),
                 kSampleRate);
}

#endif
