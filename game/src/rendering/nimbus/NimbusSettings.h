#pragma once

enum eNIMBUS_LIGHTING_MODE : int
{
    eNimbusLightingMode_Floodfill = 0,
    eNimbusLightingMode_Sharp
};

struct NimbusSettings
{
    bool enabled;
    bool blockLighting;
    bool clouds;
    bool fog;
    bool surfaceLights;
    bool volumetricLight;
    bool occlusion;
    eNIMBUS_LIGHTING_MODE lightingMode;

    int quality;
    int shadowSamples;
    float bounce;
    int volumeQuality;
    float volumeDistance;
    int sectionsPerFrame;
    int floodIterations;
    int sunSlotsPerFrame;
    int cloudSteps;

    float atmosphere;
    float airDensity;
    float anisotropy;
};
