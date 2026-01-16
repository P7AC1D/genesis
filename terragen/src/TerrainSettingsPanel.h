#pragma once

#include "genesis/procedural/TerrainGenerator.h"
#include "genesis/renderer/HeightmapTexture.h"
#include "genesis/world/ChunkManager.h"
#include <memory>

namespace Genesis {

    class VulkanDevice;

    class TerrainSettingsPanel {
    public:
        TerrainSettingsPanel();
        ~TerrainSettingsPanel();

        void Initialize(VulkanDevice& device, ChunkManager& chunkManager);
        void Shutdown();
        void Render();

        bool HasSettingsChanged() const { return m_SettingsChanged; }
        void ClearSettingsChanged() { m_SettingsChanged = false; }

    private:
        void RenderTerrainSection();
        void RenderNoiseSection();
        void RenderWarpingSection();
        void RenderColorSection();
        void RenderWaterSection();
        void RenderPreviewSection();
        void UpdateHeightmapPreview();
        void ApplySettings();

    private:
        VulkanDevice* m_Device = nullptr;
        ChunkManager* m_ChunkManager = nullptr;

        TerrainSettings m_TerrainSettings;
        float m_SeaLevel = 2.0f;
        bool m_WaterEnabled = true;
        int m_ViewDistance = 3;
        uint32_t m_Seed = 42;

        std::unique_ptr<HeightmapTexture> m_HeightmapTexture;
        bool m_SettingsChanged = false;
        bool m_NeedsPreviewUpdate = true;

        static constexpr int PREVIEW_SIZE = 128;
    };

}
