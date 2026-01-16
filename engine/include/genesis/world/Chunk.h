#pragma once

#include "genesis/renderer/Mesh.h"
#include "genesis/procedural/TerrainGenerator.h"
#include <glm/glm.hpp>
#include <memory>

namespace Genesis {

    enum class ChunkState {
        Unloaded,
        Loading,
        Loaded,
        Unloading
    };

    class VulkanDevice;

    class Chunk {
    public:
        Chunk(int x, int z, int size, float cellSize);
        ~Chunk();

        // Generate terrain for this chunk
        void Generate(const TerrainSettings& baseSettings, uint32_t worldSeed);
        
        // Upload mesh to GPU
        void Upload(VulkanDevice& device);
        
        // Unload from GPU (keep CPU data for quick reload)
        void Unload();
        
        // Full cleanup
        void Destroy();

        // Getters
        int GetChunkX() const { return m_ChunkX; }
        int GetChunkZ() const { return m_ChunkZ; }
        glm::vec2 GetChunkCoord() const { return glm::vec2(m_ChunkX, m_ChunkZ); }
        glm::vec3 GetWorldPosition() const;
        ChunkState GetState() const { return m_State; }
        Mesh* GetMesh() const { return m_Mesh.get(); }
        
        // Get height at local position within chunk
        float GetHeightAt(float localX, float localZ) const;
        
        // Check if world position is within this chunk
        bool ContainsWorldPosition(float worldX, float worldZ) const;
        
        // Object positions (world coordinates)
        const std::vector<glm::vec3>& GetTreePositions() const { return m_TreePositions; }
        const std::vector<glm::vec3>& GetRockPositions() const { return m_RockPositions; }

    private:
        // Internal generation helpers
        std::shared_ptr<Mesh> GenerateWithWorldOffset(float offsetX, float offsetZ);
        void GenerateObjects(uint32_t worldSeed);
        float GetHeightAtLocal(float localX, float localZ) const;

        int m_ChunkX;  // Chunk coordinate (not world position)
        int m_ChunkZ;
        int m_Size;    // Grid cells per side
        float m_CellSize;
        
        ChunkState m_State = ChunkState::Unloaded;
        
        std::unique_ptr<Mesh> m_Mesh;
        TerrainGenerator m_TerrainGenerator;
        
        // Object positions within this chunk (world coordinates)
        std::vector<glm::vec3> m_TreePositions;
        std::vector<glm::vec3> m_RockPositions;

        friend class ChunkManager;
    };

}
