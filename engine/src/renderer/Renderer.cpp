#include "genesis/renderer/Renderer.h"
#include "genesis/renderer/VulkanContext.h"
#include "genesis/renderer/VulkanDevice.h"
#include "genesis/renderer/VulkanSwapchain.h"
#include "genesis/renderer/VulkanPipeline.h"
#include "genesis/renderer/VulkanBuffer.h"
#include "genesis/renderer/Camera.h"
#include "genesis/renderer/Mesh.h"
#include "genesis/core/Window.h"
#include "genesis/core/Log.h"

#include <array>
#include <cmath>
#include <stdexcept>

namespace Genesis {

    Renderer::Renderer() = default;
    Renderer::~Renderer() = default;

    void Renderer::Init(Window& window) {
        GEN_INFO("Initializing Vulkan renderer...");
        m_Window = &window;

        m_Context = std::make_unique<VulkanContext>();
        m_Context->Init(window.GetNativeWindow(), true);

        m_Device = std::make_unique<VulkanDevice>();
        m_Device->Init(m_Context->GetInstance(), m_Context->GetSurface());

        m_Swapchain = std::make_unique<VulkanSwapchain>();
        m_Swapchain->Init(*m_Device, m_Context->GetSurface(), window.GetWidth(), window.GetHeight());

        CreateCommandBuffers();
        CreateSyncObjects();
        CreateDescriptorSetLayout();
        CreatePipelineLayout();
        CreatePipeline();
        CreateWaterPipeline();
        CreateUniformBuffers();
        CreateDescriptorPool();
        CreateDescriptorSets();

        GEN_INFO("Vulkan renderer initialized!");
    }

    void Renderer::Shutdown() {
        m_Device->WaitIdle();

        VkDevice device = m_Device->GetDevice();

        // Cleanup uniform buffers
        for (auto& buffer : m_UniformBuffers) {
            if (buffer) buffer->Shutdown();
        }
        m_UniformBuffers.clear();

        // Cleanup descriptor resources
        if (m_DescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;
        }

        if (m_DescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
            m_DescriptorSetLayout = VK_NULL_HANDLE;
        }

        if (m_PipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
            m_PipelineLayout = VK_NULL_HANDLE;
        }

        // Cleanup pipeline
        if (m_WaterPipeline) m_WaterPipeline->Shutdown();
        if (m_Pipeline) m_Pipeline->Shutdown();

        // Cleanup sync objects
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, m_ImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device, m_RenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, m_InFlightFences[i], nullptr);
        }

        // Cleanup swapchain and device
        if (m_Swapchain) m_Swapchain->Shutdown();
        if (m_Device) m_Device->Shutdown();
        if (m_Context) m_Context->Shutdown();

        GEN_INFO("Vulkan renderer shutdown complete.");
    }

    void Renderer::CreateCommandBuffers() {
        m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_Device->GetCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

        if (vkAllocateCommandBuffers(m_Device->GetDevice(), &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers!");
        }
    }

    void Renderer::CreateSyncObjects() {
        m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        m_ImagesInFlight.resize(m_Swapchain->GetImageCount(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(m_Device->GetDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_Device->GetDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_Device->GetDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create synchronization objects!");
            }
        }
    }

    void Renderer::CreateDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(m_Device->GetDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }
    }

    void Renderer::CreatePipelineLayout() {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(m_Device->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }
    }

    void Renderer::CreatePipeline() {
        PipelineConfig config{};
        VulkanPipeline::DefaultPipelineConfig(config);
        config.RenderPass = m_Swapchain->GetRenderPass();
        config.PipelineLayout = m_PipelineLayout;

        m_Pipeline = std::make_unique<VulkanPipeline>();
        m_Pipeline->Init(*m_Device, "assets/shaders/lowpoly.vert.spv", "assets/shaders/lowpoly.frag.spv", config);
    }

    void Renderer::CreateWaterPipeline() {
        PipelineConfig config{};
        VulkanPipeline::TransparentPipelineConfig(config);
        config.RenderPass = m_Swapchain->GetRenderPass();
        config.PipelineLayout = m_PipelineLayout;

        m_WaterPipeline = std::make_unique<VulkanPipeline>();
        m_WaterPipeline->Init(*m_Device, "assets/shaders/water.vert.spv", "assets/shaders/water.frag.spv", config);
    }

    void Renderer::CreateUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(GlobalUBO);
        m_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_UniformBuffers[i] = std::make_unique<VulkanBuffer>();
            m_UniformBuffers[i]->Init(*m_Device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            m_UniformBuffers[i]->Map();
        }
    }

    void Renderer::CreateDescriptorPool() {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(m_Device->GetDevice(), &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool!");
        }
    }

    void Renderer::CreateDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_DescriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        m_DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(m_Device->GetDevice(), &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo = m_UniformBuffers[i]->GetDescriptorInfo(sizeof(GlobalUBO));

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_DescriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &descriptorWrite, 0, nullptr);
        }
    }

    bool Renderer::BeginFrame() {
        VkDevice device = m_Device->GetDevice();

        // Wait for the frame we're about to render to be done
        vkWaitForFences(device, 1, &m_InFlightFences[m_CurrentFrameIndex], VK_TRUE, UINT64_MAX);

        // Acquire next image - use frame-indexed semaphore for acquisition
        uint32_t imageIndex;
        VkResult result = m_Swapchain->AcquireNextImage(m_ImageAvailableSemaphores[m_CurrentFrameIndex], &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapchain();
            return false;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        m_CurrentImageIndex = imageIndex;

        // Check if a previous frame is using this image (i.e. there is its fence to wait on)
        if (m_ImagesInFlight[m_CurrentImageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(device, 1, &m_ImagesInFlight[m_CurrentImageIndex], VK_TRUE, UINT64_MAX);
        }
        // Mark the image as now being in use by this frame
        m_ImagesInFlight[m_CurrentImageIndex] = m_InFlightFences[m_CurrentFrameIndex];

        vkResetFences(device, 1, &m_InFlightFences[m_CurrentFrameIndex]);

        // Begin command buffer
        VkCommandBuffer cmd = m_CommandBuffers[m_CurrentFrameIndex];
        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        // Begin render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_Swapchain->GetRenderPass();
        renderPassInfo.framebuffer = m_Swapchain->GetFramebuffer(m_CurrentImageIndex);
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_Swapchain->GetExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.1f, 0.1f, 0.15f, 1.0f}};  // Dark blue-gray background
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Set viewport and scissor
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_Swapchain->GetExtent().width);
        viewport.height = static_cast<float>(m_Swapchain->GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_Swapchain->GetExtent();
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        m_FrameStarted = true;
        ResetStats();
        return true;
    }

    void Renderer::EndFrame() {
        if (!m_FrameStarted) return;

        VkCommandBuffer cmd = m_CommandBuffers[m_CurrentFrameIndex];

        // End render pass and command buffer
        vkCmdEndRenderPass(cmd);

        if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }

        // Submit command buffer - use frame-indexed semaphores
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFrameIndex]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFrameIndex]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrameIndex]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        // Present
        VkResult result = m_Swapchain->Present(m_Device->GetPresentQueue(), m_RenderFinishedSemaphores[m_CurrentFrameIndex], m_CurrentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_SwapchainNeedsRecreation) {
            m_SwapchainNeedsRecreation = false;
            RecreateSwapchain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image!");
        }

        m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
        m_FrameStarted = false;
    }

    void Renderer::BeginScene(const Camera& camera) {
        // Update uniform buffer with camera matrices
        m_GlobalUBO.ViewMatrix = camera.GetViewMatrix();
        m_GlobalUBO.ProjectionMatrix = camera.GetProjectionMatrix();
        // Pack time into w component for water shader
        m_GlobalUBO.CameraPosition = glm::vec4(camera.GetPosition(), m_Time);
        
        // Update lighting data from LightManager
        const auto& dirLight = m_LightManager.GetDirectionalLight();
        m_GlobalUBO.SunDirection = glm::vec4(glm::normalize(dirLight.Direction), 0.0f);
        m_GlobalUBO.SunColor = glm::vec4(dirLight.Color, dirLight.Intensity);
        
        const auto& settings = m_LightManager.GetSettings();
        m_GlobalUBO.AmbientColor = glm::vec4(settings.AmbientColor, settings.AmbientIntensity);
        m_GlobalUBO.FogColorAndDensity = glm::vec4(settings.FogColor, settings.FogDensity);
        
        // Update point lights
        const auto& pointLights = m_LightManager.GetPointLights();
        m_GlobalUBO.NumPointLights.x = static_cast<int>(pointLights.size());
        
        for (size_t i = 0; i < pointLights.size() && i < MAX_POINT_LIGHTS; i++) {
            const auto& light = pointLights[i];
            m_GlobalUBO.PointLights[i].PositionAndIntensity = glm::vec4(light.Position, light.Intensity);
            // Calculate radius from attenuation (where light contribution becomes negligible)
            float radius = 16.0f / std::sqrt(light.Quadratic);
            m_GlobalUBO.PointLights[i].ColorAndRadius = glm::vec4(light.Color, radius);
        }

        m_UniformBuffers[m_CurrentFrameIndex]->WriteToBuffer(&m_GlobalUBO, sizeof(GlobalUBO));

        // Bind pipeline and descriptor sets
        VkCommandBuffer cmd = m_CommandBuffers[m_CurrentFrameIndex];
        m_Pipeline->Bind(cmd);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1,
                                &m_DescriptorSets[m_CurrentFrameIndex], 0, nullptr);
    }

    void Renderer::EndScene() {
        // Flush any batched draw calls (for future optimization)
    }

    void Renderer::Draw(const Mesh& mesh, const glm::mat4& transform) {
        if (!m_FrameStarted) return;

        VkCommandBuffer cmd = m_CommandBuffers[m_CurrentFrameIndex];

        // Push model matrix
        PushConstantData push{};
        push.ModelMatrix = transform;
        push.NormalMatrix = glm::transpose(glm::inverse(transform));

        vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &push);

        // Bind and draw mesh
        mesh.Bind(cmd);
        mesh.Draw(cmd);

        m_Stats.DrawCalls++;
        m_Stats.TriangleCount += mesh.GetIndexCount() / 3;
    }

    void Renderer::DrawWater(const Mesh& mesh, const glm::mat4& transform) {
        if (!m_FrameStarted || !m_WaterSettings.enabled) return;

        VkCommandBuffer cmd = m_CommandBuffers[m_CurrentFrameIndex];
        
        // Switch to water pipeline
        m_WaterPipeline->Bind(cmd);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1,
                                &m_DescriptorSets[m_CurrentFrameIndex], 0, nullptr);

        // Push model matrix
        PushConstantData push{};
        push.ModelMatrix = transform;
        push.NormalMatrix = glm::transpose(glm::inverse(transform));

        vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &push);

        // Bind and draw mesh
        mesh.Bind(cmd);
        mesh.Draw(cmd);

        m_Stats.DrawCalls++;
        m_Stats.TriangleCount += mesh.GetIndexCount() / 3;
        
        // Switch back to opaque pipeline for any subsequent draws
        m_Pipeline->Bind(cmd);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1,
                                &m_DescriptorSets[m_CurrentFrameIndex], 0, nullptr);
    }

    void Renderer::RenderScene(Scene& scene) {
        // Iterate through scene entities and render (future implementation)
    }

    void Renderer::OnWindowResize(uint32_t width, uint32_t height) {
        m_SwapchainNeedsRecreation = true;
    }

    void Renderer::RecreateSwapchain() {
        uint32_t width = m_Window->GetWidth();
        uint32_t height = m_Window->GetHeight();

        while (width == 0 || height == 0) {
            // Window is minimized, wait
            m_Window->PollEvents();
            width = m_Window->GetWidth();
            height = m_Window->GetHeight();
        }

        m_Device->WaitIdle();
        m_Swapchain->Recreate(width, height);

        // Reset image tracking
        m_ImagesInFlight.clear();
        m_ImagesInFlight.resize(m_Swapchain->GetImageCount(), VK_NULL_HANDLE);
    }

    void Renderer::ResetStats() {
        m_Stats.DrawCalls = 0;
        m_Stats.TriangleCount = 0;
    }

}
