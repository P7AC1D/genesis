#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Genesis {

    class VulkanContext;
    class VulkanDevice;
    class VulkanSwapchain;
    class VulkanPipeline;
    class Camera;
    class Mesh;
    class Scene;

    struct RenderStats {
        uint32_t DrawCalls = 0;
        uint32_t TriangleCount = 0;
        float FrameTime = 0.0f;
    };

    class Renderer {
    public:
        Renderer();
        ~Renderer();

        void Init(class Window& window);
        void Shutdown();

        void BeginFrame();
        void EndFrame();

        void BeginScene(const Camera& camera);
        void EndScene();

        void Submit(const Mesh& mesh, const glm::mat4& transform);
        void RenderScene(Scene& scene);

        void OnWindowResize(uint32_t width, uint32_t height);

        VulkanContext& GetContext() { return *m_Context; }
        VulkanDevice& GetDevice() { return *m_Device; }

        const RenderStats& GetStats() const { return m_Stats; }
        void ResetStats();

    private:
        std::unique_ptr<VulkanContext> m_Context;
        std::unique_ptr<VulkanDevice> m_Device;
        std::unique_ptr<VulkanSwapchain> m_Swapchain;
        std::unique_ptr<VulkanPipeline> m_Pipeline;

        RenderStats m_Stats;
        bool m_FrameStarted = false;
    };

}
