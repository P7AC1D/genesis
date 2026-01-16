#include "EditorLayer.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/PropertiesPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/AssetBrowserPanel.h"

namespace Genesis {

    EditorLayer::EditorLayer()
        : Layer("EditorLayer") {
    }

    EditorLayer::~EditorLayer() = default;

    void EditorLayer::OnAttach() {
        GEN_INFO("EditorLayer attached");

        // Create default scene
        m_EditorScene = std::make_shared<Scene>("Untitled");
        m_ActiveScene = m_EditorScene;

        // Setup editor camera
        m_EditorCamera = Camera(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
        m_EditorCamera.SetPosition({0.0f, 5.0f, 10.0f});
        m_EditorCamera.LookAt({0.0f, 0.0f, 0.0f});

        // Initialize panels (would use ImGui in full implementation)
        // m_SceneHierarchyPanel = std::make_unique<SceneHierarchyPanel>(m_ActiveScene);
        // m_PropertiesPanel = std::make_unique<PropertiesPanel>();
        // m_ViewportPanel = std::make_unique<ViewportPanel>();
        // m_AssetBrowserPanel = std::make_unique<AssetBrowserPanel>();

        // Create some default entities for testing
        auto ground = m_ActiveScene->CreateEntity("Ground");
        auto camera = m_ActiveScene->CreateEntity("Main Camera");
        auto light = m_ActiveScene->CreateEntity("Directional Light");
    }

    void EditorLayer::OnDetach() {
        GEN_INFO("EditorLayer detached");
    }

    void EditorLayer::OnUpdate(float deltaTime) {
        m_FrameTime = deltaTime;

        // Handle editor camera movement
        if (m_ViewportFocused) {
            // WASD camera controls would go here
        }

        // Update active scene
        if (m_ActiveScene) {
            m_ActiveScene->OnUpdate(deltaTime);
        }
    }

    void EditorLayer::OnRender() {
        // Render scene to viewport framebuffer
        if (m_ActiveScene) {
            auto& renderer = Application::Get().GetRenderer();
            renderer.BeginScene(m_EditorCamera);
            m_ActiveScene->OnRender(renderer);
            renderer.EndScene();
        }
    }

    void EditorLayer::OnImGuiRender() {
        // ImGui editor UI would go here
        // - Dockspace
        // - Menu bar
        // - Scene hierarchy
        // - Properties
        // - Viewport
        // - Asset browser
        // - Console
    }

    void EditorLayer::OnEvent(Event& event) {
        // Handle editor events
    }

    void EditorLayer::NewScene() {
        m_EditorScene = std::make_shared<Scene>();
        m_ActiveScene = m_EditorScene;
    }

    void EditorLayer::OpenScene() {
        // File dialog to open scene
    }

    void EditorLayer::SaveScene() {
        // Save current scene
    }

    void EditorLayer::SaveSceneAs() {
        // File dialog to save scene
    }

}
