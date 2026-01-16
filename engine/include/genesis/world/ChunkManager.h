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
        
        // Terrain settings (applied to all chunks)
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
        void SetViewDistance(int distance);

        // Stats
        int GetLoadedChunkCount() const { return static_cast<int>(m_LoadedChunks.size()); }
        
        // Access to all tree/rock positions for rendering
        const std::vector<glm::vec3>& GetAllTreePositions() const { return m_AllTreePositions; }
        const std::vector<glm::vec3>& GetAllRockPositions() const { return m_AllRockPositions; }

    private:
        // Convert world position to chunk coordinate
        glm::ivec2 WorldToChunkCoord(float worldX, float worldZ) const;
        
        // Convert chunk coordinate to world position (chunk origin)
        glm::vec3 ChunkCoordToWorld(int chunkX, int chunkZ) const;

        // Load/unload chunks
        void LoadChunk(int chunkX, int chunkZ);
        void UnloadChunk(int chunkX, int chunkZ);
        
        // Rebuild object position lists after chunk changes
        void RebuildObjectPositions();

    private:
        VulkanDevice* m_Device = nullptr;
        WorldSettings m_Settings;

        // Currently loaded chunks (key = chunk coordinate)
        std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, ChunkCoordHash> m_LoadedChunks;

        // Last camera chunk position (to detect when to update)
        glm::ivec2 m_LastCameraChunk{INT_MAX, INT_MAX};

        // Cached object positions from all loaded chunks
        std::vector<glm::vec3> m_AllTreePositions;
        std::vector<glm::vec3> m_AllRockPositions;
        
        // Terrain transform (for centering - chunks handle their own offset)
        glm::mat4 m_TerrainTransform{1.0f};
    };

}
