#pragma once

#include "genesis/procedural/DrainageGraph.h"
#include "genesis/procedural/RiverGenerator.h"
#include "genesis/procedural/LakeGenerator.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace Genesis
{

    /**
     * Section 24: Hydrology Output Fields
     *
     * Unified hydrology data structure containing all water-related fields
     * computed from drainage, rivers, and lakes. These fields are cached
     * per chunk for runtime queries by biomes, vegetation, and rendering.
     */
    struct HydrologyData
    {
        // Field: WaterType - None / Ocean / Lake / River / Stream
        std::vector<WaterType> waterType;

        // Field: WaterSurfaceHeight - For rendering water planes
        std::vector<float> waterSurfaceHeight;

        // Field: FlowAccumulation - Used by biomes for moisture
        std::vector<uint32_t> flowAccumulation;

        // Field: DistanceToWater - Used by moisture calculation
        std::vector<float> distanceToWater;

        // Field: DrainageDirection - Debug / tools
        std::vector<FlowDirection> drainageDirection;

        // Field: Slope - Used for wetland detection
        std::vector<float> slope;

        // Field: Moisture - Derived from flow, distance, humidity
        std::vector<float> moisture;

        int width = 0;
        int depth = 0;

        void Resize(int w, int d)
        {
            width = w;
            depth = d;
            size_t size = static_cast<size_t>(w) * d;
            waterType.resize(size, WaterType::None);
            waterSurfaceHeight.resize(size, 0.0f);
            flowAccumulation.resize(size, 0);
            distanceToWater.resize(size, std::numeric_limits<float>::max());
            drainageDirection.resize(size, FlowDirection::Pit);
            slope.resize(size, 0.0f);
            moisture.resize(size, 0.0f);
        }

        void Clear()
        {
            std::fill(waterType.begin(), waterType.end(), WaterType::None);
            std::fill(waterSurfaceHeight.begin(), waterSurfaceHeight.end(), 0.0f);
            std::fill(flowAccumulation.begin(), flowAccumulation.end(), 0);
            std::fill(distanceToWater.begin(), distanceToWater.end(), std::numeric_limits<float>::max());
            std::fill(drainageDirection.begin(), drainageDirection.end(), FlowDirection::Pit);
            std::fill(slope.begin(), slope.end(), 0.0f);
            std::fill(moisture.begin(), moisture.end(), 0.0f);
        }

        size_t Index(int x, int z) const { return static_cast<size_t>(z) * width + x; }
        bool InBounds(int x, int z) const { return x >= 0 && x < width && z >= 0 && z < depth; }
    };

    /**
     * Settings for hydrology computation.
     */
    struct HydrologySettings
    {
        // Maximum distance to water considered (cells)
        float maxWaterDistance = 100.0f;

        // Base humidity level (global moisture baseline)
        float baseHumidity = 0.5f;

        // How much flow accumulation contributes to moisture
        float flowMoistureWeight = 0.3f;

        // How much proximity to water contributes to moisture
        float proximityMoistureWeight = 0.5f;

        // How much base humidity contributes
        float humidityWeight = 0.2f;

        // Flow accumulation normalization factor
        float flowNormalization = 1000.0f;
    };

    /**
     * Generates unified hydrology data from drainage, rivers, and lakes.
     *
     * This class aggregates output from:
     *   - DrainageGraph (flow direction, flow accumulation, slope)
     *   - RiverGenerator (rivers, streams, water surface)
     *   - LakeGenerator (lakes, water depth)
     *
     * And computes derived fields:
     *   - DistanceToWater (BFS from water cells)
     *   - Moisture (combined from flow, proximity, humidity)
     */
    class HydrologyGenerator
    {
    public:
        HydrologyGenerator() = default;
        ~HydrologyGenerator() = default;

        /**
         * Configure hydrology settings.
         */
        void SetSettings(const HydrologySettings &settings) { m_Settings = settings; }
        const HydrologySettings &GetSettings() const { return m_Settings; }

        /**
         * Compute unified hydrology data from component systems.
         *
         * @param drainage     Computed drainage graph
         * @param rivers       Generated river network
         * @param lakes        Generated lake network
         * @param heightmap    Terrain heights
         * @param cellSize     World units per cell
         */
        void Compute(const DrainageGraph &drainage,
                     const RiverGenerator &rivers,
                     const LakeGenerator &lakes,
                     const std::vector<float> &heightmap,
                     float cellSize);

        /**
         * Get the computed hydrology data.
         */
        const HydrologyData &GetData() const { return m_Data; }

        /**
         * Query water type at a cell.
         */
        WaterType GetWaterType(int x, int z) const;

        /**
         * Query water surface height at a cell.
         */
        float GetWaterSurfaceHeight(int x, int z) const;

        /**
         * Query flow accumulation at a cell.
         */
        uint32_t GetFlowAccumulation(int x, int z) const;

        /**
         * Query distance to nearest water at a cell.
         */
        float GetDistanceToWater(int x, int z) const;

        /**
         * Query drainage direction at a cell.
         */
        FlowDirection GetDrainageDirection(int x, int z) const;

        /**
         * Query slope at a cell.
         */
        float GetSlope(int x, int z) const;

        /**
         * Query moisture at a cell.
         */
        float GetMoisture(int x, int z) const;

        /**
         * Check if a cell is any type of water.
         */
        bool IsWater(int x, int z) const;

    private:
        /**
         * Copy data from drainage graph.
         */
        void CopyDrainageData(const DrainageGraph &drainage);

        /**
         * Merge water types from rivers and lakes.
         */
        void MergeWaterTypes(const RiverGenerator &rivers,
                             const LakeGenerator &lakes);

        /**
         * Compute distance to water using multi-source BFS.
         */
        void ComputeDistanceToWater(float cellSize);

        /**
         * Compute moisture field from flow, proximity, and humidity.
         */
        void ComputeMoisture();

        HydrologySettings m_Settings;
        HydrologyData m_Data;
    };

}
