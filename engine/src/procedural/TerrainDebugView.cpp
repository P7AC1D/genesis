#include "genesis/procedural/TerrainDebugView.h"
#include "genesis/procedural/BiomeClassifier.h"
#include <algorithm>
#include <cmath>

namespace Genesis
{

    const char *GetDebugViewName(DebugViewType type)
    {
        switch (type)
        {
        case DebugViewType::None:
            return "None";
        case DebugViewType::Height:
            return "Height";
        case DebugViewType::ContinentalMask:
            return "Continental Mask";
        case DebugViewType::UpliftMask:
            return "Uplift Mask";
        case DebugViewType::Slope:
            return "Slope";
        case DebugViewType::FlowDirection:
            return "Flow Direction";
        case DebugViewType::FlowAccumulation:
            return "Flow Accumulation";
        case DebugViewType::WaterType:
            return "Water Type";
        case DebugViewType::DistanceToWater:
            return "Distance to Water";
        case DebugViewType::Temperature:
            return "Temperature";
        case DebugViewType::Moisture:
            return "Moisture";
        case DebugViewType::Fertility:
            return "Fertility";
        case DebugViewType::DominantBiome:
            return "Dominant Biome";
        case DebugViewType::DominantMaterial:
            return "Dominant Material";
        default:
            return "Unknown";
        }
    }

    // ========================================================================
    // Color Maps
    // ========================================================================

    glm::vec3 TerrainDebugView::GetBiomeColor(BiomeType biome)
    {
        // Delegate to BiomeClassifier's canonical color palette
        return BiomeClassifier::GetBiomeColor(biome);
    }

    glm::vec3 TerrainDebugView::GetMaterialColor(MaterialType material)
    {
        switch (material)
        {
        case MaterialType::Rock:
            return glm::vec3(0.5f, 0.5f, 0.5f); // Gray
        case MaterialType::Dirt:
            return glm::vec3(0.6f, 0.4f, 0.2f); // Brown
        case MaterialType::Grass:
            return glm::vec3(0.3f, 0.7f, 0.3f); // Green
        case MaterialType::Sand:
            return glm::vec3(0.9f, 0.85f, 0.6f); // Sandy
        case MaterialType::Snow:
            return glm::vec3(0.95f, 0.95f, 1.0f); // White
        case MaterialType::Ice:
            return glm::vec3(0.7f, 0.85f, 0.95f); // Light blue
        case MaterialType::Mud:
            return glm::vec3(0.4f, 0.3f, 0.2f); // Dark brown
        case MaterialType::Water:
            return glm::vec3(0.2f, 0.4f, 0.8f); // Blue
        default:
            return glm::vec3(1.0f, 0.0f, 1.0f); // Magenta (error)
        }
    }

    glm::vec3 TerrainDebugView::GetWaterTypeColor(WaterType water)
    {
        switch (water)
        {
        case WaterType::None:
            return glm::vec3(0.3f, 0.3f, 0.3f); // Dark gray
        case WaterType::Stream:
            return glm::vec3(0.4f, 0.6f, 0.8f); // Light blue
        case WaterType::River:
            return glm::vec3(0.2f, 0.4f, 0.9f); // Blue
        case WaterType::Lake:
            return glm::vec3(0.1f, 0.3f, 0.7f); // Dark blue
        case WaterType::Ocean:
            return glm::vec3(0.05f, 0.15f, 0.4f); // Deep blue
        default:
            return glm::vec3(1.0f, 0.0f, 1.0f); // Magenta (error)
        }
    }

    glm::vec3 TerrainDebugView::GetFlowDirectionColor(FlowDirection dir)
    {
        // Color-code directions using a circular hue
        switch (dir)
        {
        case FlowDirection::North:
            return glm::vec3(0.0f, 1.0f, 0.0f); // Green
        case FlowDirection::NorthEast:
            return glm::vec3(0.5f, 1.0f, 0.0f); // Yellow-green
        case FlowDirection::East:
            return glm::vec3(1.0f, 1.0f, 0.0f); // Yellow
        case FlowDirection::SouthEast:
            return glm::vec3(1.0f, 0.5f, 0.0f); // Orange
        case FlowDirection::South:
            return glm::vec3(1.0f, 0.0f, 0.0f); // Red
        case FlowDirection::SouthWest:
            return glm::vec3(1.0f, 0.0f, 0.5f); // Pink
        case FlowDirection::West:
            return glm::vec3(1.0f, 0.0f, 1.0f); // Magenta
        case FlowDirection::NorthWest:
            return glm::vec3(0.5f, 0.0f, 1.0f); // Purple
        case FlowDirection::Pit:
            return glm::vec3(0.0f, 0.0f, 0.0f); // Black (pit/sink)
        default:
            return glm::vec3(0.5f, 0.5f, 0.5f); // Gray
        }
    }

    // ========================================================================
    // Colormaps
    // ========================================================================

    DebugPixel TerrainDebugView::GrayscaleMap(float value) const
    {
        float v = glm::clamp(value, 0.0f, 1.0f);
        return DebugPixel::FromFloat3(v, v, v);
    }

    DebugPixel TerrainDebugView::TemperatureMap(float value) const
    {
        // value: -1 (cold/blue) to +1 (hot/red)
        float t = glm::clamp((value + 1.0f) * 0.5f, 0.0f, 1.0f);

        // Blue -> White -> Red
        float r, g, b;
        if (t < 0.5f)
        {
            float s = t * 2.0f;
            r = s;
            g = s;
            b = 1.0f;
        }
        else
        {
            float s = (t - 0.5f) * 2.0f;
            r = 1.0f;
            g = 1.0f - s;
            b = 1.0f - s;
        }

        return DebugPixel::FromFloat3(r, g, b);
    }

    DebugPixel TerrainDebugView::MoistureMap(float value) const
    {
        // value: 0 (dry/brown) to 1 (wet/blue-green)
        float t = glm::clamp(value, 0.0f, 1.0f);

        // Brown -> Green -> Teal
        glm::vec3 dry(0.6f, 0.4f, 0.2f);
        glm::vec3 mid(0.3f, 0.6f, 0.3f);
        glm::vec3 wet(0.2f, 0.5f, 0.6f);

        glm::vec3 color;
        if (t < 0.5f)
        {
            color = glm::mix(dry, mid, t * 2.0f);
        }
        else
        {
            color = glm::mix(mid, wet, (t - 0.5f) * 2.0f);
        }

        return DebugPixel::FromVec3(color);
    }

    DebugPixel TerrainDebugView::FertilityMap(float value) const
    {
        // value: 0 (barren/red) to 1 (fertile/green)
        float t = glm::clamp(value, 0.0f, 1.0f);

        glm::vec3 barren(0.7f, 0.3f, 0.2f);
        glm::vec3 fertile(0.2f, 0.7f, 0.3f);
        glm::vec3 color = glm::mix(barren, fertile, t);

        return DebugPixel::FromVec3(color);
    }

    DebugPixel TerrainDebugView::HeatMap(float value) const
    {
        // Black -> Blue -> Cyan -> Green -> Yellow -> Red -> White
        float t = glm::clamp(value, 0.0f, 1.0f);

        glm::vec3 color;
        if (t < 0.166f)
        {
            float s = t / 0.166f;
            color = glm::mix(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), s);
        }
        else if (t < 0.333f)
        {
            float s = (t - 0.166f) / 0.166f;
            color = glm::mix(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 1.0f), s);
        }
        else if (t < 0.5f)
        {
            float s = (t - 0.333f) / 0.166f;
            color = glm::mix(glm::vec3(0.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), s);
        }
        else if (t < 0.666f)
        {
            float s = (t - 0.5f) / 0.166f;
            color = glm::mix(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), s);
        }
        else if (t < 0.833f)
        {
            float s = (t - 0.666f) / 0.166f;
            color = glm::mix(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), s);
        }
        else
        {
            float s = (t - 0.833f) / 0.166f;
            color = glm::mix(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), s);
        }

        return DebugPixel::FromVec3(color);
    }

    // ========================================================================
    // Visualization Generators
    // ========================================================================

    void TerrainDebugView::GenerateHeightView(const std::vector<float> &heightmap,
                                              int width, int depth,
                                              float minHeight, float maxHeight)
    {
        m_TextureData.Resize(width, depth);

        float range = maxHeight - minHeight;
        if (range < 0.001f)
            range = 1.0f;

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                size_t idx = static_cast<size_t>(z) * width + x;
                float h = heightmap[idx];
                float normalized = (h - minHeight) / range;
                m_TextureData.pixels[idx] = GrayscaleMap(normalized);
            }
        }
    }

    void TerrainDebugView::GenerateContinentalView(const std::vector<float> &continentalMask,
                                                   int width, int depth)
    {
        m_TextureData.Resize(width, depth);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                size_t idx = static_cast<size_t>(z) * width + x;
                float v = continentalMask[idx];
                m_TextureData.pixels[idx] = GrayscaleMap(v);
            }
        }
    }

    void TerrainDebugView::GenerateUpliftView(const std::vector<float> &upliftMask,
                                              int width, int depth)
    {
        m_TextureData.Resize(width, depth);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                size_t idx = static_cast<size_t>(z) * width + x;
                float v = upliftMask[idx];

                // Color gradient: blue (low uplift) -> red (high uplift)
                glm::vec3 low(0.2f, 0.3f, 0.7f);
                glm::vec3 high(0.8f, 0.2f, 0.1f);
                glm::vec3 color = glm::mix(low, high, v);

                m_TextureData.pixels[idx] = DebugPixel::FromVec3(color);
            }
        }
    }

    void TerrainDebugView::GenerateSlopeView(const std::vector<float> &slope,
                                             int width, int depth)
    {
        m_TextureData.Resize(width, depth);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                size_t idx = static_cast<size_t>(z) * width + x;
                float s = slope[idx];

                // Green (flat) -> Yellow -> Red (steep)
                glm::vec3 flat(0.2f, 0.7f, 0.2f);
                glm::vec3 medium(0.9f, 0.9f, 0.2f);
                glm::vec3 steep(0.9f, 0.2f, 0.1f);

                glm::vec3 color;
                if (s < 0.5f)
                {
                    color = glm::mix(flat, medium, s * 2.0f);
                }
                else
                {
                    color = glm::mix(medium, steep, (s - 0.5f) * 2.0f);
                }

                m_TextureData.pixels[idx] = DebugPixel::FromVec3(color);
            }
        }
    }

    void TerrainDebugView::GenerateFlowDirectionView(const std::vector<FlowDirection> &flowDir,
                                                     int width, int depth)
    {
        m_TextureData.Resize(width, depth);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                size_t idx = static_cast<size_t>(z) * width + x;
                FlowDirection dir = flowDir[idx];
                glm::vec3 color = GetFlowDirectionColor(dir);
                m_TextureData.pixels[idx] = DebugPixel::FromVec3(color);
            }
        }
    }

    void TerrainDebugView::GenerateFlowAccumulationView(const std::vector<uint32_t> &flowAccum,
                                                        int width, int depth)
    {
        m_TextureData.Resize(width, depth);

        // Find max for normalization (use log scale)
        uint32_t maxAccum = 1;
        for (uint32_t acc : flowAccum)
        {
            maxAccum = std::max(maxAccum, acc);
        }

        float logMax = std::log(static_cast<float>(maxAccum) + 1.0f);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                size_t idx = static_cast<size_t>(z) * width + x;
                uint32_t acc = flowAccum[idx];

                // Log scale normalization
                float logVal = std::log(static_cast<float>(acc) + 1.0f);
                float normalized = logVal / logMax;

                m_TextureData.pixels[idx] = HeatMap(normalized);
            }
        }
    }

    void TerrainDebugView::GenerateWaterTypeView(const std::vector<WaterType> &waterType,
                                                 int width, int depth)
    {
        m_TextureData.Resize(width, depth);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                size_t idx = static_cast<size_t>(z) * width + x;
                WaterType wt = waterType[idx];
                glm::vec3 color = GetWaterTypeColor(wt);
                m_TextureData.pixels[idx] = DebugPixel::FromVec3(color);
            }
        }
    }

    void TerrainDebugView::GenerateDistanceToWaterView(const std::vector<float> &distance,
                                                       int width, int depth,
                                                       float maxDistance)
    {
        m_TextureData.Resize(width, depth);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                size_t idx = static_cast<size_t>(z) * width + x;
                float d = distance[idx];
                float normalized = glm::clamp(d / maxDistance, 0.0f, 1.0f);

                // Blue (near water) -> Brown (far from water)
                glm::vec3 near(0.2f, 0.4f, 0.8f);
                glm::vec3 far(0.6f, 0.4f, 0.2f);
                glm::vec3 color = glm::mix(near, far, normalized);

                m_TextureData.pixels[idx] = DebugPixel::FromVec3(color);
            }
        }
    }

    void TerrainDebugView::GenerateTemperatureView(const std::vector<float> &temperature,
                                                   int width, int depth)
    {
        m_TextureData.Resize(width, depth);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                size_t idx = static_cast<size_t>(z) * width + x;
                float t = temperature[idx];
                m_TextureData.pixels[idx] = TemperatureMap(t);
            }
        }
    }

    void TerrainDebugView::GenerateMoistureView(const std::vector<float> &moisture,
                                                int width, int depth)
    {
        m_TextureData.Resize(width, depth);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                size_t idx = static_cast<size_t>(z) * width + x;
                float m = moisture[idx];
                m_TextureData.pixels[idx] = MoistureMap(m);
            }
        }
    }

    void TerrainDebugView::GenerateFertilityView(const std::vector<float> &fertility,
                                                 int width, int depth)
    {
        m_TextureData.Resize(width, depth);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                size_t idx = static_cast<size_t>(z) * width + x;
                float f = fertility[idx];
                m_TextureData.pixels[idx] = FertilityMap(f);
            }
        }
    }

    void TerrainDebugView::GenerateBiomeView(const std::vector<BiomeType> &biome,
                                             int width, int depth)
    {
        m_TextureData.Resize(width, depth);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                size_t idx = static_cast<size_t>(z) * width + x;
                BiomeType b = biome[idx];
                glm::vec3 color = GetBiomeColor(b);
                m_TextureData.pixels[idx] = DebugPixel::FromVec3(color);
            }
        }
    }

    void TerrainDebugView::GenerateMaterialView(const std::vector<MaterialType> &material,
                                                int width, int depth)
    {
        m_TextureData.Resize(width, depth);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                size_t idx = static_cast<size_t>(z) * width + x;
                MaterialType mat = material[idx];
                glm::vec3 color = GetMaterialColor(mat);
                m_TextureData.pixels[idx] = DebugPixel::FromVec3(color);
            }
        }
    }

}
