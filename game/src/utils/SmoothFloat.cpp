#include "SmoothFloat.h"

#include <algorithm>

#include <glm/common.hpp>

SmoothFloat::SmoothFloat()
{
    m_value    = 0.0f;
    m_hasValue = false;
}

SmoothFloat::SmoothFloat(float value)
{
    m_value    = value;
    m_hasValue = true;
}

float SmoothFloat::Update(float target, float amount)
{
    amount = std::clamp(amount, 0.0f, 1.0f);
    if (!m_hasValue)
    {
        m_value    = target;
        m_hasValue = true;
        return m_value;
    }

    m_value = glm::mix(m_value, target, amount);
    return m_value;
}

void SmoothFloat::Reset(float value)
{
    m_value    = value;
    m_hasValue = true;
}

float SmoothFloat::GetValue() const
{
    return m_value;
}
