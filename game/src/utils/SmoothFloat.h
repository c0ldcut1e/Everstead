#pragma once

class SmoothFloat
{
public:
    SmoothFloat();
    explicit SmoothFloat(float value);

    float Update(float target, float amount);
    void Reset(float value);

    float GetValue() const;

private:
    float m_value;
    bool m_hasValue;
};
