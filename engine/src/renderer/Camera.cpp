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
        // Standard FPS camera: yaw rotates around Y, pitch rotates around X
        // At rotation (0,0,0), forward is -Z
        float yaw = glm::radians(m_Rotation.y);
        float pitch = glm::radians(m_Rotation.x);
        
        glm::vec3 front;
        front.x = sin(yaw) * cos(pitch);
        front.y = -sin(pitch);
        front.z = -cos(yaw) * cos(pitch);
        return glm::normalize(front);
    }

    glm::vec3 Camera::GetRight() const {
        // Right is perpendicular to forward on the XZ plane (yaw only)
        float yaw = glm::radians(m_Rotation.y);
        return glm::vec3(cos(yaw), 0.0f, sin(yaw));
    }

    glm::vec3 Camera::GetUp() const {
        return glm::normalize(glm::cross(GetRight(), GetForward()));
    }

    void Camera::SetAspectRatio(float aspectRatio) {
        m_AspectRatio = aspectRatio;
        RecalculateProjectionMatrix();
    }

    void Camera::RecalculateViewMatrix() {
        // Use lookAt for consistency with GetForward()
        glm::vec3 forward = GetForward();
        glm::vec3 target = m_Position + forward;
        m_ViewMatrix = glm::lookAt(m_Position, target, glm::vec3(0.0f, 1.0f, 0.0f));
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
