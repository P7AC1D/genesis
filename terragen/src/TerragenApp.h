#pragma once

#include <genesis/Genesis.h>

namespace Genesis {

    class Sandbox;
    class ImGuiLayer;

    class TerragenApp : public Application {
    public:
        TerragenApp(const ApplicationConfig& config);
        ~TerragenApp() override;

    protected:
        void OnInit() override;
        void OnShutdown() override;
        void OnUpdate(float deltaTime) override;

    private:
        Sandbox* m_Sandbox = nullptr;
        ImGuiLayer* m_ImGuiLayer = nullptr;
    };

}
