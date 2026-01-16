#include "genesis/core/Input.h"
#include "genesis/core/Application.h"
#include "genesis/core/Window.h"

#include <GLFW/glfw3.h>

namespace Genesis {

    bool Input::IsKeyPressed(int keycode) {
        auto* window = Application::Get().GetWindow().GetNativeWindow();
        auto state = glfwGetKey(window, keycode);
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool Input::IsMouseButtonPressed(int button) {
        auto* window = Application::Get().GetWindow().GetNativeWindow();
        auto state = glfwGetMouseButton(window, button);
        return state == GLFW_PRESS;
    }

    glm::vec2 Input::GetMousePosition() {
        auto* window = Application::Get().GetWindow().GetNativeWindow();
        double xPos, yPos;
        glfwGetCursorPos(window, &xPos, &yPos);
        return { static_cast<float>(xPos), static_cast<float>(yPos) };
    }

    float Input::GetMouseX() {
        return GetMousePosition().x;
    }

    float Input::GetMouseY() {
        return GetMousePosition().y;
    }

}
