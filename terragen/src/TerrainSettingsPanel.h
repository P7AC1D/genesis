#pragma once

#include "genesis/procedural/TerrainGenerator.h"
#include "genesis/procedural/TerrainIntent.h"
#include "genesis/procedural/TerrainDebugView.h"
#include "genesis/renderer/HeightmapTexture.h"
#include "genesis/world/ChunkManager.h"
#include <memory>

namespace Genesis
{

    class VulkanDevice;

    /**
     * TerrainSettingsPanel provides a two-mode UI for terrain configuration:
     *
     * AUTHORING MODE (default):
     *   - 8 high-level intent sliders (human concepts)
     *   - Preset dropdown for quick starting points
     *   - All mechanical parameters derived automatically
     *
     * ADVANCED MODE:
     *   - Full access to all ~30 mechanical parameters
     *   - For debugging and fine-tuning
     *   - Clearly marked as "expert only"
     *
     * This design follows the principle: "If a parameter cannot be reasoned
     * about in isolation, it should not be exposed directly."
     */
    class TerrainSettingsPanel
    {
    public:
        TerrainSettingsPanel();
        ~TerrainSettingsPanel();

        void Initialize(VulkanDevice &device, ChunkManager &chunkManager);
        void Shutdown();
        void Render();

        bool HasSettingsChanged() const { return m_SettingsChanged; }
        void ClearSettingsChanged() { m_SettingsChanged = false; }

    private:
        // Authoring mode (intent-based)
        void RenderIntentSection();
        void RenderPresetSection();

        // Advanced mode (mechanical parameters)
        void RenderAdvancedSection();
        void RenderTerrainSection();
        void RenderNoiseSection();
        void RenderRidgeNoiseSection();
        void RenderWarpingSection();
        void RenderErosionSection();

        // Shared sections
        void RenderColorSection();
        void RenderWaterSection();
        void RenderPreviewSection();
        void RenderDebugViewSection();

        void UpdateHeightmapPreview();
        void ApplySettings();
        void SyncIntentToSettings();
        void UpdatePresetSelection();

    private:
        VulkanDevice *m_Device = nullptr;
        ChunkManager *m_ChunkManager = nullptr;

        // Intent-based control (authoring mode)
        TerrainIntent m_Intent;
        int m_CurrentPresetIndex = 7; // "Custom" preset
        bool m_ShowAdvancedMode = false;

        // Mechanical settings (derived from intent, or manually edited in advanced mode)
        TerrainSettings m_TerrainSettings;

        // World settings
        float m_SeaLevelNormalized = 0.45f; // Normalized sea level (0.0-1.0)
        float m_SeaLevel = 2.0f;            // Computed absolute sea level (display only)
        bool m_WaterEnabled = true;
        bool m_UseOceanMask = true; // Use flood-fill ocean detection
        int m_ViewDistance = 3;
        uint32_t m_Seed = 42;

        std::unique_ptr<HeightmapTexture> m_HeightmapTexture;
        bool m_SettingsChanged = false;
        bool m_NeedsPreviewUpdate = true;

        // Debug view settings
        int m_DebugViewIndex = 0;
        TerrainDebugView m_DebugView;

        static constexpr int PREVIEW_SIZE = 128;
    };

}
