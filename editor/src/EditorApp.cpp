#include "EditorApp.h"
#include "EditorLayer.h"

namespace Genesis {

    EditorApp::EditorApp(const ApplicationConfig& config)
        : Application(config) {
    }

    EditorApp::~EditorApp() = default;

    void EditorApp::OnInit() {
        GEN_INFO("Genesis Editor initializing...");
        
        m_EditorLayer = new EditorLayer();
        PushLayer(m_EditorLayer);
    }

    void EditorApp::OnShutdown() {
        GEN_INFO("Genesis Editor shutting down...");
    }

    void EditorApp::OnUpdate(float deltaTime) {
        // Editor-specific updates
    }

    // Application factory
    Application* CreateApplication(int argc, char** argv) {
        ApplicationConfig config;
        config.Name = "Genesis Editor";
        config.Window.Title = "Genesis Editor";
        config.Window.Width = 1920;
        config.Window.Height = 1080;
        config.EnableValidationLayers = true;

        return new EditorApp(config);
    }

}
