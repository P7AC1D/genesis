#include "genesis/procedural/MaterialBlender.h"
#include <algorithm>
#include <cmath>

namespace Genesis
{

    void MaterialBlender::Compute(const std::vector<float> &heightmap,
                                  const HydrologyData &hydrology,
                                  const ClimateData &climate,
                                  float seaLevel,
                                  float heightScale)
    {
        m_Data.Resize(hydrology.width, hydrology.depth);
        m_Data.Clear();

        for (int z = 0; z < hydrology.depth; z++)
        {
            for (int x = 0; x < hydrology.width; x++)
            {
                size_t idx = m_Data.Index(x, z);

                float height = heightmap[idx];
                float heightNorm = std::clamp((height - seaLevel) / heightScale, 0.0f, 1.0f);
                float slope = hydrology.slope[idx];
                float temperature = climate.temperature[idx];
                float moisture = climate.moisture[idx];
                float fertility = climate.fertility[idx];
                float distanceToWater = hydrology.distanceToWater[idx];
                WaterType waterType = hydrology.waterType[idx];

                MaterialWeights weights = ComputeMaterialWeights(
                    height, heightNorm, slope, temperature, moisture,
                    fertility, distanceToWater, waterType);

                weights.Normalize();

                m_Data.cellMaterials[idx] = weights;
                m_Data.dominantMaterial[idx] = weights.GetDominant();
            }
        }
    }

    MaterialWeights MaterialBlender::ComputeMaterialWeights(float height,
                                                            float heightNorm,
                                                            float slope,
                                                            float temperature,
                                                            float moisture,
                                                            float fertility,
                                                            float distanceToWater,
                                                            WaterType waterType) const
    {
        MaterialWeights weights;

        // Section 29.3: Example Weight Computation

        // Water bodies get full water weight
        if (waterType != WaterType::None)
        {
            weights[MaterialType::Water] = 1.0f;
            return weights;
        }

        // Normalize slope to [0, 1] range (assume max meaningful slope ~2.0)
        float normalizedSlope = std::min(slope / 2.0f, 1.0f);

        // Helper values
        float lowSlope = 1.0f - normalizedSlope;
        float nearWater = std::max(0.0f, 1.0f - distanceToWater / m_Settings.sandDistance);
        float highMoisture = std::max(0.0f, (moisture - m_Settings.mudMoistureThreshold) /
                                                (1.0f - m_Settings.mudMoistureThreshold));

        // === Section 29.3: rockWeight = slope ===
        // Steep slopes expose rock
        float rockWeight = normalizedSlope;
        // Extra rock on very steep terrain
        if (normalizedSlope > m_Settings.rockSlopeThreshold)
        {
            float steepFactor = (normalizedSlope - m_Settings.rockSlopeThreshold) /
                                (m_Settings.steepSlopeThreshold - m_Settings.rockSlopeThreshold);
            rockWeight += steepFactor * 0.5f;
        }
        weights[MaterialType::Rock] = rockWeight;

        // === Section 29.3: snowWeight = clamp(-temperature, 0, 1) * heightNorm ===
        // Cold + high altitude = snow
        float coldFactor = std::clamp(-temperature, 0.0f, 1.0f);
        float snowWeight = coldFactor * heightNorm;
        // Snow line threshold
        if (heightNorm > m_Settings.snowLineStart)
        {
            float snowLineFactor = (heightNorm - m_Settings.snowLineStart) /
                                   (m_Settings.snowLineFull - m_Settings.snowLineStart);
            snowWeight += coldFactor * snowLineFactor * 0.5f;
        }
        weights[MaterialType::Snow] = std::clamp(snowWeight, 0.0f, 1.0f);

        // === Ice: very cold + moisture ===
        // Ice forms when temperature is below freezing and there's moisture
        float iceFactor = std::clamp(-(temperature - m_Settings.freezingPoint), 0.0f, 1.0f);
        float iceWeight = iceFactor * moisture * 0.5f;
        weights[MaterialType::Ice] = std::clamp(iceWeight, 0.0f, 1.0f);

        // === Section 29.3: grassWeight = fertility * moisture * (1 - slope) ===
        // Fertile, moist, flat terrain grows grass
        float grassWeight = 0.0f;
        if (fertility > m_Settings.grassFertilityMin &&
            moisture > m_Settings.grassMoistureMin &&
            temperature > m_Settings.snowMeltPoint)
        {
            grassWeight = fertility * moisture * lowSlope;
        }
        weights[MaterialType::Grass] = grassWeight;

        // === Section 29.3: sandWeight = nearWater * lowSlope ===
        // Beaches near water on flat terrain
        float sandWeight = 0.0f;
        if (normalizedSlope < m_Settings.sandSlopeMax && nearWater > 0.0f)
        {
            sandWeight = nearWater * lowSlope;
            // More sand in warm, dry areas
            if (temperature > 0.0f && moisture < 0.4f)
            {
                sandWeight *= 1.5f;
            }
        }
        weights[MaterialType::Sand] = std::clamp(sandWeight, 0.0f, 1.0f);

        // === Section 29.3: mudWeight = highMoisture * lowSlope ===
        // Wet flat areas become muddy
        float mudWeight = highMoisture * lowSlope;
        // More mud near water
        mudWeight += nearWater * moisture * 0.3f;
        weights[MaterialType::Mud] = std::clamp(mudWeight, 0.0f, 1.0f);

        // === Dirt: default fill for remaining areas ===
        // Low fertility, moderate conditions
        float dirtWeight = lowSlope * (1.0f - fertility) * (1.0f - moisture * 0.5f);
        // Reduce dirt where other materials dominate
        dirtWeight *= (1.0f - snowWeight) * (1.0f - sandWeight * 0.5f);
        weights[MaterialType::Dirt] = std::max(0.0f, dirtWeight);

        return weights;
    }

    MaterialWeights MaterialBlender::GetMaterialWeights(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
        {
            MaterialWeights defaultWeights;
            defaultWeights[MaterialType::Dirt] = 1.0f;
            return defaultWeights;
        }
        return m_Data.cellMaterials[m_Data.Index(x, z)];
    }

    MaterialType MaterialBlender::GetDominantMaterial(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
            return MaterialType::Dirt;
        return m_Data.dominantMaterial[m_Data.Index(x, z)];
    }

    const char *MaterialBlender::GetMaterialName(MaterialType type)
    {
        switch (type)
        {
        case MaterialType::Rock:
            return "Rock";
        case MaterialType::Dirt:
            return "Dirt";
        case MaterialType::Grass:
            return "Grass";
        case MaterialType::Sand:
            return "Sand";
        case MaterialType::Snow:
            return "Snow";
        case MaterialType::Ice:
            return "Ice";
        case MaterialType::Mud:
            return "Mud";
        case MaterialType::Water:
            return "Water";
        default:
            return "Unknown";
        }
    }

}
