#include "genesis/procedural/DrainageGraph.h"
#include <algorithm>
#include <queue>
#include <cmath>

namespace Genesis
{

    void DrainageGraph::Compute(const std::vector<float> &heightmap,
                                int width, int depth,
                                float cellSize,
                                float seaLevel)
    {
        ComputeFlowDirections(heightmap, width, depth, cellSize, seaLevel);
        ComputeFlowAccumulation();
    }

    void DrainageGraph::ComputeFlowDirections(const std::vector<float> &heightmap,
                                              int width, int depth,
                                              float cellSize,
                                              float seaLevel)
    {
        m_Data.Resize(width, depth);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                size_t idx = m_Data.Index(x, z);
                m_Data.flowDirection[idx] = ComputeCellFlowDirection(heightmap, x, z, seaLevel);
                m_Data.slope[idx] = ComputeCellSlope(heightmap, x, z, cellSize);
            }
        }
    }

    void DrainageGraph::ComputeFlowAccumulation()
    {
        int width = m_Data.width;
        int depth = m_Data.depth;
        size_t size = static_cast<size_t>(width) * depth;

        // Reset accumulation
        std::fill(m_Data.flowAccumulation.begin(), m_Data.flowAccumulation.end(), 0);

        // Count incoming edges for each cell (in-degree for topological sort)
        std::vector<int> inDegree(size, 0);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                int downX, downZ;
                if (GetDownstreamCell(x, z, downX, downZ))
                {
                    size_t downIdx = m_Data.Index(downX, downZ);
                    inDegree[downIdx]++;
                }
            }
        }

        // Initialize queue with cells that have no upstream contributors (sources)
        std::queue<size_t> ready;
        for (size_t i = 0; i < size; i++)
        {
            if (inDegree[i] == 0)
            {
                ready.push(i);
            }
        }

        // Topological traversal - process cells in upstream-to-downstream order
        while (!ready.empty())
        {
            size_t cellIdx = ready.front();
            ready.pop();

            // Each cell contributes 1 (itself) to accumulation
            m_Data.flowAccumulation[cellIdx] += 1;

            // Pass accumulation to downstream cell
            int x = static_cast<int>(cellIdx % width);
            int z = static_cast<int>(cellIdx / width);

            int downX, downZ;
            if (GetDownstreamCell(x, z, downX, downZ))
            {
                size_t downIdx = m_Data.Index(downX, downZ);

                // Add this cell's accumulation to downstream
                m_Data.flowAccumulation[downIdx] += m_Data.flowAccumulation[cellIdx];

                // Decrement in-degree and add to queue if ready
                inDegree[downIdx]--;
                if (inDegree[downIdx] == 0)
                {
                    ready.push(downIdx);
                }
            }
        }
    }

    FlowDirection DrainageGraph::ComputeCellFlowDirection(const std::vector<float> &heightmap,
                                                          int x, int z,
                                                          float seaLevel) const
    {
        int width = m_Data.width;
        int depth = m_Data.depth;
        size_t idx = m_Data.Index(x, z);
        float currentHeight = heightmap[idx];

        // Cells below sea level drain to ocean
        if (currentHeight < seaLevel)
        {
            return FlowDirection::Ocean;
        }

        // Check if at boundary
        bool atBoundary = (x == 0 || x == width - 1 || z == 0 || z == depth - 1);
        if (atBoundary)
        {
            return FlowDirection::Boundary;
        }

        // Find steepest descent neighbor using D8 algorithm
        // flowDir = argmin(neighbor heights), weighted by distance
        float maxDropPerDist = 0.0f;
        int bestDir = -1;

        for (int dir = 0; dir < 8; dir++)
        {
            int nx = x + FLOW_OFFSET_X[dir];
            int nz = z + FLOW_OFFSET_Z[dir];

            // Skip out-of-bounds neighbors
            if (nx < 0 || nx >= width || nz < 0 || nz >= depth)
            {
                continue;
            }

            size_t nIdx = m_Data.Index(nx, nz);
            float neighborHeight = heightmap[nIdx];

            // Calculate drop per unit distance (steepest descent)
            float drop = currentHeight - neighborHeight;
            float dropPerDist = drop / FLOW_DISTANCE[dir];

            if (dropPerDist > maxDropPerDist)
            {
                maxDropPerDist = dropPerDist;
                bestDir = dir;
            }
        }

        // If no downhill neighbor found, this is a pit (local minimum)
        if (bestDir < 0)
        {
            // Check if all neighbors are same height (flat)
            bool allSame = true;
            for (int dir = 0; dir < 8; dir++)
            {
                int nx = x + FLOW_OFFSET_X[dir];
                int nz = z + FLOW_OFFSET_Z[dir];
                if (nx >= 0 && nx < width && nz >= 0 && nz < depth)
                {
                    size_t nIdx = m_Data.Index(nx, nz);
                    if (std::abs(heightmap[nIdx] - currentHeight) > 0.0001f)
                    {
                        allSame = false;
                        break;
                    }
                }
            }
            return allSame ? FlowDirection::Flat : FlowDirection::Pit;
        }

        return static_cast<FlowDirection>(bestDir);
    }

    float DrainageGraph::ComputeCellSlope(const std::vector<float> &heightmap,
                                          int x, int z,
                                          float cellSize) const
    {
        int width = m_Data.width;
        int depth = m_Data.depth;

        // Use central differences for interior cells, one-sided for edges
        float dhdx, dhdz;

        if (x > 0 && x < width - 1)
        {
            size_t idxL = m_Data.Index(x - 1, z);
            size_t idxR = m_Data.Index(x + 1, z);
            dhdx = (heightmap[idxR] - heightmap[idxL]) / (2.0f * cellSize);
        }
        else if (x == 0)
        {
            size_t idx0 = m_Data.Index(x, z);
            size_t idx1 = m_Data.Index(x + 1, z);
            dhdx = (heightmap[idx1] - heightmap[idx0]) / cellSize;
        }
        else
        {
            size_t idx0 = m_Data.Index(x - 1, z);
            size_t idx1 = m_Data.Index(x, z);
            dhdx = (heightmap[idx1] - heightmap[idx0]) / cellSize;
        }

        if (z > 0 && z < depth - 1)
        {
            size_t idxU = m_Data.Index(x, z - 1);
            size_t idxD = m_Data.Index(x, z + 1);
            dhdz = (heightmap[idxD] - heightmap[idxU]) / (2.0f * cellSize);
        }
        else if (z == 0)
        {
            size_t idx0 = m_Data.Index(x, z);
            size_t idx1 = m_Data.Index(x, z + 1);
            dhdz = (heightmap[idx1] - heightmap[idx0]) / cellSize;
        }
        else
        {
            size_t idx0 = m_Data.Index(x, z - 1);
            size_t idx1 = m_Data.Index(x, z);
            dhdz = (heightmap[idx1] - heightmap[idx0]) / cellSize;
        }

        // Slope magnitude
        return std::sqrt(dhdx * dhdx + dhdz * dhdz);
    }

    FlowDirection DrainageGraph::GetFlowDirection(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
        {
            return FlowDirection::Boundary;
        }
        return m_Data.flowDirection[m_Data.Index(x, z)];
    }

    uint32_t DrainageGraph::GetFlowAccumulation(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
        {
            return 0;
        }
        return m_Data.flowAccumulation[m_Data.Index(x, z)];
    }

    float DrainageGraph::GetSlope(int x, int z) const
    {
        if (!m_Data.InBounds(x, z))
        {
            return 0.0f;
        }
        return m_Data.slope[m_Data.Index(x, z)];
    }

    bool DrainageGraph::GetDownstreamCell(int x, int z, int &outX, int &outZ) const
    {
        if (!m_Data.InBounds(x, z))
        {
            return false;
        }

        FlowDirection dir = m_Data.flowDirection[m_Data.Index(x, z)];

        // Only valid flow directions (0-7) have downstream cells
        if (static_cast<uint8_t>(dir) > 7)
        {
            return false;
        }

        int dirIdx = static_cast<int>(dir);
        outX = x + FLOW_OFFSET_X[dirIdx];
        outZ = z + FLOW_OFFSET_Z[dirIdx];

        return m_Data.InBounds(outX, outZ);
    }

    std::vector<size_t> DrainageGraph::TraceFlowPath(int startX, int startZ) const
    {
        std::vector<size_t> path;

        if (!m_Data.InBounds(startX, startZ))
        {
            return path;
        }

        int x = startX;
        int z = startZ;
        size_t maxSteps = static_cast<size_t>(m_Data.width) * m_Data.depth;

        for (size_t step = 0; step < maxSteps; step++)
        {
            path.push_back(m_Data.Index(x, z));

            int nextX, nextZ;
            if (!GetDownstreamCell(x, z, nextX, nextZ))
            {
                break; // Reached terminus
            }

            x = nextX;
            z = nextZ;
        }

        return path;
    }

    std::vector<glm::ivec2> DrainageGraph::FindRiverCells(uint32_t minAccumulation) const
    {
        std::vector<glm::ivec2> riverCells;

        for (int z = 0; z < m_Data.depth; z++)
        {
            for (int x = 0; x < m_Data.width; x++)
            {
                if (m_Data.flowAccumulation[m_Data.Index(x, z)] >= minAccumulation)
                {
                    riverCells.emplace_back(x, z);
                }
            }
        }

        return riverCells;
    }

    std::vector<glm::ivec2> DrainageGraph::FindPits() const
    {
        std::vector<glm::ivec2> pits;

        for (int z = 0; z < m_Data.depth; z++)
        {
            for (int x = 0; x < m_Data.width; x++)
            {
                if (m_Data.flowDirection[m_Data.Index(x, z)] == FlowDirection::Pit)
                {
                    pits.emplace_back(x, z);
                }
            }
        }

        return pits;
    }

}
