#include "genesis/renderer/VulkanPipeline.h"
#include "genesis/renderer/VulkanDevice.h"
#include "genesis/renderer/Mesh.h"
#include "genesis/core/Log.h"

#include <fstream>
#include <stdexcept>

namespace Genesis {

    VulkanPipeline::~VulkanPipeline() {
        if (m_Pipeline != VK_NULL_HANDLE) {
            Shutdown();
        }
    }

    void VulkanPipeline::Init(VulkanDevice& device, const std::string& vertPath, const std::string& fragPath,
                              const PipelineConfig& config) {
        m_Device = &device;

        auto vertShaderCode = ReadFile(vertPath);
        auto fragShaderCode = ReadFile(fragPath);

        m_VertShaderModule = CreateShaderModule(vertShaderCode);
        m_FragShaderModule = CreateShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo shaderStages[2];

        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = m_VertShaderModule;
        shaderStages[0].pName = "main";
        shaderStages[0].flags = 0;
        shaderStages[0].pNext = nullptr;
        shaderStages[0].pSpecializationInfo = nullptr;

        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = m_FragShaderModule;
        shaderStages[1].pName = "main";
        shaderStages[1].flags = 0;
        shaderStages[1].pNext = nullptr;
        shaderStages[1].pSpecializationInfo = nullptr;

        auto bindingDescriptions = Vertex::GetBindingDescriptions();
        auto attributeDescriptions = Vertex::GetAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        m_PipelineLayout = config.PipelineLayout;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &config.InputAssemblyState;
        pipelineInfo.pViewportState = &config.ViewportState;
        pipelineInfo.pRasterizationState = &config.RasterizationState;
        pipelineInfo.pMultisampleState = &config.MultisampleState;
        pipelineInfo.pColorBlendState = &config.ColorBlendState;
        pipelineInfo.pDepthStencilState = &config.DepthStencilState;
        pipelineInfo.pDynamicState = &config.DynamicStateInfo;
        pipelineInfo.layout = config.PipelineLayout;
        pipelineInfo.renderPass = config.RenderPass;
        pipelineInfo.subpass = config.Subpass;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(m_Device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }
    }

    void VulkanPipeline::Shutdown() {
        VkDevice device = m_Device->GetDevice();

        if (m_VertShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, m_VertShaderModule, nullptr);
            m_VertShaderModule = VK_NULL_HANDLE;
        }

        if (m_FragShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, m_FragShaderModule, nullptr);
            m_FragShaderModule = VK_NULL_HANDLE;
        }

        if (m_Pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, m_Pipeline, nullptr);
            m_Pipeline = VK_NULL_HANDLE;
        }
    }

    void VulkanPipeline::Bind(VkCommandBuffer commandBuffer) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    }

    void VulkanPipeline::DefaultPipelineConfig(PipelineConfig& config) {
        config.InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        config.InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        config.InputAssemblyState.primitiveRestartEnable = VK_FALSE;

        config.ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        config.ViewportState.viewportCount = 1;
        config.ViewportState.pViewports = nullptr;
        config.ViewportState.scissorCount = 1;
        config.ViewportState.pScissors = nullptr;

        config.RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        config.RasterizationState.depthClampEnable = VK_FALSE;
        config.RasterizationState.rasterizerDiscardEnable = VK_FALSE;
        config.RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
        config.RasterizationState.lineWidth = 1.0f;
        config.RasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
        config.RasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        config.RasterizationState.depthBiasEnable = VK_FALSE;

        config.MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        config.MultisampleState.sampleShadingEnable = VK_FALSE;
        config.MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        config.ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        config.ColorBlendAttachment.blendEnable = VK_FALSE;

        config.ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        config.ColorBlendState.logicOpEnable = VK_FALSE;
        config.ColorBlendState.attachmentCount = 1;
        config.ColorBlendState.pAttachments = &config.ColorBlendAttachment;

        config.DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        config.DepthStencilState.depthTestEnable = VK_TRUE;
        config.DepthStencilState.depthWriteEnable = VK_TRUE;
        config.DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
        config.DepthStencilState.depthBoundsTestEnable = VK_FALSE;
        config.DepthStencilState.stencilTestEnable = VK_FALSE;

        config.DynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        config.DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        config.DynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(config.DynamicStates.size());
        config.DynamicStateInfo.pDynamicStates = config.DynamicStates.data();
    }

    void VulkanPipeline::TransparentPipelineConfig(PipelineConfig& config) {
        // Start with default config
        DefaultPipelineConfig(config);
        
        // Enable alpha blending
        config.ColorBlendAttachment.blendEnable = VK_TRUE;
        config.ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        config.ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        config.ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        config.ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        config.ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        config.ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        
        // Still do depth test but don't write to depth buffer for transparency
        config.DepthStencilState.depthWriteEnable = VK_FALSE;
        
        // Disable backface culling for water (visible from both sides)
        config.RasterizationState.cullMode = VK_CULL_MODE_NONE;
    }

    std::vector<char> VulkanPipeline::ReadFile(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filepath);
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    VkShaderModule VulkanPipeline::CreateShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(m_Device->GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module!");
        }

        return shaderModule;
    }

}
