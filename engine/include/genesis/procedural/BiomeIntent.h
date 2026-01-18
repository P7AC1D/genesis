#pragma once

#include <cstdint>

namespace Genesis
{

    /**
     * Section 26: BiomeIntent (Climate Authoring Layer)
     *
     * BiomeIntent provides high-level, artist-friendly controls for climate
     * that drives the biome system. Like TerrainIntent, all values are
     * normalized to [0, 1] for intuitive authoring.
     *
     * Section 25: Biome System Overview
     * Biomes:
     *   - Do NOT affect geometry
     *   - Are derived from climate + hydrology
     *   - Are soft classifications (blended, not hard boundaries)
     *
     * Biomes influence:
     *   - Surface materials
     *   - Vegetation
     *   - Color
     *   - Minor shading variations
     */
    struct BiomeIntent
    {
        // Size of climate regions
        // 0 = small, varied climate zones
        // 1 = large, uniform climate bands
        float climateScale = 0.5f;

        // Global warmth
        // 0 = cold world (arctic dominant)
        // 1 = hot world (tropical dominant)
        float temperatureBias = 0.5f;

        // Temperature variation strength
        // 0 = uniform temperature everywhere
        // 1 = extreme temperature differences (poles vs equator)
        float temperatureRange = 0.5f;

        // Rainfall baseline
        // 0 = dry world (deserts common)
        // 1 = wet world (rainforests common)
        float humidity = 0.5f;

        // Dryness bias
        // 0 = moisture retained well
        // 1 = rapid evaporation, arid conditions
        float aridity = 0.5f;

        // Vegetation potential
        // 0 = barren, minimal plant life
        // 1 = lush, dense vegetation
        float fertility = 0.5f;

        // Climate noise distortion
        // 0 = predictable climate bands
        // 1 = chaotic, unpredictable climate distribution
        float chaos = 0.3f;

        /**
         * Validate and clamp all values to [0, 1].
         */
        void Validate()
        {
            climateScale = Clamp(climateScale);
            temperatureBias = Clamp(temperatureBias);
            temperatureRange = Clamp(temperatureRange);
            humidity = Clamp(humidity);
            aridity = Clamp(aridity);
            fertility = Clamp(fertility);
            chaos = Clamp(chaos);
        }

        /**
         * Create a temperate climate preset.
         */
        static BiomeIntent Temperate()
        {
            BiomeIntent intent;
            intent.climateScale = 0.5f;
            intent.temperatureBias = 0.5f;
            intent.temperatureRange = 0.6f;
            intent.humidity = 0.5f;
            intent.aridity = 0.3f;
            intent.fertility = 0.6f;
            intent.chaos = 0.3f;
            return intent;
        }

        /**
         * Create a tropical climate preset.
         */
        static BiomeIntent Tropical()
        {
            BiomeIntent intent;
            intent.climateScale = 0.6f;
            intent.temperatureBias = 0.8f;
            intent.temperatureRange = 0.2f;
            intent.humidity = 0.8f;
            intent.aridity = 0.2f;
            intent.fertility = 0.9f;
            intent.chaos = 0.4f;
            return intent;
        }

        /**
         * Create an arid/desert climate preset.
         */
        static BiomeIntent Arid()
        {
            BiomeIntent intent;
            intent.climateScale = 0.7f;
            intent.temperatureBias = 0.7f;
            intent.temperatureRange = 0.8f;
            intent.humidity = 0.2f;
            intent.aridity = 0.9f;
            intent.fertility = 0.2f;
            intent.chaos = 0.2f;
            return intent;
        }

        /**
         * Create an arctic/polar climate preset.
         */
        static BiomeIntent Arctic()
        {
            BiomeIntent intent;
            intent.climateScale = 0.6f;
            intent.temperatureBias = 0.1f;
            intent.temperatureRange = 0.4f;
            intent.humidity = 0.3f;
            intent.aridity = 0.4f;
            intent.fertility = 0.1f;
            intent.chaos = 0.2f;
            return intent;
        }

    private:
        static float Clamp(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
    };

    /**
     * Climate settings derived from BiomeIntent.
     * These are the mechanical parameters used by climate field generation.
     */
    struct ClimateSettings
    {
        // Temperature field
        float baseTemperature = 0.5f;       // Global temperature offset
        float temperatureAmplitude = 0.5f;  // Temperature variation range
        float temperatureFrequency = 0.01f; // Noise frequency for temperature

        // Latitude effects
        float latitudeInfluence = 0.5f;    // How much latitude affects temperature
        float elevationLapseRate = 0.006f; // Temperature drop per unit elevation

        // Precipitation field
        float basePrecipitation = 0.5f;       // Global rainfall level
        float precipitationVariation = 0.3f;  // Rainfall variation
        float precipitationFrequency = 0.02f; // Noise frequency

        // Evaporation / aridity
        float evaporationRate = 0.3f; // How fast moisture evaporates
        float waterRetention = 0.5f;  // How well terrain holds moisture

        // Vegetation
        float vegetationThreshold = 0.3f; // Minimum moisture for vegetation
        float vegetationDensity = 0.5f;   // Vegetation coverage multiplier

        // Domain warping for climate zones
        float climateWarpStrength = 0.2f;
        float climateWarpScale = 0.5f;
    };

    /**
     * Maps BiomeIntent to ClimateSettings.
     * Similar pattern to TerrainIntentMapper.
     */
    class BiomeIntentMapper
    {
    public:
        /**
         * Derive climate settings from biome intent.
         */
        static ClimateSettings DeriveSettings(const BiomeIntent &intent);

    private:
        static float Lerp(float a, float b, float t) { return a + (b - a) * t; }

        static void DeriveTemperature(const BiomeIntent &intent, ClimateSettings &settings);
        static void DerivePrecipitation(const BiomeIntent &intent, ClimateSettings &settings);
        static void DeriveVegetation(const BiomeIntent &intent, ClimateSettings &settings);
        static void DeriveClimateWarping(const BiomeIntent &intent, ClimateSettings &settings);
    };

}
