#pragma once

#include <vulkan/vulkan.h>

namespace Genesis {

    class VulkanDevice;

    class VulkanBuffer {
    public:
        VulkanBuffer() = default;
        ~VulkanBuffer();

        void Init(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties);
        void Shutdown();

        void Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void Unmap();

        void WriteToBuffer(const void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        VkBuffer GetBuffer() const { return m_Buffer; }
        VkDeviceMemory GetMemory() const { return m_Memory; }
        void* GetMappedMemory() const { return m_Mapped; }
        VkDeviceSize GetSize() const { return m_Size; }

        VkDescriptorBufferInfo GetDescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        static void CopyBuffer(VulkanDevice& device, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                                        VkMemoryPropertyFlags properties);

    private:
        VulkanDevice* m_Device = nullptr;
        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;
        void* m_Mapped = nullptr;
        VkDeviceSize m_Size = 0;
    };

}
