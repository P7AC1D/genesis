#pragma once

#include "genesis/procedural/Noise.h"
#include "genesis/procedural/TerrainDebugView.h"
#include "genesis/procedural/MaterialBlender.h"
#include "genesis/procedural/BiomeClassifier.h"
#include "genesis/renderer/Mesh.h"
#include <memory>

namespace Genesis
{

    struct TerrainSettings
    {
        // Grid dimensions
        int width = 64;
        int depth = 64;
        float cellSize = 1.0f;

        // Height settings
        float heightScale = 10.0f;
        float baseHeight = 0.0f;

        // Debug Settings (Section 32)
        DebugViewSettings debugView;

        // Continental field settings (geological field system)
        bool useContinentalField = true;      // Enable continental land/ocean generation
        float continentalFrequency = 0.0003f; // Lower = larger landmasses
        float oceanThreshold = 0.45f;         // Continental value below which = ocean
        float coastlineBlend = 0.05f;         // Smoothstep epsilon for coastlines
        float oceanDepth = 50.0f;             // Maximum ocean depth below sea level
        float oceanFloorVariation = 0.3f;     // Noise variation on ocean floor

        // Noise settings
        uint32_t seed = 12345;
        float noiseScale = 0.05f;
        int octaves = 4;
        float persistence = 0.5f;
        float lacunarity = 2.0f;

        // Domain warping settings
        bool useWarp = true;
        float warpStrength = 0.5f; // How much to distort (0 = none, 1 = strong)
        float warpScale = 0.5f;    // Scale of warp noise relative to terrain noise
        int warpLevels = 2;        // Number of warp iterations (1-3 recommended)

        // Ridge noise settings for mountain ranges
        bool useRidgeNoise = true; // Enable ridge noise for sharp peaks
        float ridgeWeight = 0.7f;  // Blend weight (0 = all base, 1 = all ridge)
        float ridgePower = 2.0f;   // Sharpness exponent (higher = sharper peaks)
        float ridgeScale = 1.0f;   // Scale multiplier for ridge noise
        float peakBoost = 0.3f;    // Extra sharpening at mountain peaks (pow4 ridge boost)

        // Tectonic uplift mask - controls where mountains appear
        bool useUpliftMask = true;        // Enable uplift mask for realistic mountain bands
        float upliftScale = 0.02f;        // Scale of uplift noise (lower = larger mountain regions)
        float upliftThresholdLow = 0.4f;  // Below this = plains (no mountains)
        float upliftThresholdHigh = 0.7f; // Above this = full mountains
        float upliftPower = 1.5f;         // Sharpening power for uplift transitions

        // Erosion settings
        bool useErosion = true;             // Enable erosion effects
        float slopeErosionStrength = 0.15f; // How much steep slopes erode (0-1)
        float slopeThreshold = 0.5f;        // Slope angle threshold for erosion
        float valleyDepth = 0.3f;           // How deep valleys carve (0-1)

        // Hydraulic erosion (higher quality, slower)
        bool useHydraulicErosion = false; // Enable particle-based erosion
        int erosionIterations = 100;      // Number of water droplet simulations
        float erosionInertia = 0.05f;     // How much droplets maintain direction
        float erosionCapacity = 4.0f;     // Sediment carrying capacity
        float erosionDeposition = 0.3f;   // Rate of sediment deposition
        float erosionEvaporation = 0.02f; // Water evaporation rate

        // Low-poly style
        bool flatShading = true; // Use face normals instead of smooth normals

        // Biome-based coloring
        bool useBiomeColors = true; // Use biome classification for terrain colors
    };

    class TerrainGenerator
    {
    public:
        TerrainGenerator();
        explicit TerrainGenerator(const TerrainSettings &settings);

        void SetSettings(const TerrainSettings &settings);
        const TerrainSettings &GetSettings() const { return m_Settings; }

        // Generate terrain mesh at local origin
        std::shared_ptr<Mesh> Generate();

        // Generate terrain mesh at world offset (for chunked terrain)
        std::shared_ptr<Mesh> GenerateAtOffset(float offsetX, float offsetZ);

        // Generate terrain mesh at world offset with material-based coloring
        std::shared_ptr<Mesh> GenerateAtOffset(float offsetX, float offsetZ, const MaterialBlender *materialBlender);

        // Generate terrain mesh at world offset with biome-based coloring
        std::shared_ptr<Mesh> GenerateAtOffset(float offsetX, float offsetZ, const MaterialBlender *materialBlender, const BiomeClassifier *biomeClassifier);

        // Generate heightmap array at local origin
        std::vector<float> GenerateHeightmap();

        // Generate heightmap array at world offset (for chunked terrain)
        std::vector<float> GenerateHeightmapAtOffset(float offsetX, float offsetZ);

        // Get height at world position (for placing objects)
        float GetHeightAt(float worldX, float worldZ) const;

        // Get the cached heightmap (must call GenerateHeightmap first)
        const std::vector<float> &GetCachedHeightmap() const { return m_CachedHeightmap; }

        // Get color from material weights (material-based coloring)
        static glm::vec3 GetMaterialColor(const MaterialWeights &weights);

        // Sample raw height at world position (no erosion/shaping)
        // Useful for querying terrain height before full generation
        float SampleRawHeight(float worldX, float worldZ) const;

    private:
        // Apply erosion to heightmap in-place
        void ApplySlopeErosion(std::vector<float> &heightmap, int width, int depth) const;
        void ApplyHydraulicErosion(std::vector<float> &heightmap, int width, int depth, float offsetX, float offsetZ) const;
        void ApplyPeakShaping(std::vector<float> &heightmap, int width, int depth) const;

        // Build mesh from heightmap
        std::shared_ptr<Mesh> BuildMeshFromHeightmap(const std::vector<float> &heightmap, float offsetX, float offsetZ, const MaterialBlender *materialBlender = nullptr, const BiomeClassifier *biomeClassifier = nullptr) const;

        // Bilinear height sampling for hydraulic erosion
        float SampleHeightBilinear(const std::vector<float> &heightmap, int width, float x, float z) const;
        glm::vec2 SampleGradientBilinear(const std::vector<float> &heightmap, int width, float x, float z) const;

        TerrainSettings m_Settings;
        SimplexNoise m_Noise;
        std::vector<float> m_CachedHeightmap;
        glm::vec2 m_ChunkOrigin{0.0f, 0.0f}; // World origin of cached heightmap
    };

}
