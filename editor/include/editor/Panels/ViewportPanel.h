#pragma once

#include <genesis/Genesis.h>
#include <glm/glm.hpp>

namespace Genesis {

    class ViewportPanel {
    public:
        ViewportPanel() = default;

        void OnImGuiRender();
        void OnUpdate(float deltaTime);

        glm::vec2 GetSize() const { return m_ViewportSize; }
        bool IsFocused() const { return m_IsFocused; }
        bool IsHovered() const { return m_IsHovered; }

    private:
        glm::vec2 m_ViewportSize{0.0f, 0.0f};
        glm::vec2 m_ViewportBounds[2];
        bool m_IsFocused = false;
        bool m_IsHovered = false;
    };

}
