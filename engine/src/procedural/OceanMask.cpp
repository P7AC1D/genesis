#include "genesis/procedural/OceanMask.h"
#include <algorithm>

namespace Genesis
{

    void OceanMask::Initialize(int width, int depth)
    {
        m_Width = width;
        m_Depth = depth;

        int totalCells = width * depth;
        m_BelowSeaMask.resize(totalCells, false);
        m_OceanMask.resize(totalCells, false);

        m_Boundary.Resize(std::max(width, depth));
        m_FloodFillComplete = false;
    }

    void OceanMask::GenerateBelowSeaMask(const std::vector<float> &heightmap, float seaLevel)
    {
        if (heightmap.size() != static_cast<size_t>(m_Width * m_Depth))
        {
            return;
        }

        for (int i = 0; i < m_Width * m_Depth; i++)
        {
            m_BelowSeaMask[i] = heightmap[i] < seaLevel;
        }

        // Reset ocean mask - needs new flood fill
        std::fill(m_OceanMask.begin(), m_OceanMask.end(), false);
        m_Boundary.Clear();
        m_FloodFillComplete = false;
    }

    void OceanMask::FloodFillFromBoundary(
        const std::function<bool(ChunkEdge)> &isAtWorldBoundary,
        const ChunkOceanBoundary *neighborBoundary)
    {

        std::vector<std::pair<int, int>> seeds;

        // Collect seed points from world boundaries
        // At world boundary, all below-sea cells on that edge are potential ocean seeds

        // -X edge (west boundary)
        if (isAtWorldBoundary(ChunkEdge::NegativeX))
        {
            for (int z = 0; z < m_Depth; z++)
            {
                if (m_BelowSeaMask[CellIndex(0, z)])
                {
                    seeds.emplace_back(0, z);
                }
            }
        }
        else if (neighborBoundary)
        {
            // Propagate from neighbor's +X edge
            for (int z = 0; z < m_Depth && z < static_cast<int>(neighborBoundary->posX.size()); z++)
            {
                if (neighborBoundary->posX[z] && m_BelowSeaMask[CellIndex(0, z)])
                {
                    seeds.emplace_back(0, z);
                }
            }
        }

        // +X edge (east boundary)
        if (isAtWorldBoundary(ChunkEdge::PositiveX))
        {
            for (int z = 0; z < m_Depth; z++)
            {
                if (m_BelowSeaMask[CellIndex(m_Width - 1, z)])
                {
                    seeds.emplace_back(m_Width - 1, z);
                }
            }
        }
        else if (neighborBoundary)
        {
            for (int z = 0; z < m_Depth && z < static_cast<int>(neighborBoundary->negX.size()); z++)
            {
                if (neighborBoundary->negX[z] && m_BelowSeaMask[CellIndex(m_Width - 1, z)])
                {
                    seeds.emplace_back(m_Width - 1, z);
                }
            }
        }

        // -Z edge (north boundary)
        if (isAtWorldBoundary(ChunkEdge::NegativeZ))
        {
            for (int x = 0; x < m_Width; x++)
            {
                if (m_BelowSeaMask[CellIndex(x, 0)])
                {
                    seeds.emplace_back(x, 0);
                }
            }
        }
        else if (neighborBoundary)
        {
            for (int x = 0; x < m_Width && x < static_cast<int>(neighborBoundary->posZ.size()); x++)
            {
                if (neighborBoundary->posZ[x] && m_BelowSeaMask[CellIndex(x, 0)])
                {
                    seeds.emplace_back(x, 0);
                }
            }
        }

        // +Z edge (south boundary)
        if (isAtWorldBoundary(ChunkEdge::PositiveZ))
        {
            for (int x = 0; x < m_Width; x++)
            {
                if (m_BelowSeaMask[CellIndex(x, m_Depth - 1)])
                {
                    seeds.emplace_back(x, m_Depth - 1);
                }
            }
        }
        else if (neighborBoundary)
        {
            for (int x = 0; x < m_Width && x < static_cast<int>(neighborBoundary->negZ.size()); x++)
            {
                if (neighborBoundary->negZ[x] && m_BelowSeaMask[CellIndex(x, m_Depth - 1)])
                {
                    seeds.emplace_back(x, m_Depth - 1);
                }
            }
        }

        // Perform BFS flood fill from seeds
        FloodFillBFS(seeds);
        UpdateBoundaryFromMask();
        m_FloodFillComplete = true;
    }

    void OceanMask::PropagateFromNeighbor(ChunkEdge edge, const std::vector<bool> &neighborEdge)
    {
        std::vector<std::pair<int, int>> seeds;

        switch (edge)
        {
        case ChunkEdge::NegativeX:
            // Neighbor is on our -X side, their +X edge connects to our x=0
            for (int z = 0; z < m_Depth && z < static_cast<int>(neighborEdge.size()); z++)
            {
                if (neighborEdge[z] && m_BelowSeaMask[CellIndex(0, z)] && !m_OceanMask[CellIndex(0, z)])
                {
                    seeds.emplace_back(0, z);
                }
            }
            break;
        case ChunkEdge::PositiveX:
            // Neighbor is on our +X side, their -X edge connects to our x=width-1
            for (int z = 0; z < m_Depth && z < static_cast<int>(neighborEdge.size()); z++)
            {
                if (neighborEdge[z] && m_BelowSeaMask[CellIndex(m_Width - 1, z)] && !m_OceanMask[CellIndex(m_Width - 1, z)])
                {
                    seeds.emplace_back(m_Width - 1, z);
                }
            }
            break;
        case ChunkEdge::NegativeZ:
            // Neighbor is on our -Z side, their +Z edge connects to our z=0
            for (int x = 0; x < m_Width && x < static_cast<int>(neighborEdge.size()); x++)
            {
                if (neighborEdge[x] && m_BelowSeaMask[CellIndex(x, 0)] && !m_OceanMask[CellIndex(x, 0)])
                {
                    seeds.emplace_back(x, 0);
                }
            }
            break;
        case ChunkEdge::PositiveZ:
            // Neighbor is on our +Z side, their -Z edge connects to our z=depth-1
            for (int x = 0; x < m_Width && x < static_cast<int>(neighborEdge.size()); x++)
            {
                if (neighborEdge[x] && m_BelowSeaMask[CellIndex(x, m_Depth - 1)] && !m_OceanMask[CellIndex(x, m_Depth - 1)])
                {
                    seeds.emplace_back(x, m_Depth - 1);
                }
            }
            break;
        }

        if (!seeds.empty())
        {
            FloodFillBFS(seeds);
            UpdateBoundaryFromMask();
        }
    }

    void OceanMask::FloodFillBFS(const std::vector<std::pair<int, int>> &seeds)
    {
        if (seeds.empty())
            return;

        std::queue<std::pair<int, int>> queue;

        // Initialize queue with seeds
        for (const auto &seed : seeds)
        {
            int idx = CellIndex(seed.first, seed.second);
            if (!m_OceanMask[idx] && m_BelowSeaMask[idx])
            {
                m_OceanMask[idx] = true;
                queue.push(seed);
            }
        }

        // 4-directional neighbors
        const int dx[] = {-1, 1, 0, 0};
        const int dz[] = {0, 0, -1, 1};

        while (!queue.empty())
        {
            auto [x, z] = queue.front();
            queue.pop();

            for (int dir = 0; dir < 4; dir++)
            {
                int nx = x + dx[dir];
                int nz = z + dz[dir];

                if (IsValidCell(nx, nz))
                {
                    int nidx = CellIndex(nx, nz);
                    if (m_BelowSeaMask[nidx] && !m_OceanMask[nidx])
                    {
                        m_OceanMask[nidx] = true;
                        queue.emplace(nx, nz);
                    }
                }
            }
        }
    }

    void OceanMask::UpdateBoundaryFromMask()
    {
        // Update boundary arrays from current ocean mask
        for (int z = 0; z < m_Depth; z++)
        {
            if (z < static_cast<int>(m_Boundary.negX.size()))
            {
                m_Boundary.negX[z] = m_OceanMask[CellIndex(0, z)];
            }
            if (z < static_cast<int>(m_Boundary.posX.size()))
            {
                m_Boundary.posX[z] = m_OceanMask[CellIndex(m_Width - 1, z)];
            }
        }

        for (int x = 0; x < m_Width; x++)
        {
            if (x < static_cast<int>(m_Boundary.negZ.size()))
            {
                m_Boundary.negZ[x] = m_OceanMask[CellIndex(x, 0)];
            }
            if (x < static_cast<int>(m_Boundary.posZ.size()))
            {
                m_Boundary.posZ[x] = m_OceanMask[CellIndex(x, m_Depth - 1)];
            }
        }
    }

    bool OceanMask::IsOcean(int x, int z) const
    {
        if (!IsValidCell(x, z))
            return false;
        return m_OceanMask[CellIndex(x, z)];
    }

    bool OceanMask::IsBelowSeaLevel(int x, int z) const
    {
        if (!IsValidCell(x, z))
            return false;
        return m_BelowSeaMask[CellIndex(x, z)];
    }

    bool OceanMask::IsInlandLake(int x, int z) const
    {
        if (!IsValidCell(x, z))
            return false;
        int idx = CellIndex(x, z);
        return m_BelowSeaMask[idx] && !m_OceanMask[idx];
    }

} // namespace Genesis
