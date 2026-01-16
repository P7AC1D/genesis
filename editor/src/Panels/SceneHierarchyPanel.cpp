#include "Panels/SceneHierarchyPanel.h"

namespace Genesis {

    SceneHierarchyPanel::SceneHierarchyPanel(const std::shared_ptr<Scene>& scene)
        : m_Scene(scene) {
    }

    void SceneHierarchyPanel::SetScene(const std::shared_ptr<Scene>& scene) {
        m_Scene = scene;
        m_SelectedEntity = {};
    }

    void SceneHierarchyPanel::OnImGuiRender() {
        // ImGui::Begin("Scene Hierarchy");
        // Draw all entities in scene
        // ImGui::End();
    }

    void SceneHierarchyPanel::DrawEntityNode(Entity entity) {
        // Draw entity in tree view
    }

    void SceneHierarchyPanel::DrawComponents(Entity entity) {
        // Draw component editors
    }

}
