#include "genesis/renderer/Camera.h"

namespace Genesis {

    Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip)
        : m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip) {
        RecalculateProjectionMatrix();
        RecalculateViewMatrix();
    }

    void Camera::SetPerspective(float fov, float aspectRatio, float nearClip, float farClip) {
        m_IsOrthographic = false;
        m_FOV = fov;
        m_AspectRatio = aspectRatio;
        m_NearClip = nearClip;
        m_FarClip = farClip;
        RecalculateProjectionMatrix();
    }

    void Camera::SetOrthographic(float size, float nearClip, float farClip) {
        m_IsOrthographic = true;
        m_OrthographicSize = size;
        m_NearClip = nearClip;
        m_FarClip = farClip;
        RecalculateProjectionMatrix();
    }

    void Camera::SetPosition(const glm::vec3& position) {
        m_Position = position;
        RecalculateViewMatrix();
    }

    void Camera::SetRotation(const glm::vec3& rotation) {
        m_Rotation = rotation;
        RecalculateViewMatrix();
    }

    void Camera::LookAt(const glm::vec3& target, const glm::vec3& up) {
        m_ViewMatrix = glm::lookAt(m_Position, target, up);
    }

    glm::vec3 Camera::GetForward() const {
        glm::vec3 front;
        front.x = cos(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
        front.y = sin(glm::radians(m_Rotation.x));
        front.z = sin(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
        return glm::normalize(front);
    }

    glm::vec3 Camera::GetRight() const {
        return glm::normalize(glm::cross(GetForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }

    glm::vec3 Camera::GetUp() const {
        return glm::normalize(glm::cross(GetRight(), GetForward()));
    }

    void Camera::SetAspectRatio(float aspectRatio) {
        m_AspectRatio = aspectRatio;
        RecalculateProjectionMatrix();
    }

    void Camera::RecalculateViewMatrix() {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position)
            * glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f))
            * glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f))
            * glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        m_ViewMatrix = glm::inverse(transform);
    }

    void Camera::RecalculateProjectionMatrix() {
        if (m_IsOrthographic) {
            float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
            float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
            float orthoBottom = -m_OrthographicSize * 0.5f;
            float orthoTop = m_OrthographicSize * 0.5f;

            m_ProjectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_NearClip, m_FarClip);
        } else {
            m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
        }

        // Flip Y for Vulkan
        m_ProjectionMatrix[1][1] *= -1;
    }

}
