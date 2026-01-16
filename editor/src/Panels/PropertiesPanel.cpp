#include "Panels/PropertiesPanel.h"

namespace Genesis {

    void PropertiesPanel::OnImGuiRender() {
        // ImGui::Begin("Properties");
        if (m_SelectedEntity) {
            DrawComponents(m_SelectedEntity);
        }
        // ImGui::End();
    }

    void PropertiesPanel::DrawComponents(Entity entity) {
        // Draw all component editors for selected entity
        // - Transform
        // - Mesh Renderer
        // - Camera
        // - Lights
        // - etc.
    }

}
