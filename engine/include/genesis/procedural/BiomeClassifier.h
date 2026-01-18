#pragma once

#include "genesis/procedural/ClimateGenerator.h"
#include "genesis/procedural/WetlandDetector.h"
#include <vector>
#include <array>

namespace Genesis
{

    /**
     * Section 28: Biome Classification (Derived)
     *
     * Biomes are derived via continuous thresholds, not enums.
     * Biome values are weights, not labels - allowing smooth blending.
     */

    /**
     * Biome types for classification.
     * These are reference points, actual biome assignment uses weights.
     */
    enum class BiomeType : uint8_t
    {
        Polar = 0,       // Frozen, ice
        Tundra = 1,      // Cold, sparse vegetation
        Boreal = 2,      // Cold forest (taiga)
        Temperate = 3,   // Moderate climate, deciduous
        Mediterranean = 4, // Warm, dry summers
        Grassland = 5,   // Plains, savanna
        Desert = 6,      // Hot and dry
        Tropical = 7,    // Hot and wet
        Rainforest = 8,  // Very hot and very wet
        Wetland = 9,     // High moisture, low slope

        Count = 10
    };

    /**
     * Biome weights for soft classification.
     * All weights sum to 1.0 for proper blending.
     */
    struct BiomeWeights
    {
        std::array<float, static_cast<size_t>(BiomeType::Count)> weights = {};

        BiomeWeights()
        {
            weights.fill(0.0f);
        }

        float &operator[](BiomeType type)
        {
            return weights[static_cast<size_t>(type)];
        }

        float operator[](BiomeType type) const
        {
            return weights[static_cast<size_t>(type)];
        }

        /**
         * Get the dominant biome type.
         */
        BiomeType GetDominant() const
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

            return static_cast<BiomeType>(maxIdx);
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
        }
    };

    /**
     * Biome classification data per cell.
     */
    struct BiomeData
    {
        std::vector<BiomeWeights> cellBiomes; // Biome weights per cell
        std::vector<BiomeType> dominantBiome; // Dominant biome per cell

        int width = 0;
        int depth = 0;

        void Resize(int w, int d)
        {
            width = w;
            depth = d;
            size_t size = static_cast<size_t>(w) * d;
            cellBiomes.resize(size);
            dominantBiome.resize(size, BiomeType::Temperate);
        }

        void Clear()
        {
            for (auto &bw : cellBiomes)
            {
                bw = BiomeWeights();
            }
            std::fill(dominantBiome.begin(), dominantBiome.end(), BiomeType::Temperate);
        }

        size_t Index(int x, int z) const { return static_cast<size_t>(z) * width + x; }
        bool InBounds(int x, int z) const { return x >= 0 && x < width && z >= 0 && z < depth; }
    };

    /**
     * Classifies terrain cells into biomes based on climate data.
     *
     * Section 28: Biomes are derived via continuous thresholds.
     * Example logic from spec:
     *   if (temperature < -0.6) biome = Polar;
     *   else if (moisture < 0.15) biome = Desert;
     *   else if (moisture > 0.7 && temperature > 0.4) biome = Rainforest;
     *   else if (temperature < 0.0) biome = Boreal;
     *   else biome = Temperate;
     */
    class BiomeClassifier
    {
    public:
        BiomeClassifier() = default;
        ~BiomeClassifier() = default;

        /**
         * Classify all cells based on climate data.
         *
         * @param climate  Climate data (temperature, moisture, fertility)
         * @param wetlands Wetland data for wetland biome classification
         */
        void Classify(const ClimateData &climate,
                      const WetlandData *wetlands = nullptr);

        /**
         * Get the biome data.
         */
        const BiomeData &GetData() const { return m_Data; }

        /**
         * Get biome weights at a cell.
         */
        BiomeWeights GetBiomeWeights(int x, int z) const;

        /**
         * Get dominant biome at a cell.
         */
        BiomeType GetDominantBiome(int x, int z) const;

        /**
         * Get biome name as string.
         */
        static const char *GetBiomeName(BiomeType type);

    private:
        /**
         * Compute biome weights for a single cell.
         * Uses smooth thresholds for soft classification.
         */
        BiomeWeights ComputeBiomeWeights(float temperature, float moisture,
                                          float fertility, bool isWetland) const;

        /**
         * Smooth threshold function for biome boundaries.
         * Returns weight [0, 1] based on distance from threshold.
         */
        float SmoothThreshold(float value, float threshold, float width) const;

        BiomeData m_Data;
    };

}
