#pragma once

#include <genesis/Genesis.h>
#include <memory>

namespace Genesis {

    class SceneHierarchyPanel {
    public:
        SceneHierarchyPanel() = default;
        SceneHierarchyPanel(const std::shared_ptr<Scene>& scene);

        void SetScene(const std::shared_ptr<Scene>& scene);
        void OnImGuiRender();

        Entity GetSelectedEntity() const { return m_SelectedEntity; }
        void SetSelectedEntity(Entity entity) { m_SelectedEntity = entity; }

    private:
        void DrawEntityNode(Entity entity);
        void DrawComponents(Entity entity);

    private:
        std::shared_ptr<Scene> m_Scene;
        Entity m_SelectedEntity;
    };

}
