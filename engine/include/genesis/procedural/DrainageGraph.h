#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace Genesis
{

    /**
     * DrainageGraph computes water flow topology from terrain heightmaps.
     *
     * This is the foundation for:
     *   - River generation
     *   - Lake detection
     *   - Erosion simulation
     *   - Biome moisture calculation
     *
     * The system computes two primary fields:
     *   1. Flow Direction - which neighbor each cell drains to
     *   2. Flow Accumulation - how many upstream cells contribute to each cell
     */

    // Flow direction encoded as neighbor index (0-7) or special values
    enum class FlowDirection : uint8_t
    {
        // 8-connected neighbors (clockwise from east)
        East = 0,      // +X
        SouthEast = 1, // +X, +Z
        South = 2,     // +Z
        SouthWest = 3, // -X, +Z
        West = 4,      // -X
        NorthWest = 5, // -X, -Z
        North = 6,     // -Z
        NorthEast = 7, // +X, -Z

        // Special values
        Pit = 8,       // Local minimum (no outflow)
        Flat = 9,      // All neighbors same height
        Boundary = 10, // At chunk/world edge
        Ocean = 11     // Below sea level, drains to ocean
    };

    // Offset table for 8-connected neighbors
    constexpr int FLOW_OFFSET_X[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    constexpr int FLOW_OFFSET_Z[8] = {0, 1, 1, 1, 0, -1, -1, -1};

    // Distance weights for diagonal vs cardinal neighbors (sqrt(2) for diagonals)
    constexpr float FLOW_DISTANCE[8] = {1.0f, 1.414f, 1.0f, 1.414f, 1.0f, 1.414f, 1.0f, 1.414f};

    /**
     * Stores drainage computation results for a terrain grid.
     */
    struct DrainageData
    {
        int width = 0;
        int depth = 0;

        // Flow direction for each cell (width * depth)
        std::vector<FlowDirection> flowDirection;

        // Flow accumulation for each cell (upstream cell count)
        std::vector<uint32_t> flowAccumulation;

        // Slope magnitude at each cell (for erosion calculations)
        std::vector<float> slope;

        // Whether each cell is part of a lake/depression
        std::vector<bool> isLake;

        void Resize(int w, int d)
        {
            width = w;
            depth = d;
            size_t size = static_cast<size_t>(w) * d;
            flowDirection.resize(size, FlowDirection::Pit);
            flowAccumulation.resize(size, 0);
            slope.resize(size, 0.0f);
            isLake.resize(size, false);
        }

        void Clear()
        {
            std::fill(flowDirection.begin(), flowDirection.end(), FlowDirection::Pit);
            std::fill(flowAccumulation.begin(), flowAccumulation.end(), 0);
            std::fill(slope.begin(), slope.end(), 0.0f);
            std::fill(isLake.begin(), isLake.end(), false);
        }

        size_t Index(int x, int z) const { return static_cast<size_t>(z) * width + x; }
        bool InBounds(int x, int z) const { return x >= 0 && x < width && z >= 0 && z < depth; }
    };

    /**
     * Computes drainage topology from heightmap data.
     */
    class DrainageGraph
    {
    public:
        DrainageGraph() = default;
        ~DrainageGraph() = default;

        /**
         * Compute full drainage graph from heightmap.
         *
         * @param heightmap  Terrain heights (row-major, width * depth)
         * @param width      Grid width
         * @param depth      Grid depth
         * @param cellSize   World units per cell (for slope calculation)
         * @param seaLevel   Sea level height (cells below drain to ocean)
         */
        void Compute(const std::vector<float> &heightmap,
                     int width, int depth,
                     float cellSize,
                     float seaLevel);

        /**
         * Compute only flow directions (faster, for erosion only).
         */
        void ComputeFlowDirections(const std::vector<float> &heightmap,
                                   int width, int depth,
                                   float cellSize,
                                   float seaLevel);

        /**
         * Compute flow accumulation from existing flow directions.
         * Must call ComputeFlowDirections first.
         */
        void ComputeFlowAccumulation();

        /**
         * Get the drainage data.
         */
        const DrainageData &GetData() const { return m_Data; }
        DrainageData &GetData() { return m_Data; }

        /**
         * Get flow direction at a cell.
         */
        FlowDirection GetFlowDirection(int x, int z) const;

        /**
         * Get flow accumulation at a cell.
         */
        uint32_t GetFlowAccumulation(int x, int z) const;

        /**
         * Get slope at a cell.
         */
        float GetSlope(int x, int z) const;

        /**
         * Get the downstream neighbor coordinates.
         * Returns false if cell is a pit/boundary/ocean.
         */
        bool GetDownstreamCell(int x, int z, int &outX, int &outZ) const;

        /**
         * Trace flow path from a cell to its terminus (pit, boundary, or ocean).
         * Returns the path as a list of cell indices.
         */
        std::vector<size_t> TraceFlowPath(int startX, int startZ) const;

        /**
         * Find all cells with flow accumulation above threshold.
         * These are potential river cells.
         */
        std::vector<glm::ivec2> FindRiverCells(uint32_t minAccumulation) const;

        /**
         * Find all pit cells (local minima that form lakes).
         */
        std::vector<glm::ivec2> FindPits() const;

    private:
        // Compute flow direction for a single cell using steepest descent
        FlowDirection ComputeCellFlowDirection(const std::vector<float> &heightmap,
                                               int x, int z,
                                               float seaLevel) const;

        // Compute slope magnitude for a cell
        float ComputeCellSlope(const std::vector<float> &heightmap,
                               int x, int z,
                               float cellSize) const;

        DrainageData m_Data;
    };

}
