#include "genesis/core/Application.h"
#include "genesis/core/Window.h"
#include "genesis/core/Log.h"
#include "genesis/core/Layer.h"
#include "genesis/renderer/Renderer.h"
#include "genesis/imgui/ImGuiLayer.h"

#include <GLFW/glfw3.h>
#include <chrono>

namespace Genesis
{

    Application *Application::s_Instance = nullptr;

    // LayerStack implementation
    class LayerStack
    {
    public:
        LayerStack() = default;
        ~LayerStack()
        {
            for (Layer *layer : m_Layers)
            {
                layer->OnDetach();
                delete layer;
            }
        }

        void PushLayer(Layer *layer)
        {
            m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
            m_LayerInsertIndex++;
            layer->OnAttach();
        }

        void PushOverlay(Layer *overlay)
        {
            m_Layers.emplace_back(overlay);
            overlay->OnAttach();
        }

        void PopLayer(Layer *layer)
        {
            auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);
            if (it != m_Layers.begin() + m_LayerInsertIndex)
            {
                layer->OnDetach();
                m_Layers.erase(it);
                m_LayerInsertIndex--;
            }
        }

        void PopOverlay(Layer *overlay)
        {
            auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);
            if (it != m_Layers.end())
            {
                overlay->OnDetach();
                m_Layers.erase(it);
            }
        }

        std::vector<Layer *>::iterator begin() { return m_Layers.begin(); }
        std::vector<Layer *>::iterator end() { return m_Layers.end(); }

        Layer *FindImGuiLayer()
        {
            for (Layer *layer : m_Layers)
            {
                if (layer->GetName() == "ImGuiLayer")
                    return layer;
            }
            return nullptr;
        }

    private:
        std::vector<Layer *> m_Layers;
        unsigned int m_LayerInsertIndex = 0;
    };

    Application::Application(const ApplicationConfig &config)
        : m_Config(config)
    {
        GEN_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;

        Log::Init();
        GEN_INFO("Genesis Engine v0.1.0 initializing...");

        m_Window = Window::Create(config.Window);
        m_LayerStack = std::make_unique<LayerStack>();
        m_Renderer = std::make_unique<Renderer>();
        m_Renderer->Init(*m_Window);

        GEN_INFO("Genesis Engine initialized successfully!");
    }

    Application::~Application()
    {
        OnShutdown();

        // Destroy layers first (they may hold GPU resources)
        m_LayerStack.reset();

        // Then shutdown renderer
        m_Renderer->Shutdown();
        m_Renderer.reset();

        m_Window.reset();
        Log::Shutdown();
        s_Instance = nullptr;
    }

    void Application::Run()
    {
        OnInit();

        auto lastTime = std::chrono::high_resolution_clock::now();

        while (m_Running && !m_Window->ShouldClose())
        {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;

            ProcessEvents();

            // Update layers
            for (Layer *layer : *m_LayerStack)
            {
                layer->OnUpdate(deltaTime);
            }

            OnUpdate(deltaTime);

            // Render - only if BeginFrame succeeds (swapchain might be recreating)
            if (m_Renderer->BeginFrame())
            {
                for (Layer *layer : *m_LayerStack)
                {
                    layer->OnRender();
                }

                // ImGui rendering
                Layer *imguiLayerBase = m_LayerStack->FindImGuiLayer();
                if (imguiLayerBase)
                {
                    ImGuiLayer *imguiLayer = static_cast<ImGuiLayer *>(imguiLayerBase);
                    imguiLayer->Begin();
                    for (Layer *layer : *m_LayerStack)
                    {
                        layer->OnImGuiRender();
                    }
                    imguiLayer->End();
                }

                m_Renderer->EndFrame();
            }
        }
    }

    void Application::Close()
    {
        m_Running = false;
    }

    void Application::PushLayer(Layer *layer)
    {
        m_LayerStack->PushLayer(layer);
    }

    void Application::PushOverlay(Layer *overlay)
    {
        m_LayerStack->PushOverlay(overlay);
    }

    void Application::ProcessEvents()
    {
        m_Window->PollEvents();
    }

}
