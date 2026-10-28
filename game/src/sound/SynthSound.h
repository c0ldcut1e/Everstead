#pragma once

#include <cstdint>
#include <vector>

#include <AL/al.h>

class SynthSound
{
public:
    explicit SynthSound(int channels);
    virtual ~SynthSound() = default;

    void Init();
    void Destroy();
    void Tick(float volume);

    void Start();
    void Stop();
    bool IsPlaying() const;
    ALuint GetSource() const;

protected:
    struct BandPass
    {
        float ic1;
        float ic2;
    };

    float Filter(BandPass *filter, float input, float cutoff, float q);
    float Noise();

    virtual void OnStart();
    virtual bool HasMoreSamples() const;
    virtual void FillSamples(short *samples) = 0;

    static constexpr int kFrameCount = 1024;
    static constexpr int kSampleRate = 44100;

private:
    void FillBuffer(ALuint buffer);

    static constexpr int kBufferCount = 8;

    int m_channels;
    ALuint m_source;
    ALuint m_buffers[kBufferCount];
    bool m_playing;
    uint32_t m_noise;
    std::vector<short> m_samples;
};
