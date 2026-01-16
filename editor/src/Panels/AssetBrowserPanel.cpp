#include "Panels/AssetBrowserPanel.h"

namespace Genesis {

    AssetBrowserPanel::AssetBrowserPanel()
        : m_BaseDirectory("assets"), m_CurrentDirectory(m_BaseDirectory) {
    }

    void AssetBrowserPanel::OnImGuiRender() {
        // ImGui::Begin("Asset Browser");
        
        // Back button
        // if (m_CurrentDirectory != m_BaseDirectory) {
        //     if (ImGui::Button("<-")) {
        //         m_CurrentDirectory = m_CurrentDirectory.parent_path();
        //     }
        // }
        
        // Draw directory contents as icons
        // for (auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory)) {
        //     const auto& path = entry.path();
        //     auto relativePath = std::filesystem::relative(path, m_BaseDirectory);
        //     std::string filename = relativePath.filename().string();
        //     
        //     if (entry.is_directory()) {
        //         // Draw folder icon
        //     } else {
        //         // Draw file icon based on extension
        //     }
        // }
        
        // ImGui::End();
    }

}
