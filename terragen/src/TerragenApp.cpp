#include "TerragenApp.h"
#include "Sandbox.h"

namespace Genesis {

    TerragenApp::TerragenApp(const ApplicationConfig& config)
        : Application(config) {
    }

    TerragenApp::~TerragenApp() = default;

    void TerragenApp::OnInit() {
        GEN_INFO("Terragen initializing...");
        
        // Push ImGui layer as overlay
        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);

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
        config.Window.Width = 1600;
        config.Window.Height = 900;
        config.EnableValidationLayers = true;

        return new TerragenApp(config);
    }

}
