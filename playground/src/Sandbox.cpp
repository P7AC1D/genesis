#include "Sandbox.h"

namespace Genesis {

    Sandbox::Sandbox()
        : Layer("Sandbox") {
    }

    Sandbox::~Sandbox() = default;

    void Sandbox::OnAttach() {
        GEN_INFO("Sandbox layer attached - Testing Genesis Engine features");

        // Setup camera
        m_Camera = Camera(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
        m_Camera.SetPosition(m_CameraPosition);

        SetupScene();
    }

    void Sandbox::OnDetach() {
        GEN_INFO("Sandbox layer detached");

        // Cleanup test meshes
        if (m_CubeMesh) m_CubeMesh->Shutdown();
        if (m_PlaneMesh) m_PlaneMesh->Shutdown();
        if (m_TreeMesh) m_TreeMesh->Shutdown();
        if (m_RockMesh) m_RockMesh->Shutdown();
    }

    void Sandbox::OnUpdate(float deltaTime) {
        m_FrameTime = deltaTime;
        m_FrameCount++;

        // Simple WASD camera controller
        float velocity = m_CameraSpeed * deltaTime;

        if (Input::IsKeyPressed(static_cast<int>(Key::W))) {
            m_CameraPosition += m_Camera.GetForward() * velocity;
        }
        if (Input::IsKeyPressed(static_cast<int>(Key::S))) {
            m_CameraPosition -= m_Camera.GetForward() * velocity;
        }
        if (Input::IsKeyPressed(static_cast<int>(Key::A))) {
            m_CameraPosition -= m_Camera.GetRight() * velocity;
        }
        if (Input::IsKeyPressed(static_cast<int>(Key::D))) {
            m_CameraPosition += m_Camera.GetRight() * velocity;
        }
        if (Input::IsKeyPressed(static_cast<int>(Key::Space))) {
            m_CameraPosition.y += velocity;
        }
        if (Input::IsKeyPressed(static_cast<int>(Key::LeftShift))) {
            m_CameraPosition.y -= velocity;
        }

        // Mouse look (when right mouse button is held)
        if (Input::IsMouseButtonPressed(static_cast<int>(Mouse::Right))) {
            glm::vec2 mousePos = Input::GetMousePosition();
            
            if (m_FirstMouse) {
                m_LastMousePosition = mousePos;
                m_FirstMouse = false;
            }

            float xOffset = (mousePos.x - m_LastMousePosition.x) * m_MouseSensitivity;
            float yOffset = (m_LastMousePosition.y - mousePos.y) * m_MouseSensitivity;

            m_LastMousePosition = mousePos;

            m_CameraRotation.y += xOffset;
            m_CameraRotation.x += yOffset;

            // Clamp pitch
            if (m_CameraRotation.x > 89.0f) m_CameraRotation.x = 89.0f;
            if (m_CameraRotation.x < -89.0f) m_CameraRotation.x = -89.0f;
        } else {
            m_FirstMouse = true;
        }

        m_Camera.SetPosition(m_CameraPosition);
        m_Camera.SetRotation(m_CameraRotation);

        // Update scene
        if (m_Scene) {
            m_Scene->OnUpdate(deltaTime);
        }
    }

    void Sandbox::OnRender() {
        auto& renderer = Application::Get().GetRenderer();
        
        renderer.BeginScene(m_Camera);

        // Render scene
        if (m_Scene) {
            m_Scene->OnRender(renderer);
        }

        // Render test meshes with transforms
        if (m_PlaneMesh) {
            glm::mat4 groundTransform = glm::scale(glm::mat4(1.0f), glm::vec3(50.0f, 1.0f, 50.0f));
            renderer.Submit(*m_PlaneMesh, groundTransform);
        }

        if (m_CubeMesh) {
            glm::mat4 cubeTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));
            renderer.Submit(*m_CubeMesh, cubeTransform);
        }

        // Render procedural trees
        if (m_TreeMesh) {
            for (int i = 0; i < 10; i++) {
                float x = (i - 5) * 3.0f;
                float z = -5.0f + (i % 3) * 2.0f;
                glm::mat4 treeTransform = glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.0f, z));
                renderer.Submit(*m_TreeMesh, treeTransform);
            }
        }

        // Render procedural rocks
        if (m_RockMesh) {
            for (int i = 0; i < 5; i++) {
                float x = (i - 2) * 2.5f + 0.5f;
                float z = 3.0f;
                glm::mat4 rockTransform = glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.15f, z));
                renderer.Submit(*m_RockMesh, rockTransform);
            }
        }

        renderer.EndScene();
    }

    void Sandbox::OnImGuiRender() {
        // Debug overlay would go here
        // - FPS counter
        // - Camera position
        // - Render stats
    }

    void Sandbox::OnEvent(Event& event) {
        // Handle events
    }

    void Sandbox::SetupScene() {
        m_Scene = std::make_shared<Scene>("Playground Scene");

        // Create test meshes using Vulkan device
        auto& device = Application::Get().GetRenderer().GetDevice();
        
        m_CubeMesh = Mesh::CreateCube(device, {0.8f, 0.2f, 0.2f});
        m_PlaneMesh = Mesh::CreatePlane(device, 1.0f, {0.3f, 0.7f, 0.3f});
        m_TreeMesh = Mesh::CreateLowPolyTree(device);
        m_RockMesh = Mesh::CreateLowPolyRock(device);

        // Create scene entities
        auto ground = m_Scene->CreateEntity("Ground");
        auto testCube = m_Scene->CreateEntity("Test Cube");
        auto mainCamera = m_Scene->CreateEntity("Main Camera");
        auto sun = m_Scene->CreateEntity("Sun");

        GenerateProceduralTerrain();
        PlaceProceduralObjects();

        GEN_INFO("Scene setup complete with {} entities", 4);
    }

    void Sandbox::GenerateProceduralTerrain() {
        // Future: Generate low-poly terrain using noise
        GEN_DEBUG("Procedural terrain generation placeholder");
    }

    void Sandbox::PlaceProceduralObjects() {
        // Future: Place trees, rocks, etc. using procedural rules
        GEN_DEBUG("Procedural object placement placeholder");
    }

}
