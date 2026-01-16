#pragma once

#include <memory>
#include <string>

namespace Genesis {

    struct WindowProps {
        std::string Title;
        uint32_t Width;
        uint32_t Height;

        WindowProps(const std::string& title = "Genesis Engine",
                    uint32_t width = 1600,
                    uint32_t height = 900)
            : Title(title), Width(width), Height(height) {}
    };

    struct ApplicationConfig {
        std::string Name = "Genesis Application";
        WindowProps Window;
        bool EnableValidationLayers = true;
    };

    class Window;
    class LayerStack;
    class Renderer;

    class Application {
    public:
        Application(const ApplicationConfig& config);
        virtual ~Application();

        void Run();
        void Close();

        void PushLayer(class Layer* layer);
        void PushOverlay(class Layer* overlay);

        Window& GetWindow() { return *m_Window; }
        static Application& Get() { return *s_Instance; }

        Renderer& GetRenderer() { return *m_Renderer; }

    protected:
        virtual void OnInit() {}
        virtual void OnShutdown() {}
        virtual void OnUpdate(float deltaTime) {}

    private:
        void ProcessEvents();

    private:
        ApplicationConfig m_Config;
        std::unique_ptr<Window> m_Window;
        std::unique_ptr<LayerStack> m_LayerStack;
        std::unique_ptr<Renderer> m_Renderer;
        bool m_Running = true;
        float m_LastFrameTime = 0.0f;

        static Application* s_Instance;
    };

    // Defined by client application
    Application* CreateApplication(int argc, char** argv);

}
