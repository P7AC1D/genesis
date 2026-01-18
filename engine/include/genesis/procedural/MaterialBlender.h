#pragma once

#include "genesis/procedural/ClimateGenerator.h"
#include "genesis/procedural/HydrologyData.h"
#include <vector>
#include <array>
#include <cstdint>

namespace Genesis
{

    /**
     * Section 29: Surface Material Blending
     *
     * Materials are blended based on terrain properties.
     * Weights are computed per-cell and normalized for rendering.
     */

    /**
     * Section 29.1: Material types
     */
    enum class MaterialType : uint8_t
    {
        Rock = 0,
        Dirt = 1,
        Grass = 2,
        Sand = 3,
        Snow = 4,
        Ice = 5,
        Mud = 6,
        Water = 7,

        Count = 8
    };

    /**
     * Material weights for blending.
     * All weights sum to 1.0 after normalization.
     */
    struct MaterialWeights
    {
        std::array<float, static_cast<size_t>(MaterialType::Count)> weights = {};

        MaterialWeights()
        {
            weights.fill(0.0f);
        }

        float &operator[](MaterialType type)
        {
            return weights[static_cast<size_t>(type)];
        }

        float operator[](MaterialType type) const
        {
            return weights[static_cast<size_t>(type)];
        }

        /**
         * Get the dominant material type.
         */
        MaterialType GetDominant() const
        {
            size_t maxIdx = 0;
            float maxWeight = weights[0];

            for (size_t i = 1; i < weights.size(); i++)
            {
                if (weights[i] > maxWeight)
                {
                    maxWeight = weights[i];
                    maxIdx = i;
                }
            }

            return static_cast<MaterialType>(maxIdx);
        }

        /**
         * Normalize weights to sum to 1.0.
         */
        void Normalize()
        {
            float sum = 0.0f;
            for (float w : weights)
            {
                sum += w;
            }

            if (sum > 0.0f)
            {
                for (float &w : weights)
                {
                    w /= sum;
                }
            }
            else
            {
                // Default to dirt if no weights
                weights[static_cast<size_t>(MaterialType::Dirt)] = 1.0f;
            }
        }
    };

    /**
     * Settings for material blending.
     */
    struct MaterialSettings
    {
        // Slope thresholds
        float rockSlopeThreshold = 0.5f;  // Slope above which rock dominates
        float steepSlopeThreshold = 0.8f; // Very steep = pure rock

        // Height thresholds (normalized)
        float snowLineStart = 0.7f; // Height where snow begins
        float snowLineFull = 0.9f;  // Height where snow dominates

        // Temperature thresholds
        float freezingPoint = -0.3f; // Temperature for ice formation
        float snowMeltPoint = 0.1f;  // Temperature where snow melts

        // Moisture thresholds
        float mudMoistureThreshold = 0.7f; // Moisture level for mud
        float grassMoistureMin = 0.3f;     // Minimum moisture for grass

        // Water proximity
        float sandDistance = 10.0f; // Distance from water for sand
        float sandSlopeMax = 0.15f; // Maximum slope for sand

        // Fertility
        float grassFertilityMin = 0.2f; // Minimum fertility for grass
    };

    /**
     * Material data per cell.
     */
    struct MaterialData
    {
        std::vector<MaterialWeights> cellMaterials; // Material weights per cell
        std::vector<MaterialType> dominantMaterial; // Dominant material per cell

        int width = 0;
        int depth = 0;

        void Resize(int w, int d)
        {
            width = w;
            depth = d;
            size_t size = static_cast<size_t>(w) * d;
            cellMaterials.resize(size);
            dominantMaterial.resize(size, MaterialType::Dirt);
        }

        void Clear()
        {
            for (auto &mw : cellMaterials)
            {
                mw = MaterialWeights();
            }
            std::fill(dominantMaterial.begin(), dominantMaterial.end(), MaterialType::Dirt);
        }

        size_t Index(int x, int z) const { return static_cast<size_t>(z) * width + x; }
        bool InBounds(int x, int z) const { return x >= 0 && x < width && z >= 0 && z < depth; }
    };

    /**
     * Computes surface material weights from terrain and climate data.
     *
     * Section 29.2: Weight inputs depend on:
     *   - Slope → Rock exposure
     *   - Height → Snow
     *   - Temperature → Ice vs snow
     *   - Moisture → Mud, grass
     *   - Fertility → Vegetation
     *   - WaterType → Sand, silt
     */
    class MaterialBlender
    {
    public:
        MaterialBlender() = default;
        ~MaterialBlender() = default;

        /**
         * Configure material settings.
         */
        void SetSettings(const MaterialSettings &settings) { m_Settings = settings; }
        const MaterialSettings &GetSettings() const { return m_Settings; }

        /**
         * Compute material weights for all cells.
         *
         * @param heightmap   Terrain heights
         * @param hydrology   Hydrology data (water type, distance, moisture)
         * @param climate     Climate data (temperature, moisture, fertility)
         * @param seaLevel    Sea level height
         * @param heightScale Maximum terrain height for normalization
         */
        void Compute(const std::vector<float> &heightmap,
                     const HydrologyData &hydrology,
                     const ClimateData &climate,
                     float seaLevel,
                     float heightScale);

        /**
         * Get the material data.
         */
        const MaterialData &GetData() const { return m_Data; }

        /**
         * Get material weights at a cell.
         */
        MaterialWeights GetMaterialWeights(int x, int z) const;

        /**
         * Get dominant material at a cell.
         */
        MaterialType GetDominantMaterial(int x, int z) const;

        /**
         * Get material name as string.
         */
        static const char *GetMaterialName(MaterialType type);

    private:
        /**
         * Section 29.3: Compute material weights for a single cell.
         */
        MaterialWeights ComputeMaterialWeights(float height,
                                               float heightNorm,
                                               float slope,
                                               float temperature,
                                               float moisture,
                                               float fertility,
                                               float distanceToWater,
                                               WaterType waterType) const;

        MaterialSettings m_Settings;
        MaterialData m_Data;
    };

}
