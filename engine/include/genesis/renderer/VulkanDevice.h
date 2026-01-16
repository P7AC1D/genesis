#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

namespace Genesis {

    struct QueueFamilyIndices {
        std::optional<uint32_t> GraphicsFamily;
        std::optional<uint32_t> PresentFamily;

        bool IsComplete() const {
            return GraphicsFamily.has_value() && PresentFamily.has_value();
        }
    };

    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    class VulkanDevice {
    public:
        VulkanDevice() = default;
        ~VulkanDevice();

        void Init(VkInstance instance, VkSurfaceKHR surface);
        void Shutdown();

        VkDevice GetDevice() const { return m_Device; }
        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        VkQueue GetPresentQueue() const { return m_PresentQueue; }
        VkCommandPool GetCommandPool() const { return m_CommandPool; }

        const QueueFamilyIndices& GetQueueFamilies() const { return m_QueueFamilies; }
        SwapchainSupportDetails QuerySwapchainSupport() const;

        void WaitIdle() const;

        VkCommandBuffer BeginSingleTimeCommands();
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

    private:
        void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
        void CreateLogicalDevice(VkSurfaceKHR surface);
        void CreateCommandPool();

        bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
        bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
        SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const;

    private:
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue m_PresentQueue = VK_NULL_HANDLE;
        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

        QueueFamilyIndices m_QueueFamilies;

        const std::vector<const char*> m_DeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
    };

}
