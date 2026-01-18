#include "genesis/procedural/HydrologyData.h"
#include <algorithm>
#include <cmath>
#include <queue>

namespace Genesis
{

    void HydrologyGenerator::Compute(const DrainageGraph &drainage,
                                     const RiverGenerator &rivers,
                                     const LakeGenerator &lakes,
                                     const std::vector<float> &heightmap,
                                     float cellSize)
    {
        const DrainageData &drainData = drainage.GetData();
        m_Data.Resize(drainData.width, drainData.depth);
        m_Data.Clear();

        // Step 1: Copy drainage data (flow direction, accumulation, slope)
        CopyDrainageData(drainage);

        // Step 2: Merge water types from rivers and lakes
        MergeWaterTypes(rivers, lakes);

        // Step 3: Compute distance to water via BFS
        ComputeDistanceToWater(cellSize);

        // Step 4: Compute moisture field
        ComputeMoisture();
    }

    void HydrologyGenerator::CopyDrainageData(const DrainageGraph &drainage)
    {
        const DrainageData &src = drainage.GetData();

        for (int z = 0; z < m_Data.depth; z++)
        {
            for (int x = 0; x < m_Data.width; x++)
            {
                size_t idx = m_Data.Index(x, z);
                m_Data.drainageDirection[idx] = src.flowDirection[idx];
                m_Data.flowAccumulation[idx] = src.flowAccumulation[idx];
                m_Data.slope[idx] = src.slope[idx];
            }
        }
    }

    void HydrologyGenerator::MergeWaterTypes(const RiverGenerator &rivers,
                                             const LakeGenerator &lakes)
    {
        const RiverNetwork &riverNet = rivers.GetNetwork();
        const LakeNetwork &lakeNet = lakes.GetNetwork();

        for (int z = 0; z < m_Data.depth; z++)
        {
            for (int x = 0; x < m_Data.width; x++)
            {
                size_t idx = m_Data.Index(x, z);

                // Check river network first (rivers take precedence for type)
                WaterType riverType = WaterType::None;
                if (riverNet.InBounds(x, z))
                {
                    size_t rIdx = riverNet.Index(x, z);
                    riverType = riverNet.cellWaterType[rIdx];
                    if (riverType != WaterType::None)
                    {
                        m_Data.waterSurfaceHeight[idx] = riverNet.cellSurfaceHeight[rIdx];
                    }
                }

                // Check lake network
                WaterType lakeType = WaterType::None;
                if (lakeNet.InBounds(x, z))
                {
                    int lakeIdx = lakeNet.cellLakeIndex[lakeNet.Index(x, z)];
                    if (lakeIdx >= 0)
                    {
                        lakeType = WaterType::Lake;
                        const LakeBasin &basin = lakeNet.lakes[lakeIdx];
                        m_Data.waterSurfaceHeight[idx] = basin.surfaceHeight;
                    }
                }

                // Determine final water type (priority: Ocean > Lake > River > Stream)
                if (riverType == WaterType::Ocean)
                {
                    m_Data.waterType[idx] = WaterType::Ocean;
                }
                else if (lakeType == WaterType::Lake)
                {
                    m_Data.waterType[idx] = WaterType::Lake;
                }
                else if (riverType == WaterType::River)
                {
                    m_Data.waterType[idx] = WaterType::River;
                }
                else if (riverType == WaterType::Stream)
                {
                    m_Data.waterType[idx] = WaterType::Stream;
                }
            }
        }
    }

    void HydrologyGenerator::ComputeDistanceToWater(float cellSize)
    {
        // Multi-source BFS from all water cells
        std::queue<std::pair<int, int>> queue;

        // Initialize: all water cells have distance 0
        for (int z = 0; z < m_Data.depth; z++)
        {
            for (int x = 0; x < m_Data.width; x++)
            {
                size_t idx = m_Data.Index(x, z);
                if (m_Data.waterType[idx] != WaterType::None)
                {
                    m_Data.distanceToWater[idx] = 0.0f;
                    queue.push({x, z});
                }
            }
        }

        // BFS expansion
        while (!queue.empty())
        {
            auto [cx, cz] = queue.front();
            queue.pop();

            size_t cIdx = m_Data.Index(cx, cz);
            float currentDist = m_Data.distanceToWater[cIdx];

            // Check 8 neighbors
            for (int d = 0; d < 8; d++)
            {
                int nx = cx + FLOW_OFFSET_X[d];
                int nz = cz + FLOW_OFFSET_Z[d];

                if (!m_Data.InBounds(nx, nz))
                    continue;

                size_t nIdx = m_Data.Index(nx, nz);
                float stepDist = FLOW_DISTANCE[d] * cellSize;
                float newDist = currentDist + stepDist;

                // Only update if we found a shorter path and within max distance
                if (newDist < m_Data.distanceToWater[nIdx] &&
                    newDist < m_Settings.maxWaterDistance)
                {
                    m_Data.distanceToWater[nIdx] = newDist;
                    queue.push({nx, nz});
                }
            }
        }
    }

    void HydrologyGenerator::ComputeMoisture()
    {
        for (int z = 0; z < m_Data.depth; z++)
        {
            for (int x = 0; x < m_Data.width; x++)
            {
                size_t idx = m_Data.Index(x, z);

                // Component 1: Flow accumulation contribution
                // Higher flow = more water passing through = more moisture
                float flowFactor = static_cast<float>(m_Data.flowAccumulation[idx]) /
                                   m_Settings.flowNormalization;
                flowFactor = std::min(flowFactor, 1.0f);

                // Component 2: Proximity to water contribution
                // Closer to water = more moisture
                float proximityFactor = 1.0f - (m_Data.distanceToWater[idx] /
                                                m_Settings.maxWaterDistance);
                proximityFactor = std::max(proximityFactor, 0.0f);

                // Component 3: Base humidity
                float humidityFactor = m_Settings.baseHumidity;

                // Water cells have maximum moisture
                if (m_Data.waterType[idx] != WaterType::None)
                {
                    m_Data.moisture[idx] = 1.0f;
                }
                else
                {
                    // Weighted combination
                    m_Data.moisture[idx] =
                        flowFactor * m_Settings.flowMoistureWeight +
                        proximityFactor * m_Settings.proximityMoistureWeight +
                        humidityFactor * m_Settings.humidityWeight;

                    // Clamp to [0, 1]
                    m_Data.moisture[idx] = std::clamp(m_Data.moisture[idx], 0.0f, 1.0f);
                }
            }
        }
    }

    WaterType HydrologyGenerator::GetWaterType(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
            return WaterType::None;
        return m_Data.waterType[m_Data.Index(x, z)];
    }

    float HydrologyGenerator::GetWaterSurfaceHeight(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
            return 0.0f;
        return m_Data.waterSurfaceHeight[m_Data.Index(x, z)];
    }

    uint32_t HydrologyGenerator::GetFlowAccumulation(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
            return 0;
        return m_Data.flowAccumulation[m_Data.Index(x, z)];
    }

    float HydrologyGenerator::GetDistanceToWater(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
            return std::numeric_limits<float>::max();
        return m_Data.distanceToWater[m_Data.Index(x, z)];
    }

    FlowDirection HydrologyGenerator::GetDrainageDirection(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
            return FlowDirection::Pit;
        return m_Data.drainageDirection[m_Data.Index(x, z)];
    }

    float HydrologyGenerator::GetSlope(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
            return 0.0f;
        return m_Data.slope[m_Data.Index(x, z)];
    }

    float HydrologyGenerator::GetMoisture(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
            return 0.0f;
        return m_Data.moisture[m_Data.Index(x, z)];
    }

    bool HydrologyGenerator::IsWater(int x, int z) const
    {
        return GetWaterType(x, z) != WaterType::None;
    }

}
