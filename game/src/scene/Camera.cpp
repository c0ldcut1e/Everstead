#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera()
{
    m_position = {0.0, 0.0, 0.0};
    m_front    = {0.0, 0.0, -1.0};
    m_up       = {0.0, 1.0, 0.0};
}

void Camera::SetPosition(const glm::dvec3 &position)
{
    m_position = position;
}

const glm::dvec3 &Camera::GetPosition() const
{
    return m_position;
}

const glm::dvec3 &Camera::GetFront() const
{
    return m_front;
}

void Camera::SetDirection(const glm::dvec3 &front, const glm::dvec3 &up)
{
    m_front = front;
    m_up    = up;
}

glm::dmat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(glm::dvec3(0.0, 0.0, 0.0), m_front, m_up);
}

#endif
