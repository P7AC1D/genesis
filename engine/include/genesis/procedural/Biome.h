#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Genesis {

    // Available biome types
    enum class BiomeType {
        Ocean,
        Beach,
        Plains,
        Forest,
        Desert,
        Tundra,
        Swamp,
        Mountains,
        Count
    };

    // Visual and gameplay settings for a biome
    struct BiomeSettings {
        std::string name;
        BiomeType type;
        
        // Terrain colors (blended based on height within biome)
        glm::vec3 lowColor;      // Color at lowest terrain
        glm::vec3 midColor;      // Color at mid terrain
        glm::vec3 highColor;     // Color at high terrain
        glm::vec3 peakColor;     // Color at peaks
        
        // Height color thresholds (0-1 normalized)
        float lowThreshold = 0.25f;
        float midThreshold = 0.5f;
        float highThreshold = 0.75f;
        
        // Terrain modification
        float heightMultiplier = 1.0f;   // Scales terrain height
        float roughness = 1.0f;          // Additional noise detail
        
        // Object densities (objects per 100 square units)
        float treeDensity = 1.0f;
        float rockDensity = 0.7f;
        float grassDensity = 0.0f;       // Future: grass/plants
        
        // Object height restrictions (normalized 0-1)
        float minTreeHeight = 0.25f;
        float maxTreeHeight = 0.75f;
        float minRockHeight = 0.1f;
        float maxRockHeight = 0.9f;
        
        // Water
        bool hasWater = true;
        glm::vec3 waterColor = glm::vec3(0.1f, 0.4f, 0.6f);
    };

    // Biome data for a specific point
    struct BiomeData {
        BiomeType primaryBiome;
        BiomeType secondaryBiome;  // For blending at transitions
        float blendFactor;          // 0 = 100% primary, 1 = 100% secondary
        float temperature;          // Raw temperature value
        float moisture;             // Raw moisture value
    };

    class SimplexNoise;

    // Generates and manages biomes based on climate simulation
    class BiomeGenerator {
    public:
        BiomeGenerator();
        ~BiomeGenerator() = default;
        
        // Initialize with world seed
        void Initialize(uint32_t worldSeed);
        
        // Get biome at world coordinates
        BiomeData GetBiomeAt(float worldX, float worldZ) const;
        
        // Get biome settings for a biome type
        static const BiomeSettings& GetBiomeSettings(BiomeType type);
        
        // Get interpolated color based on biome and height
        glm::vec3 GetTerrainColor(const BiomeData& biome, float normalizedHeight) const;
        
        // Get modified height based on biome
        float ModifyHeight(const BiomeData& biome, float baseHeight) const;
        
        // Check if objects should spawn at this location
        bool ShouldSpawnTree(const BiomeData& biome, float normalizedHeight, float randomValue) const;
        bool ShouldSpawnRock(const BiomeData& biome, float normalizedHeight, float randomValue) const;
        
        // Configuration
        void SetTemperatureScale(float scale) { m_TemperatureScale = scale; }
        void SetMoistureScale(float scale) { m_MoistureScale = scale; }
        void SetBlendDistance(float distance) { m_BlendDistance = distance; }
        
    private:
        // Climate noise sampling
        float SampleTemperature(float worldX, float worldZ) const;
        float SampleMoisture(float worldX, float worldZ) const;
        
        // Biome selection from climate values
        BiomeType SelectBiome(float temperature, float moisture, float height) const;
        
        // Initialize default biome settings
        static void InitializeBiomeSettings();
        
    private:
        uint32_t m_WorldSeed = 0;
        std::unique_ptr<SimplexNoise> m_TemperatureNoise;
        std::unique_ptr<SimplexNoise> m_MoistureNoise;
        
        // Noise scales (larger = slower transitions)
        float m_TemperatureScale = 0.002f;
        float m_MoistureScale = 0.003f;
        float m_BlendDistance = 0.1f;  // How much to blend at biome edges
        
        // Static biome definitions
        static std::vector<BiomeSettings> s_BiomeSettings;
        static bool s_BiomeSettingsInitialized;
    };

} // namespace Genesis
