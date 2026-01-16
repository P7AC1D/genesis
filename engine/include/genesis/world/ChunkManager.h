#pragma once

#include "genesis/world/Chunk.h"
#include "genesis/procedural/TerrainGenerator.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Genesis {

    class VulkanDevice;
    class Renderer;

    struct WorldSettings {
        // Chunk settings
        int chunkSize = 32;           // Grid cells per chunk side
        float cellSize = 1.0f;        // World units per cell
        int viewDistance = 3;         // Chunks to load in each direction
        
        // World seed
        uint32_t seed = 12345;
        
        // Water settings
        float seaLevel = 2.0f;        // Height of water surface
        bool waterEnabled = true;
        
        // Terrain settings
        TerrainSettings terrainSettings;
    };

    // Hash function for chunk coordinates
    struct ChunkCoordHash {
        size_t operator()(const glm::ivec2& coord) const {
            return std::hash<int>()(coord.x) ^ (std::hash<int>()(coord.y) << 16);
        }
    };

    class ChunkManager {
    public:
        ChunkManager();
        ~ChunkManager();

        void Initialize(VulkanDevice& device, const WorldSettings& settings);
        void Shutdown();

        // Update chunks based on camera position
        void Update(const glm::vec3& cameraPosition);

        // Render all loaded chunks
        void Render(Renderer& renderer);

        // Get height at world position (samples from appropriate chunk)
        float GetHeightAt(float worldX, float worldZ) const;

        // Get chunk at world position
        Chunk* GetChunkAt(float worldX, float worldZ) const;
        Chunk* GetChunkByCoord(int chunkX, int chunkZ) const;

        // Settings
        const WorldSettings& GetSettings() const { return m_Settings; }
        WorldSettings& GetSettings() { return m_Settings; }
        void SetViewDistance(int distance);
        void RegenerateAllChunks();
        void UpdateTerrainSettings(const TerrainSettings& settings);
        void UpdateWorldSettings(float seaLevel, bool waterEnabled);
        
        // Request chunk regeneration (deferred to next Update call)
        void RequestRegeneration() { m_NeedsRegeneration = true; }
        bool NeedsRegeneration() const { return m_NeedsRegeneration; }

        // Stats
        int GetLoadedChunkCount() const { return static_cast<int>(m_LoadedChunks.size()); }
        
        // Access to all tree/rock positions for rendering
        const std::vector<glm::vec3>& GetAllTreePositions() const { return m_AllTreePositions; }
        const std::vector<glm::vec3>& GetAllRockPositions() const { return m_AllRockPositions; }

    private:
        glm::ivec2 WorldToChunkCoord(float worldX, float worldZ) const;
        glm::vec3 ChunkCoordToWorld(int chunkX, int chunkZ) const;
        void LoadChunk(int chunkX, int chunkZ);
        void UnloadChunk(int chunkX, int chunkZ);
        void RebuildObjectPositions();
        void PerformRegeneration();

    private:
        VulkanDevice* m_Device = nullptr;
        WorldSettings m_Settings;

        std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, ChunkCoordHash> m_LoadedChunks;
        glm::ivec2 m_LastCameraChunk{INT_MAX, INT_MAX};

        std::vector<glm::vec3> m_AllTreePositions;
        std::vector<glm::vec3> m_AllRockPositions;
        
        glm::mat4 m_TerrainTransform{1.0f};
        bool m_NeedsRegeneration = false;
    };

}
