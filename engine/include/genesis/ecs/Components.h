#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>
#include <memory>

namespace Genesis {

    class Mesh;

    // Tag component for naming entities
    struct TagComponent {
        std::string Tag;

        TagComponent() = default;
        TagComponent(const std::string& tag) : Tag(tag) {}
    };

    // Transform component
    struct TransformComponent {
        glm::vec3 Position{0.0f, 0.0f, 0.0f};
        glm::vec3 Rotation{0.0f, 0.0f, 0.0f};  // Euler angles in radians
        glm::vec3 Scale{1.0f, 1.0f, 1.0f};

        TransformComponent() = default;
        TransformComponent(const glm::vec3& position) : Position(position) {}

        glm::mat4 GetTransform() const {
            glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));
            return glm::translate(glm::mat4(1.0f), Position)
                 * rotation
                 * glm::scale(glm::mat4(1.0f), Scale);
        }

        glm::vec3 GetForward() const {
            return glm::normalize(glm::vec3(
                sin(Rotation.y) * cos(Rotation.x),
                sin(Rotation.x),
                cos(Rotation.y) * cos(Rotation.x)
            ));
        }

        glm::vec3 GetRight() const {
            return glm::normalize(glm::cross(GetForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
        }

        glm::vec3 GetUp() const {
            return glm::normalize(glm::cross(GetRight(), GetForward()));
        }
    };

    // Mesh renderer component
    struct MeshRendererComponent {
        std::shared_ptr<Mesh> MeshData;
        glm::vec3 Color{1.0f, 1.0f, 1.0f};
        bool Visible = true;
        bool CastShadows = true;
        bool ReceiveShadows = true;

        MeshRendererComponent() = default;
        MeshRendererComponent(std::shared_ptr<Mesh> mesh) : MeshData(mesh) {}
    };

    // Camera component
    struct CameraComponent {
        bool Primary = true;
        bool FixedAspectRatio = false;
        float FOV = 45.0f;
        float NearClip = 0.1f;
        float FarClip = 1000.0f;

        CameraComponent() = default;
    };

    // Light components
    struct DirectionalLightComponent {
        glm::vec3 Direction{-0.2f, -1.0f, -0.3f};
        glm::vec3 Color{1.0f, 1.0f, 1.0f};
        float Intensity = 1.0f;
        bool CastShadows = true;

        DirectionalLightComponent() = default;
    };

    struct PointLightComponent {
        glm::vec3 Color{1.0f, 1.0f, 1.0f};
        float Intensity = 1.0f;
        float Radius = 10.0f;

        PointLightComponent() = default;
    };

    // For procedural generation
    struct ProceduralMeshComponent {
        enum class Type { None, Terrain, Tree, Rock, Water };
        Type MeshType = Type::None;
        int Seed = 0;
        float NoiseScale = 1.0f;
        int Subdivisions = 1;
        bool NeedsRegeneration = true;

        ProceduralMeshComponent() = default;
    };

    // Physics placeholder (for future)
    struct RigidbodyComponent {
        enum class BodyType { Static, Dynamic, Kinematic };
        BodyType Type = BodyType::Static;
        float Mass = 1.0f;
        float Drag = 0.0f;
        float AngularDrag = 0.05f;
        bool UseGravity = true;

        RigidbodyComponent() = default;
    };

    struct BoxColliderComponent {
        glm::vec3 Size{1.0f, 1.0f, 1.0f};
        glm::vec3 Offset{0.0f, 0.0f, 0.0f};
        bool IsTrigger = false;

        BoxColliderComponent() = default;
    };

}
