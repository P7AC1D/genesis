#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Genesis {

    class Camera {
    public:
        Camera() = default;
        Camera(float fov, float aspectRatio, float nearClip, float farClip);

        void SetPerspective(float fov, float aspectRatio, float nearClip, float farClip);
        void SetOrthographic(float size, float nearClip, float farClip);

        void SetPosition(const glm::vec3& position);
        void SetRotation(const glm::vec3& rotation);

        void LookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

        const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
        const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
        glm::mat4 GetViewProjectionMatrix() const { return m_ProjectionMatrix * m_ViewMatrix; }

        const glm::vec3& GetPosition() const { return m_Position; }
        const glm::vec3& GetRotation() const { return m_Rotation; }

        glm::vec3 GetForward() const;
        glm::vec3 GetRight() const;
        glm::vec3 GetUp() const;

        float GetFOV() const { return m_FOV; }
        float GetAspectRatio() const { return m_AspectRatio; }
        float GetNearClip() const { return m_NearClip; }
        float GetFarClip() const { return m_FarClip; }

        void SetAspectRatio(float aspectRatio);

    private:
        void RecalculateViewMatrix();
        void RecalculateProjectionMatrix();

    private:
        glm::mat4 m_ProjectionMatrix{1.0f};
        glm::mat4 m_ViewMatrix{1.0f};

        glm::vec3 m_Position{0.0f, 0.0f, 0.0f};
        glm::vec3 m_Rotation{0.0f, 0.0f, 0.0f};

        float m_FOV = 45.0f;
        float m_AspectRatio = 1.778f;
        float m_NearClip = 0.1f;
        float m_FarClip = 1000.0f;

        bool m_IsOrthographic = false;
        float m_OrthographicSize = 10.0f;
    };

}
