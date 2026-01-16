#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace Genesis {

    class VulkanDevice;

    struct PipelineConfig {
        VkPipelineViewportStateCreateInfo ViewportState{};
        VkPipelineInputAssemblyStateCreateInfo InputAssemblyState{};
        VkPipelineRasterizationStateCreateInfo RasterizationState{};
        VkPipelineMultisampleStateCreateInfo MultisampleState{};
        VkPipelineColorBlendAttachmentState ColorBlendAttachment{};
        VkPipelineColorBlendStateCreateInfo ColorBlendState{};
        VkPipelineDepthStencilStateCreateInfo DepthStencilState{};
        std::vector<VkDynamicState> DynamicStates;
        VkPipelineDynamicStateCreateInfo DynamicStateInfo{};
        VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
        VkRenderPass RenderPass = VK_NULL_HANDLE;
        uint32_t Subpass = 0;
    };

    class VulkanPipeline {
    public:
        VulkanPipeline() = default;
        ~VulkanPipeline();

        void Init(VulkanDevice& device, const std::string& vertPath, const std::string& fragPath,
                  const PipelineConfig& config);
        void Shutdown();

        void Bind(VkCommandBuffer commandBuffer);

        VkPipeline GetPipeline() const { return m_Pipeline; }
        VkPipelineLayout GetLayout() const { return m_PipelineLayout; }

        static void DefaultPipelineConfig(PipelineConfig& config);
        static void TransparentPipelineConfig(PipelineConfig& config);

    private:
        std::vector<char> ReadFile(const std::string& filepath);
        VkShaderModule CreateShaderModule(const std::vector<char>& code);

    private:
        VulkanDevice* m_Device = nullptr;
        VkPipeline m_Pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        VkShaderModule m_VertShaderModule = VK_NULL_HANDLE;
        VkShaderModule m_FragShaderModule = VK_NULL_HANDLE;
    };

}
