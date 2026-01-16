#pragma once

#include <genesis/Genesis.h>

namespace Genesis {

    class EditorLayer;

    class EditorApp : public Application {
    public:
        EditorApp(const ApplicationConfig& config);
        ~EditorApp() override;

    protected:
        void OnInit() override;
        void OnShutdown() override;
        void OnUpdate(float deltaTime) override;

    private:
        EditorLayer* m_EditorLayer = nullptr;
    };

}
