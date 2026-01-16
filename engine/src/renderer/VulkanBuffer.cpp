#include "genesis/renderer/VulkanBuffer.h"
#include "genesis/renderer/VulkanDevice.h"

#include <cstring>
#include <stdexcept>

namespace Genesis {

    VulkanBuffer::~VulkanBuffer() {
        if (m_Buffer != VK_NULL_HANDLE) {
            Shutdown();
        }
    }

    void VulkanBuffer::Init(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage,
                            VkMemoryPropertyFlags properties) {
        m_Device = &device;
        m_Size = size;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device.GetDevice(), &bufferInfo, nullptr, &m_Buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device.GetDevice(), m_Buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(device.GetPhysicalDevice(), memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device.GetDevice(), &allocInfo, nullptr, &m_Memory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device.GetDevice(), m_Buffer, m_Memory, 0);
    }

    void VulkanBuffer::Shutdown() {
        Unmap();

        if (m_Buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_Device->GetDevice(), m_Buffer, nullptr);
            m_Buffer = VK_NULL_HANDLE;
        }

        if (m_Memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_Device->GetDevice(), m_Memory, nullptr);
            m_Memory = VK_NULL_HANDLE;
        }
    }

    void VulkanBuffer::Map(VkDeviceSize size, VkDeviceSize offset) {
        vkMapMemory(m_Device->GetDevice(), m_Memory, offset, size, 0, &m_Mapped);
    }

    void VulkanBuffer::Unmap() {
        if (m_Mapped) {
            vkUnmapMemory(m_Device->GetDevice(), m_Memory);
            m_Mapped = nullptr;
        }
    }

    void VulkanBuffer::WriteToBuffer(const void* data, VkDeviceSize size, VkDeviceSize offset) {
        if (size == VK_WHOLE_SIZE) {
            memcpy(m_Mapped, data, m_Size);
        } else {
            char* memOffset = static_cast<char*>(m_Mapped) + offset;
            memcpy(memOffset, data, size);
        }
    }

    void VulkanBuffer::Flush(VkDeviceSize size, VkDeviceSize offset) {
        VkMappedMemoryRange mappedRange{};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = m_Memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        vkFlushMappedMemoryRanges(m_Device->GetDevice(), 1, &mappedRange);
    }

    VkDescriptorBufferInfo VulkanBuffer::GetDescriptorInfo(VkDeviceSize size, VkDeviceSize offset) {
        return VkDescriptorBufferInfo{m_Buffer, offset, size};
    }

    void VulkanBuffer::CopyBuffer(VulkanDevice& device, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = device.BeginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        device.EndSingleTimeCommands(commandBuffer);
    }

    uint32_t VulkanBuffer::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                                          VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

}
