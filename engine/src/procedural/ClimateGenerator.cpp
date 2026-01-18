#include "genesis/procedural/ClimateGenerator.h"
#include <algorithm>
#include <cmath>

namespace Genesis
{

    void ClimateGenerator::Initialize(SimplexNoise *noise, const ClimateSettings &settings)
    {
        m_Noise = noise;
        m_Settings = settings;
    }

    void ClimateGenerator::Generate(const std::vector<float> &heightmap,
                                    const HydrologyData &hydrology,
                                    float seaLevel,
                                    float heightScale,
                                    float cellSize,
                                    float worldOffsetX,
                                    float worldOffsetZ)
    {
        m_Data.Resize(hydrology.width, hydrology.depth);
        m_Data.Clear();

        // Compute rain shadow first (needed for moisture)
        ComputeRainShadow(heightmap, cellSize);

        // Section 27.1: Temperature field
        ComputeTemperature(heightmap, seaLevel, heightScale, cellSize,
                           worldOffsetX, worldOffsetZ);

        // Section 27.2: Moisture field
        ComputeMoisture(heightmap, hydrology, seaLevel, heightScale, cellSize,
                        worldOffsetX, worldOffsetZ);

        // Section 27.3: Fertility field
        ComputeFertility(hydrology);
    }

    void ClimateGenerator::ComputeTemperature(const std::vector<float> &heightmap,
                                              float seaLevel,
                                              float heightScale,
                                              float cellSize,
                                              float worldOffsetX,
                                              float worldOffsetZ)
    {
        // Section 27.1: Temperature Field (Latitude-Free)
        // tempNoise = FBM(x * climateFreq, z * climateFreq)
        // temperature = temperatureBias + tempNoise * temperatureRange - altitudeCooling(height)

        for (int z = 0; z < m_Data.depth; z++)
        {
            for (int x = 0; x < m_Data.width; x++)
            {
                size_t idx = m_Data.Index(x, z);

                // World coordinates for noise sampling
                float worldX = worldOffsetX + x * cellSize;
                float worldZ = worldOffsetZ + z * cellSize;

                // Sample climate noise
                float tempNoise = m_Noise->FBM(
                    worldX * m_Settings.temperatureFrequency,
                    worldZ * m_Settings.temperatureFrequency,
                    4,    // octaves
                    0.5f, // persistence
                    2.0f  // lacunarity
                );

                // Altitude cooling
                float height = heightmap[idx];
                float altCooling = ComputeAltitudeCooling(height, seaLevel, heightScale);
                m_Data.altitudeCooling[idx] = altCooling;

                // Section 27.1 formula:
                // temperature = temperatureBias + tempNoise * temperatureRange - altitudeCooling
                // Map temperatureBias from [0,1] to [-1,1] range
                float tempBias = (m_Settings.baseTemperature - 0.5f) * 2.0f;

                float temperature = tempBias +
                                    tempNoise * m_Settings.temperatureAmplitude -
                                    altCooling * m_Settings.elevationLapseRate * heightScale;

                // Clamp to [-1, 1]
                m_Data.temperature[idx] = std::clamp(temperature, -1.0f, 1.0f);
            }
        }
    }

    void ClimateGenerator::ComputeMoisture(const std::vector<float> &heightmap,
                                           const HydrologyData &hydrology,
                                           float seaLevel,
                                           float heightScale,
                                           float cellSize,
                                           float worldOffsetX,
                                           float worldOffsetZ)
    {
        // Section 27.2: Moisture Field
        // moisture = humidity + waterProximityBoost - rainShadowPenalty - altitudePenalty

        for (int z = 0; z < m_Data.depth; z++)
        {
            for (int x = 0; x < m_Data.width; x++)
            {
                size_t idx = m_Data.Index(x, z);

                // World coordinates for noise sampling
                float worldX = worldOffsetX + x * cellSize;
                float worldZ = worldOffsetZ + z * cellSize;

                // Base humidity from settings (rainfall baseline)
                float humidity = m_Settings.basePrecipitation;

                // Add precipitation noise variation
                float precipNoise = m_Noise->FBM(
                    worldX * m_Settings.precipitationFrequency,
                    worldZ * m_Settings.precipitationFrequency,
                    3,    // octaves
                    0.5f, // persistence
                    2.0f  // lacunarity
                );
                humidity += precipNoise * m_Settings.precipitationVariation;

                // Water proximity boost (from hydrology)
                float distanceToWater = hydrology.distanceToWater[idx];
                float maxDist = 100.0f; // Max distance for proximity effect
                float waterProximityBoost = 0.0f;
                if (distanceToWater < maxDist)
                {
                    waterProximityBoost = (1.0f - distanceToWater / maxDist) * 0.3f;
                }

                // Rain shadow penalty (mountains block moisture)
                float rainShadowPenalty = m_Data.rainShadow[idx] * 0.5f;

                // Altitude penalty (high elevations are drier)
                float height = heightmap[idx];
                float altitudePenalty = 0.0f;
                if (height > seaLevel)
                {
                    float normalizedAlt = (height - seaLevel) / heightScale;
                    altitudePenalty = normalizedAlt * 0.3f;
                }

                // Evaporation based on aridity setting
                float evaporationLoss = m_Settings.evaporationRate * 0.2f;

                // Section 27.2 formula:
                // moisture = humidity + waterProximityBoost - rainShadowPenalty - altitudePenalty
                float moisture = humidity + waterProximityBoost -
                                 rainShadowPenalty - altitudePenalty - evaporationLoss;

                // Water cells have maximum moisture
                if (hydrology.waterType[idx] != WaterType::None)
                {
                    moisture = 1.0f;
                }

                // Clamp to [0, 1]
                m_Data.moisture[idx] = std::clamp(moisture, 0.0f, 1.0f);
            }
        }
    }

    void ClimateGenerator::ComputeFertility(const HydrologyData &hydrology)
    {
        // Section 27.3: Fertility Field
        // fertility = fertilityIntent * moisture * (1 - slope)
        // Steep slopes are infertile by design

        for (int z = 0; z < m_Data.depth; z++)
        {
            for (int x = 0; x < m_Data.width; x++)
            {
                size_t idx = m_Data.Index(x, z);

                float moisture = m_Data.moisture[idx];
                float slope = hydrology.slope[idx];

                // Normalize slope to [0, 1] range (assume max meaningful slope ~2.0)
                float normalizedSlope = std::min(slope / 2.0f, 1.0f);

                // Section 27.3 formula:
                // fertility = fertilityIntent * moisture * (1 - slope)
                float fertility = m_Settings.vegetationDensity *
                                  moisture *
                                  (1.0f - normalizedSlope);

                // Water cells have no fertility (underwater)
                if (hydrology.waterType[idx] != WaterType::None)
                {
                    fertility = 0.0f;
                }

                // Clamp to [0, 1]
                m_Data.fertility[idx] = std::clamp(fertility, 0.0f, 1.0f);
            }
        }
    }

    float ClimateGenerator::ComputeAltitudeCooling(float height, float seaLevel,
                                                   float heightScale) const
    {
        // Section 27.1: altitudeCooling = clamp((height - seaLevel) / heightScale, 0, 1)
        if (height <= seaLevel)
        {
            return 0.0f;
        }

        float cooling = (height - seaLevel) / heightScale;
        return std::clamp(cooling, 0.0f, 1.0f);
    }

    void ClimateGenerator::ComputeRainShadow(const std::vector<float> &heightmap,
                                             float cellSize)
    {
        // Rain shadow: mountains block moisture from prevailing wind direction
        // Assume prevailing wind from west (negative X direction)
        // Cells downwind (east) of mountains receive less moisture

        // First pass: compute max upwind elevation for each cell
        std::vector<float> maxUpwindHeight(heightmap.size(), 0.0f);

        // Scan from west to east (following wind direction)
        for (int z = 0; z < m_Data.depth; z++)
        {
            float runningMax = 0.0f;

            for (int x = 0; x < m_Data.width; x++)
            {
                size_t idx = m_Data.Index(x, z);
                float height = heightmap[idx];

                // Update running maximum
                runningMax = std::max(runningMax, height);

                // Decay the running max slightly (distance effect)
                runningMax *= 0.995f;

                maxUpwindHeight[idx] = runningMax;
            }
        }

        // Second pass: compute rain shadow intensity
        for (int z = 0; z < m_Data.depth; z++)
        {
            for (int x = 0; x < m_Data.width; x++)
            {
                size_t idx = m_Data.Index(x, z);
                float height = heightmap[idx];
                float upwindMax = maxUpwindHeight[idx];

                // Rain shadow intensity based on height difference
                // If we're lower than upwind terrain, we're in shadow
                if (upwindMax > height)
                {
                    float shadowDepth = (upwindMax - height) / 50.0f; // Normalize
                    m_Data.rainShadow[idx] = std::clamp(shadowDepth, 0.0f, 1.0f);
                }
                else
                {
                    m_Data.rainShadow[idx] = 0.0f;
                }
            }
        }
    }

    float ClimateGenerator::GetTemperature(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
            return 0.0f;
        return m_Data.temperature[m_Data.Index(x, z)];
    }

    float ClimateGenerator::GetMoisture(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
            return 0.5f;
        return m_Data.moisture[m_Data.Index(x, z)];
    }

    float ClimateGenerator::GetFertility(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
            return 0.0f;
        return m_Data.fertility[m_Data.Index(x, z)];
    }

}
