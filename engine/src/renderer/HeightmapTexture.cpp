#include "genesis/renderer/HeightmapTexture.h"
#include "genesis/renderer/VulkanDevice.h"
#include "genesis/core/Log.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include <cstring>
#include <algorithm>
#include <cmath>

namespace Genesis
{

    HeightmapTexture::~HeightmapTexture()
    {
        Destroy();
    }

    void HeightmapTexture::Create(VulkanDevice &device, int width, int height)
    {
        m_Device = &device;
        m_Width = width;
        m_Height = height;

        CreateImage(device);
        CreateSampler(device);
        CreateDescriptorSet(device);

        GEN_DEBUG("HeightmapTexture created ({}x{})", width, height);
    }

    void HeightmapTexture::Destroy()
    {
        if (!m_Device)
            return;

        VkDevice device = m_Device->GetDevice();
        m_Device->WaitIdle();

        if (m_DescriptorSet != VK_NULL_HANDLE)
        {
            ImGui_ImplVulkan_RemoveTexture(m_DescriptorSet);
            m_DescriptorSet = VK_NULL_HANDLE;
        }

        if (m_StagingBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(device, m_StagingBuffer, nullptr);
            m_StagingBuffer = VK_NULL_HANDLE;
        }
        if (m_StagingMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, m_StagingMemory, nullptr);
            m_StagingMemory = VK_NULL_HANDLE;
        }
        if (m_Sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(device, m_Sampler, nullptr);
            m_Sampler = VK_NULL_HANDLE;
        }
        if (m_ImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, m_ImageView, nullptr);
            m_ImageView = VK_NULL_HANDLE;
        }
        if (m_Image != VK_NULL_HANDLE)
        {
            vkDestroyImage(device, m_Image, nullptr);
            m_Image = VK_NULL_HANDLE;
        }
        if (m_ImageMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, m_ImageMemory, nullptr);
            m_ImageMemory = VK_NULL_HANDLE;
        }

        m_Device = nullptr;
    }

    void HeightmapTexture::CreateImage(VulkanDevice &device)
    {
        VkDevice vkDevice = device.GetDevice();

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(m_Width);
        imageInfo.extent.height = static_cast<uint32_t>(m_Height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        if (vkCreateImage(vkDevice, &imageInfo, nullptr, &m_Image) != VK_SUCCESS)
        {
            GEN_ERROR("Failed to create heightmap image!");
            return;
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(vkDevice, m_Image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(device, memRequirements.memoryTypeBits,
                                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &m_ImageMemory) != VK_SUCCESS)
        {
            GEN_ERROR("Failed to allocate heightmap image memory!");
            return;
        }

        vkBindImageMemory(vkDevice, m_Image, m_ImageMemory, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_Image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(vkDevice, &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS)
        {
            GEN_ERROR("Failed to create heightmap image view!");
            return;
        }

        VkDeviceSize bufferSize = m_Width * m_Height * 4;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &m_StagingBuffer) != VK_SUCCESS)
        {
            GEN_ERROR("Failed to create staging buffer!");
            return;
        }

        VkMemoryRequirements stagingMemReq;
        vkGetBufferMemoryRequirements(vkDevice, m_StagingBuffer, &stagingMemReq);

        VkMemoryAllocateInfo stagingAllocInfo{};
        stagingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        stagingAllocInfo.allocationSize = stagingMemReq.size;
        stagingAllocInfo.memoryTypeIndex = FindMemoryType(device, stagingMemReq.memoryTypeBits,
                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(vkDevice, &stagingAllocInfo, nullptr, &m_StagingMemory) != VK_SUCCESS)
        {
            GEN_ERROR("Failed to allocate staging buffer memory!");
            return;
        }

        vkBindBufferMemory(vkDevice, m_StagingBuffer, m_StagingMemory, 0);
    }

    void HeightmapTexture::CreateSampler(VulkanDevice &device)
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(device.GetDevice(), &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS)
        {
            GEN_ERROR("Failed to create heightmap sampler!");
        }
    }

    void HeightmapTexture::CreateDescriptorSet(VulkanDevice &device)
    {
        m_DescriptorSet = ImGui_ImplVulkan_AddTexture(m_Sampler, m_ImageView,
                                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void HeightmapTexture::Update(const std::vector<float> &heightData, float minHeight, float maxHeight)
    {
        if (!m_Device || m_Image == VK_NULL_HANDLE)
            return;

        VkDevice device = m_Device->GetDevice();

        std::vector<uint8_t> pixels(m_Width * m_Height * 4);

        float range = maxHeight - minHeight;
        if (range < 0.001f)
            range = 1.0f;

        for (int y = 0; y < m_Height; y++)
        {
            for (int x = 0; x < m_Width; x++)
            {
                int idx = y * m_Width + x;
                float h = (idx < static_cast<int>(heightData.size())) ? heightData[idx] : 0.0f;
                float normalized = (h - minHeight) / range;
                normalized = std::clamp(normalized, 0.0f, 1.0f);

                uint8_t gray = static_cast<uint8_t>(normalized * 255.0f);

                int pixelIdx = idx * 4;
                pixels[pixelIdx + 0] = gray;
                pixels[pixelIdx + 1] = gray;
                pixels[pixelIdx + 2] = gray;
                pixels[pixelIdx + 3] = 255;
            }
        }

        void *data;
        vkMapMemory(device, m_StagingMemory, 0, pixels.size(), 0, &data);
        memcpy(data, pixels.data(), pixels.size());
        vkUnmapMemory(device, m_StagingMemory);

        VkCommandBuffer commandBuffer = m_Device->BeginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_Image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height), 1};

        vkCmdCopyBufferToImage(commandBuffer, m_StagingBuffer, m_Image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);

        m_Device->EndSingleTimeCommands(commandBuffer);
    }

    uint32_t HeightmapTexture::FindMemoryType(VulkanDevice &device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(device.GetPhysicalDevice(), &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        GEN_ERROR("Failed to find suitable memory type!");
        return 0;
    }

}
