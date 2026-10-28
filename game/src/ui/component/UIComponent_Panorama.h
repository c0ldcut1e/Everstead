#pragma once

#include <string>

#include "UIComponent.h"

class Texture;

class UIComponent_Panorama : public UIComponent
{
public:
    explicit UIComponent_Panorama(const std::string &folderPath);

    void Render() override;

    void SetYawSpeed(float yawSpeed);
    void SetPitchSpeed(float pitchSpeed);
    void SetPitchAmplitude(float pitchAmplitude);

private:
    void RotateFace(int face);
    void DrawFace(Texture *texture);
    void DrawDarkening(float width, float height);

    Texture *m_textures[6];
    float m_yaw;
    float m_pitchTime;
    float m_yawSpeed;
    float m_pitchSpeed;
    float m_pitchBase;
    float m_pitchAmplitude;
};
