#include "PlaygroundApp.h"
#include "Sandbox.h"

namespace Genesis {

    PlaygroundApp::PlaygroundApp(const ApplicationConfig& config)
        : Application(config) {
    }

    PlaygroundApp::~PlaygroundApp() = default;

    void PlaygroundApp::OnInit() {
        GEN_INFO("Genesis Playground initializing...");
        
        m_Sandbox = new Sandbox();
        PushLayer(m_Sandbox);
    }

    void PlaygroundApp::OnShutdown() {
        GEN_INFO("Genesis Playground shutting down...");
    }

    void PlaygroundApp::OnUpdate(float deltaTime) {
        // Playground-specific updates
    }

    // Application factory
    Application* CreateApplication(int argc, char** argv) {
        ApplicationConfig config;
        config.Name = "Genesis Playground";
        config.Window.Title = "Genesis Playground";
        config.Window.Width = 1280;
        config.Window.Height = 720;
        config.EnableValidationLayers = true;

        return new PlaygroundApp(config);
    }

}
