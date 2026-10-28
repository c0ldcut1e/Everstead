#pragma once

#include <cstdint>
#include <vector>

#include "SynthSound.h"

class DimensionTransitionSound : public SynthSound
{
public:
    DimensionTransitionSound();

    void Init();
    void Finish(double finishSeconds);
    void SetStability(float stability);

private:
    void OnStart() override;
    bool HasMoreSamples() const override;
    void FillSamples(short *samples) override;

    static constexpr int kHistoryFrames = 8192;

    bool m_finished;
    double m_finishSeconds;
    int64_t m_sample;
    float m_envelope;
    float m_stability;
    float m_crackle;
    float m_dangerPhases[3];
    float m_holdLeft;
    float m_holdRight;
    int m_holdCounter;
    float m_pitchJump;
    int m_pitchHold;
    float m_screechPhase;
    float m_screechFreq;
    int m_screechLeft;
    float m_ringPhase;
    int m_cutLeft;
    std::vector<float> m_history;
    int m_historyPos;
    int m_stutterStart;
    int m_stutterLength;
    int m_stutterOffset;
    int m_stutterLeft;
    float m_dronePhases[4];
    float m_shimmerPhases[3];
    float m_boomPhase;
    BandPass m_rush[2];
    BandPass m_openWhoosh;
    BandPass m_closeWhoosh[2];
    BandPass m_crackleFilter;
};
