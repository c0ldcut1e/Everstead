#ifndef EVERSTEAD_DEDICATED_SERVER

#include "DimensionTransitionSound.h"

#include <algorithm>
#include <cmath>
#include <numbers>

DimensionTransitionSound::DimensionTransitionSound() : SynthSound(2)
{
    m_finished      = false;
    m_finishSeconds = 0.0;
    m_sample        = 0;
    m_envelope      = 0.0f;
    m_stability     = 1.0f;
    m_crackle       = 0.0f;
    m_crackleFilter = {};
    m_holdLeft      = 0.0f;
    m_holdRight     = 0.0f;
    m_holdCounter   = 0;
    m_pitchJump     = 1.0f;
    m_pitchHold     = 0;
    m_screechPhase  = 0.0f;
    m_screechFreq   = 0.0f;
    m_screechLeft   = 0;
    m_ringPhase     = 0.0f;
    m_cutLeft       = 0;
    m_historyPos    = 0;
    m_stutterStart  = 0;
    m_stutterLength = 0;
    m_stutterOffset = 0;
    m_stutterLeft   = 0;
    std::fill(m_dangerPhases, m_dangerPhases + 3, 0.0f);
    m_boomPhase  = 0.0f;
    m_openWhoosh = {};
    std::fill(m_dronePhases, m_dronePhases + 4, 0.0f);
    std::fill(m_shimmerPhases, m_shimmerPhases + 3, 0.0f);
    std::fill(m_rush, m_rush + 2, BandPass{});
    std::fill(m_closeWhoosh, m_closeWhoosh + 2, BandPass{});
}

void DimensionTransitionSound::Init()
{
    SynthSound::Init();
    m_history.resize(kHistoryFrames * 2);
}

void DimensionTransitionSound::Finish(double finishSeconds)
{
    m_finished      = true;
    m_finishSeconds = finishSeconds;
}

void DimensionTransitionSound::SetStability(float stability)
{
    m_stability = stability;
}

void DimensionTransitionSound::OnStart()
{
    if (!IsPlaying())
    {
        m_envelope = 0.0f;
    }

    m_sample   = 0;
    m_finished = false;
}

bool DimensionTransitionSound::HasMoreSamples() const
{
    return !m_finished || m_sample < (int64_t) ((m_finishSeconds + 1.5) * kSampleRate);
}

// Every layer follows the same timeline as the shader, so sample time lines up with the picture as long as the queue never starves
void DimensionTransitionSound::FillSamples(short *samples)
{
    const float kTau             = 2.0f * (float) std::numbers::pi;
    const float kDroneRatios[4]  = {1.0f, 1.003f, 1.498f, 2.004f};
    const float kDroneAmps[4]    = {0.3f, 0.25f, 0.12f, 0.1f};
    const float kShimmerFreqs[3] = {659.26f, 987.77f, 1318.51f};
    const float kShimmerRates[3] = {0.31f, 0.47f, 0.73f};
    const float kShimmerPans[3]  = {0.25f, 0.75f, 0.45f};

    for (int i = 0; i < kFrameCount; i++)
    {
        double time = (double) m_sample / kSampleRate;
        float t     = (float) time;
        m_sample++;

        float ramp     = 1.0f - expf(-t / 2.5f);
        float open     = std::min(t / 0.9f, 1.0f);
        float close    = 0.0f;
        float outLevel = 1.0f;
        if (m_finished)
        {
            float since = (float) (time - m_finishSeconds);
            close       = std::clamp(since, 0.0f, 1.0f);
            float fade  = std::clamp(since / 1.4f, 0.0f, 1.0f);
            outLevel    = 1.0f - fade * fade * (3.0f - 2.0f * fade);
        }

        // We smooth the level so restarting mid fade out doesn't click
        m_envelope += (std::min(t / 0.3f, 1.0f) * outLevel - m_envelope) * 0.002f;

        float instability = 1.0f - m_stability;
        float drone       = 0.0f;
        if (instability > 0.0f && --m_pitchHold <= 0)
        {
            const float kJumps[5] = {1.0f, 0.5f, 0.71f, 1.41f, 2.0f};
            m_pitchJump           = kJumps[(int) ((Noise() * 0.5f + 0.5f) * 4.99f)];
            m_pitchHold           = (int) ((0.04f + 0.3f * (Noise() * 0.5f + 0.5f)) * kSampleRate);
        }

        float droneFreq = 41.2f * (1.0f + 0.5f * ramp) * (instability > 0.0f ? m_pitchJump : 1.0f) *
                          exp2f(close * 0.5f + instability * 0.15f * sinf(kTau * 3.7f * t) * sinf(kTau * 0.41f * t));
        for (int j = 0; j < 4; j++)
        {
            drone += sinf(kTau * m_dronePhases[j]) * kDroneAmps[j];
            m_dronePhases[j] += droneFreq * kDroneRatios[j] / kSampleRate;
            m_dronePhases[j] -= floorf(m_dronePhases[j]);
        }

        float rushCutoff = 250.0f + 2200.0f * ramp + 300.0f * sinf(kTau * 0.23f * t);
        float rushLevel  = 2.0f * (0.25f + 0.75f * ramp);
        float left       = drone + Filter(&m_rush[0], Noise(), rushCutoff, 1.2f) * rushLevel;
        float right      = drone + Filter(&m_rush[1], Noise(), rushCutoff * 1.07f, 1.2f) * rushLevel;

        float shimmerLevel = 0.04f * ramp + 0.12f * sinf((float) std::numbers::pi * close);
        for (int j = 0; j < 3; j++)
        {
            float shimmer = sinf(kTau * m_shimmerPhases[j]) * (0.5f + 0.5f * sinf(kTau * kShimmerRates[j] * t)) * shimmerLevel;
            left += shimmer * (1.0f - kShimmerPans[j]);
            right += shimmer * kShimmerPans[j];
            m_shimmerPhases[j] += kShimmerFreqs[j] / kSampleRate;
            m_shimmerPhases[j] -= floorf(m_shimmerPhases[j]);
        }

        float boom = sinf(kTau * m_boomPhase) * expf(-2.5f * t) * std::min(t / 0.005f, 1.0f) * 0.6f;
        m_boomPhase += (30.0f + 90.0f * expf(-3.0f * t)) / kSampleRate;
        m_boomPhase -= floorf(m_boomPhase);
        left += boom;
        right += boom;

        if (instability > 0.0f)
        {
            if (Noise() > 1.0f - 0.003f * instability)
            {
                m_crackle = 1.0f;
            }

            float crackle = Filter(&m_crackleFilter, Noise(), 900.0f + 2500.0f * m_crackle, 0.7f) * m_crackle * instability * 6.0f;
            m_crackle *= 0.9985f;
            float dropout = 1.0f - instability * 0.85f * (sinf(kTau * 0.9f * t) * sinf(kTau * 2.3f * t) > 0.5f ? 1.0f : 0.0f);

            // These are a tritone and a minor second over a distorted sub, so the chord never resolves
            const float kDangerFreqs[3] = {27.5f, 155.56f, 164.81f};
            float siren                 = exp2f(0.25f * sinf(kTau * 0.6f * t));
            float danger                = 0.0f;
            for (int j = 0; j < 3; j++)
            {
                danger += sinf(kTau * m_dangerPhases[j]) * (j == 0 ? 1.0f : 0.35f);
                m_dangerPhases[j] += kDangerFreqs[j] * (j == 0 ? 1.0f : siren) / kSampleRate;
                m_dangerPhases[j] -= floorf(m_dangerPhases[j]);
            }

            danger = tanhf(danger * 3.0f) * 0.5f * instability * (0.6f + 0.4f * sinf(kTau * 1.3f * t));
            left   = tanhf((left * dropout + danger) * (1.0f + 2.0f * instability)) + crackle;
            right  = tanhf((right * dropout + danger) * (1.0f + 2.0f * instability)) + crackle;

            // We crush the bit depth and sample rate in bursts for a broken signal sound
            if (m_holdCounter <= 0)
            {
                m_holdLeft    = left;
                m_holdRight   = right;
                bool crushed  = sinf(kTau * 0.37f * t) * sinf(kTau * 1.7f * t) > 0.2f;
                m_holdCounter = crushed ? 1 + (int) (instability * 12.0f) : 1;
            }

            m_holdCounter--;
            float levels = 4.0f + 60.0f * m_stability;
            left         = roundf(m_holdLeft * levels) / levels;
            right        = roundf(m_holdRight * levels) / levels;
        }

        if (open < 1.0f)
        {
            float whoosh = Filter(&m_openWhoosh, Noise(), 300.0f + 2700.0f * open, 2.0f) * sinf((float) std::numbers::pi * open) * 3.0f;
            left += whoosh;
            right += whoosh;
        }

        if (close > 0.0f && close < 1.0f)
        {
            float closeCutoff = 6000.0f - 5200.0f * close;
            float closeLevel  = sinf((float) std::numbers::pi * close) * 3.0f;
            left += Filter(&m_closeWhoosh[0], Noise(), closeCutoff, 2.5f) * closeLevel;
            right += Filter(&m_closeWhoosh[1], Noise(), closeCutoff * 0.94f, 2.5f) * closeLevel;
        }

        if (instability > 0.0f)
        {
            if (m_screechLeft <= 0 && Noise() > 1.0f - 0.00006f * instability)
            {
                m_screechLeft = (int) ((0.15f + 0.4f * (Noise() * 0.5f + 0.5f)) * kSampleRate);
                m_screechFreq = 1800.0f + 2400.0f * (Noise() * 0.5f + 0.5f);
            }

            if (m_screechLeft > 0)
            {
                m_screechLeft--;
                m_screechFreq *= 1.0f + 0.00003f * Noise();
                float screech = (fmodf(m_screechPhase, 1.0f) * 2.0f - 1.0f) * 0.25f * instability;
                m_screechPhase += m_screechFreq * (1.0f + 0.02f * sinf(kTau * 31.0f * t)) / kSampleRate;
                m_screechPhase -= floorf(m_screechPhase);
                left += screech;
                right -= screech;
            }

            float ring = sinf(kTau * m_ringPhase);
            m_ringPhase += (70.0f + 400.0f * (0.5f + 0.5f * sinf(kTau * 0.21f * t))) / kSampleRate;
            m_ringPhase -= floorf(m_ringPhase);
            left  = left * (1.0f - 0.5f * instability) + left * ring * 0.5f * instability;
            right = right * (1.0f - 0.5f * instability) - right * ring * 0.5f * instability;

            m_history[m_historyPos * 2]     = left;
            m_history[m_historyPos * 2 + 1] = right;
            m_historyPos                    = (m_historyPos + 1) % kHistoryFrames;

            // We loop a short slice of what just played so the sound stutters like a skipping signal
            if (m_stutterLeft <= 0 && Noise() > 1.0f - 0.00015f * instability)
            {
                m_stutterLength = (int) ((0.015f + 0.1f * (Noise() * 0.5f + 0.5f)) * kSampleRate);
                m_stutterStart  = (m_historyPos - m_stutterLength + kHistoryFrames) % kHistoryFrames;
                m_stutterOffset = 0;
                m_stutterLeft   = m_stutterLength * (2 + (int) ((Noise() * 0.5f + 0.5f) * 6.0f));
            }

            if (m_stutterLeft > 0)
            {
                int index       = (m_stutterStart + m_stutterOffset) % kHistoryFrames;
                left            = m_history[index * 2];
                right           = m_history[index * 2 + 1];
                m_stutterOffset = (m_stutterOffset + 1) % m_stutterLength;
                m_stutterLeft--;
            }

            if (m_cutLeft <= 0 && Noise() > 1.0f - 0.0001f * instability)
            {
                m_cutLeft = (int) ((0.02f + 0.08f * (Noise() * 0.5f + 0.5f)) * kSampleRate);
            }

            if (m_cutLeft > 0)
            {
                m_cutLeft--;
                left  = 0.0f;
                right = 0.0f;
            }
        }

        samples[i * 2]     = (short) (tanhf(left * m_envelope * 0.6f) * 0.52f * 32767.0f);
        samples[i * 2 + 1] = (short) (tanhf(right * m_envelope * 0.6f) * 0.52f * 32767.0f);
    }
}

#endif
