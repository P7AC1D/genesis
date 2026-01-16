#pragma once

#include <genesis/Genesis.h>
#include <memory>

namespace Genesis {

    // Sandbox layer for testing engine features
    class Sandbox : public Layer {
    public:
        Sandbox();
        ~Sandbox() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float deltaTime) override;
        void OnRender() override;
        void OnImGuiRender() override;
        void OnEvent(Event& event) override;

    private:
        void SetupScene();

    private:
        std::shared_ptr<Scene> m_Scene;
        Camera m_Camera;

        // Chunk-based world
        ChunkManager m_ChunkManager;

        // Shared meshes for objects
        std::unique_ptr<Mesh> m_CubeMesh;
        std::unique_ptr<Mesh> m_TreeMesh;
        std::unique_ptr<Mesh> m_RockMesh;

        // Camera controller state
        glm::vec3 m_CameraPosition{0.0f, 15.0f, 40.0f};
        glm::vec3 m_CameraRotation{-15.0f, 0.0f, 0.0f};
        float m_CameraSpeed = 15.0f;
        float m_MouseSensitivity = 0.1f;

        glm::vec2 m_LastMousePosition{0.0f, 0.0f};
        bool m_FirstMouse = true;

        // Debug info
        float m_FrameTime = 0.0f;
        int m_FrameCount = 0;
    };

}
