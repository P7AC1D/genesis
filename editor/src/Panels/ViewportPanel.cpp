#include "Panels/ViewportPanel.h"

namespace Genesis {

    void ViewportPanel::OnImGuiRender() {
        // ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        // ImGui::Begin("Viewport");
        
        // m_IsFocused = ImGui::IsWindowFocused();
        // m_IsHovered = ImGui::IsWindowHovered();
        
        // Get viewport size
        // auto viewportPanelSize = ImGui::GetContentRegionAvail();
        // m_ViewportSize = {viewportPanelSize.x, viewportPanelSize.y};
        
        // Draw framebuffer texture
        // ImGui::Image(textureID, ImVec2{m_ViewportSize.x, m_ViewportSize.y});
        
        // ImGui::End();
        // ImGui::PopStyleVar();
    }

    void ViewportPanel::OnUpdate(float deltaTime) {
        // Handle viewport-specific updates
    }

}
