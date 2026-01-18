#include "TerrainSettingsPanel.h"
#include "genesis/renderer/VulkanDevice.h"
#include "genesis/procedural/TerrainGenerator.h"
#include "genesis/core/Log.h"

#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace Genesis
{

    TerrainSettingsPanel::TerrainSettingsPanel() = default;
    TerrainSettingsPanel::~TerrainSettingsPanel() = default;

    void TerrainSettingsPanel::Initialize(VulkanDevice &device, ChunkManager &chunkManager)
    {
        m_Device = &device;
        m_ChunkManager = &chunkManager;

        const auto &worldSettings = chunkManager.GetSettings();
        m_TerrainSettings = worldSettings.terrainSettings;
        m_SeaLevelNormalized = worldSettings.seaLevelNormalized;
        m_SeaLevel = worldSettings.seaLevel;
        m_WaterEnabled = worldSettings.waterEnabled;
        m_UseOceanMask = worldSettings.useOceanMask;
        m_ViewDistance = worldSettings.viewDistance;
        m_Seed = worldSettings.seed;

        // Initialize with default "Rolling Temperate" preset
        const auto &presets = TerrainIntentMapper::GetPresets();
        m_CurrentPresetIndex = 4; // "Rolling Temperate"
        if (m_CurrentPresetIndex < static_cast<int>(presets.size()))
        {
            m_Intent = presets[m_CurrentPresetIndex].intent;
        }
        SyncIntentToSettings();

        m_HeightmapTexture = std::make_unique<HeightmapTexture>();
        m_HeightmapTexture->Create(device, PREVIEW_SIZE, PREVIEW_SIZE);

        m_NeedsPreviewUpdate = true;
    }

    void TerrainSettingsPanel::Shutdown()
    {
        if (m_HeightmapTexture)
        {
            m_HeightmapTexture->Destroy();
            m_HeightmapTexture.reset();
        }
        m_Device = nullptr;
        m_ChunkManager = nullptr;
    }

    void TerrainSettingsPanel::Render()
    {
        ImGui::SetNextWindowSize(ImVec2(380, 750), ImGuiCond_FirstUseEver);
        ImGui::Begin("Terrain Settings", nullptr, ImGuiWindowFlags_NoCollapse);

        RenderPreviewSection();
        ImGui::Separator();

        // Mode toggle at the top
        ImGui::Checkbox("Advanced Mode", &m_ShowAdvancedMode);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Toggle between Authoring mode (intent-based, recommended)\nand Advanced mode (raw parameters, expert only).");

        ImGui::Separator();

        if (!m_ShowAdvancedMode)
        {
            // Authoring mode: Intent + Presets
            RenderPresetSection();
            ImGui::Separator();
            RenderIntentSection();
        }
        else
        {
            // Advanced mode: All mechanical parameters
            RenderAdvancedSection();
        }

        ImGui::Separator();
        RenderColorSection();
        ImGui::Separator();
        RenderWaterSection();
        ImGui::Separator();
        RenderDebugViewSection();
        ImGui::Separator();

        ImGui::Spacing();
        if (ImGui::Button("Apply Changes", ImVec2(-1, 30)))
        {
            GEN_INFO("Apply Changes button clicked!");
            ApplySettings();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Rebuilds terrain chunks using the current settings.\nTip: the preview updates live; the world updates when you Apply.");

        if (m_NeedsPreviewUpdate)
        {
            UpdateHeightmapPreview();
            m_NeedsPreviewUpdate = false;
        }

        ImGui::End();
    }

    void TerrainSettingsPanel::RenderPresetSection()
    {
        if (ImGui::CollapsingHeader("Terrain Presets", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const auto &presets = TerrainIntentMapper::GetPresets();

            // Preset dropdown
            if (ImGui::BeginCombo("Preset", presets[m_CurrentPresetIndex].name.c_str()))
            {
                for (int i = 0; i < static_cast<int>(presets.size()); i++)
                {
                    bool isSelected = (m_CurrentPresetIndex == i);
                    if (ImGui::Selectable(presets[i].name.c_str(), isSelected))
                    {
                        m_CurrentPresetIndex = i;
                        m_Intent = presets[i].intent;
                        SyncIntentToSettings();
                        m_NeedsPreviewUpdate = true;
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();

                    // Show description on hover
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", presets[i].description.c_str());
                }
                ImGui::EndCombo();
            }

            // Show current preset description
            ImGui::TextWrapped("%s", presets[m_CurrentPresetIndex].description.c_str());
        }
    }

    void TerrainSettingsPanel::RenderIntentSection()
    {
        if (ImGui::CollapsingHeader("Terrain Intent", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextWrapped("These 8 controls define the character of your terrain.\nAll mechanical parameters are derived automatically.");
            ImGui::Spacing();

            bool changed = false;

            // World-scale structure
            ImGui::Text("World Structure:");
            if (ImGui::SliderFloat("Continental Scale", &m_Intent.continentalScale, 0.0f, 1.0f, "%.2f"))
                changed = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Size of major landmasses.\n0 = small islands\n1 = vast continents");

            if (ImGui::SliderFloat("Elevation Range", &m_Intent.elevationRange, 0.0f, 1.0f, "%.2f"))
                changed = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Overall height contrast.\n0 = flat terrain\n1 = extreme vertical relief");

            ImGui::Spacing();
            ImGui::Text("Mountains:");
            if (ImGui::SliderFloat("Mountain Coverage", &m_Intent.mountainCoverage, 0.0f, 1.0f, "%.2f"))
                changed = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("What percentage of the world is mountainous.\n0 = plains only\n1 = mountains everywhere");

            if (ImGui::SliderFloat("Mountain Sharpness", &m_Intent.mountainSharpness, 0.0f, 1.0f, "%.2f"))
                changed = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Peak shape character.\n0 = rounded, ancient hills\n1 = jagged, dramatic peaks");

            ImGui::Spacing();
            ImGui::Text("Surface Character:");
            if (ImGui::SliderFloat("Ruggedness", &m_Intent.ruggedness, 0.0f, 1.0f, "%.2f"))
                changed = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Small-scale surface roughness.\n0 = smooth, gentle terrain\n1 = highly detailed, noisy");

            if (ImGui::SliderFloat("Erosion Age", &m_Intent.erosionAge, 0.0f, 1.0f, "%.2f"))
                changed = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Degree of weathering.\n0 = young, fresh terrain\n1 = ancient, heavily eroded");

            ImGui::Spacing();
            ImGui::Text("Hydrology:");
            if (ImGui::SliderFloat("River Strength", &m_Intent.riverStrength, 0.0f, 1.0f, "%.2f"))
                changed = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Valley carving dominance.\n0 = weak streams\n1 = dominant river systems");

            ImGui::Spacing();
            ImGui::Text("Stylistic:");
            if (ImGui::SliderFloat("Chaos", &m_Intent.chaos, 0.0f, 1.0f, "%.2f"))
                changed = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Breaks symmetry and predictability.\n0 = orderly, predictable terrain\n1 = wild, chaotic formations");

            if (changed)
            {
                SyncIntentToSettings();
                UpdatePresetSelection();
                m_NeedsPreviewUpdate = true;
            }

            // Seed control (independent of intent)
            ImGui::Spacing();
            ImGui::Separator();
            int seed = static_cast<int>(m_Seed);
            if (ImGui::InputInt("Seed", &seed))
            {
                m_Seed = static_cast<uint32_t>(std::max(0, seed));
                m_NeedsPreviewUpdate = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Random seed for generation.\nSame seed + same intent = same world pattern.");

            if (ImGui::SliderInt("View Distance", &m_ViewDistance, 1, 6))
            {
                // View distance affects chunk loading, not preview
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("How many chunks load around the camera.\nHigher = see farther but costs performance.");
        }
    }

    void TerrainSettingsPanel::RenderAdvancedSection()
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
        ImGui::TextWrapped("ADVANCED MODE: Raw mechanical parameters.\nChanges here may create unrealistic terrain.");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        RenderTerrainSection();
        ImGui::Separator();
        RenderNoiseSection();
        ImGui::Separator();
        RenderRidgeNoiseSection();
        ImGui::Separator();
        RenderWarpingSection();
        ImGui::Separator();
        RenderErosionSection();
    }

    void TerrainSettingsPanel::RenderPreviewSection()
    {
        if (ImGui::CollapsingHeader("Heightmap Preview", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (m_HeightmapTexture && m_HeightmapTexture->IsValid())
            {
                float availWidth = ImGui::GetContentRegionAvail().x;
                float previewSize = std::min(availWidth, 256.0f);

                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - previewSize) * 0.5f);
                ImGui::Image(reinterpret_cast<ImTextureID>(m_HeightmapTexture->GetDescriptorSet()),
                             ImVec2(previewSize, previewSize));

                if (ImGui::Button("Refresh Preview", ImVec2(-1, 0)))
                {
                    m_NeedsPreviewUpdate = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Recomputes the preview texture using the current UI values.\nThis does not regenerate the world until Apply.");
            }
            else
            {
                ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "Preview unavailable");
            }
        }
    }

    void TerrainSettingsPanel::RenderTerrainSection()
    {
        if (ImGui::CollapsingHeader("Terrain", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::SliderFloat("Height Scale", &m_TerrainSettings.heightScale, 1.0f, 50.0f, "%.1f"))
            {
                m_NeedsPreviewUpdate = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Vertical exaggeration (world units).\nHigher = taller mountains and deeper valleys.");

            if (ImGui::SliderFloat("Base Height", &m_TerrainSettings.baseHeight, -20.0f, 20.0f, "%.1f"))
            {
                m_NeedsPreviewUpdate = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Adds a constant offset to all terrain heights (world units).\nUse this to raise/lower the entire landscape.");

            int seed = static_cast<int>(m_Seed);
            if (ImGui::InputInt("Seed", &seed))
            {
                m_Seed = static_cast<uint32_t>(std::max(0, seed));
                m_NeedsPreviewUpdate = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Random seed for generation.\nSame seed + same settings = same world pattern.");

            if (ImGui::SliderInt("View Distance", &m_ViewDistance, 1, 6))
            {
                // View distance affects chunk loading, not preview
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("How many chunks load around the camera.\nHigher = see farther but costs CPU/GPU and memory.");
        }
    }

    void TerrainSettingsPanel::RenderNoiseSection()
    {
        if (ImGui::CollapsingHeader("Noise Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::SliderFloat("Noise Scale", &m_TerrainSettings.noiseScale, 0.001f, 0.2f, "%.3f"))
            {
                m_NeedsPreviewUpdate = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Controls feature size.\nLower = broad landforms; higher = smaller, noisier detail.");

            if (ImGui::SliderInt("Octaves", &m_TerrainSettings.octaves, 1, 8))
            {
                m_NeedsPreviewUpdate = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Number of layered noise passes.\nMore octaves = more detail, but slower.");

            if (ImGui::SliderFloat("Persistence", &m_TerrainSettings.persistence, 0.1f, 0.9f, "%.2f"))
            {
                m_NeedsPreviewUpdate = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("How much each octave contributes (amplitude falloff).\nLower = smoother terrain; higher = rougher.");

            if (ImGui::SliderFloat("Lacunarity", &m_TerrainSettings.lacunarity, 1.0f, 4.0f, "%.2f"))
            {
                m_NeedsPreviewUpdate = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Frequency multiplier per octave.\nHigher = more small-scale detail per octave.");
        }
    }

    void TerrainSettingsPanel::RenderWarpingSection()
    {
        if (ImGui::CollapsingHeader("Domain Warping"))
        {
            if (ImGui::Checkbox("Enable Warping", &m_TerrainSettings.useWarp))
            {
                m_NeedsPreviewUpdate = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Distorts the noise sampling coordinates.\nBreaks up grid artifacts and makes terrain feel more organic (slower).");

            if (m_TerrainSettings.useWarp)
            {
                if (ImGui::SliderFloat("Warp Strength", &m_TerrainSettings.warpStrength, 0.0f, 2.0f, "%.2f"))
                {
                    m_NeedsPreviewUpdate = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("How strongly the coordinates are warped.\n0 = off; higher = more twisting/chaos.");

                if (ImGui::SliderFloat("Warp Scale", &m_TerrainSettings.warpScale, 0.1f, 2.0f, "%.2f"))
                {
                    m_NeedsPreviewUpdate = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Size of the warping pattern.\nLower = broad warps; higher = tighter, noisier warps.");

                if (ImGui::SliderInt("Warp Levels", &m_TerrainSettings.warpLevels, 1, 4))
                {
                    m_NeedsPreviewUpdate = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("How many times to warp the coordinates.\nMore levels = richer shapes but slower (1-3 recommended).");
            }
        }
    }

    void TerrainSettingsPanel::RenderRidgeNoiseSection()
    {
        if (ImGui::CollapsingHeader("Ridge Noise (Mountains)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Checkbox("Enable Ridge Noise", &m_TerrainSettings.useRidgeNoise))
            {
                m_NeedsPreviewUpdate = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Creates sharp crests and spines (mountain ridges) instead of smooth hills.\nRecommended ON for mountain terrain.");

            if (m_TerrainSettings.useRidgeNoise)
            {
                if (ImGui::SliderFloat("Ridge Weight", &m_TerrainSettings.ridgeWeight, 0.0f, 1.0f, "%.2f"))
                {
                    m_NeedsPreviewUpdate = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("How much ridge noise affects height.\n0 = base terrain only; 1 = ridges dominate.\nIf Uplift Mask is enabled, ridges are limited to mountain bands.");

                if (ImGui::SliderFloat("Ridge Sharpness", &m_TerrainSettings.ridgePower, 1.0f, 4.0f, "%.1f"))
                {
                    m_NeedsPreviewUpdate = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Sharpness of ridge peaks (power exponent).\n1 = softer ridges; 3-4 = very sharp, jagged peaks.");

                if (ImGui::SliderFloat("Ridge Scale", &m_TerrainSettings.ridgeScale, 0.5f, 2.0f, "%.2f"))
                {
                    m_NeedsPreviewUpdate = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Spacing of ridge lines.\nLower = wider mountain spines; higher = tighter, more frequent ridges.");

                if (ImGui::SliderFloat("Peak Boost", &m_TerrainSettings.peakBoost, 0.0f, 1.0f, "%.2f"))
                {
                    m_NeedsPreviewUpdate = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Extra sharpening at mountain peaks (pow4 ridge boost).\nAdds dramatic pointed summits to the highest ridges.");

                ImGui::Separator();
                ImGui::Text("Tectonic Uplift Mask");

                if (ImGui::Checkbox("Enable Uplift Mask", &m_TerrainSettings.useUpliftMask))
                {
                    m_NeedsPreviewUpdate = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Limits mountains to large-scale bands.\nCreates plains, foothills, and distinct mountain ranges.");

                if (m_TerrainSettings.useUpliftMask)
                {
                    if (ImGui::SliderFloat("Uplift Scale", &m_TerrainSettings.upliftScale, 0.005f, 0.1f, "%.3f"))
                    {
                        m_NeedsPreviewUpdate = true;
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Size of tectonic regions (sample frequency).\nLower = larger continents/range bands; higher = smaller patches.");

                    if (ImGui::SliderFloat("Plains Threshold", &m_TerrainSettings.upliftThresholdLow, 0.0f, 0.6f, "%.2f"))
                    {
                        m_NeedsPreviewUpdate = true;
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Uplift values below this become plains (ridge contribution fades out).");

                    if (ImGui::SliderFloat("Mountain Threshold", &m_TerrainSettings.upliftThresholdHigh, 0.4f, 1.0f, "%.2f"))
                    {
                        m_NeedsPreviewUpdate = true;
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Uplift values above this become full mountains (maximum ridge contribution).\nTip: keep this higher than Plains Threshold.");

                    if (ImGui::SliderFloat("Transition Sharpness", &m_TerrainSettings.upliftPower, 0.5f, 3.0f, "%.1f"))
                    {
                        m_NeedsPreviewUpdate = true;
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Sharpens the plains→foothills→mountains transition.\nHigher = more distinct mountain bands.");
                }
            }
        }
    }

    void TerrainSettingsPanel::RenderColorSection()
    {
        if (ImGui::CollapsingHeader("Colors & Shading"))
        {
            ImGui::Checkbox("Flat Shading", &m_TerrainSettings.flatShading);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Uses per-face normals for a faceted low-poly look.\nOff = smoother lighting (if supported by the mesh).");

            ImGui::Checkbox("Height-Based Colors", &m_TerrainSettings.useHeightColors);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Applies water/sand/grass/rock/snow bands based on height.\nIf off, terrain uses the default material color.");

            if (m_TerrainSettings.useHeightColors)
            {
                ImGui::Text("Height Thresholds (normalized):");
                ImGui::SliderFloat("Water Level", &m_TerrainSettings.waterLevel, 0.0f, 1.0f, "%.2f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Below this height = water color band.\nValues are normalized: 0 = lowest, 1 = highest.");

                ImGui::SliderFloat("Sand Level", &m_TerrainSettings.sandLevel, 0.0f, 1.0f, "%.2f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Between water and sand level = beach/shoreline band.\nTip: keep Sand Level slightly above Water Level.");

                ImGui::SliderFloat("Grass Level", &m_TerrainSettings.grassLevel, 0.0f, 1.0f, "%.2f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Upper limit for grass band.\nAbove this, terrain transitions toward rock/snow.");

                ImGui::SliderFloat("Rock Level", &m_TerrainSettings.rockLevel, 0.0f, 1.0f, "%.2f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Above this height = rock/snow.\nTip: higher Rock Level makes less exposed rock.");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Slope-Based Coloring:");

                if (ImGui::Checkbox("Enable Slope Coloring", &m_TerrainSettings.useSlopeColoring))
                {
                    m_NeedsPreviewUpdate = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Colors steep slopes as rock/cliff regardless of height.\nCreates realistic cliff faces on mountains.");

                if (m_TerrainSettings.useSlopeColoring)
                {
                    if (ImGui::SliderFloat("Slope Threshold", &m_TerrainSettings.slopeColorThreshold, 0.0f, 1.0f, "%.2f"))
                    {
                        m_NeedsPreviewUpdate = true;
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Slope value where rock color starts.\n0 = even flat surfaces become rock\n0.4 = moderate slopes\n0.7 = only steep cliffs");

                    if (ImGui::SliderFloat("Slope Blend", &m_TerrainSettings.slopeColorBlend, 0.0f, 0.5f, "%.2f"))
                    {
                        m_NeedsPreviewUpdate = true;
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Transition smoothness from vegetation to rock.\nLower = sharper edge, higher = gradual blend.");

                    float slopeColor[3] = {m_TerrainSettings.steepSlopeColor.r,
                                           m_TerrainSettings.steepSlopeColor.g,
                                           m_TerrainSettings.steepSlopeColor.b};
                    if (ImGui::ColorEdit3("Cliff Color", slopeColor))
                    {
                        m_TerrainSettings.steepSlopeColor = glm::vec3(slopeColor[0], slopeColor[1], slopeColor[2]);
                        m_NeedsPreviewUpdate = true;
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Color used for steep cliff faces.\nTypically a rock/stone gray.");
                }
            }
        }
    }

    void TerrainSettingsPanel::RenderWaterSection()
    {
        if (ImGui::CollapsingHeader("Water & Oceans"))
        {
            ImGui::Checkbox("Enable Water", &m_WaterEnabled);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Toggles the water surface rendering.\nIf off, only the terrain mesh is shown.");

            if (m_WaterEnabled)
            {
                ImGui::Spacing();
                ImGui::Text("Sea Level:");

                // Normalized sea level slider (0.0 - 1.0)
                if (ImGui::SliderFloat("Normalized", &m_SeaLevelNormalized, 0.0f, 1.0f, "%.2f"))
                {
                    // Compute absolute sea level for display
                    m_SeaLevel = m_TerrainSettings.baseHeight +
                                 m_TerrainSettings.heightScale * m_SeaLevelNormalized;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Sea level as fraction of terrain height range.\n0.45 is typical (45%% of height range).\nFormula: seaLevel = baseHeight + heightScale * normalized");

                // Display computed absolute sea level (read-only)
                ImGui::Text("Absolute: %.2f world units", m_SeaLevel);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Computed sea level in world space.\nChanges automatically when terrain scale changes.");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Ocean Detection:");

                ImGui::Checkbox("Use Ocean Mask", &m_UseOceanMask);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Distinguishes oceans from inland lakes.\nOceans are below-sea cells connected to world boundaries.\nInland lakes are below-sea but isolated.");
            }
        }
    }

    void TerrainSettingsPanel::RenderErosionSection()
    {
        if (ImGui::CollapsingHeader("Erosion"))
        {
            ImGui::TextWrapped("Simulates weathering and water flow to add natural erosion patterns.");
            ImGui::Spacing();

            if (ImGui::Checkbox("Enable Erosion", &m_TerrainSettings.useErosion))
            {
                m_NeedsPreviewUpdate = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Applies erosion post-processing to terrain heights.\nErodes steep slopes and deepens valleys.");

            if (m_TerrainSettings.useErosion)
            {
                ImGui::Text("Slope-Based Erosion:");
                if (ImGui::SliderFloat("Erosion Strength", &m_TerrainSettings.slopeErosionStrength, 0.0f, 1.0f, "%.2f"))
                {
                    m_NeedsPreviewUpdate = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("How aggressively steep slopes are eroded.\nHigher = more material removed from cliffs and ridges.");

                if (ImGui::SliderFloat("Slope Threshold", &m_TerrainSettings.slopeThreshold, 0.0f, 2.0f, "%.2f"))
                {
                    m_NeedsPreviewUpdate = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Minimum slope angle before erosion kicks in.\nLower = even gentle hills erode; higher = only steep cliffs erode.");

                if (ImGui::SliderFloat("Valley Depth", &m_TerrainSettings.valleyDepth, 0.0f, 1.0f, "%.2f"))
                {
                    m_NeedsPreviewUpdate = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Deepens low-lying areas where water would collect.\nCreates river valleys and gullies.");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Hydraulic Erosion (Advanced):");

                ImGui::Checkbox("Enable Hydraulic Erosion", &m_TerrainSettings.useHydraulicErosion);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Simulates water droplets carrying sediment.\nMore realistic but slower; only applied to world chunks.");

                if (m_TerrainSettings.useHydraulicErosion)
                {
                    ImGui::SliderInt("Iterations", &m_TerrainSettings.erosionIterations, 10, 500);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Number of water droplets simulated per chunk.\nMore iterations = smoother, more natural erosion.");

                    ImGui::SliderFloat("Inertia", &m_TerrainSettings.erosionInertia, 0.0f, 0.5f, "%.3f");
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("How much droplets resist direction changes.\nHigher = longer, straighter flow paths.");

                    ImGui::SliderFloat("Sediment Capacity", &m_TerrainSettings.erosionCapacity, 1.0f, 10.0f, "%.1f");
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Max sediment a droplet can carry.\nHigher = deeper cuts in steep terrain.");

                    ImGui::SliderFloat("Deposition Rate", &m_TerrainSettings.erosionDeposition, 0.0f, 1.0f, "%.2f");
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("How quickly sediment drops in flat areas.\nHigher = more sediment fans and alluvial deposits.");

                    ImGui::SliderFloat("Evaporation Rate", &m_TerrainSettings.erosionEvaporation, 0.0f, 0.1f, "%.3f");
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("How quickly droplets lose water and stop.\nHigher = shorter flow paths.");
                }
            }
        }
    }

    void TerrainSettingsPanel::RenderDebugViewSection()
    {
        if (ImGui::CollapsingHeader("Debug Views (Section 32)"))
        {
            ImGui::TextWrapped("Visualize intermediate terrain fields for debugging and validation.");
            ImGui::Spacing();

            // Debug view type selector
            const char *viewNames[] = {
                "None",
                "Height",
                "Continental Mask",
                "Uplift Mask",
                "Slope",
                "Flow Direction",
                "Flow Accumulation",
                "Water Type",
                "Distance to Water",
                "Temperature",
                "Moisture",
                "Fertility",
                "Dominant Biome",
                "Dominant Material"};

            if (ImGui::Combo("View Type", &m_DebugViewIndex, viewNames, IM_ARRAYSIZE(viewNames)))
            {
                m_DebugView.GetSettings().activeView = static_cast<DebugViewType>(m_DebugViewIndex);
            }

            if (m_DebugViewIndex > 0)
            {
                ImGui::SliderFloat("Overlay Opacity", &m_DebugView.GetSettings().overlayOpacity, 0.0f, 1.0f, "%.2f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Transparency of the debug overlay.\n0 = invisible, 1 = fully opaque.");

                ImGui::Checkbox("Show Chunk Boundaries", &m_DebugView.GetSettings().showChunkBoundaries);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Draws lines at chunk edges to visualize chunk seams.");

                // Color legend for categorical views
                if (m_DebugViewIndex == static_cast<int>(DebugViewType::WaterType))
                {
                    ImGui::Separator();
                    ImGui::Text("Legend:");
                    ImGui::BulletText("Black: None");
                    ImGui::BulletText("Light Blue: Stream");
                    ImGui::BulletText("Blue: River");
                    ImGui::BulletText("Dark Blue: Lake");
                    ImGui::BulletText("Deep Blue: Ocean");
                }
                else if (m_DebugViewIndex == static_cast<int>(DebugViewType::FlowDirection))
                {
                    ImGui::Separator();
                    ImGui::Text("Legend (Flow Direction):");
                    ImGui::BulletText("Green: North");
                    ImGui::BulletText("Yellow: East");
                    ImGui::BulletText("Red: South");
                    ImGui::BulletText("Magenta: West");
                    ImGui::BulletText("Black: Pit/Sink");
                }
                else if (m_DebugViewIndex == static_cast<int>(DebugViewType::DominantBiome))
                {
                    ImGui::Separator();
                    ImGui::Text("Legend (Biomes):");
                    ImGui::BulletText("White-Blue: Polar");
                    ImGui::BulletText("Gray-Blue: Tundra");
                    ImGui::BulletText("Dark Green: Boreal");
                    ImGui::BulletText("Medium Green: Temperate");
                    ImGui::BulletText("Olive: Mediterranean");
                    ImGui::BulletText("Yellow-Green: Grassland");
                    ImGui::BulletText("Sandy: Desert");
                    ImGui::BulletText("Bright Green: Tropical");
                    ImGui::BulletText("Deep Green: Rainforest");
                    ImGui::BulletText("Teal: Wetland");
                }
                else if (m_DebugViewIndex == static_cast<int>(DebugViewType::DominantMaterial))
                {
                    ImGui::Separator();
                    ImGui::Text("Legend (Materials):");
                    ImGui::BulletText("Gray: Rock");
                    ImGui::BulletText("Brown: Dirt");
                    ImGui::BulletText("Green: Grass");
                    ImGui::BulletText("Sandy: Sand");
                    ImGui::BulletText("White: Snow");
                    ImGui::BulletText("Light Blue: Ice");
                    ImGui::BulletText("Dark Brown: Mud");
                    ImGui::BulletText("Blue: Water");
                }
            }
        }
    }

    void TerrainSettingsPanel::SyncIntentToSettings()
    {
        m_TerrainSettings = TerrainIntentMapper::DeriveSettings(m_Intent);
        // Ensure the seed from the panel is propagated to settings
        // DeriveSettings uses a default seed, so we override it here
        m_TerrainSettings.seed = m_Seed;
    }

    void TerrainSettingsPanel::UpdatePresetSelection()
    {
        // Check if current intent matches any preset exactly
        const auto &presets = TerrainIntentMapper::GetPresets();
        for (int i = 0; i < static_cast<int>(presets.size()) - 1; i++) // Skip "Custom"
        {
            if (m_Intent == presets[i].intent)
            {
                m_CurrentPresetIndex = i;
                return;
            }
        }
        // No match found, switch to "Custom"
        m_CurrentPresetIndex = static_cast<int>(presets.size()) - 1;
    }

    void TerrainSettingsPanel::UpdateHeightmapPreview()
    {
        if (!m_HeightmapTexture || !m_HeightmapTexture->IsValid())
            return;

        // Use TerrainGenerator as single source of truth for preview generation
        TerrainSettings previewSettings = m_TerrainSettings;
        previewSettings.width = PREVIEW_SIZE - 1;
        previewSettings.depth = PREVIEW_SIZE - 1;
        previewSettings.cellSize = 2.0f; // Preview cell size for sampling
        previewSettings.seed = m_Seed;
        previewSettings.flatShading = true;

        TerrainGenerator previewGenerator(previewSettings);

        // Generate heightmap using the canonical algorithm
        std::vector<float> heightData = previewGenerator.GenerateHeightmap();

        // Calculate min/max heights
        float minHeight = std::numeric_limits<float>::max();
        float maxHeight = std::numeric_limits<float>::lowest();
        for (float h : heightData)
        {
            minHeight = std::min(minHeight, h);
            maxHeight = std::max(maxHeight, h);
        }

        m_HeightmapTexture->Update(heightData, minHeight, maxHeight);
    }

    void TerrainSettingsPanel::ApplySettings()
    {
        if (!m_ChunkManager)
        {
            GEN_ERROR("ApplySettings: ChunkManager is null!");
            return;
        }

        // Compute absolute sea level from normalized value
        m_SeaLevel = m_TerrainSettings.baseHeight +
                     m_TerrainSettings.heightScale * m_SeaLevelNormalized;

        GEN_INFO("Applying new terrain settings:");
        GEN_INFO("  Height Scale: {} -> {}", m_ChunkManager->GetSettings().terrainSettings.heightScale, m_TerrainSettings.heightScale);
        GEN_INFO("  Noise Scale: {} -> {}", m_ChunkManager->GetSettings().terrainSettings.noiseScale, m_TerrainSettings.noiseScale);
        GEN_INFO("  Octaves: {} -> {}", m_ChunkManager->GetSettings().terrainSettings.octaves, m_TerrainSettings.octaves);
        GEN_INFO("  Sea Level (normalized): {} -> {}", m_ChunkManager->GetSettings().seaLevelNormalized, m_SeaLevelNormalized);
        GEN_INFO("  Sea Level (absolute): {} -> {}", m_ChunkManager->GetSettings().seaLevel, m_SeaLevel);
        GEN_INFO("  Use Ocean Mask: {} -> {}", m_ChunkManager->GetSettings().useOceanMask, m_UseOceanMask);

        auto &worldSettings = m_ChunkManager->GetSettings();
        worldSettings.terrainSettings = m_TerrainSettings;
        worldSettings.seaLevelNormalized = m_SeaLevelNormalized;
        worldSettings.seaLevel = m_SeaLevel;
        worldSettings.waterEnabled = m_WaterEnabled;
        worldSettings.useOceanMask = m_UseOceanMask;
        worldSettings.seed = m_Seed;

        if (worldSettings.viewDistance != m_ViewDistance)
        {
            m_ChunkManager->SetViewDistance(m_ViewDistance);
        }

        m_ChunkManager->RegenerateAllChunks();
        m_SettingsChanged = true;

        GEN_INFO("Terrain regeneration complete");
    }

}
