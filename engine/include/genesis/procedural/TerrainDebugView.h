#pragma once

#include "genesis/procedural/HydrologyData.h"
#include "genesis/procedural/ClimateGenerator.h"
#include "genesis/procedural/BiomeClassifier.h"
#include "genesis/procedural/MaterialBlender.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace Genesis
{

    /**
     * Section 32: Debug & Validation Views
     *
     * Provides visualization of intermediate terrain fields for debugging
     * and validation. Each field can be rendered as a colorized texture.
     */

    /**
     * Available debug visualization types.
     */
    enum class DebugViewType : uint8_t
    {
        None = 0,         // No debug overlay
        Height,           // Terrain heightmap (grayscale)
        ContinentalMask,  // Continental noise field
        UpliftMask,       // Tectonic uplift regions
        Slope,            // Terrain slope (steepness)
        FlowDirection,    // Drainage flow directions (color-coded arrows)
        FlowAccumulation, // Flow accumulation (river detection)
        WaterType,        // Water body classification
        DistanceToWater,  // Distance field from water
        Temperature,      // Temperature field (blue-red gradient)
        Moisture,         // Moisture field (brown-green gradient)
        Fertility,        // Fertility field (red-green gradient)
        DominantBiome,    // Biome classification (categorical colors)
        DominantMaterial, // Surface material (categorical colors)

        Count
    };

    /**
     * Returns the display name for a debug view type.
     */
    const char *GetDebugViewName(DebugViewType type);

    /**
     * Settings for debug visualization.
     */
    struct DebugViewSettings
    {
        DebugViewType activeView = DebugViewType::None;
        float overlayOpacity = 0.7f;
        bool showChunkBoundaries = false;
        bool showFlowArrows = false;

        // Value range overrides (0 = auto)
        float minValue = 0.0f;
        float maxValue = 0.0f;
    };

    /**
     * RGBA pixel for debug textures.
     */
    struct DebugPixel
    {
        uint8_t r, g, b, a;

        DebugPixel() : r(0), g(0), b(0), a(255) {}
        DebugPixel(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
            : r(r_), g(g_), b(b_), a(a_) {}

        static DebugPixel FromFloat3(float r, float g, float b, float a = 1.0f)
        {
            return DebugPixel(
                static_cast<uint8_t>(glm::clamp(r * 255.0f, 0.0f, 255.0f)),
                static_cast<uint8_t>(glm::clamp(g * 255.0f, 0.0f, 255.0f)),
                static_cast<uint8_t>(glm::clamp(b * 255.0f, 0.0f, 255.0f)),
                static_cast<uint8_t>(glm::clamp(a * 255.0f, 0.0f, 255.0f)));
        }

        static DebugPixel FromVec3(const glm::vec3 &color, float a = 1.0f)
        {
            return FromFloat3(color.r, color.g, color.b, a);
        }
    };

    /**
     * Generated debug texture data.
     */
    struct DebugTextureData
    {
        std::vector<DebugPixel> pixels;
        int width = 0;
        int height = 0;

        void Resize(int w, int h)
        {
            width = w;
            height = h;
            pixels.resize(static_cast<size_t>(w) * h);
        }

        void Clear(DebugPixel fill = DebugPixel(0, 0, 0, 255))
        {
            std::fill(pixels.begin(), pixels.end(), fill);
        }

        size_t Index(int x, int y) const
        {
            return static_cast<size_t>(y) * width + x;
        }

        const uint8_t *GetData() const
        {
            return reinterpret_cast<const uint8_t *>(pixels.data());
        }

        size_t GetDataSize() const
        {
            return pixels.size() * sizeof(DebugPixel);
        }
    };

    /**
     * Generates debug visualization textures from terrain data.
     */
    class TerrainDebugView
    {
    public:
        TerrainDebugView() = default;
        ~TerrainDebugView() = default;

        /**
         * Configure debug view settings.
         */
        void SetSettings(const DebugViewSettings &settings) { m_Settings = settings; }
        const DebugViewSettings &GetSettings() const { return m_Settings; }
        DebugViewSettings &GetSettings() { return m_Settings; }

        /**
         * Generate debug texture for heightmap visualization.
         */
        void GenerateHeightView(const std::vector<float> &heightmap,
                                int width, int depth,
                                float minHeight, float maxHeight);

        /**
         * Generate debug texture for continental mask.
         */
        void GenerateContinentalView(const std::vector<float> &continentalMask,
                                     int width, int depth);

        /**
         * Generate debug texture for uplift mask.
         */
        void GenerateUpliftView(const std::vector<float> &upliftMask,
                                int width, int depth);

        /**
         * Generate debug texture for slope visualization.
         */
        void GenerateSlopeView(const std::vector<float> &slope,
                               int width, int depth);

        /**
         * Generate debug texture for flow direction (color-coded).
         */
        void GenerateFlowDirectionView(const std::vector<FlowDirection> &flowDir,
                                       int width, int depth);

        /**
         * Generate debug texture for flow accumulation (log scale heatmap).
         */
        void GenerateFlowAccumulationView(const std::vector<uint32_t> &flowAccum,
                                          int width, int depth);

        /**
         * Generate debug texture for water type classification.
         */
        void GenerateWaterTypeView(const std::vector<WaterType> &waterType,
                                   int width, int depth);

        /**
         * Generate debug texture for distance to water field.
         */
        void GenerateDistanceToWaterView(const std::vector<float> &distance,
                                         int width, int depth,
                                         float maxDistance);

        /**
         * Generate debug texture for temperature field.
         */
        void GenerateTemperatureView(const std::vector<float> &temperature,
                                     int width, int depth);

        /**
         * Generate debug texture for moisture field.
         */
        void GenerateMoistureView(const std::vector<float> &moisture,
                                  int width, int depth);

        /**
         * Generate debug texture for fertility field.
         */
        void GenerateFertilityView(const std::vector<float> &fertility,
                                   int width, int depth);

        /**
         * Generate debug texture for dominant biome.
         */
        void GenerateBiomeView(const std::vector<BiomeType> &biome,
                               int width, int depth);

        /**
         * Generate debug texture for dominant material.
         */
        void GenerateMaterialView(const std::vector<MaterialType> &material,
                                  int width, int depth);

        /**
         * Get the generated debug texture data.
         */
        const DebugTextureData &GetTextureData() const { return m_TextureData; }

        /**
         * Get color for a biome type.
         */
        static glm::vec3 GetBiomeColor(BiomeType biome);

        /**
         * Get color for a material type.
         */
        static glm::vec3 GetMaterialColor(MaterialType material);

        /**
         * Get color for a water type.
         */
        static glm::vec3 GetWaterTypeColor(WaterType water);

        /**
         * Get color for a flow direction.
         */
        static glm::vec3 GetFlowDirectionColor(FlowDirection dir);

    private:
        /**
         * Apply grayscale colormap (black to white).
         */
        DebugPixel GrayscaleMap(float value) const;

        /**
         * Apply temperature colormap (blue to red).
         */
        DebugPixel TemperatureMap(float value) const;

        /**
         * Apply moisture colormap (brown to blue-green).
         */
        DebugPixel MoistureMap(float value) const;

        /**
         * Apply fertility colormap (red to green).
         */
        DebugPixel FertilityMap(float value) const;

        /**
         * Apply heatmap colormap (black, blue, cyan, green, yellow, red, white).
         */
        DebugPixel HeatMap(float value) const;

        DebugViewSettings m_Settings;
        DebugTextureData m_TextureData;
    };

}
