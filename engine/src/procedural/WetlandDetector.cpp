#include "genesis/procedural/WetlandDetector.h"
#include <algorithm>
#include <cmath>

namespace Genesis
{

    void WetlandDetector::Detect(const HydrologyData &hydrology)
    {
        m_Data.Resize(hydrology.width, hydrology.depth);
        m_Data.Clear();

        for (int z = 0; z < hydrology.depth; z++)
        {
            for (int x = 0; x < hydrology.width; x++)
            {
                size_t idx = m_Data.Index(x, z);

                // Skip water cells - they are water bodies, not wetlands
                if (hydrology.waterType[idx] != WaterType::None)
                {
                    continue;
                }

                // Get hydrology conditions
                float distanceToWater = hydrology.distanceToWater[idx];
                float slope = hydrology.slope[idx];
                float moisture = hydrology.moisture[idx];
                uint32_t flowAccum = hydrology.flowAccumulation[idx];

                // Section 23: Wetlands form where:
                // nearRiver && lowSlope && highMoisture
                float intensity = ComputeWetlandIntensity(distanceToWater, slope,
                                                          moisture, flowAccum);

                if (intensity > 0.0f)
                {
                    m_Data.isWetland[idx] = true;
                    m_Data.wetlandIntensity[idx] = intensity;

                    // Floodplain: near river with very low slope
                    // More specifically associated with river flooding
                    bool nearRiver = distanceToWater < m_Settings.maxWaterDistance * 0.5f;
                    bool veryLowSlope = slope < m_Settings.maxSlope * 0.5f;
                    bool highFlow = flowAccum > m_Settings.minFlowAccumulation * 2;

                    if (nearRiver && veryLowSlope && highFlow)
                    {
                        m_Data.isFloodplain[idx] = true;
                    }
                }
            }
        }
    }

    float WetlandDetector::ComputeWetlandIntensity(float distanceToWater,
                                                   float slope,
                                                   float moisture,
                                                   uint32_t flowAccum) const
    {
        // Check basic wetland conditions
        // nearRiver && lowSlope && highMoisture

        // Condition 1: Near water
        bool nearWater = distanceToWater < m_Settings.maxWaterDistance;
        if (!nearWater)
            return 0.0f;

        // Condition 2: Low slope
        bool lowSlope = slope < m_Settings.maxSlope;
        if (!lowSlope)
            return 0.0f;

        // Condition 3: High moisture (from moisture field OR high flow accumulation)
        bool highMoisture = moisture > m_Settings.minMoisture ||
                            flowAccum > m_Settings.minFlowAccumulation;
        if (!highMoisture)
            return 0.0f;

        // All conditions met - compute intensity based on how ideal conditions are

        // Distance factor: closer to water = higher intensity
        float distanceFactor = 1.0f - (distanceToWater / m_Settings.maxWaterDistance);
        distanceFactor = std::max(distanceFactor, 0.0f);

        // Slope factor: flatter = higher intensity
        float slopeFactor = 1.0f - (slope / m_Settings.maxSlope);
        slopeFactor = std::max(slopeFactor, 0.0f);

        // Moisture factor: wetter = higher intensity
        float moistureFactor = (moisture - m_Settings.minMoisture) /
                               (1.0f - m_Settings.minMoisture);
        moistureFactor = std::clamp(moistureFactor, 0.0f, 1.0f);

        // Flow factor: more flow = higher intensity (indicates active drainage)
        float flowFactor = static_cast<float>(flowAccum) /
                           static_cast<float>(m_Settings.minFlowAccumulation * 10);
        flowFactor = std::min(flowFactor, 1.0f);

        // Combine factors - use geometric mean for balanced contribution
        float intensity = std::sqrt(distanceFactor * slopeFactor) *
                          std::max(moistureFactor, flowFactor);

        return std::clamp(intensity, 0.0f, 1.0f);
    }

    bool WetlandDetector::IsWetland(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
            return false;
        return m_Data.isWetland[m_Data.Index(x, z)];
    }

    float WetlandDetector::GetWetlandIntensity(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
            return 0.0f;
        return m_Data.wetlandIntensity[m_Data.Index(x, z)];
    }

    bool WetlandDetector::IsFloodplain(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
            return false;
        return m_Data.isFloodplain[m_Data.Index(x, z)];
    }

}
