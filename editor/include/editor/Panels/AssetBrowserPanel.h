#pragma once

#include <genesis/Genesis.h>
#include <filesystem>

namespace Genesis {

    class AssetBrowserPanel {
    public:
        AssetBrowserPanel();

        void OnImGuiRender();

    private:
        std::filesystem::path m_CurrentDirectory;
        std::filesystem::path m_BaseDirectory;

        // Thumbnail cache would go here
    };

}
