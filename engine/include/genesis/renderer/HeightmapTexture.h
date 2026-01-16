#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Genesis {

    class VulkanDevice;

    class HeightmapTexture {
    public:
        HeightmapTexture() = default;
        ~HeightmapTexture();

        void Create(VulkanDevice& device, int width, int height);
        void Destroy();
        void Update(const std::vector<float>& heightData, float minHeight, float maxHeight);

        VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }
        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }
        bool IsValid() const { return m_Image != VK_NULL_HANDLE; }

    private:
        void CreateImage(VulkanDevice& device);
        void CreateSampler(VulkanDevice& device);
        void CreateDescriptorSet(VulkanDevice& device);
        uint32_t FindMemoryType(VulkanDevice& device, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    private:
        VulkanDevice* m_Device = nullptr;
        int m_Width = 0;
        int m_Height = 0;

        VkImage m_Image = VK_NULL_HANDLE;
        VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
        VkImageView m_ImageView = VK_NULL_HANDLE;
        VkSampler m_Sampler = VK_NULL_HANDLE;
        VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;

        VkBuffer m_StagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_StagingMemory = VK_NULL_HANDLE;
    };

}
