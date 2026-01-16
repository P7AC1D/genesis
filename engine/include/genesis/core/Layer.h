#pragma once

#include <string>

namespace Genesis {

    class Layer {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer() = default;

        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(float deltaTime) {}
        virtual void OnRender() {}
        virtual void OnImGuiRender() {}
        virtual void OnEvent(class Event& event) {}

        const std::string& GetName() const { return m_Name; }

    protected:
        std::string m_Name;
    };

}
