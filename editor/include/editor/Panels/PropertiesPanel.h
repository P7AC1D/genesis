#pragma once

#include <genesis/Genesis.h>

namespace Genesis {

    class PropertiesPanel {
    public:
        PropertiesPanel() = default;

        void OnImGuiRender();
        void SetSelectedEntity(Entity entity) { m_SelectedEntity = entity; }

    private:
        void DrawComponents(Entity entity);

        template<typename T>
        void DrawComponent(const std::string& name, Entity entity, void (*uiFunction)(T&));

    private:
        Entity m_SelectedEntity;
    };

}
