#pragma once

#include "genesis/procedural/DrainageGraph.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace Genesis
{

    /**
     * Represents a detected lake basin.
     */
    struct LakeBasin
    {
        std::vector<glm::ivec2> cells; // All cells in the basin
        glm::ivec2 lowestCell;         // Deepest point in the basin
        glm::ivec2 spillPoint;         // Lowest surrounding saddle
        float basinFloor;              // Height of lowest cell
        float spillHeight;             // Height of spill point
        float surfaceHeight;           // Water surface height (= spillHeight)
        float volume;                  // Approximate volume (cells * depth)
        bool hasOutflow;               // True if lake has a spill point
        int outflowDirection;          // Direction to outflow cell (-1 if none)
    };

    /**
     * Settings for lake generation.
     */
    struct LakeSettings
    {
        // Minimum cells to form a lake (avoid tiny puddles)
        int minBasinSize = 50;

        // Minimum depth difference between basin floor and spill point (world units)
        float minBasinDepth = 1.0f;

        // Maximum lake depth (for visual/gameplay limits)
        float maxDepth = 50.0f;

        // Bed flattening factor (0 = no change, 1 = perfectly flat)
        float bedFlatness = 0.7f;

        // Shoreline smoothing radius (in cells)
        int shorelineRadius = 2;

        // Shoreline blend factor (smoothing strength)
        float shorelineBlend = 0.5f;

        // Outflow channel depth multiplier
        float outflowDepth = 0.3f;

        // Outflow channel width (in cells)
        int outflowWidth = 1;
    };

    /**
     * Lake network data structure.
     */
    struct LakeNetwork
    {
        std::vector<LakeBasin> lakes;     // All detected lakes
        std::vector<int> cellLakeIndex;   // Lake index per cell (-1 if not lake)
        std::vector<float> cellLakeDepth; // Water depth per cell (0 if not lake)
        std::vector<bool> isLakeSurface;  // True if cell is underwater

        int width = 0;
        int depth = 0;

        void Resize(int w, int d)
        {
            width = w;
            depth = d;
            size_t size = static_cast<size_t>(w) * d;
            cellLakeIndex.resize(size, -1);
            cellLakeDepth.resize(size, 0.0f);
            isLakeSurface.resize(size, false);
        }

        void Clear()
        {
            lakes.clear();
            std::fill(cellLakeIndex.begin(), cellLakeIndex.end(), -1);
            std::fill(cellLakeDepth.begin(), cellLakeDepth.end(), 0.0f);
            std::fill(isLakeSurface.begin(), isLakeSurface.end(), false);
        }

        size_t Index(int x, int z) const { return static_cast<size_t>(z) * width + x; }
        bool InBounds(int x, int z) const { return x >= 0 && x < width && z >= 0 && z < depth; }
    };

    /**
     * Generates lakes from terrain depressions (closed basins).
     *
     * Section 22: Lakes (Closed Basins)
     *
     * A lake forms when:
     *   - Cell is below sea level, OR
     *   - Cell has no downhill flow path to ocean
     *
     * Lakes are filled to the lowest spill point.
     */
    class LakeGenerator
    {
    public:
        LakeGenerator() = default;
        ~LakeGenerator() = default;

        /**
         * Configure lake generation settings.
         */
        void SetSettings(const LakeSettings &settings) { m_Settings = settings; }
        const LakeSettings &GetSettings() const { return m_Settings; }

        /**
         * Generate lakes from drainage and heightmap data.
         *
         * @param drainage   Computed drainage graph
         * @param heightmap  Terrain heights
         * @param seaLevel   Sea level height
         */
        void Generate(const DrainageGraph &drainage,
                      const std::vector<float> &heightmap,
                      float seaLevel);

        /**
         * Apply lake modifications to heightmap.
         *   - Flatten lake beds
         *   - Smooth shorelines
         *   - Carve outflows
         *
         * @param heightmap  Terrain to modify (in-place)
         * @param cellSize   World units per cell
         */
        void ApplyLakes(std::vector<float> &heightmap, float cellSize) const;

        /**
         * Get the lake network data.
         */
        const LakeNetwork &GetNetwork() const { return m_Network; }

        /**
         * Get lake index at a cell (-1 if not a lake).
         */
        int GetLakeIndex(int x, int z) const;

        /**
         * Get water depth at a cell (0 if not in a lake).
         */
        float GetWaterDepth(int x, int z) const;

        /**
         * Get lake surface height at a cell.
         * Returns 0 if not in a lake.
         */
        float GetSurfaceHeight(int x, int z) const;

        /**
         * Check if a cell is part of a lake.
         */
        bool IsLake(int x, int z) const;

        /**
         * Get the basin containing a specific cell.
         * Returns nullptr if cell is not in a lake.
         */
        const LakeBasin *GetBasinAt(int x, int z) const;

    private:
        /**
         * Section 22.1: Basin Detection
         * Trace flow graph - if no ocean reachability → basin
         */
        void DetectBasins(const DrainageGraph &drainage,
                          const std::vector<float> &heightmap,
                          float seaLevel);

        /**
         * Flood-fill from a pit cell to find connected basin cells.
         */
        LakeBasin FloodFillBasin(const DrainageGraph &drainage,
                                 const std::vector<float> &heightmap,
                                 int startX, int startZ,
                                 float fillHeight,
                                 std::vector<bool> &visited) const;

        /**
         * Section 22.2: Lake Fill
         * Find lowest surrounding saddle and fill basin to that height.
         */
        void FillLakes(const std::vector<float> &heightmap);

        /**
         * Find the lowest spill point around a basin.
         */
        bool FindSpillPoint(const LakeBasin &basin,
                            const std::vector<float> &heightmap,
                            glm::ivec2 &outSpillPoint,
                            float &outSpillHeight,
                            int &outDirection) const;

        /**
         * Section 22.3: Lake Terrain Adjustment
         *   - Lake bed flattened
         *   - Shorelines smoothed
         *   - Outflow carved if spill exists
         */
        void FlattenLakeBed(std::vector<float> &heightmap,
                            const LakeBasin &basin) const;

        void SmoothShorelines(std::vector<float> &heightmap,
                              const LakeBasin &basin,
                              float cellSize) const;

        void CarveOutflow(std::vector<float> &heightmap,
                          const LakeBasin &basin,
                          float cellSize) const;

        /**
         * Check if a cell can reach the ocean via flow.
         */
        bool CanReachOcean(const DrainageGraph &drainage, int x, int z) const;

        LakeSettings m_Settings;
        LakeNetwork m_Network;
    };

}
