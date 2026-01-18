#pragma once

#include "genesis/procedural/DrainageGraph.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace Genesis
{

    /**
     * Water body classification based on flow accumulation.
     */
    enum class WaterType : uint8_t
    {
        None = 0,   // No water
        Stream = 1, // Small water flow
        River = 2,  // Major water flow
        Lake = 3,   // Standing water in depression
        Ocean = 4   // Connected to world boundary below sea level
    };

    /**
     * Settings for river generation, derived from TerrainIntent.
     */
    struct RiverSettings
    {
        // Classification thresholds (scale with riverStrength)
        // if (flowAccum > majorRiverThreshold) waterType = River
        // else if (flowAccum > streamThreshold) waterType = Stream
        uint32_t streamThreshold = 50;      // Min flow accumulation for stream
        uint32_t majorRiverThreshold = 500; // Min flow accumulation for river

        // River geometry
        // riverWidth = sqrt(flowAccum) * riverWidthScale
        float riverWidthScale = 0.1f; // Width multiplier
        float minRiverWidth = 1.0f;   // Minimum river width in world units
        float maxRiverWidth = 20.0f;  // Maximum river width in world units

        // Terrain interaction
        float channelDepth = 2.0f; // How deep rivers carve into terrain
        float bankSlope = 0.5f;    // Slope of river banks (0 = vertical, 1 = 45°)
        float bedFlatness = 0.8f;  // How flat the riverbed is (0-1)

        // Erosion influence
        float erosionMultiplier = 2.0f; // Extra erosion along rivers
        float depositionRate = 0.3f;    // Sediment deposition on banks
    };

    /**
     * Represents a single river segment (cell in the river network).
     */
    struct RiverSegment
    {
        glm::ivec2 cell;     // Grid cell coordinates
        float width;         // River width at this point
        float depth;         // River depth at this point
        float surfaceHeight; // Water surface height
        WaterType type;      // Stream or River
        uint32_t flowAccum;  // Flow accumulation at this cell
        int downstreamIndex; // Index of downstream segment (-1 if terminus)
    };

    /**
     * Represents a complete river from source to terminus.
     */
    struct RiverPath
    {
        std::vector<size_t> segmentIndices; // Indices into RiverNetwork::segments
        glm::ivec2 source;                  // Starting cell
        glm::ivec2 terminus;                // Ending cell (ocean, lake, or pit)
        WaterType terminusType;             // What the river flows into
        float totalLength;                  // Path length in cells
        uint32_t maxAccumulation;           // Peak flow accumulation
    };

    /**
     * Complete river network for a terrain region.
     */
    struct RiverNetwork
    {
        std::vector<RiverSegment> segments;   // All river/stream segments
        std::vector<RiverPath> rivers;        // Individual river paths
        std::vector<WaterType> cellWaterType; // Water type per terrain cell
        std::vector<float> cellRiverWidth;    // River width per cell (0 if not river)
        std::vector<float> cellSurfaceHeight; // Water surface height per cell

        int width = 0;
        int depth = 0;

        void Resize(int w, int d)
        {
            width = w;
            depth = d;
            size_t size = static_cast<size_t>(w) * d;
            cellWaterType.resize(size, WaterType::None);
            cellRiverWidth.resize(size, 0.0f);
            cellSurfaceHeight.resize(size, 0.0f);
        }

        void Clear()
        {
            segments.clear();
            rivers.clear();
            std::fill(cellWaterType.begin(), cellWaterType.end(), WaterType::None);
            std::fill(cellRiverWidth.begin(), cellRiverWidth.end(), 0.0f);
            std::fill(cellSurfaceHeight.begin(), cellSurfaceHeight.end(), 0.0f);
        }

        size_t Index(int x, int z) const { return static_cast<size_t>(z) * width + x; }
        bool InBounds(int x, int z) const { return x >= 0 && x < width && z >= 0 && z < depth; }
    };

    /**
     * Generates rivers and streams from drainage data.
     */
    class RiverGenerator
    {
    public:
        RiverGenerator() = default;
        ~RiverGenerator() = default;

        /**
         * Configure river generation from intent parameters.
         * @param riverStrength  TerrainIntent::riverStrength [0,1]
         * @param cellSize       World units per cell
         */
        void Configure(float riverStrength, float cellSize);

        /**
         * Set river settings directly.
         */
        void SetSettings(const RiverSettings &settings) { m_Settings = settings; }
        const RiverSettings &GetSettings() const { return m_Settings; }

        /**
         * Generate river network from drainage graph.
         * @param drainage   Computed drainage data
         * @param heightmap  Terrain heights
         * @param seaLevel   Sea level for ocean detection
         */
        void Generate(const DrainageGraph &drainage,
                      const std::vector<float> &heightmap,
                      float seaLevel);

        /**
         * Apply river carving to heightmap.
         * Modifies terrain to create river channels.
         * Post-erosion: height = min(height, riverSurfaceHeight)
         * @param heightmap  Terrain to modify (in-place)
         * @param cellSize   World units per cell
         */
        void CarveRivers(std::vector<float> &heightmap, float cellSize) const;

        /**
         * Get the generated river network.
         */
        const RiverNetwork &GetNetwork() const { return m_Network; }

        /**
         * Get water type at a cell.
         */
        WaterType GetWaterType(int x, int z) const;

        /**
         * Get river width at a cell (0 if not a river).
         */
        float GetRiverWidth(int x, int z) const;

        /**
         * Get river surface height at a cell.
         * Returns 0 if not a river.
         */
        float GetRiverSurfaceHeight(int x, int z) const;

        /**
         * Check if a cell is part of any water body.
         */
        bool IsWater(int x, int z) const;

    private:
        // Classify cells based on flow accumulation
        void ClassifyCells(const DrainageGraph &drainage,
                           const std::vector<float> &heightmap,
                           float seaLevel);

        // Build river segments from classified cells
        void BuildSegments(const DrainageGraph &drainage,
                           const std::vector<float> &heightmap);

        // Trace individual river paths
        void TraceRiverPaths(const DrainageGraph &drainage);

        // Calculate river width from flow accumulation
        // riverWidth = sqrt(flowAccum) * riverWidthScale
        float CalculateRiverWidth(uint32_t flowAccumulation) const;

        // Calculate river depth from width
        float CalculateRiverDepth(float width) const;

        RiverSettings m_Settings;
        RiverNetwork m_Network;
        float m_CellSize = 1.0f;
    };

}
