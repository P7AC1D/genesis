#include "genesis/renderer/Renderer.h"
#include "genesis/renderer/VulkanContext.h"
#include "genesis/renderer/VulkanDevice.h"
#include "genesis/renderer/VulkanSwapchain.h"
#include "genesis/renderer/VulkanPipeline.h"
#include "genesis/renderer/Camera.h"
#include "genesis/renderer/Mesh.h"
#include "genesis/core/Window.h"
#include "genesis/core/Log.h"

namespace Genesis {

    Renderer::Renderer() = default;
    Renderer::~Renderer() = default;

    void Renderer::Init(Window& window) {
        GEN_INFO("Initializing Vulkan renderer...");

        m_Context = std::make_unique<VulkanContext>();
        m_Context->Init(window.GetNativeWindow(), true);

        m_Device = std::make_unique<VulkanDevice>();
        m_Device->Init(m_Context->GetInstance(), m_Context->GetSurface());

        m_Swapchain = std::make_unique<VulkanSwapchain>();
        m_Swapchain->Init(*m_Device, m_Context->GetSurface(), window.GetWidth(), window.GetHeight());

        // Pipeline will be created when shaders are loaded
        GEN_INFO("Vulkan renderer initialized!");
    }

    void Renderer::Shutdown() {
        m_Device->WaitIdle();

        if (m_Pipeline) m_Pipeline->Shutdown();
        if (m_Swapchain) m_Swapchain->Shutdown();
        if (m_Device) m_Device->Shutdown();
        if (m_Context) m_Context->Shutdown();

        GEN_INFO("Vulkan renderer shutdown complete.");
    }

    void Renderer::BeginFrame() {
        m_FrameStarted = true;
        ResetStats();
    }

    void Renderer::EndFrame() {
        m_FrameStarted = false;
    }

    void Renderer::BeginScene(const Camera& camera) {
        // Set view-projection matrix for rendering
    }

    void Renderer::EndScene() {
        // Flush batched draw calls
    }

    void Renderer::Submit(const Mesh& mesh, const glm::mat4& transform) {
        // Submit mesh for rendering
        m_Stats.DrawCalls++;
        m_Stats.TriangleCount += mesh.GetIndexCount() / 3;
    }

    void Renderer::RenderScene(Scene& scene) {
        // Iterate through scene entities and render
    }

    void Renderer::OnWindowResize(uint32_t width, uint32_t height) {
        m_Device->WaitIdle();
        m_Swapchain->Recreate(width, height);
    }

    void Renderer::ResetStats() {
        m_Stats.DrawCalls = 0;
        m_Stats.TriangleCount = 0;
    }

}
