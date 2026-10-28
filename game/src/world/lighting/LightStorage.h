#pragma once

class LightStorage
{
public:
    LightStorage();

    void Reset(float skyLightClamp);
    void Update(float targetSkyLightClamp, double delta, bool smooth);

    float GetSkyLightClamp() const;

private:
    float m_skyLightClamp;
    bool m_initialized;
};
