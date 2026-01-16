#include "genesis/procedural/Biome.h"
#include "genesis/procedural/Noise.h"
#include <algorithm>
#include <cmath>

namespace Genesis {

    // Static members
    std::vector<BiomeSettings> BiomeGenerator::s_BiomeSettings;
    bool BiomeGenerator::s_BiomeSettingsInitialized = false;

    BiomeGenerator::BiomeGenerator() {
        InitializeBiomeSettings();
    }

    void BiomeGenerator::Initialize(uint32_t worldSeed) {
        m_WorldSeed = worldSeed;
        
        // Create separate noise generators for temperature and moisture
        // Using different seed offsets to ensure independent patterns
        m_TemperatureNoise = std::make_unique<SimplexNoise>(worldSeed + 1000);
        m_MoistureNoise = std::make_unique<SimplexNoise>(worldSeed + 2000);
    }

    void BiomeGenerator::InitializeBiomeSettings() {
        if (s_BiomeSettingsInitialized) return;
        
        s_BiomeSettings.resize(static_cast<size_t>(BiomeType::Count));
        
        // Ocean - deep water areas
        auto& ocean = s_BiomeSettings[static_cast<size_t>(BiomeType::Ocean)];
        ocean.name = "Ocean";
        ocean.type = BiomeType::Ocean;
        ocean.lowColor = glm::vec3(0.1f, 0.2f, 0.4f);   // Deep blue
        ocean.midColor = glm::vec3(0.15f, 0.3f, 0.5f);
        ocean.highColor = glm::vec3(0.2f, 0.35f, 0.5f);
        ocean.peakColor = glm::vec3(0.7f, 0.7f, 0.6f);  // Sandy if exposed
        ocean.heightMultiplier = 0.5f;
        ocean.treeDensity = 0.0f;
        ocean.rockDensity = 0.2f;
        ocean.waterColor = glm::vec3(0.05f, 0.2f, 0.4f);
        
        // Beach - sandy shores
        auto& beach = s_BiomeSettings[static_cast<size_t>(BiomeType::Beach)];
        beach.name = "Beach";
        beach.type = BiomeType::Beach;
        beach.lowColor = glm::vec3(0.76f, 0.7f, 0.5f);   // Wet sand
        beach.midColor = glm::vec3(0.86f, 0.8f, 0.6f);   // Dry sand
        beach.highColor = glm::vec3(0.8f, 0.75f, 0.55f);
        beach.peakColor = glm::vec3(0.7f, 0.65f, 0.5f);
        beach.heightMultiplier = 0.3f;
        beach.roughness = 0.5f;
        beach.treeDensity = 0.1f;  // Palm trees occasionally
        beach.rockDensity = 0.3f;
        beach.minTreeHeight = 0.3f;
        beach.maxTreeHeight = 0.6f;
        
        // Plains - grasslands
        auto& plains = s_BiomeSettings[static_cast<size_t>(BiomeType::Plains)];
        plains.name = "Plains";
        plains.type = BiomeType::Plains;
        plains.lowColor = glm::vec3(0.4f, 0.55f, 0.2f);  // Yellow-green grass
        plains.midColor = glm::vec3(0.5f, 0.65f, 0.25f);
        plains.highColor = glm::vec3(0.45f, 0.5f, 0.25f);
        plains.peakColor = glm::vec3(0.5f, 0.5f, 0.45f); // Rocky tops
        plains.heightMultiplier = 0.6f;
        plains.roughness = 0.7f;
        plains.treeDensity = 0.3f;  // Sparse trees
        plains.rockDensity = 0.4f;
        plains.minTreeHeight = 0.25f;
        plains.maxTreeHeight = 0.7f;
        
        // Forest - dense woodland
        auto& forest = s_BiomeSettings[static_cast<size_t>(BiomeType::Forest)];
        forest.name = "Forest";
        forest.type = BiomeType::Forest;
        forest.lowColor = glm::vec3(0.2f, 0.35f, 0.15f);  // Dark forest floor
        forest.midColor = glm::vec3(0.25f, 0.45f, 0.18f); // Forest green
        forest.highColor = glm::vec3(0.3f, 0.4f, 0.2f);   // Lighter green
        forest.peakColor = glm::vec3(0.4f, 0.4f, 0.35f);  // Rocky peaks
        forest.heightMultiplier = 1.0f;
        forest.roughness = 1.2f;
        forest.treeDensity = 2.5f;  // Dense trees
        forest.rockDensity = 0.5f;
        forest.minTreeHeight = 0.2f;
        forest.maxTreeHeight = 0.8f;
        
        // Desert - hot and dry
        auto& desert = s_BiomeSettings[static_cast<size_t>(BiomeType::Desert)];
        desert.name = "Desert";
        desert.type = BiomeType::Desert;
        desert.lowColor = glm::vec3(0.85f, 0.75f, 0.5f);  // Light sand
        desert.midColor = glm::vec3(0.9f, 0.8f, 0.55f);   // Bright sand
        desert.highColor = glm::vec3(0.8f, 0.65f, 0.4f);  // Orange sand
        desert.peakColor = glm::vec3(0.7f, 0.55f, 0.35f); // Red rock
        desert.heightMultiplier = 0.8f;
        desert.roughness = 0.6f;
        desert.treeDensity = 0.05f;  // Rare cacti
        desert.rockDensity = 0.8f;   // More rocks
        desert.hasWater = false;
        desert.minTreeHeight = 0.2f;
        desert.maxTreeHeight = 0.5f;
        
        // Tundra - cold and sparse
        auto& tundra = s_BiomeSettings[static_cast<size_t>(BiomeType::Tundra)];
        tundra.name = "Tundra";
        tundra.type = BiomeType::Tundra;
        tundra.lowColor = glm::vec3(0.7f, 0.75f, 0.8f);   // Frozen ground
        tundra.midColor = glm::vec3(0.8f, 0.85f, 0.9f);   // Snow patches
        tundra.highColor = glm::vec3(0.9f, 0.92f, 0.95f); // More snow
        tundra.peakColor = glm::vec3(0.95f, 0.97f, 1.0f); // Pure snow
        tundra.heightMultiplier = 1.2f;
        tundra.roughness = 1.0f;
        tundra.treeDensity = 0.15f;  // Sparse evergreens
        tundra.rockDensity = 0.6f;
        tundra.waterColor = glm::vec3(0.4f, 0.5f, 0.6f);  // Icy water
        tundra.minTreeHeight = 0.2f;
        tundra.maxTreeHeight = 0.6f;
        
        // Swamp - wet and murky
        auto& swamp = s_BiomeSettings[static_cast<size_t>(BiomeType::Swamp)];
        swamp.name = "Swamp";
        swamp.type = BiomeType::Swamp;
        swamp.lowColor = glm::vec3(0.25f, 0.3f, 0.2f);   // Murky water edge
        swamp.midColor = glm::vec3(0.3f, 0.38f, 0.22f);  // Swamp green
        swamp.highColor = glm::vec3(0.35f, 0.4f, 0.25f); // Drier areas
        swamp.peakColor = glm::vec3(0.4f, 0.4f, 0.3f);
        swamp.heightMultiplier = 0.4f;  // Flat terrain
        swamp.roughness = 0.5f;
        swamp.treeDensity = 1.5f;  // Moderate trees
        swamp.rockDensity = 0.3f;
        swamp.waterColor = glm::vec3(0.2f, 0.3f, 0.15f); // Murky green water
        swamp.minTreeHeight = 0.15f;
        swamp.maxTreeHeight = 0.5f;
        
        // Mountains - high altitude
        auto& mountains = s_BiomeSettings[static_cast<size_t>(BiomeType::Mountains)];
        mountains.name = "Mountains";
        mountains.type = BiomeType::Mountains;
        mountains.lowColor = glm::vec3(0.35f, 0.4f, 0.3f);  // Mountain grass
        mountains.midColor = glm::vec3(0.5f, 0.5f, 0.45f);  // Rocky
        mountains.highColor = glm::vec3(0.6f, 0.6f, 0.55f); // Exposed rock
        mountains.peakColor = glm::vec3(0.9f, 0.92f, 0.95f);// Snow caps
        mountains.heightMultiplier = 2.0f;  // Tall peaks
        mountains.roughness = 1.5f;
        mountains.treeDensity = 0.4f;
        mountains.rockDensity = 1.2f;
        mountains.minTreeHeight = 0.15f;
        mountains.maxTreeHeight = 0.5f;  // Trees only at lower elevations
        
        s_BiomeSettingsInitialized = true;
    }

    float BiomeGenerator::SampleTemperature(float worldX, float worldZ) const {
        if (!m_TemperatureNoise) return 0.5f;
        
        // Multi-octave sampling for more natural variation
        float temp = m_TemperatureNoise->FBM(
            worldX * m_TemperatureScale,
            worldZ * m_TemperatureScale,
            3, 0.5f, 2.0f);
        
        // Normalize to 0-1 range
        return (temp + 1.0f) * 0.5f;
    }

    float BiomeGenerator::SampleMoisture(float worldX, float worldZ) const {
        if (!m_MoistureNoise) return 0.5f;
        
        float moisture = m_MoistureNoise->FBM(
            worldX * m_MoistureScale,
            worldZ * m_MoistureScale,
            3, 0.5f, 2.0f);
        
        return (moisture + 1.0f) * 0.5f;
    }

    BiomeType BiomeGenerator::SelectBiome(float temperature, float moisture, float height) const {
        // Very low height is always ocean
        if (height < 0.15f) {
            return BiomeType::Ocean;
        }
        
        // Beach near water level
        if (height < 0.25f) {
            return BiomeType::Beach;
        }
        
        // Very high areas are mountains
        if (height > 0.7f) {
            return BiomeType::Mountains;
        }
        
        // Use temperature/moisture for other biomes
        // Temperature: 0 = cold, 1 = hot
        // Moisture: 0 = dry, 1 = wet
        
        if (temperature < 0.3f) {
            // Cold
            return BiomeType::Tundra;
        } else if (temperature > 0.7f) {
            // Hot
            if (moisture < 0.3f) {
                return BiomeType::Desert;
            } else if (moisture > 0.6f) {
                return BiomeType::Swamp;
            } else {
                return BiomeType::Plains;
            }
        } else {
            // Temperate
            if (moisture < 0.35f) {
                return BiomeType::Plains;
            } else if (moisture > 0.55f) {
                return BiomeType::Forest;
            } else {
                // Mix based on temperature
                return temperature > 0.5f ? BiomeType::Plains : BiomeType::Forest;
            }
        }
    }

    BiomeData BiomeGenerator::GetBiomeAt(float worldX, float worldZ) const {
        BiomeData data;
        
        data.temperature = SampleTemperature(worldX, worldZ);
        data.moisture = SampleMoisture(worldX, worldZ);
        
        // We need height to determine ocean/beach/mountains
        // For now, use a placeholder - actual height will modify this
        float heightEstimate = 0.5f;
        
        data.primaryBiome = SelectBiome(data.temperature, data.moisture, heightEstimate);
        
        // Sample nearby points for potential blending
        float offset = 10.0f;  // Sample distance
        BiomeType nearbyBiomes[4];
        nearbyBiomes[0] = SelectBiome(
            SampleTemperature(worldX + offset, worldZ),
            SampleMoisture(worldX + offset, worldZ), heightEstimate);
        nearbyBiomes[1] = SelectBiome(
            SampleTemperature(worldX - offset, worldZ),
            SampleMoisture(worldX - offset, worldZ), heightEstimate);
        nearbyBiomes[2] = SelectBiome(
            SampleTemperature(worldX, worldZ + offset),
            SampleMoisture(worldX, worldZ + offset), heightEstimate);
        nearbyBiomes[3] = SelectBiome(
            SampleTemperature(worldX, worldZ - offset),
            SampleMoisture(worldX, worldZ - offset), heightEstimate);
        
        // Find a different biome nearby for blending
        data.secondaryBiome = data.primaryBiome;
        data.blendFactor = 0.0f;
        
        for (int i = 0; i < 4; i++) {
            if (nearbyBiomes[i] != data.primaryBiome) {
                data.secondaryBiome = nearbyBiomes[i];
                // Crude blend factor based on climate distance
                float tempDist = std::abs(data.temperature - 0.5f);
                float moistDist = std::abs(data.moisture - 0.5f);
                data.blendFactor = std::max(0.0f, m_BlendDistance - std::min(tempDist, moistDist)) / m_BlendDistance;
                data.blendFactor *= 0.5f;  // Max 50% blend
                break;
            }
        }
        
        return data;
    }

    const BiomeSettings& BiomeGenerator::GetBiomeSettings(BiomeType type) {
        InitializeBiomeSettings();
        return s_BiomeSettings[static_cast<size_t>(type)];
    }

    glm::vec3 BiomeGenerator::GetTerrainColor(const BiomeData& biome, float normalizedHeight) const {
        const auto& primary = GetBiomeSettings(biome.primaryBiome);
        
        // Get color from primary biome based on height
        glm::vec3 primaryColor;
        if (normalizedHeight < primary.lowThreshold) {
            primaryColor = primary.lowColor;
        } else if (normalizedHeight < primary.midThreshold) {
            float t = (normalizedHeight - primary.lowThreshold) / (primary.midThreshold - primary.lowThreshold);
            primaryColor = glm::mix(primary.lowColor, primary.midColor, t);
        } else if (normalizedHeight < primary.highThreshold) {
            float t = (normalizedHeight - primary.midThreshold) / (primary.highThreshold - primary.midThreshold);
            primaryColor = glm::mix(primary.midColor, primary.highColor, t);
        } else {
            float t = std::min(1.0f, (normalizedHeight - primary.highThreshold) / (1.0f - primary.highThreshold));
            primaryColor = glm::mix(primary.highColor, primary.peakColor, t);
        }
        
        // Blend with secondary biome if at transition
        if (biome.blendFactor > 0.01f && biome.secondaryBiome != biome.primaryBiome) {
            const auto& secondary = GetBiomeSettings(biome.secondaryBiome);
            
            glm::vec3 secondaryColor;
            if (normalizedHeight < secondary.lowThreshold) {
                secondaryColor = secondary.lowColor;
            } else if (normalizedHeight < secondary.midThreshold) {
                float t = (normalizedHeight - secondary.lowThreshold) / (secondary.midThreshold - secondary.lowThreshold);
                secondaryColor = glm::mix(secondary.lowColor, secondary.midColor, t);
            } else if (normalizedHeight < secondary.highThreshold) {
                float t = (normalizedHeight - secondary.midThreshold) / (secondary.highThreshold - secondary.midThreshold);
                secondaryColor = glm::mix(secondary.midColor, secondary.highColor, t);
            } else {
                float t = std::min(1.0f, (normalizedHeight - secondary.highThreshold) / (1.0f - secondary.highThreshold));
                secondaryColor = glm::mix(secondary.highColor, secondary.peakColor, t);
            }
            
            primaryColor = glm::mix(primaryColor, secondaryColor, biome.blendFactor);
        }
        
        return primaryColor;
    }

    float BiomeGenerator::ModifyHeight(const BiomeData& biome, float baseHeight) const {
        const auto& primary = GetBiomeSettings(biome.primaryBiome);
        float modifiedHeight = baseHeight * primary.heightMultiplier;
        
        // Blend with secondary if transitioning
        if (biome.blendFactor > 0.01f && biome.secondaryBiome != biome.primaryBiome) {
            const auto& secondary = GetBiomeSettings(biome.secondaryBiome);
            float secondaryHeight = baseHeight * secondary.heightMultiplier;
            modifiedHeight = glm::mix(modifiedHeight, secondaryHeight, biome.blendFactor);
        }
        
        return modifiedHeight;
    }

    bool BiomeGenerator::ShouldSpawnTree(const BiomeData& biome, float normalizedHeight, float randomValue) const {
        const auto& settings = GetBiomeSettings(biome.primaryBiome);
        
        // Check height restrictions
        if (normalizedHeight < settings.minTreeHeight || normalizedHeight > settings.maxTreeHeight) {
            return false;
        }
        
        // Probability based on density (density of 1.0 = 1 per 100 sq units = 1% chance per check)
        float probability = settings.treeDensity * 0.01f;
        
        // Blend with secondary biome
        if (biome.blendFactor > 0.01f && biome.secondaryBiome != biome.primaryBiome) {
            const auto& secondary = GetBiomeSettings(biome.secondaryBiome);
            if (normalizedHeight >= secondary.minTreeHeight && normalizedHeight <= secondary.maxTreeHeight) {
                float secondaryProb = secondary.treeDensity * 0.01f;
                probability = glm::mix(probability, secondaryProb, biome.blendFactor);
            }
        }
        
        return randomValue < probability;
    }

    bool BiomeGenerator::ShouldSpawnRock(const BiomeData& biome, float normalizedHeight, float randomValue) const {
        const auto& settings = GetBiomeSettings(biome.primaryBiome);
        
        // Check height restrictions
        if (normalizedHeight < settings.minRockHeight || normalizedHeight > settings.maxRockHeight) {
            return false;
        }
        
        float probability = settings.rockDensity * 0.01f;
        
        // Blend with secondary biome
        if (biome.blendFactor > 0.01f && biome.secondaryBiome != biome.primaryBiome) {
            const auto& secondary = GetBiomeSettings(biome.secondaryBiome);
            if (normalizedHeight >= secondary.minRockHeight && normalizedHeight <= secondary.maxRockHeight) {
                float secondaryProb = secondary.rockDensity * 0.01f;
                probability = glm::mix(probability, secondaryProb, biome.blendFactor);
            }
        }
        
        return randomValue < probability;
    }

} // namespace Genesis
