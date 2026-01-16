#pragma once

#include <genesis/Genesis.h>
#include <memory>

namespace Genesis {

    class SceneHierarchyPanel;
    class PropertiesPanel;
    class ViewportPanel;
    class AssetBrowserPanel;

    class EditorLayer : public Layer {
    public:
        EditorLayer();
        ~EditorLayer() override;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float deltaTime) override;
        void OnRender() override;
        void OnImGuiRender() override;
        void OnEvent(Event& event) override;

    private:
        void NewScene();
        void OpenScene();
        void SaveScene();
        void SaveSceneAs();

    private:
        std::shared_ptr<Scene> m_ActiveScene;
        std::shared_ptr<Scene> m_EditorScene;

        // Editor Camera
        Camera m_EditorCamera;
        glm::vec2 m_ViewportSize{0.0f, 0.0f};
        bool m_ViewportFocused = false;
        bool m_ViewportHovered = false;

        // Panels
        std::unique_ptr<SceneHierarchyPanel> m_SceneHierarchyPanel;
        std::unique_ptr<PropertiesPanel> m_PropertiesPanel;
        std::unique_ptr<ViewportPanel> m_ViewportPanel;
        std::unique_ptr<AssetBrowserPanel> m_AssetBrowserPanel;

        // Editor State
        Entity m_SelectedEntity;
        int m_GizmoType = -1;

        // Performance
        float m_FrameTime = 0.0f;
    };

}
