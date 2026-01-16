#pragma once

#include <string>
#include <functional>

struct GLFWwindow;

namespace Genesis {

    struct WindowProps;

    class Window {
    public:
        using EventCallbackFn = std::function<void(class Event&)>;

        Window(const WindowProps& props);
        ~Window();

        void OnUpdate();
        void PollEvents();

        uint32_t GetWidth() const { return m_Data.Width; }
        uint32_t GetHeight() const { return m_Data.Height; }
        float GetAspectRatio() const { return static_cast<float>(m_Data.Width) / static_cast<float>(m_Data.Height); }

        void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }
        void SetVSync(bool enabled);
        bool IsVSync() const { return m_Data.VSync; }

        GLFWwindow* GetNativeWindow() const { return m_Window; }
        bool ShouldClose() const;

        static std::unique_ptr<Window> Create(const WindowProps& props);

    private:
        void Init(const WindowProps& props);
        void Shutdown();
        void SetupCallbacks();

    private:
        GLFWwindow* m_Window = nullptr;

        struct WindowData {
            std::string Title;
            uint32_t Width = 0;
            uint32_t Height = 0;
            bool VSync = false;
            EventCallbackFn EventCallback;
        };

        WindowData m_Data;
    };

}
