#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Genesis {

    class VulkanDevice;

    class VulkanSwapchain {
    public:
        VulkanSwapchain() = default;
        ~VulkanSwapchain();

        void Init(VulkanDevice& device, VkSurfaceKHR surface, uint32_t width, uint32_t height);
        void Shutdown();
        void Recreate(uint32_t width, uint32_t height);

        VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
        VkFormat GetImageFormat() const { return m_ImageFormat; }
        VkExtent2D GetExtent() const { return m_Extent; }
        VkRenderPass GetRenderPass() const { return m_RenderPass; }
        
        size_t GetImageCount() const { return m_Images.size(); }
        VkFramebuffer GetFramebuffer(uint32_t index) const { return m_Framebuffers[index]; }

        VkResult AcquireNextImage(VkSemaphore semaphore, uint32_t* imageIndex);
        VkResult Present(VkQueue queue, VkSemaphore waitSemaphore, uint32_t imageIndex);

        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    private:
        void CreateSwapchain(uint32_t width, uint32_t height);
        void CreateImageViews();
        void CreateRenderPass();
        void CreateDepthResources();
        void CreateFramebuffers();
        void Cleanup();

        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);
        VkFormat FindDepthFormat();

    private:
        VulkanDevice* m_Device = nullptr;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        VkFormat m_ImageFormat;
        VkExtent2D m_Extent;

        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;
        std::vector<VkFramebuffer> m_Framebuffers;

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;

        VkImage m_DepthImage = VK_NULL_HANDLE;
        VkDeviceMemory m_DepthImageMemory = VK_NULL_HANDLE;
        VkImageView m_DepthImageView = VK_NULL_HANDLE;
    };

}
