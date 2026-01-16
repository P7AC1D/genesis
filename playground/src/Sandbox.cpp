#include "Sandbox.h"
#include <genesis/renderer/VulkanDevice.h>
#include <random>

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

        // Cleanup meshes
        if (m_TerrainMesh) {
            m_TerrainMesh->Shutdown();
            m_TerrainMesh.reset();
        }
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
        
        // Only render if frame started successfully
        if (!renderer.IsFrameInProgress()) return;

        renderer.BeginScene(m_Camera);

        // Render procedural terrain
        if (m_TerrainMesh) {
            // Center the terrain around origin
            const auto& settings = m_TerrainGenerator.GetSettings();
            float offsetX = -(settings.width * settings.cellSize) / 2.0f;
            float offsetZ = -(settings.depth * settings.cellSize) / 2.0f;
            glm::mat4 terrainTransform = glm::translate(glm::mat4(1.0f), glm::vec3(offsetX, 0.0f, offsetZ));
            renderer.Draw(*m_TerrainMesh, terrainTransform);
        }

        // Render a marker cube at origin
        if (m_CubeMesh) {
            glm::mat4 cubeTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.0f, 0.0f));
            renderer.Draw(*m_CubeMesh, cubeTransform);
        }

        // Render trees at procedurally placed positions
        if (m_TreeMesh) {
            for (const auto& pos : m_TreePositions) {
                glm::mat4 treeTransform = glm::translate(glm::mat4(1.0f), pos);
                renderer.Draw(*m_TreeMesh, treeTransform);
            }
        }

        // Render rocks at procedurally placed positions
        if (m_RockMesh) {
            for (const auto& pos : m_RockPositions) {
                glm::mat4 rockTransform = glm::translate(glm::mat4(1.0f), pos);
                renderer.Draw(*m_RockMesh, rockTransform);
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

        auto& device = Application::Get().GetRenderer().GetDevice();
        
        // Create test meshes
        m_CubeMesh = Mesh::CreateCube(device, {0.8f, 0.2f, 0.2f});
        m_TreeMesh = Mesh::CreateLowPolyTree(device);
        m_RockMesh = Mesh::CreateLowPolyRock(device);

        // Generate procedural terrain
        GenerateProceduralTerrain();
        
        // Place objects on terrain
        PlaceProceduralObjects();

        // Create scene entities
        auto terrain = m_Scene->CreateEntity("Terrain");
        auto testCube = m_Scene->CreateEntity("Marker Cube");
        auto mainCamera = m_Scene->CreateEntity("Main Camera");
        auto sun = m_Scene->CreateEntity("Sun");

        GEN_INFO("Scene setup complete with procedural terrain ({}x{} grid)", 
            m_TerrainGenerator.GetSettings().width, 
            m_TerrainGenerator.GetSettings().depth);
    }

    void Sandbox::GenerateProceduralTerrain() {
        GEN_INFO("Generating procedural terrain...");
        
        // Configure terrain settings
        TerrainSettings settings;
        settings.width = 64;
        settings.depth = 64;
        settings.cellSize = 1.0f;
        settings.heightScale = 8.0f;
        settings.baseHeight = 0.0f;
        settings.seed = 42;
        settings.noiseScale = 0.04f;
        settings.octaves = 5;
        settings.persistence = 0.5f;
        settings.lacunarity = 2.0f;
        settings.flatShading = true;  // Low-poly look
        settings.useHeightColors = true;
        
        m_TerrainGenerator.SetSettings(settings);
        
        // Generate the terrain mesh
        auto terrainMeshPtr = m_TerrainGenerator.Generate();
        
        // Upload to GPU
        auto& device = Application::Get().GetRenderer().GetDevice();
        m_TerrainMesh = std::make_unique<Mesh>(device, terrainMeshPtr->GetVertices(), terrainMeshPtr->GetIndices());
        
        GEN_INFO("Terrain generated: {} vertices, {} triangles", 
            terrainMeshPtr->GetVertices().size(),
            terrainMeshPtr->GetIndices().size() / 3);
    }

    void Sandbox::PlaceProceduralObjects() {
        GEN_INFO("Placing procedural objects on terrain...");
        
        const auto& settings = m_TerrainGenerator.GetSettings();
        float halfWidth = (settings.width * settings.cellSize) / 2.0f;
        float halfDepth = (settings.depth * settings.cellSize) / 2.0f;
        
        std::mt19937 rng(settings.seed + 1);  // Different seed for object placement
        std::uniform_real_distribution<float> distX(-halfWidth * 0.8f, halfWidth * 0.8f);
        std::uniform_real_distribution<float> distZ(-halfDepth * 0.8f, halfDepth * 0.8f);
        
        // Place trees
        int numTrees = 30;
        m_TreePositions.reserve(numTrees);
        
        for (int i = 0; i < numTrees; i++) {
            float x = distX(rng);
            float z = distZ(rng);
            
            // Get terrain height at this position (accounting for terrain offset)
            float terrainX = x + halfWidth;
            float terrainZ = z + halfDepth;
            float y = m_TerrainGenerator.GetHeightAt(terrainX, terrainZ);
            
            // Only place trees above water level
            float normalizedHeight = y / settings.heightScale;
            if (normalizedHeight > settings.sandLevel) {
                m_TreePositions.push_back(glm::vec3(x, y, z));
            }
        }
        
        // Place rocks
        int numRocks = 20;
        m_RockPositions.reserve(numRocks);
        
        for (int i = 0; i < numRocks; i++) {
            float x = distX(rng);
            float z = distZ(rng);
            
            float terrainX = x + halfWidth;
            float terrainZ = z + halfDepth;
            float y = m_TerrainGenerator.GetHeightAt(terrainX, terrainZ);
            
            // Place rocks anywhere except deep water
            float normalizedHeight = y / settings.heightScale;
            if (normalizedHeight > settings.waterLevel * 0.5f) {
                m_RockPositions.push_back(glm::vec3(x, y + 0.1f, z));
            }
        }
        
        GEN_INFO("Placed {} trees and {} rocks", m_TreePositions.size(), m_RockPositions.size());
    }

}
