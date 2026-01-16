#pragma once

#include <genesis/Genesis.h>

namespace Genesis {

    class Sandbox;

    class PlaygroundApp : public Application {
    public:
        PlaygroundApp(const ApplicationConfig& config);
        ~PlaygroundApp() override;

    protected:
        void OnInit() override;
        void OnShutdown() override;
        void OnUpdate(float deltaTime) override;

    private:
        Sandbox* m_Sandbox = nullptr;
    };

}
