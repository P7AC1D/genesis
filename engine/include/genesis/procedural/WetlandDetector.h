#pragma once

#include "genesis/procedural/HydrologyData.h"
#include <vector>

namespace Genesis
{

    /**
     * Section 23: Wetlands & Floodplains
     *
     * Wetlands form where:
     *   nearRiver && lowSlope && highMoisture
     *
     * These are NOT water bodies, but biome modifiers that influence:
     *   - Surface materials (mud, marsh)
     *   - Vegetation (reeds, wetland plants)
     *   - Color (darker, more saturated)
     */

    /**
     * Settings for wetland detection.
     */
    struct WetlandSettings
    {
        // Maximum distance to water to be considered "near river" (world units)
        float maxWaterDistance = 15.0f;

        // Maximum slope to be considered "low slope" (gradient)
        float maxSlope = 0.1f;

        // Minimum moisture to be considered "high moisture"
        float minMoisture = 0.6f;

        // Minimum flow accumulation (alternative moisture indicator)
        uint32_t minFlowAccumulation = 50;

        // Wetland intensity falloff distance from ideal conditions
        float intensityFalloff = 10.0f;
    };

    /**
     * Wetland data per cell.
     */
    struct WetlandData
    {
        // Is this cell part of a wetland?
        std::vector<bool> isWetland;

        // Wetland intensity [0, 1] for blending biome effects
        std::vector<float> wetlandIntensity;

        // Floodplain indicator (near river, very low slope)
        std::vector<bool> isFloodplain;

        int width = 0;
        int depth = 0;

        void Resize(int w, int d)
        {
            width = w;
            depth = d;
            size_t size = static_cast<size_t>(w) * d;
            isWetland.resize(size, false);
            wetlandIntensity.resize(size, 0.0f);
            isFloodplain.resize(size, false);
        }

        void Clear()
        {
            std::fill(isWetland.begin(), isWetland.end(), false);
            std::fill(wetlandIntensity.begin(), wetlandIntensity.end(), 0.0f);
            std::fill(isFloodplain.begin(), isFloodplain.end(), false);
        }

        size_t Index(int x, int z) const { return static_cast<size_t>(z) * width + x; }
        bool InBounds(int x, int z) const { return x >= 0 && x < width && z >= 0 && z < depth; }
    };

    /**
     * Detects wetlands and floodplains from hydrology data.
     *
     * Wetlands are biome modifiers, not geometry changes.
     * They affect surface materials, vegetation, and coloring.
     */
    class WetlandDetector
    {
    public:
        WetlandDetector() = default;
        ~WetlandDetector() = default;

        /**
         * Configure wetland detection settings.
         */
        void SetSettings(const WetlandSettings &settings) { m_Settings = settings; }
        const WetlandSettings &GetSettings() const { return m_Settings; }

        /**
         * Detect wetlands from hydrology data.
         *
         * @param hydrology  Computed hydrology data
         */
        void Detect(const HydrologyData &hydrology);

        /**
         * Get the wetland data.
         */
        const WetlandData &GetData() const { return m_Data; }

        /**
         * Check if a cell is a wetland.
         */
        bool IsWetland(int x, int z) const;

        /**
         * Get wetland intensity at a cell [0, 1].
         */
        float GetWetlandIntensity(int x, int z) const;

        /**
         * Check if a cell is a floodplain.
         */
        bool IsFloodplain(int x, int z) const;

    private:
        /**
         * Compute wetland intensity based on conditions.
         * Returns 0 if not a wetland, >0 based on how ideal conditions are.
         */
        float ComputeWetlandIntensity(float distanceToWater,
                                      float slope,
                                      float moisture,
                                      uint32_t flowAccum) const;

        WetlandSettings m_Settings;
        WetlandData m_Data;
    };

}
