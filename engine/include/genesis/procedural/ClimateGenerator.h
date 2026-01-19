#pragma once

#include "genesis/procedural/BiomeIntent.h"
#include "genesis/procedural/HydrologyData.h"
#include "genesis/procedural/Noise.h"
#include <glm/glm.hpp>
#include <vector>

namespace Genesis
{

    /**
     * Section 27: Climate Field Generation
     *
     * Climate fields are continuous values derived from:
     *   - Noise functions (FBM)
     *   - Terrain properties (altitude, slope)
     *   - Hydrology (water proximity)
     *   - BiomeIntent parameters
     */
    struct ClimateData
    {
        // Section 27.1: Temperature field [-1, 1]
        // Negative = cold (polar), Positive = hot (tropical)
        std::vector<float> temperature;

        // Section 27.2: Moisture field [0, 1]
        // 0 = arid, 1 = saturated
        std::vector<float> moisture;

        // Section 27.3: Fertility field [0, 1]
        // 0 = barren, 1 = lush
        std::vector<float> fertility;

        // Altitude cooling factor [0, 1]
        std::vector<float> altitudeCooling;

        // Rain shadow intensity [0, 1]
        std::vector<float> rainShadow;

        int width = 0;
        int depth = 0;

        void Resize(int w, int d)
        {
            width = w;
            depth = d;
            size_t size = static_cast<size_t>(w) * d;
            temperature.resize(size, 0.0f);
            moisture.resize(size, 0.5f);
            fertility.resize(size, 0.5f);
            altitudeCooling.resize(size, 0.0f);
            rainShadow.resize(size, 0.0f);
        }

        void Clear()
        {
            std::fill(temperature.begin(), temperature.end(), 0.0f);
            std::fill(moisture.begin(), moisture.end(), 0.5f);
            std::fill(fertility.begin(), fertility.end(), 0.5f);
            std::fill(altitudeCooling.begin(), altitudeCooling.end(), 0.0f);
            std::fill(rainShadow.begin(), rainShadow.end(), 0.0f);
        }

        size_t Index(int x, int z) const { return static_cast<size_t>(z) * width + x; }
        bool InBounds(int x, int z) const { return x >= 0 && x < width && z >= 0 && z < depth; }
    };

    /**
     * Generates climate fields from terrain, hydrology, and BiomeIntent.
     */
    class ClimateGenerator
    {
    public:
        ClimateGenerator() = default;
        ~ClimateGenerator() = default;

        /**
         * Initialize with noise generator and settings.
         */
        void Initialize(SimplexNoise *noise, const ClimateSettings &settings);

        /**
         * Generate all climate fields.
         *
         * @param heightmap   Terrain heights
         * @param hydrology   Hydrology data (moisture, water proximity)
         * @param seaLevel    Sea level height
         * @param heightScale Maximum terrain height for altitude normalization
         * @param cellSize    World units per cell
         * @param worldOffsetX World X offset for noise sampling
         * @param worldOffsetZ World Z offset for noise sampling
         */
        void Generate(const std::vector<float> &heightmap,
                      const HydrologyData &hydrology,
                      float seaLevel,
                      float heightScale,
                      float cellSize,
                      float worldOffsetX,
                      float worldOffsetZ);

        /**
         * Generate climate fields with explicit grid dimensions.
         * Use this when hydrology data is not available.
         */
        void Generate(const std::vector<float> &heightmap,
                      int gridWidth,
                      int gridDepth,
                      float seaLevel,
                      float heightScale,
                      float cellSize,
                      float worldOffsetX,
                      float worldOffsetZ);

        /**
         * Get the climate data.
         */
        const ClimateData &GetData() const { return m_Data; }

        /**
         * Query temperature at a cell [-1, 1].
         */
        float GetTemperature(int x, int z) const;

        /**
         * Query moisture at a cell [0, 1].
         */
        float GetMoisture(int x, int z) const;

        /**
         * Query fertility at a cell [0, 1].
         */
        float GetFertility(int x, int z) const;

    private:
        /**
         * Section 27.1: Temperature Field
         * temperature = temperatureBias + tempNoise * temperatureRange - altitudeCooling
         */
        void ComputeTemperature(const std::vector<float> &heightmap,
                                float seaLevel,
                                float heightScale,
                                float cellSize,
                                float worldOffsetX,
                                float worldOffsetZ);

        /**
         * Section 27.2: Moisture Field
         * moisture = humidity + waterProximityBoost - rainShadowPenalty - altitudePenalty
         */
        void ComputeMoisture(const std::vector<float> &heightmap,
                             const HydrologyData &hydrology,
                             float seaLevel,
                             float heightScale,
                             float cellSize,
                             float worldOffsetX,
                             float worldOffsetZ);

        /**
         * Section 27.3: Fertility Field
         * fertility = fertilityIntent * moisture * (1 - slope)
         */
        void ComputeFertility(const HydrologyData &hydrology);

        /**
         * Compute altitude cooling factor.
         * altitudeCooling = clamp((height - seaLevel) / heightScale, 0, 1)
         */
        float ComputeAltitudeCooling(float height, float seaLevel, float heightScale) const;

        /**
         * Compute rain shadow from terrain gradients.
         * Mountains block moisture from prevailing wind direction.
         */
        void ComputeRainShadow(const std::vector<float> &heightmap,
                               float cellSize);

        /**
         * Simplified moisture calculation using only world-coherent noise.
         * Used for distant chunks to ensure seamless boundaries.
         */
        void ComputeSimplifiedMoisture(const std::vector<float> &heightmap,
                                       float seaLevel,
                                       float heightScale,
                                       float cellSize,
                                       float worldOffsetX,
                                       float worldOffsetZ);

        /**
         * Simplified fertility calculation based on temperature and moisture.
         * Used for distant chunks to ensure seamless boundaries.
         */
        void ComputeSimplifiedFertility();

        SimplexNoise *m_Noise = nullptr;
        ClimateSettings m_Settings;
        ClimateData m_Data;
    };

}
