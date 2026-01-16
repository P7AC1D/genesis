#include "Sandbox.h"
#include <genesis/renderer/VulkanDevice.h>

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
        m_Camera.SetRotation(m_CameraRotation);

        SetupScene();
    }

    void Sandbox::OnDetach() {
        GEN_INFO("Sandbox layer detached");

        // Wait for GPU to finish before cleanup
        Application::Get().GetRenderer().GetDevice().WaitIdle();

        // Shutdown chunk manager first
        m_ChunkManager.Shutdown();

        // Cleanup meshes
        if (m_RockMesh) {
            m_RockMesh->Shutdown();
            m_RockMesh.reset();
        }
        if (m_TreeMesh) {
            m_TreeMesh->Shutdown();
            m_TreeMesh.reset();
        }
        if (m_CubeMesh) {
            m_CubeMesh->Shutdown();
            m_CubeMesh.reset();
        }
    }

    void Sandbox::OnUpdate(float deltaTime) {
        m_FrameTime = deltaTime;
        m_FrameCount++;
        m_TotalTime += deltaTime;  // Track total time for water animation

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

        // Time of day controls: T to advance, Shift+T to reverse
        if (Input::IsKeyPressed(static_cast<int>(Key::T))) {
            if (Input::IsKeyPressed(static_cast<int>(Key::LeftShift))) {
                m_TimeOfDay -= deltaTime * 2.0f;  // 2 hours per second backwards
            } else {
                m_TimeOfDay += deltaTime * 2.0f;  // 2 hours per second forward
            }
            if (m_TimeOfDay < 0) m_TimeOfDay += 24.0f;
            if (m_TimeOfDay >= 24.0f) m_TimeOfDay -= 24.0f;
            
            Application::Get().GetRenderer().GetLightManager().SetTimeOfDay(m_TimeOfDay);
        }

        m_Camera.SetPosition(m_CameraPosition);
        m_Camera.SetRotation(m_CameraRotation);
        
        // Update renderer time for water animation
        Application::Get().GetRenderer().SetTime(m_TotalTime);

        // Update chunk manager based on camera position
        m_ChunkManager.Update(m_CameraPosition);

        // Update scene
        if (m_Scene) {
            m_Scene->OnUpdate(deltaTime);
        }
    }

    void Sandbox::OnRender() {
        auto& renderer = Application::Get().GetRenderer();
        
        // Only render if frame started successfully
        if (!renderer.IsFrameInProgress()) return;

        renderer.BeginScene(m_Camera);

        // Render all loaded terrain chunks
        m_ChunkManager.Render(renderer);

        // Render a marker cube at origin
        if (m_CubeMesh) {
            glm::mat4 cubeTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, 0.0f));
            renderer.Draw(*m_CubeMesh, cubeTransform);
        }

        // Render trees from all loaded chunks
        if (m_TreeMesh) {
            for (const auto& pos : m_ChunkManager.GetAllTreePositions()) {
                glm::mat4 treeTransform = glm::translate(glm::mat4(1.0f), pos);
                renderer.Draw(*m_TreeMesh, treeTransform);
            }
        }

        // Render rocks from all loaded chunks
        if (m_RockMesh) {
            for (const auto& pos : m_ChunkManager.GetAllRockPositions()) {
                glm::mat4 rockTransform = glm::translate(glm::mat4(1.0f), pos);
                renderer.Draw(*m_RockMesh, rockTransform);
            }
        }

        renderer.EndScene();
    }

    void Sandbox::OnImGuiRender() {
        // Debug overlay would go here
    }

    void Sandbox::OnEvent(Event& event) {
        // Handle events
    }

    void Sandbox::SetupScene() {
        m_Scene = std::make_shared<Scene>("Playground Scene");

        auto& device = Application::Get().GetRenderer().GetDevice();
        
        // Create shared object meshes
        m_CubeMesh = Mesh::CreateCube(device, {0.8f, 0.2f, 0.2f});
        m_TreeMesh = Mesh::CreateLowPolyTree(device);
        m_RockMesh = Mesh::CreateLowPolyRock(device);

        // Initialize chunk-based world with biomes and water
        WorldSettings worldSettings;
        worldSettings.chunkSize = 32;
        worldSettings.cellSize = 1.0f;
        worldSettings.viewDistance = 3;
        worldSettings.seed = 42;
        worldSettings.seaLevel = 2.0f;
        worldSettings.waterEnabled = true;
        
        // Enable biome system
        worldSettings.biomesEnabled = true;
        worldSettings.temperatureScale = 0.003f;  // Larger biomes
        worldSettings.moistureScale = 0.004f;
        
        worldSettings.terrainSettings.heightScale = 8.0f;
        worldSettings.terrainSettings.noiseScale = 0.04f;
        worldSettings.terrainSettings.octaves = 5;
        worldSettings.terrainSettings.persistence = 0.5f;
        worldSettings.terrainSettings.lacunarity = 2.0f;
        worldSettings.terrainSettings.flatShading = true;
        worldSettings.terrainSettings.useHeightColors = true;
        
        // Domain warping for more organic terrain shapes
        worldSettings.terrainSettings.useWarp = true;
        worldSettings.terrainSettings.warpStrength = 0.4f;  // Moderate distortion
        worldSettings.terrainSettings.warpScale = 0.5f;     // Warp at half terrain scale
        worldSettings.terrainSettings.warpLevels = 2;       // Two levels of warping
        
        m_ChunkManager.Initialize(device, worldSettings);

        // Initial chunk load
        m_ChunkManager.Update(m_CameraPosition);

        // Setup lighting
        SetupLighting();

        GEN_INFO("Scene setup complete with biome-based world ({}x{} chunks visible)", 
            worldSettings.viewDistance * 2 + 1, worldSettings.viewDistance * 2 + 1);
    }

    void Sandbox::SetupLighting() {
        auto& lightManager = Application::Get().GetRenderer().GetLightManager();
        
        // Set initial time of day
        lightManager.SetTimeOfDay(m_TimeOfDay);
        
        // Add a few point lights for testing
        Light campfireLight;
        campfireLight.Position = glm::vec3(5.0f, 2.0f, 5.0f);
        campfireLight.Color = glm::vec3(1.0f, 0.6f, 0.2f);  // Orange/fire color
        campfireLight.Intensity = 2.0f;
        lightManager.AddPointLight(campfireLight);
        
        // Enable subtle fog for atmosphere
        lightManager.SetFogDensity(0.005f);
        
        GEN_INFO("Lighting setup complete - Time of day: {}:00, Press T to change time", static_cast<int>(m_TimeOfDay));
    }

}
