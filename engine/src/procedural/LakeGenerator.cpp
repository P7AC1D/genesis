#include "genesis/procedural/LakeGenerator.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_set>

namespace Genesis
{

    void LakeGenerator::Generate(const DrainageGraph &drainage,
                                 const std::vector<float> &heightmap,
                                 float seaLevel)
    {
        const DrainageData &data = drainage.GetData();
        m_Network.Resize(data.width, data.depth);
        m_Network.Clear();

        // Section 22.1: Basin Detection
        DetectBasins(drainage, heightmap, seaLevel);

        // Section 22.2: Lake Fill
        FillLakes(heightmap);
    }

    void LakeGenerator::DetectBasins(const DrainageGraph &drainage,
                                     const std::vector<float> &heightmap,
                                     float seaLevel)
    {
        const DrainageData &data = drainage.GetData();
        std::vector<bool> visited(static_cast<size_t>(data.width) * data.depth, false);

        // Find all pit cells (local minima)
        std::vector<glm::ivec2> pits = drainage.FindPits();

        for (const auto &pit : pits)
        {
            size_t pitIdx = static_cast<size_t>(pit.y) * data.width + pit.x;

            // Skip if already visited or below sea level (ocean)
            if (visited[pitIdx])
                continue;

            float pitHeight = heightmap[pitIdx];
            if (pitHeight < seaLevel)
            {
                // Below sea level - mark as visited but don't create lake
                visited[pitIdx] = true;
                continue;
            }

            // Section 22.1: Check if pit can reach ocean
            // If no ocean reachability → basin
            if (!CanReachOcean(drainage, pit.x, pit.y))
            {
                // This pit forms a closed basin - flood fill to find extent
                // Start with a high fill height and refine later
                float maxFillHeight = pitHeight + m_Settings.maxDepth;
                LakeBasin basin = FloodFillBasin(drainage, heightmap,
                                                 pit.x, pit.y,
                                                 maxFillHeight, visited);

                // Only keep basins above minimum size
                if (static_cast<int>(basin.cells.size()) >= m_Settings.minBasinSize)
                {
                    int lakeIndex = static_cast<int>(m_Network.lakes.size());

                    // Mark cells as belonging to this lake
                    for (const auto &cell : basin.cells)
                    {
                        size_t idx = m_Network.Index(cell.x, cell.y);
                        m_Network.cellLakeIndex[idx] = lakeIndex;
                    }

                    m_Network.lakes.push_back(basin);
                }
            }
            else
            {
                visited[pitIdx] = true;
            }
        }
    }

    bool LakeGenerator::CanReachOcean(const DrainageGraph &drainage, int x, int z) const
    {
        const DrainageData &data = drainage.GetData();
        const int maxIterations = data.width * data.depth; // Prevent infinite loops

        int currentX = x;
        int currentZ = z;

        for (int i = 0; i < maxIterations; i++)
        {
            FlowDirection dir = drainage.GetFlowDirection(currentX, currentZ);

            // Reached ocean or boundary - can reach ocean
            if (dir == FlowDirection::Ocean || dir == FlowDirection::Boundary)
            {
                return true;
            }

            // Reached pit or flat - cannot reach ocean
            if (dir == FlowDirection::Pit || dir == FlowDirection::Flat)
            {
                return false;
            }

            // Follow flow direction
            int nextX, nextZ;
            if (!drainage.GetDownstreamCell(currentX, currentZ, nextX, nextZ))
            {
                return false; // No downstream cell
            }

            currentX = nextX;
            currentZ = nextZ;
        }

        return false; // Exceeded iteration limit
    }

    LakeBasin LakeGenerator::FloodFillBasin(const DrainageGraph &drainage,
                                            const std::vector<float> &heightmap,
                                            int startX, int startZ,
                                            float fillHeight,
                                            std::vector<bool> &visited) const
    {
        const DrainageData &data = drainage.GetData();
        LakeBasin basin;
        basin.lowestCell = glm::ivec2(startX, startZ);
        basin.basinFloor = heightmap[static_cast<size_t>(startZ) * data.width + startX];
        basin.spillPoint = glm::ivec2(-1, -1);
        basin.spillHeight = fillHeight;
        basin.hasOutflow = false;
        basin.outflowDirection = -1;
        basin.volume = 0.0f;

        // Priority queue for filling (lowest cells first)
        // pair<height, ivec2>
        auto cmp = [](const std::pair<float, glm::ivec2> &a,
                      const std::pair<float, glm::ivec2> &b)
        {
            return a.first > b.first; // Min heap
        };
        std::priority_queue<std::pair<float, glm::ivec2>,
                            std::vector<std::pair<float, glm::ivec2>>,
                            decltype(cmp)>
            frontier(cmp);

        // Start from the pit
        size_t startIdx = static_cast<size_t>(startZ) * data.width + startX;
        visited[startIdx] = true;
        basin.cells.push_back(glm::ivec2(startX, startZ));

        // Add all neighbors to frontier
        for (int d = 0; d < 8; d++)
        {
            int nx = startX + FLOW_OFFSET_X[d];
            int nz = startZ + FLOW_OFFSET_Z[d];

            if (!data.InBounds(nx, nz))
                continue;

            size_t nIdx = static_cast<size_t>(nz) * data.width + nx;
            if (!visited[nIdx])
            {
                float h = heightmap[nIdx];
                frontier.push({h, glm::ivec2(nx, nz)});
            }
        }

        // Flood fill - expand basin to lowest surrounding saddle
        while (!frontier.empty())
        {
            auto [height, cell] = frontier.top();
            frontier.pop();

            size_t idx = static_cast<size_t>(cell.y) * data.width + cell.x;
            if (visited[idx])
                continue;

            // Check if this cell flows into our basin
            // If it drains elsewhere, this is a potential spill point
            FlowDirection dir = drainage.GetFlowDirection(cell.x, cell.y);

            // If this cell is higher than our current spill height, stop
            if (height > basin.spillHeight)
            {
                continue;
            }

            // Check if this cell drains into the basin or is a boundary
            int downX, downZ;
            bool hasDownstream = drainage.GetDownstreamCell(cell.x, cell.y, downX, downZ);

            if (hasDownstream)
            {
                size_t downIdx = static_cast<size_t>(downZ) * data.width + downX;
                bool drainsIntoBasis = visited[downIdx];

                if (!drainsIntoBasis)
                {
                    // This cell drains outside - potential spill point
                    if (height < basin.spillHeight)
                    {
                        basin.spillHeight = height;
                        basin.spillPoint = cell;
                        basin.hasOutflow = true;

                        // Find outflow direction
                        for (int d = 0; d < 8; d++)
                        {
                            if (downX == cell.x + FLOW_OFFSET_X[d] &&
                                downZ == cell.y + FLOW_OFFSET_Z[d])
                            {
                                basin.outflowDirection = d;
                                break;
                            }
                        }
                    }
                    continue; // Don't add to basin
                }
            }

            // Add cell to basin
            visited[idx] = true;
            basin.cells.push_back(cell);

            // Track lowest point
            if (height < basin.basinFloor)
            {
                basin.basinFloor = height;
                basin.lowestCell = cell;
            }

            // Add neighbors to frontier
            for (int d = 0; d < 8; d++)
            {
                int nx = cell.x + FLOW_OFFSET_X[d];
                int nz = cell.y + FLOW_OFFSET_Z[d];

                if (!data.InBounds(nx, nz))
                    continue;

                size_t nIdx = static_cast<size_t>(nz) * data.width + nx;
                if (!visited[nIdx])
                {
                    float h = heightmap[nIdx];
                    frontier.push({h, glm::ivec2(nx, nz)});
                }
            }
        }

        return basin;
    }

    void LakeGenerator::FillLakes(const std::vector<float> &heightmap)
    {
        // Section 22.2: Lake Fill
        // Lakes are filled to the lowest spill point
        // Lake surface becomes flat water plane

        for (size_t i = 0; i < m_Network.lakes.size(); i++)
        {
            LakeBasin &basin = m_Network.lakes[i];

            // Find actual spill point if not already found
            if (!basin.hasOutflow)
            {
                glm::ivec2 spill;
                float spillH;
                int spillDir;
                if (FindSpillPoint(basin, heightmap, spill, spillH, spillDir))
                {
                    basin.spillPoint = spill;
                    basin.spillHeight = spillH;
                    basin.hasOutflow = true;
                    basin.outflowDirection = spillDir;
                }
                else
                {
                    // No spill point found - fill to max depth
                    basin.spillHeight = basin.basinFloor + m_Settings.maxDepth;
                }
            }

            // Set surface height
            basin.surfaceHeight = basin.spillHeight;

            // Calculate volume and water depth per cell
            basin.volume = 0.0f;
            for (const auto &cell : basin.cells)
            {
                size_t idx = m_Network.Index(cell.x, cell.y);
                float terrainH = heightmap[idx];

                if (terrainH < basin.surfaceHeight)
                {
                    float depth = basin.surfaceHeight - terrainH;
                    m_Network.cellLakeDepth[idx] = depth;
                    m_Network.isLakeSurface[idx] = true;
                    basin.volume += depth;
                }
            }
        }
    }

    bool LakeGenerator::FindSpillPoint(const LakeBasin &basin,
                                       const std::vector<float> &heightmap,
                                       glm::ivec2 &outSpillPoint,
                                       float &outSpillHeight,
                                       int &outDirection) const
    {
        // Find the lowest cell on the basin boundary that leads outside
        float lowestSpill = std::numeric_limits<float>::max();
        bool found = false;

        // Build set of basin cells for quick lookup
        std::unordered_set<int64_t> basinCells;
        for (const auto &cell : basin.cells)
        {
            int64_t key = (static_cast<int64_t>(cell.y) << 32) | cell.x;
            basinCells.insert(key);
        }

        // Check all boundary cells
        for (const auto &cell : basin.cells)
        {
            for (int d = 0; d < 8; d++)
            {
                int nx = cell.x + FLOW_OFFSET_X[d];
                int nz = cell.y + FLOW_OFFSET_Z[d];

                if (!m_Network.InBounds(nx, nz))
                    continue;

                int64_t nKey = (static_cast<int64_t>(nz) << 32) | nx;
                if (basinCells.find(nKey) != basinCells.end())
                    continue; // Neighbor is in basin

                // This neighbor is outside the basin
                size_t idx = m_Network.Index(cell.x, cell.y);
                float cellHeight = heightmap[idx];

                // Spill height is the max of cell height and neighbor height
                size_t nIdx = m_Network.Index(nx, nz);
                float neighborHeight = heightmap[nIdx];
                float spillHeight = std::max(cellHeight, neighborHeight);

                if (spillHeight < lowestSpill)
                {
                    lowestSpill = spillHeight;
                    outSpillPoint = cell;
                    outSpillHeight = spillHeight;
                    outDirection = d;
                    found = true;
                }
            }
        }

        return found;
    }

    void LakeGenerator::ApplyLakes(std::vector<float> &heightmap, float cellSize) const
    {
        // Section 22.3: Lake Terrain Adjustment
        for (const auto &basin : m_Network.lakes)
        {
            // Lake bed flattened
            FlattenLakeBed(heightmap, basin);

            // Shorelines smoothed
            SmoothShorelines(heightmap, basin, cellSize);

            // Outflow carved if spill exists
            if (basin.hasOutflow)
            {
                CarveOutflow(heightmap, basin, cellSize);
            }
        }
    }

    void LakeGenerator::FlattenLakeBed(std::vector<float> &heightmap,
                                       const LakeBasin &basin) const
    {
        // Blend heights toward a flatter bed
        float targetBed = basin.basinFloor;

        for (const auto &cell : basin.cells)
        {
            size_t idx = m_Network.Index(cell.x, cell.y);
            float currentH = heightmap[idx];

            // Only flatten cells below water surface
            if (currentH < basin.surfaceHeight)
            {
                // Blend toward basin floor based on flatness setting
                float newH = currentH + (targetBed - currentH) * m_Settings.bedFlatness;
                heightmap[idx] = newH;
            }
        }
    }

    void LakeGenerator::SmoothShorelines(std::vector<float> &heightmap,
                                         const LakeBasin &basin,
                                         float cellSize) const
    {
        // Build set of basin cells
        std::unordered_set<int64_t> basinCells;
        for (const auto &cell : basin.cells)
        {
            int64_t key = (static_cast<int64_t>(cell.y) << 32) | cell.x;
            basinCells.insert(key);
        }

        int radius = m_Settings.shorelineRadius;

        // Find and smooth shoreline cells
        for (const auto &cell : basin.cells)
        {
            bool isShore = false;

            // Check if this is a shoreline cell (adjacent to non-basin)
            for (int d = 0; d < 8; d++)
            {
                int nx = cell.x + FLOW_OFFSET_X[d];
                int nz = cell.y + FLOW_OFFSET_Z[d];

                if (!m_Network.InBounds(nx, nz))
                {
                    isShore = true;
                    break;
                }

                int64_t nKey = (static_cast<int64_t>(nz) << 32) | nx;
                if (basinCells.find(nKey) == basinCells.end())
                {
                    isShore = true;
                    break;
                }
            }

            if (!isShore)
                continue;

            // Smooth cells near this shoreline
            for (int dz = -radius; dz <= radius; dz++)
            {
                for (int dx = -radius; dx <= radius; dx++)
                {
                    int nx = cell.x + dx;
                    int nz = cell.y + dz;

                    if (!m_Network.InBounds(nx, nz))
                        continue;

                    int64_t nKey = (static_cast<int64_t>(nz) << 32) | nx;
                    if (basinCells.find(nKey) != basinCells.end())
                        continue; // Skip cells in basin

                    size_t nIdx = m_Network.Index(nx, nz);
                    float dist = std::sqrt(static_cast<float>(dx * dx + dz * dz));

                    if (dist <= radius)
                    {
                        // Smoothstep blend from shore to terrain
                        float t = dist / static_cast<float>(radius);
                        t = t * t * (3.0f - 2.0f * t); // Smoothstep

                        // Blend toward water surface height
                        float currentH = heightmap[nIdx];
                        float targetH = basin.surfaceHeight;
                        float blendedH = targetH + (currentH - targetH) * t;

                        // Only lower terrain, don't raise
                        if (blendedH < currentH)
                        {
                            heightmap[nIdx] = currentH * (1.0f - m_Settings.shorelineBlend) +
                                              blendedH * m_Settings.shorelineBlend;
                        }
                    }
                }
            }
        }
    }

    void LakeGenerator::CarveOutflow(std::vector<float> &heightmap,
                                     const LakeBasin &basin,
                                     float cellSize) const
    {
        if (!basin.hasOutflow || basin.outflowDirection < 0)
            return;

        // Carve a channel from spill point downstream
        int x = basin.spillPoint.x;
        int z = basin.spillPoint.y;
        int dir = basin.outflowDirection;

        // Target depth below spill height
        float channelFloor = basin.spillHeight - basin.spillHeight * m_Settings.outflowDepth;

        // Carve several cells in outflow direction
        int carveLength = 5; // Cells to carve
        int width = m_Settings.outflowWidth;

        for (int i = 0; i < carveLength; i++)
        {
            // Move in outflow direction
            int cx = x + FLOW_OFFSET_X[dir] * i;
            int cz = z + FLOW_OFFSET_Z[dir] * i;

            if (!m_Network.InBounds(cx, cz))
                break;

            // Carve center and width
            for (int w = -width; w <= width; w++)
            {
                // Perpendicular direction
                int perpDir = (dir + 2) % 8;
                int wx = cx + FLOW_OFFSET_X[perpDir] * w;
                int wz = cz + FLOW_OFFSET_Z[perpDir] * w;

                if (!m_Network.InBounds(wx, wz))
                    continue;

                size_t idx = m_Network.Index(wx, wz);
                float currentH = heightmap[idx];

                // Gradual deepening from spill point
                float t = static_cast<float>(i) / carveLength;
                float targetH = basin.spillHeight * (1.0f - t) + channelFloor * t;

                // Bank slope for cells away from center
                float bankFactor = 1.0f - std::abs(static_cast<float>(w)) / (width + 1);
                targetH = currentH + (targetH - currentH) * bankFactor;

                // Only lower terrain
                if (targetH < currentH)
                {
                    heightmap[idx] = targetH;
                }
            }
        }
    }

    int LakeGenerator::GetLakeIndex(int x, int z) const
    {
        if (!m_Network.InBounds(x, z))
            return -1;
        return m_Network.cellLakeIndex[m_Network.Index(x, z)];
    }

    float LakeGenerator::GetWaterDepth(int x, int z) const
    {
        if (!m_Network.InBounds(x, z))
            return 0.0f;
        return m_Network.cellLakeDepth[m_Network.Index(x, z)];
    }

    float LakeGenerator::GetSurfaceHeight(int x, int z) const
    {
        int lakeIdx = GetLakeIndex(x, z);
        if (lakeIdx < 0 || lakeIdx >= static_cast<int>(m_Network.lakes.size()))
            return 0.0f;
        return m_Network.lakes[lakeIdx].surfaceHeight;
    }

    bool LakeGenerator::IsLake(int x, int z) const
    {
        return GetLakeIndex(x, z) >= 0;
    }

    const LakeBasin *LakeGenerator::GetBasinAt(int x, int z) const
    {
        int idx = GetLakeIndex(x, z);
        if (idx < 0 || idx >= static_cast<int>(m_Network.lakes.size()))
            return nullptr;
        return &m_Network.lakes[idx];
    }

}
