#include "Sandbox.h"
#include "TerrainSettingsPanel.h"
#include <genesis/renderer/VulkanDevice.h>
#include <imgui.h>

namespace Genesis
{

    Sandbox::Sandbox()
        : Layer("Sandbox")
    {
    }

    Sandbox::~Sandbox() = default;

    void Sandbox::OnAttach()
    {
        GEN_INFO("Terragen sandbox layer attached");

        // Setup camera
        m_Camera = Camera(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
        m_Camera.SetPosition(m_CameraPosition);
        m_Camera.SetRotation(m_CameraRotation);

        SetupScene();
    }

    void Sandbox::OnDetach()
    {
        GEN_INFO("Sandbox layer detached");

        // Wait for GPU to finish before cleanup
        Application::Get().GetRenderer().GetDevice().WaitIdle();

        // Shutdown terrain panel first
        if (m_TerrainPanel)
        {
            m_TerrainPanel->Shutdown();
            m_TerrainPanel.reset();
        }

        // Shutdown chunk manager first
        m_ChunkManager.Shutdown();

        // Cleanup meshes
        if (m_RockMesh)
        {
            m_RockMesh->Shutdown();
            m_RockMesh.reset();
        }
        if (m_TreeMesh)
        {
            m_TreeMesh->Shutdown();
            m_TreeMesh.reset();
        }
        if (m_CubeMesh)
        {
            m_CubeMesh->Shutdown();
            m_CubeMesh.reset();
        }
    }

    void Sandbox::OnUpdate(float deltaTime)
    {
        m_FrameTime = deltaTime;
        m_FrameCount++;
        m_TotalTime += deltaTime; // Track total time for water animation

        // Simple WASD camera controller
        float velocity = m_CameraSpeed * deltaTime;

        if (Input::IsKeyPressed(static_cast<int>(Key::W)))
        {
            m_CameraPosition += m_Camera.GetForward() * velocity;
        }
        if (Input::IsKeyPressed(static_cast<int>(Key::S)))
        {
            m_CameraPosition -= m_Camera.GetForward() * velocity;
        }
        if (Input::IsKeyPressed(static_cast<int>(Key::A)))
        {
            m_CameraPosition -= m_Camera.GetRight() * velocity;
        }
        if (Input::IsKeyPressed(static_cast<int>(Key::D)))
        {
            m_CameraPosition += m_Camera.GetRight() * velocity;
        }
        if (Input::IsKeyPressed(static_cast<int>(Key::Space)))
        {
            m_CameraPosition.y += velocity;
        }
        if (Input::IsKeyPressed(static_cast<int>(Key::LeftShift)))
        {
            m_CameraPosition.y -= velocity;
        }

        // Mouse look (when right mouse button is held)
        if (Input::IsMouseButtonPressed(static_cast<int>(Mouse::Right)))
        {
            glm::vec2 mousePos = Input::GetMousePosition();

            if (m_FirstMouse)
            {
                m_LastMousePosition = mousePos;
                m_FirstMouse = false;
            }

            float xOffset = (mousePos.x - m_LastMousePosition.x) * m_MouseSensitivity;
            float yOffset = (mousePos.y - m_LastMousePosition.y) * m_MouseSensitivity;

            m_LastMousePosition = mousePos;

            m_CameraRotation.y += xOffset;
            m_CameraRotation.x += yOffset;

            // Clamp pitch
            if (m_CameraRotation.x > 89.0f)
                m_CameraRotation.x = 89.0f;
            if (m_CameraRotation.x < -89.0f)
                m_CameraRotation.x = -89.0f;
        }
        else
        {
            m_FirstMouse = true;
        }

        // Time of day controls: T to advance, Shift+T to reverse
        if (Input::IsKeyPressed(static_cast<int>(Key::T)))
        {
            if (Input::IsKeyPressed(static_cast<int>(Key::LeftShift)))
            {
                m_TimeOfDay -= deltaTime * 2.0f; // 2 hours per second backwards
            }
            else
            {
                m_TimeOfDay += deltaTime * 2.0f; // 2 hours per second forward
            }
            if (m_TimeOfDay < 0)
                m_TimeOfDay += 24.0f;
            if (m_TimeOfDay >= 24.0f)
                m_TimeOfDay -= 24.0f;

            Application::Get().GetRenderer().GetLightManager().SetTimeOfDay(m_TimeOfDay);
        }

        // Toggle UI with Tab key (using simple debounce)
        static bool tabWasPressed = false;
        bool tabPressed = Input::IsKeyPressed(static_cast<int>(Key::Tab));
        if (tabPressed && !tabWasPressed)
        {
            m_ShowUI = !m_ShowUI;
        }
        tabWasPressed = tabPressed;

        m_Camera.SetPosition(m_CameraPosition);
        m_Camera.SetRotation(m_CameraRotation);

        // Update renderer time for water animation
        Application::Get().GetRenderer().SetTime(m_TotalTime);

        // Update chunk manager based on camera position
        m_ChunkManager.Update(m_CameraPosition);

        // Update scene
        if (m_Scene)
        {
            m_Scene->OnUpdate(deltaTime);
        }
    }

    void Sandbox::OnRender()
    {
        auto &renderer = Application::Get().GetRenderer();

        // Only render if frame started successfully
        if (!renderer.IsFrameInProgress())
            return;

        renderer.BeginScene(m_Camera);

        // Render all loaded terrain chunks
        m_ChunkManager.Render(renderer);

        // Render a marker cube at origin
        if (m_CubeMesh)
        {
            glm::mat4 cubeTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, 0.0f));
            renderer.Draw(*m_CubeMesh, cubeTransform);
        }

        // Render trees from all loaded chunks
        if (m_TreeMesh)
        {
            for (const auto &pos : m_ChunkManager.GetAllTreePositions())
            {
                glm::mat4 treeTransform = glm::translate(glm::mat4(1.0f), pos);
                renderer.Draw(*m_TreeMesh, treeTransform);
            }
        }

        // Render rocks from all loaded chunks
        if (m_RockMesh)
        {
            for (const auto &pos : m_ChunkManager.GetAllRockPositions())
            {
                glm::mat4 rockTransform = glm::translate(glm::mat4(1.0f), pos);
                renderer.Draw(*m_RockMesh, rockTransform);
            }
        }

        renderer.EndScene();
    }

    void Sandbox::OnImGuiRender()
    {
        if (!m_ShowUI)
            return;

        // Render terrain settings panel
        if (m_TerrainPanel)
        {
            m_TerrainPanel->Render();
        }

        // Render debug/stats overlay
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_FirstUseEver);
        ImGui::Begin("Stats & Controls", nullptr, ImGuiWindowFlags_NoCollapse);

        ImGui::Text("FPS: %.1f (%.2f ms)", 1.0f / m_FrameTime, m_FrameTime * 1000.0f);
        ImGui::Text("Chunks: %d", m_ChunkManager.GetLoadedChunkCount());
        ImGui::Text("Trees: %zu", m_ChunkManager.GetAllTreePositions().size());
        ImGui::Text("Rocks: %zu", m_ChunkManager.GetAllRockPositions().size());

        ImGui::Separator();
        ImGui::Text("Camera Position:");
        ImGui::Text("  X: %.1f  Y: %.1f  Z: %.1f",
                    m_CameraPosition.x, m_CameraPosition.y, m_CameraPosition.z);

        ImGui::Separator();
        ImGui::SliderFloat("Time of Day", &m_TimeOfDay, 0.0f, 24.0f, "%.1f h");
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            Application::Get().GetRenderer().GetLightManager().SetTimeOfDay(m_TimeOfDay);
        }

        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::BulletText("WASD - Move");
        ImGui::BulletText("Space/Shift - Up/Down");
        ImGui::BulletText("Right Mouse - Look");
        ImGui::BulletText("T/Shift+T - Time");
        ImGui::BulletText("Tab - Toggle UI");

        ImGui::End();
    }

    void Sandbox::OnEvent(Event &event)
    {
        // Handle events
    }

    void Sandbox::SetupScene()
    {
        m_Scene = std::make_shared<Scene>("Terragen Scene");

        auto &device = Application::Get().GetRenderer().GetDevice();

        // Create shared object meshes
        m_CubeMesh = Mesh::CreateCube(device, {0.8f, 0.2f, 0.2f});
        m_TreeMesh = Mesh::CreateLowPolyTree(device);
        m_RockMesh = Mesh::CreateLowPolyRock(device);

        // Initialize chunk-based world
        WorldSettings worldSettings;

        // Terrain resolution (2x): keep chunk world size the same but increase mesh density.
        // Old: 32 cells @ 1.0 units/cell => 32 world units/chunk
        // New: 64 cells @ 0.5 units/cell => 32 world units/chunk
        static constexpr int kChunkCellsPerSide = 64;
        static constexpr float kWorldUnitsPerCell = 0.5f;
        worldSettings.chunkSize = kChunkCellsPerSide; // Grid cells per chunk
        worldSettings.cellSize = kWorldUnitsPerCell;  // World units per cell
        worldSettings.viewDistance = 3;               // Load 7x7 chunks
        worldSettings.seed = 42;
        worldSettings.seaLevel = 2.0f;
        worldSettings.waterEnabled = true;

        // Terrain settings
        worldSettings.terrainSettings.heightScale = 10.0f;
        worldSettings.terrainSettings.noiseScale = 0.03f;
        worldSettings.terrainSettings.octaves = 5;
        worldSettings.terrainSettings.persistence = 0.5f;
        worldSettings.terrainSettings.lacunarity = 2.0f;
        worldSettings.terrainSettings.flatShading = true;
        worldSettings.terrainSettings.useHeightColors = true;

        // Domain warping for organic terrain
        worldSettings.terrainSettings.useWarp = true;
        worldSettings.terrainSettings.warpStrength = 0.4f;
        worldSettings.terrainSettings.warpScale = 0.5f;
        worldSettings.terrainSettings.warpLevels = 2;

        m_ChunkManager.Initialize(device, worldSettings);

        // Initial chunk load
        m_ChunkManager.Update(m_CameraPosition);

        // Initialize terrain settings panel
        m_TerrainPanel = std::make_unique<TerrainSettingsPanel>();
        m_TerrainPanel->Initialize(device, m_ChunkManager);

        // Setup lighting
        SetupLighting();

        GEN_INFO("Scene setup complete ({}x{} chunks visible)",
                 worldSettings.viewDistance * 2 + 1, worldSettings.viewDistance * 2 + 1);
    }

    void Sandbox::SetupLighting()
    {
        auto &lightManager = Application::Get().GetRenderer().GetLightManager();

        // Set initial time of day
        lightManager.SetTimeOfDay(m_TimeOfDay);

        // Add a few point lights for testing
        Light campfireLight;
        campfireLight.Position = glm::vec3(5.0f, 2.0f, 5.0f);
        campfireLight.Color = glm::vec3(1.0f, 0.6f, 0.2f); // Orange/fire color
        campfireLight.Intensity = 2.0f;
        lightManager.AddPointLight(campfireLight);

        // Enable subtle fog for atmosphere
        lightManager.SetFogDensity(0.005f);

        GEN_INFO("Lighting setup complete - Time of day: {}:00, Press T to change time", static_cast<int>(m_TimeOfDay));
    }

}
