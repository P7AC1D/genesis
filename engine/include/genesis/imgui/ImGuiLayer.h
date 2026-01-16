#pragma once

#include "genesis/core/Layer.h"
#include <vulkan/vulkan.h>

namespace Genesis {

    class VulkanDevice;
    class VulkanSwapchain;

    class ImGuiLayer : public Layer {
    public:
        ImGuiLayer();
        ~ImGuiLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnEvent(Event& event) override;

        void Begin();
        void End();

        void SetBlockEvents(bool block) { m_BlockEvents = block; }

    private:
        void CreateDescriptorPool();
        void SetupImGuiStyle();

    private:
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        bool m_BlockEvents = true;
    };

}
