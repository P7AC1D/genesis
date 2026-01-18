#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <queue>
#include <unordered_set>

namespace Genesis
{

    /**
     * OceanMask handles the distinction between oceans and inland lakes.
     *
     * Ocean cells are defined as:
     *   isBelowSea = height < seaLevel;
     *   isOcean = isBelowSea && oceanConnected;
     *
     * Where oceanConnected is determined via flood fill from world bounds.
     * This prevents inland seas from being misclassified as oceans.
     */

    // Edge connectivity flags for cross-chunk flood fill propagation
    enum class ChunkEdge : uint8_t
    {
        NegativeX = 0, // -X edge (west)
        PositiveX = 1, // +X edge (east)
        NegativeZ = 2, // -Z edge (north)
        PositiveZ = 3  // +Z edge (south)
    };

    // Stores which edges of a chunk have ocean connectivity
    struct ChunkOceanBoundary
    {
        std::vector<bool> negX; // Ocean connectivity along -X edge
        std::vector<bool> posX; // Ocean connectivity along +X edge
        std::vector<bool> negZ; // Ocean connectivity along -Z edge
        std::vector<bool> posZ; // Ocean connectivity along +Z edge

        void Resize(int size)
        {
            negX.resize(size, false);
            posX.resize(size, false);
            negZ.resize(size, false);
            posZ.resize(size, false);
        }

        void Clear()
        {
            std::fill(negX.begin(), negX.end(), false);
            std::fill(posX.begin(), posX.end(), false);
            std::fill(negZ.begin(), negZ.end(), false);
            std::fill(posZ.begin(), posZ.end(), false);
        }
    };

    class OceanMask
    {
    public:
        OceanMask() = default;
        ~OceanMask() = default;

        /**
         * Initialize the ocean mask for a chunk
         * @param width Grid width (cells)
         * @param depth Grid depth (cells)
         */
        void Initialize(int width, int depth);

        /**
         * Generate the below-sea-level mask from heightmap data
         * @param heightmap Terrain heights (row-major, width * depth)
         * @param seaLevel Global sea level height
         */
        void GenerateBelowSeaMask(const std::vector<float> &heightmap, float seaLevel);

        /**
         * Perform flood fill from world bounds to determine ocean connectivity.
         * Call this for chunks at world boundaries, or propagate from neighbors.
         *
         * @param isAtWorldBoundary Function that returns true if a given edge is at world boundary
         * @param neighborBoundary Optional boundary data from neighboring chunks for propagation
         */
        void FloodFillFromBoundary(
            const std::function<bool(ChunkEdge)> &isAtWorldBoundary,
            const ChunkOceanBoundary *neighborBoundary = nullptr);

        /**
         * Propagate ocean connectivity from a neighboring chunk's edge
         * @param edge Which edge the neighbor is on
         * @param neighborEdge The neighbor's edge connectivity data
         */
        void PropagateFromNeighbor(ChunkEdge edge, const std::vector<bool> &neighborEdge);

        /**
         * Get the current boundary connectivity for cross-chunk propagation
         */
        const ChunkOceanBoundary &GetBoundary() const { return m_Boundary; }

        /**
         * Check if a cell is ocean (below sea level AND connected to world boundary)
         * @param x Cell X coordinate
         * @param z Cell Z coordinate
         */
        bool IsOcean(int x, int z) const;

        /**
         * Check if a cell is below sea level (may be ocean or inland lake)
         * @param x Cell X coordinate
         * @param z Cell Z coordinate
         */
        bool IsBelowSeaLevel(int x, int z) const;

        /**
         * Check if a cell is an inland lake (below sea level but NOT connected to ocean)
         * @param x Cell X coordinate
         * @param z Cell Z coordinate
         */
        bool IsInlandLake(int x, int z) const;

        /**
         * Get the full ocean mask array
         */
        const std::vector<bool> &GetOceanMask() const { return m_OceanMask; }

        /**
         * Get the below-sea-level mask array
         */
        const std::vector<bool> &GetBelowSeaMask() const { return m_BelowSeaMask; }

        /**
         * Check if flood fill has been performed
         */
        bool IsFloodFillComplete() const { return m_FloodFillComplete; }

        /**
         * Mark that this chunk needs flood fill recomputation
         */
        void MarkDirty() { m_FloodFillComplete = false; }

        int GetWidth() const { return m_Width; }
        int GetDepth() const { return m_Depth; }

    private:
        int CellIndex(int x, int z) const { return z * m_Width + x; }
        bool IsValidCell(int x, int z) const
        {
            return x >= 0 && x < m_Width && z >= 0 && z < m_Depth;
        }

        void FloodFillBFS(const std::vector<std::pair<int, int>> &seeds);
        void UpdateBoundaryFromMask();

        int m_Width = 0;
        int m_Depth = 0;

        std::vector<bool> m_BelowSeaMask; // Cells below sea level
        std::vector<bool> m_OceanMask;    // Cells that are ocean (connected to boundary)

        ChunkOceanBoundary m_Boundary; // Edge connectivity for cross-chunk propagation
        bool m_FloodFillComplete = false;
    };

    /**
     * Utility functions for global sea level calculation
     */
    namespace SeaLevelUtils
    {

        /**
         * Calculate absolute sea level from normalized value
         * seaLevel = baseHeight + heightScale * seaLevelNormalized
         *
         * @param baseHeight Terrain base height offset
         * @param heightScale Terrain height scale multiplier
         * @param seaLevelNormalized Normalized sea level (0.0 - 1.0, typical: 0.45)
         * @return Absolute sea level in world units
         */
        inline float CalculateSeaLevel(float baseHeight, float heightScale, float seaLevelNormalized)
        {
            return baseHeight + heightScale * seaLevelNormalized;
        }

        /**
         * Calculate normalized sea level from absolute value
         * Inverse of CalculateSeaLevel
         */
        inline float CalculateNormalizedSeaLevel(float baseHeight, float heightScale, float absoluteSeaLevel)
        {
            if (heightScale <= 0.0f)
                return 0.45f;
            return (absoluteSeaLevel - baseHeight) / heightScale;
        }
    }

} // namespace Genesis
