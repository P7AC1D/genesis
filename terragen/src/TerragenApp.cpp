#include "TerragenApp.h"
#include "Sandbox.h"

namespace Genesis {

    TerragenApp::TerragenApp(const ApplicationConfig& config)
        : Application(config) {
    }

    TerragenApp::~TerragenApp() = default;

    void TerragenApp::OnInit() {
        GEN_INFO("Terragen initializing...");
        
        m_Sandbox = new Sandbox();
        PushLayer(m_Sandbox);
    }

    void TerragenApp::OnShutdown() {
        GEN_INFO("Terragen shutting down...");
    }

    void TerragenApp::OnUpdate(float deltaTime) {
        // Terragen-specific updates
    }

    // Application factory
    Application* CreateApplication(int argc, char** argv) {
        ApplicationConfig config;
        config.Name = "Terragen";
        config.Window.Title = "Terragen";
        config.Window.Width = 1280;
        config.Window.Height = 720;
        config.EnableValidationLayers = true;

        return new TerragenApp(config);
    }

}
