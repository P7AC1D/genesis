#include "genesis/procedural/BiomeClassifier.h"
#include <algorithm>
#include <cmath>

namespace Genesis
{

    void BiomeClassifier::Classify(const ClimateData &climate,
                                   const WetlandData *wetlands)
    {
        m_Data.Resize(climate.width, climate.depth);
        m_Data.Clear();

        for (int z = 0; z < climate.depth; z++)
        {
            for (int x = 0; x < climate.width; x++)
            {
                size_t idx = m_Data.Index(x, z);

                float temperature = climate.temperature[idx];
                float moisture = climate.moisture[idx];
                float fertility = climate.fertility[idx];

                bool isWetland = false;
                if (wetlands && wetlands->InBounds(x, z))
                {
                    isWetland = wetlands->isWetland[wetlands->Index(x, z)];
                }

                // Compute biome weights
                BiomeWeights weights = ComputeBiomeWeights(temperature, moisture,
                                                           fertility, isWetland);
                weights.Normalize();

                m_Data.cellBiomes[idx] = weights;
                m_Data.dominantBiome[idx] = weights.GetDominant();
            }
        }
    }

    BiomeWeights BiomeClassifier::ComputeBiomeWeights(float temperature, float moisture,
                                                      float fertility, bool isWetland) const
    {
        BiomeWeights weights;

        // Section 28: Biome classification via continuous thresholds
        // Temperature range: [-1, 1], Moisture range: [0, 1]

        // Wetland override - if wetland conditions met, strongly weight wetland
        if (isWetland)
        {
            weights[BiomeType::Wetland] = 0.7f;
            // Still allow other biomes to blend
        }

        // === Temperature-based classifications ===

        // Polar: temperature < -0.6
        // Very cold, frozen
        float polarWeight = SmoothThreshold(-temperature, 0.6f, 0.15f);
        weights[BiomeType::Polar] += polarWeight;

        // Tundra: temperature in [-0.6, -0.3]
        // Cold but not frozen
        float tundraWeight = SmoothThreshold(-temperature, 0.3f, 0.15f) *
                             (1.0f - SmoothThreshold(-temperature, 0.6f, 0.15f));
        weights[BiomeType::Tundra] += tundraWeight;

        // Boreal: temperature < 0.0 (cold)
        // Cold forest
        float borealWeight = SmoothThreshold(-temperature, 0.0f, 0.2f) *
                             (1.0f - SmoothThreshold(-temperature, 0.3f, 0.15f)) *
                             SmoothThreshold(moisture, 0.3f, 0.15f);
        weights[BiomeType::Boreal] += borealWeight;

        // === Moisture-based classifications ===

        // Desert: moisture < 0.15
        // Hot and dry
        float desertWeight = SmoothThreshold(-moisture, -0.15f, 0.1f) *
                             SmoothThreshold(temperature, -0.2f, 0.2f);
        weights[BiomeType::Desert] += desertWeight;

        // Grassland/Savanna: moderate moisture, warm
        float grasslandWeight = SmoothThreshold(moisture, 0.2f, 0.15f) *
                                SmoothThreshold(-moisture, -0.5f, 0.15f) *
                                SmoothThreshold(temperature, 0.0f, 0.2f);
        weights[BiomeType::Grassland] += grasslandWeight;

        // Mediterranean: warm, moderate-low moisture
        float medWeight = SmoothThreshold(temperature, 0.2f, 0.2f) *
                          SmoothThreshold(moisture, 0.2f, 0.15f) *
                          SmoothThreshold(-moisture, -0.5f, 0.15f);
        weights[BiomeType::Mediterranean] += medWeight;

        // === High moisture + temperature combinations ===

        // Tropical: hot and wet (but not extreme)
        // moisture > 0.5 && temperature > 0.3
        float tropicalWeight = SmoothThreshold(moisture, 0.5f, 0.15f) *
                               SmoothThreshold(temperature, 0.3f, 0.15f) *
                               (1.0f - SmoothThreshold(moisture, 0.7f, 0.15f));
        weights[BiomeType::Tropical] += tropicalWeight;

        // Rainforest: moisture > 0.7 && temperature > 0.4
        // Very hot and very wet
        float rainforestWeight = SmoothThreshold(moisture, 0.7f, 0.1f) *
                                 SmoothThreshold(temperature, 0.4f, 0.15f);
        weights[BiomeType::Rainforest] += rainforestWeight;

        // === Default: Temperate ===
        // Moderate conditions - fills in the gaps
        float tempWeight = SmoothThreshold(temperature, -0.3f, 0.2f) *
                           SmoothThreshold(-temperature, -0.5f, 0.2f) *
                           SmoothThreshold(moisture, 0.25f, 0.15f) *
                           SmoothThreshold(-moisture, -0.7f, 0.15f);
        weights[BiomeType::Temperate] += tempWeight;

        // Ensure at least some weight exists
        float totalWeight = 0.0f;
        for (size_t i = 0; i < static_cast<size_t>(BiomeType::Count); i++)
        {
            totalWeight += weights.weights[i];
        }

        if (totalWeight < 0.01f)
        {
            // Default to temperate if no strong classification
            weights[BiomeType::Temperate] = 1.0f;
        }

        return weights;
    }

    float BiomeClassifier::SmoothThreshold(float value, float threshold, float width) const
    {
        // Smooth step function centered at threshold with given width
        // Returns 0 when value << threshold, 1 when value >> threshold
        float t = (value - threshold + width) / (2.0f * width);
        t = std::clamp(t, 0.0f, 1.0f);

        // Smoothstep interpolation
        return t * t * (3.0f - 2.0f * t);
    }

    BiomeWeights BiomeClassifier::GetBiomeWeights(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
        {
            BiomeWeights defaultWeights;
            defaultWeights[BiomeType::Temperate] = 1.0f;
            return defaultWeights;
        }
        return m_Data.cellBiomes[m_Data.Index(x, z)];
    }

    BiomeType BiomeClassifier::GetDominantBiome(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
            return BiomeType::Temperate;
        return m_Data.dominantBiome[m_Data.Index(x, z)];
    }

    const char *BiomeClassifier::GetBiomeName(BiomeType type)
    {
        switch (type)
        {
        case BiomeType::Polar:
            return "Polar";
        case BiomeType::Tundra:
            return "Tundra";
        case BiomeType::Boreal:
            return "Boreal";
        case BiomeType::Temperate:
            return "Temperate";
        case BiomeType::Mediterranean:
            return "Mediterranean";
        case BiomeType::Grassland:
            return "Grassland";
        case BiomeType::Desert:
            return "Desert";
        case BiomeType::Tropical:
            return "Tropical";
        case BiomeType::Rainforest:
            return "Rainforest";
        case BiomeType::Wetland:
            return "Wetland";
        default:
            return "Unknown";
        }
    }

    glm::vec3 BiomeClassifier::GetBiomeColor(BiomeType type)
    {
        switch (type)
        {
        case BiomeType::Polar:
            return glm::vec3(0.95f, 0.95f, 1.0f); // White-blue (ice/snow)
        case BiomeType::Tundra:
            return glm::vec3(0.7f, 0.75f, 0.8f); // Gray-blue (frozen ground)
        case BiomeType::Boreal:
            return glm::vec3(0.2f, 0.4f, 0.3f); // Dark green (conifer forest)
        case BiomeType::Temperate:
            return glm::vec3(0.3f, 0.6f, 0.3f); // Medium green (deciduous)
        case BiomeType::Mediterranean:
            return glm::vec3(0.6f, 0.7f, 0.4f); // Olive (dry scrubland)
        case BiomeType::Grassland:
            return glm::vec3(0.7f, 0.8f, 0.4f); // Yellow-green (savanna)
        case BiomeType::Desert:
            return glm::vec3(0.9f, 0.8f, 0.5f); // Sandy yellow
        case BiomeType::Tropical:
            return glm::vec3(0.2f, 0.7f, 0.3f); // Bright green (lush)
        case BiomeType::Rainforest:
            return glm::vec3(0.1f, 0.5f, 0.2f); // Deep green (dense canopy)
        case BiomeType::Wetland:
            return glm::vec3(0.3f, 0.5f, 0.5f); // Teal (marsh/swamp)
        default:
            return glm::vec3(0.5f, 0.5f, 0.5f); // Gray (unknown)
        }
    }

    glm::vec3 BiomeClassifier::GetBlendedBiomeColor(const BiomeWeights &weights)
    {
        glm::vec3 blendedColor(0.0f);

        for (size_t i = 0; i < static_cast<size_t>(BiomeType::Count); i++)
        {
            float weight = weights.weights[i];
            if (weight > 0.0f)
            {
                BiomeType biomeType = static_cast<BiomeType>(i);
                blendedColor += GetBiomeColor(biomeType) * weight;
            }
        }

        return blendedColor;
    }

}
