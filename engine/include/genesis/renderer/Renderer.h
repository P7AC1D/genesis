#pragma once

#include "genesis/renderer/Light.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace Genesis {

    class VulkanContext;
    class VulkanDevice;
    class VulkanSwapchain;
    class VulkanPipeline;
    class VulkanBuffer;
    class Camera;
    class Mesh;
    class Scene;

    struct RenderStats {
        uint32_t DrawCalls = 0;
        uint32_t TriangleCount = 0;
        float FrameTime = 0.0f;
    };

    // Push constant for model matrix
    struct PushConstantData {
        glm::mat4 ModelMatrix{1.0f};
        glm::mat4 NormalMatrix{1.0f};
    };

    // Point light data for shader (must match shader layout)
    struct PointLightData {
        glm::vec4 PositionAndIntensity{0.0f};  // xyz = position, w = intensity
        glm::vec4 ColorAndRadius{1.0f};         // xyz = color, w = radius
    };

    // Uniform buffer for global scene data
    struct GlobalUBO {
        glm::mat4 ViewMatrix{1.0f};
        glm::mat4 ProjectionMatrix{1.0f};
        glm::vec4 CameraPosition{0.0f};
        
        // Directional light (sun)
        glm::vec4 SunDirection{-0.2f, -1.0f, -0.3f, 0.0f};
        glm::vec4 SunColor{1.0f, 0.95f, 0.9f, 1.0f};  // xyz = color, w = intensity
        
        // Ambient
        glm::vec4 AmbientColor{0.15f, 0.15f, 0.2f, 1.0f};  // xyz = color, w = intensity
        
        // Point lights
        PointLightData PointLights[MAX_POINT_LIGHTS];
        glm::ivec4 NumPointLights{0};  // x = count, yzw = padding
        
        // Fog
        glm::vec4 FogColorAndDensity{0.7f, 0.8f, 0.9f, 0.0f};  // xyz = color, w = density
    };

    class Renderer {
    public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        Renderer();
        ~Renderer();

        void Init(class Window& window);
        void Shutdown();

        bool BeginFrame();
        void EndFrame();

        void BeginScene(const Camera& camera);
        void EndScene();

        void Draw(const Mesh& mesh, const glm::mat4& transform);
        void RenderScene(Scene& scene);

        void OnWindowResize(uint32_t width, uint32_t height);

        VulkanContext& GetContext() { return *m_Context; }
        VulkanDevice& GetDevice() { return *m_Device; }
        VkCommandBuffer GetCurrentCommandBuffer() const { return m_CommandBuffers[m_CurrentFrameIndex]; }
        
        // Lighting
        LightManager& GetLightManager() { return m_LightManager; }
        const LightManager& GetLightManager() const { return m_LightManager; }

        const RenderStats& GetStats() const { return m_Stats; }
        void ResetStats();

        bool IsFrameInProgress() const { return m_FrameStarted; }

    private:
        void CreateCommandBuffers();
        void CreateSyncObjects();
        void CreateDescriptorSetLayout();
        void CreatePipelineLayout();
        void CreatePipeline();
        void CreateUniformBuffers();
        void CreateDescriptorPool();
        void CreateDescriptorSets();

        void RecreateSwapchain();

    private:
        std::unique_ptr<VulkanContext> m_Context;
        std::unique_ptr<VulkanDevice> m_Device;
        std::unique_ptr<VulkanSwapchain> m_Swapchain;
        std::unique_ptr<VulkanPipeline> m_Pipeline;

        // Command buffers
        std::vector<VkCommandBuffer> m_CommandBuffers;

        // Synchronization
        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence> m_InFlightFences;
        std::vector<VkFence> m_ImagesInFlight;  // Track which fence is associated with each swapchain image

        // Descriptor resources
        VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_DescriptorSets;
        std::vector<std::unique_ptr<VulkanBuffer>> m_UniformBuffers;

        // Frame state
        uint32_t m_CurrentFrameIndex = 0;
        uint32_t m_CurrentImageIndex = 0;
        bool m_FrameStarted = false;
        bool m_SwapchainNeedsRecreation = false;

        // Window reference
        class Window* m_Window = nullptr;

        // Current scene data
        GlobalUBO m_GlobalUBO;
        
        // Lighting
        LightManager m_LightManager;

        RenderStats m_Stats;
    };

}

