#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

struct GLFWwindow;

namespace Genesis {

    class VulkanContext {
    public:
        VulkanContext() = default;
        ~VulkanContext();

        void Init(GLFWwindow* window, bool enableValidation = true);
        void Shutdown();

        VkInstance GetInstance() const { return m_Instance; }
        VkSurfaceKHR GetSurface() const { return m_Surface; }
        bool ValidationEnabled() const { return m_ValidationEnabled; }

    private:
        void CreateInstance(bool enableValidation);
        void SetupDebugMessenger();
        void CreateSurface(GLFWwindow* window);

        bool CheckValidationLayerSupport();
        std::vector<const char*> GetRequiredExtensions();

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT type,
            const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
            void* userData);

    private:
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        bool m_ValidationEnabled = false;

        const std::vector<const char*> m_ValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
    };

}
