#pragma once

#include "genesis/procedural/Noise.h"
#include "genesis/procedural/TerrainIntent.h"
#include <glm/glm.hpp>

namespace Genesis
{

    /**
     * GeologicalFields represents the five orthogonal field values sampled at a world position.
     *
     * Terrain is generated as continuous overlapping geological fields, not discrete biomes.
     * Every terrain feature emerges from the interaction of these five fields:
     *
     *   1. Continental field  - Determines land vs ocean, macro topology
     *   2. Elevation field    - Controls overall height amplitude
     *   3. Uplift field       - Determines where mountains can exist
     *   4. Ridge field        - Determines mountain shape
     *   5. Erosion field      - Determines terrain age and realism
     *
     * No feature exists in isolation - the final height emerges from field interactions.
     */
    struct GeologicalFields
    {
        // Layer 1: Continental field [-1, 1]
        // Negative values = ocean, positive values = land
        // Magnitude indicates distance from coastline
        float continental = 0.0f;

        // Layer 2: Elevation amplitude [0, 1]
        // Modulates local height range (0 = flat, 1 = maximum relief)
        float elevationAmplitude = 0.5f;

        // Layer 3: Uplift mask [0, 1]
        // Mountain eligibility (0 = plains, 1 = full mountain potential)
        float upliftMask = 0.0f;

        // Layer 4: Ridge value [0, 1]
        // Mountain shape detail (higher = sharper ridges)
        float ridgeValue = 0.0f;

        // Layer 5: Erosion age [0, 1]
        // Local terrain maturity (0 = young/sharp, 1 = ancient/rounded)
        float erosionAge = 0.5f;

        // Derived: Ocean mask [0, 1]
        // Smoothstepped continental threshold (1 = fully ocean, 0 = fully land)
        float oceanMask = 0.0f;
    };

    /**
     * Settings for geological field generation.
     * These are derived from TerrainIntent but can be manually configured.
     */
    struct GeologicalFieldSettings
    {
        // Continental field settings
        float continentalFrequency = 0.0003f; // Lower = larger landmasses
        float continentalOctaves = 2;         // FBM octaves for continental noise
        float oceanThreshold = 0.45f;         // Continental value below which = ocean
        float coastlineBlend = 0.05f;         // Smoothstep epsilon for coastlines

        // Ocean depth settings
        float oceanDepthMin = 30.0f;      // Minimum ocean depth
        float oceanDepthMax = 80.0f;      // Maximum ocean depth (scaled by elevationRange)
        float oceanFloorVariation = 0.3f; // Noise variation on ocean floor

        // Elevation amplitude field settings
        float elevationFieldFrequency = 0.0008f; // Frequency of amplitude variations
        float elevationFieldOctaves = 2;

        // Uplift field settings (overrides from TerrainSettings if specified)
        float upliftFrequency = 0.0f;     // 0 = use TerrainSettings.upliftScale
        float upliftThresholdLow = 0.0f;  // 0 = use TerrainSettings
        float upliftThresholdHigh = 0.0f; // 0 = use TerrainSettings

        // Erosion age field settings (spatial variation)
        bool useSpatialErosion = true;         // Enable spatial erosion age variation
        float erosionFieldFrequency = 0.0005f; // Frequency of erosion age variation
        float erosionAgeBase = 0.5f;           // Base erosion age (from TerrainIntent)
        float erosionAgeVariation = 0.3f;      // How much spatial variation (±)
    };

    /**
     * Generates and samples geological fields at world positions.
     *
     * This class provides the field-based terrain generation system where terrain
     * emerges from the interaction of orthogonal geological fields rather than
     * additive noise layers.
     *
     * Usage:
     *   GeologicalFieldGenerator generator(intent, seed);
     *   GeologicalFields fields = generator.SampleFields(worldX, worldZ);
     *   float height = generator.ComputeBaseHeight(fields, terrainSettings);
     */
    class GeologicalFieldGenerator
    {
    public:
        GeologicalFieldGenerator();
        GeologicalFieldGenerator(const TerrainIntent &intent, uint32_t seed = 12345);

        /**
         * Configure the generator from a TerrainIntent.
         * This derives GeologicalFieldSettings from intent parameters.
         */
        void Configure(const TerrainIntent &intent, uint32_t seed);

        /**
         * Manually set field settings for fine-grained control.
         */
        void SetFieldSettings(const GeologicalFieldSettings &settings);
        const GeologicalFieldSettings &GetFieldSettings() const { return m_FieldSettings; }

        /**
         * Sample all geological fields at a world position.
         * Returns a GeologicalFields struct with all five field values.
         */
        GeologicalFields SampleFields(float worldX, float worldZ) const;

        /**
         * Sample only the continental field (for ocean detection without full sampling).
         */
        float SampleContinentalField(float worldX, float worldZ) const;

        /**
         * Compute the ocean mask from a continental value.
         * Uses smoothstep for gradual coastline transitions.
         */
        float ComputeOceanMask(float continentalValue) const;

        /**
         * Compute ocean depth at a position.
         * Returns depth below sea level (positive = underwater).
         */
        float ComputeOceanDepth(float worldX, float worldZ, float oceanMask) const;

        /**
         * Compute the final base height from sampled fields.
         * This combines all field interactions into a single height value.
         *
         * @param fields      Sampled geological fields at this position
         * @param baseNoise   Base terrain noise value from FBM [-1, 1]
         * @param ridgeNoise  Ridge noise value [0, 1]
         * @param settings    TerrainSettings for height scale and other params
         * @return            World-space height value
         */
        float ComputeBaseHeight(const GeologicalFields &fields,
                                float baseNoise,
                                float ridgeNoise,
                                const struct TerrainSettings &settings) const;

        /**
         * Get the current TerrainIntent used for configuration.
         */
        const TerrainIntent &GetIntent() const { return m_Intent; }

    private:
        // Derive field settings from intent
        void DeriveFieldSettings(const TerrainIntent &intent);

        // Smoothstep utility
        static float Smoothstep(float edge0, float edge1, float x);

        // Lerp utility
        static float Lerp(float a, float b, float t) { return a + t * (b - a); }

        TerrainIntent m_Intent;
        GeologicalFieldSettings m_FieldSettings;
        SimplexNoise m_Noise;
        uint32_t m_Seed = 12345;
    };

}
