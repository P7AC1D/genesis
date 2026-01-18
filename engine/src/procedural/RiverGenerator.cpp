#include "genesis/procedural/RiverGenerator.h"
#include <algorithm>
#include <cmath>
#include <queue>

namespace Genesis
{

    void RiverGenerator::Configure(float riverStrength, float cellSize)
    {
        m_CellSize = cellSize;

        // Thresholds scale inversely with riverStrength
        // Higher riverStrength = lower thresholds = more rivers
        m_Settings.streamThreshold = static_cast<uint32_t>(
            100.0f - 80.0f * riverStrength); // 100 -> 20
        m_Settings.majorRiverThreshold = static_cast<uint32_t>(
            1000.0f - 800.0f * riverStrength); // 1000 -> 200

        // River geometry scales with riverStrength
        m_Settings.riverWidthScale = 0.05f + 0.15f * riverStrength; // 0.05 -> 0.2
        m_Settings.channelDepth = 1.0f + 3.0f * riverStrength;      // 1 -> 4

        // Bank characteristics
        m_Settings.bankSlope = 0.6f - 0.3f * riverStrength;   // 0.6 -> 0.3 (steeper with strength)
        m_Settings.bedFlatness = 0.6f + 0.3f * riverStrength; // 0.6 -> 0.9
    }

    void RiverGenerator::Generate(const DrainageGraph &drainage,
                                  const std::vector<float> &heightmap,
                                  float seaLevel)
    {
        const DrainageData &data = drainage.GetData();
        m_Network.Resize(data.width, data.depth);
        m_Network.Clear();

        // Step 1: Classify cells based on flow accumulation
        ClassifyCells(drainage, heightmap, seaLevel);

        // Step 2: Build river segments
        BuildSegments(drainage, heightmap);

        // Step 3: Trace river paths from sources to termini
        TraceRiverPaths(drainage);
    }

    void RiverGenerator::ClassifyCells(const DrainageGraph &drainage,
                                       const std::vector<float> &heightmap,
                                       float seaLevel)
    {
        const DrainageData &data = drainage.GetData();

        for (int z = 0; z < data.depth; z++)
        {
            for (int x = 0; x < data.width; x++)
            {
                size_t idx = m_Network.Index(x, z);
                uint32_t flowAccum = drainage.GetFlowAccumulation(x, z);
                float height = heightmap[idx];

                // Check if below sea level (ocean)
                if (height < seaLevel)
                {
                    FlowDirection dir = drainage.GetFlowDirection(x, z);
                    if (dir == FlowDirection::Ocean || dir == FlowDirection::Boundary)
                    {
                        m_Network.cellWaterType[idx] = WaterType::Ocean;
                        continue;
                    }
                }

                // Section 21.1: River Classification
                // if (flowAccum > majorRiverThreshold) waterType = River
                // else if (flowAccum > streamThreshold) waterType = Stream
                if (flowAccum > m_Settings.majorRiverThreshold)
                {
                    m_Network.cellWaterType[idx] = WaterType::River;
                    m_Network.cellRiverWidth[idx] = CalculateRiverWidth(flowAccum);
                }
                else if (flowAccum > m_Settings.streamThreshold)
                {
                    m_Network.cellWaterType[idx] = WaterType::Stream;
                    m_Network.cellRiverWidth[idx] = CalculateRiverWidth(flowAccum);
                }

                // Check for pits (potential lakes)
                FlowDirection dir = drainage.GetFlowDirection(x, z);
                if (dir == FlowDirection::Pit)
                {
                    m_Network.cellWaterType[idx] = WaterType::Lake;
                }
            }
        }
    }

    void RiverGenerator::BuildSegments(const DrainageGraph &drainage,
                                       const std::vector<float> &heightmap)
    {
        const DrainageData &data = drainage.GetData();

        for (int z = 0; z < data.depth; z++)
        {
            for (int x = 0; x < data.width; x++)
            {
                size_t idx = m_Network.Index(x, z);
                WaterType type = m_Network.cellWaterType[idx];

                // Only create segments for rivers and streams
                if (type != WaterType::River && type != WaterType::Stream)
                {
                    continue;
                }

                uint32_t flowAccum = drainage.GetFlowAccumulation(x, z);
                float width = CalculateRiverWidth(flowAccum);
                float depth = CalculateRiverDepth(width);
                float terrainHeight = heightmap[idx];

                RiverSegment segment;
                segment.cell = glm::ivec2(x, z);
                segment.width = width;
                segment.depth = depth;
                segment.surfaceHeight = terrainHeight - depth * 0.5f; // Surface at half depth
                segment.type = type;
                segment.flowAccum = flowAccum;
                segment.downstreamIndex = -1; // Will be set in TraceRiverPaths

                // Store surface height in network
                m_Network.cellSurfaceHeight[idx] = segment.surfaceHeight;

                m_Network.segments.push_back(segment);
            }
        }
    }

    void RiverGenerator::TraceRiverPaths(const DrainageGraph &drainage)
    {
        const DrainageData &data = drainage.GetData();

        // Build a map from cell to segment index
        std::vector<int> cellToSegment(static_cast<size_t>(data.width) * data.depth, -1);
        for (size_t i = 0; i < m_Network.segments.size(); i++)
        {
            const auto &seg = m_Network.segments[i];
            size_t idx = m_Network.Index(seg.cell.x, seg.cell.y);
            cellToSegment[idx] = static_cast<int>(i);
        }

        // Link segments to their downstream neighbors
        for (size_t i = 0; i < m_Network.segments.size(); i++)
        {
            auto &seg = m_Network.segments[i];
            int downX, downZ;
            if (drainage.GetDownstreamCell(seg.cell.x, seg.cell.y, downX, downZ))
            {
                size_t downIdx = m_Network.Index(downX, downZ);
                seg.downstreamIndex = cellToSegment[downIdx];
            }
        }

        // Find river sources (segments with no upstream river/stream contributors)
        std::vector<int> upstreamCount(m_Network.segments.size(), 0);
        for (const auto &seg : m_Network.segments)
        {
            if (seg.downstreamIndex >= 0)
            {
                upstreamCount[seg.downstreamIndex]++;
            }
        }

        // Trace paths from each source
        for (size_t i = 0; i < m_Network.segments.size(); i++)
        {
            if (upstreamCount[i] == 0)
            {
                // This is a source - trace downstream
                RiverPath path;
                path.source = m_Network.segments[i].cell;
                path.maxAccumulation = 0;
                path.totalLength = 0;

                int currentIdx = static_cast<int>(i);
                while (currentIdx >= 0)
                {
                    path.segmentIndices.push_back(currentIdx);
                    const auto &seg = m_Network.segments[currentIdx];
                    path.maxAccumulation = std::max(path.maxAccumulation, seg.flowAccum);
                    path.totalLength += 1.0f;
                    path.terminus = seg.cell;

                    currentIdx = seg.downstreamIndex;
                }

                // Determine terminus type
                size_t termIdx = m_Network.Index(path.terminus.x, path.terminus.y);
                FlowDirection termDir = drainage.GetFlowDirection(path.terminus.x, path.terminus.y);

                if (termDir == FlowDirection::Ocean || termDir == FlowDirection::Boundary)
                {
                    path.terminusType = WaterType::Ocean;
                }
                else if (termDir == FlowDirection::Pit)
                {
                    path.terminusType = WaterType::Lake;
                }
                else
                {
                    path.terminusType = WaterType::None;
                }

                m_Network.rivers.push_back(path);
            }
        }
    }

    void RiverGenerator::CarveRivers(std::vector<float> &heightmap, float cellSize) const
    {
        int width = m_Network.width;
        int depth = m_Network.depth;

        for (const auto &segment : m_Network.segments)
        {
            int cx = segment.cell.x;
            int cz = segment.cell.y;
            size_t centerIdx = m_Network.Index(cx, cz);

            // Calculate carve depth based on river depth
            float carveDepth = segment.depth * m_Settings.channelDepth;

            // River surface height
            float riverSurface = heightmap[centerIdx] - carveDepth;

            // Section 21.3: Post-erosion: height = min(height, riverSurfaceHeight)
            heightmap[centerIdx] = std::min(heightmap[centerIdx], riverSurface);

            // Carve banks based on river width
            int bankRadius = static_cast<int>(std::ceil(segment.width / (2.0f * cellSize)));

            for (int dz = -bankRadius; dz <= bankRadius; dz++)
            {
                for (int dx = -bankRadius; dx <= bankRadius; dx++)
                {
                    if (dx == 0 && dz == 0)
                        continue;

                    int nx = cx + dx;
                    int nz = cz + dz;

                    if (!m_Network.InBounds(nx, nz))
                        continue;

                    size_t nIdx = m_Network.Index(nx, nz);
                    float dist = std::sqrt(static_cast<float>(dx * dx + dz * dz)) * cellSize;
                    float halfWidth = segment.width * 0.5f;

                    if (dist <= halfWidth)
                    {
                        // Inside river channel - flatten to riverbed
                        float bedHeight = riverSurface - carveDepth * m_Settings.bedFlatness;
                        heightmap[nIdx] = std::min(heightmap[nIdx], bedHeight);
                    }
                    else if (dist <= halfWidth + segment.width)
                    {
                        // Bank region - smooth transition
                        float t = (dist - halfWidth) / segment.width;
                        t = t * t * (3.0f - 2.0f * t); // Smoothstep

                        float bankHeight = riverSurface + (heightmap[nIdx] - riverSurface) * t;
                        bankHeight = std::max(bankHeight, riverSurface); // Don't go below river surface

                        // Only carve down, never raise terrain
                        if (bankHeight < heightmap[nIdx])
                        {
                            heightmap[nIdx] = bankHeight;
                        }
                    }
                }
            }
        }
    }

    float RiverGenerator::CalculateRiverWidth(uint32_t flowAccumulation) const
    {
        // Section 21.2: riverWidth = sqrt(flowAccum) * riverWidthScale
        float width = std::sqrt(static_cast<float>(flowAccumulation)) * m_Settings.riverWidthScale;
        return std::clamp(width, m_Settings.minRiverWidth, m_Settings.maxRiverWidth);
    }

    float RiverGenerator::CalculateRiverDepth(float width) const
    {
        // Depth scales with width but less dramatically
        // Typical river depth/width ratio is ~0.1-0.2
        return width * 0.15f;
    }

    WaterType RiverGenerator::GetWaterType(int x, int z) const
    {
        if (!m_Network.InBounds(x, z))
        {
            return WaterType::None;
        }
        return m_Network.cellWaterType[m_Network.Index(x, z)];
    }

    float RiverGenerator::GetRiverWidth(int x, int z) const
    {
        if (!m_Network.InBounds(x, z))
        {
            return 0.0f;
        }
        return m_Network.cellRiverWidth[m_Network.Index(x, z)];
    }

    float RiverGenerator::GetRiverSurfaceHeight(int x, int z) const
    {
        if (!m_Network.InBounds(x, z))
        {
            return 0.0f;
        }
        return m_Network.cellSurfaceHeight[m_Network.Index(x, z)];
    }

    bool RiverGenerator::IsWater(int x, int z) const
    {
        return GetWaterType(x, z) != WaterType::None;
    }

}
