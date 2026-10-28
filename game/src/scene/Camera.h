#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Camera
{
public:
    Camera();

    void SetPosition(const glm::dvec3 &position);
    const glm::dvec3 &GetPosition() const;

    const glm::dvec3 &GetFront() const;

    void SetDirection(const glm::dvec3 &front, const glm::dvec3 &up);

    glm::dmat4 GetViewMatrix() const;

private:
    glm::dvec3 m_position;
    glm::dvec3 m_front;
    glm::dvec3 m_up;
};
