#include "genesis/world/ChunkManager.h"
#include "genesis/renderer/Renderer.h"
#include "genesis/renderer/VulkanDevice.h"
#include "genesis/core/Log.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace Genesis {

    ChunkManager::ChunkManager() = default;
    
    ChunkManager::~ChunkManager() {
        Shutdown();
    }

    void ChunkManager::Initialize(VulkanDevice& device, const WorldSettings& settings) {
        m_Device = &device;
        m_Settings = settings;
        
        // Apply default terrain settings if not configured
        if (m_Settings.terrainSettings.heightScale == 0) {
            m_Settings.terrainSettings.heightScale = 8.0f;
            m_Settings.terrainSettings.noiseScale = 0.04f;
            m_Settings.terrainSettings.octaves = 5;
            m_Settings.terrainSettings.persistence = 0.5f;
            m_Settings.terrainSettings.lacunarity = 2.0f;
            m_Settings.terrainSettings.flatShading = true;
            m_Settings.terrainSettings.useHeightColors = true;
        }
        
        GEN_INFO("ChunkManager initialized (chunk size: {}, view distance: {})", 
            m_Settings.chunkSize, m_Settings.viewDistance);
    }

    void ChunkManager::Shutdown() {
        // Wait for GPU before cleanup
        if (m_Device) {
            m_Device->WaitIdle();
        }
        
        for (auto& [coord, chunk] : m_LoadedChunks) {
            chunk->Destroy();
        }
        m_LoadedChunks.clear();
        m_AllTreePositions.clear();
        m_AllRockPositions.clear();
        m_Device = nullptr;
    }

    glm::ivec2 ChunkManager::WorldToChunkCoord(float worldX, float worldZ) const {
        float chunkWorldSize = m_Settings.chunkSize * m_Settings.cellSize;
        int chunkX = static_cast<int>(std::floor(worldX / chunkWorldSize));
        int chunkZ = static_cast<int>(std::floor(worldZ / chunkWorldSize));
        return glm::ivec2(chunkX, chunkZ);
    }

    glm::vec3 ChunkManager::ChunkCoordToWorld(int chunkX, int chunkZ) const {
        float chunkWorldSize = m_Settings.chunkSize * m_Settings.cellSize;
        return glm::vec3(chunkX * chunkWorldSize, 0.0f, chunkZ * chunkWorldSize);
    }

    void ChunkManager::Update(const glm::vec3& cameraPosition) {
        if (!m_Device) return;
        
        glm::ivec2 cameraChunk = WorldToChunkCoord(cameraPosition.x, cameraPosition.z);
        
        // Only update if camera moved to a different chunk
        if (cameraChunk == m_LastCameraChunk) return;
        m_LastCameraChunk = cameraChunk;
        
        // Determine which chunks should be loaded
        std::vector<glm::ivec2> chunksToLoad;
        int viewDist = m_Settings.viewDistance;
        
        for (int z = -viewDist; z <= viewDist; z++) {
            for (int x = -viewDist; x <= viewDist; x++) {
                glm::ivec2 coord(cameraChunk.x + x, cameraChunk.y + z);
                
                // Check if not already loaded
                if (m_LoadedChunks.find(coord) == m_LoadedChunks.end()) {
                    chunksToLoad.push_back(coord);
                }
            }
        }
        
        // Determine which chunks should be unloaded
        std::vector<glm::ivec2> chunksToUnload;
        for (const auto& [coord, chunk] : m_LoadedChunks) {
            int dx = std::abs(coord.x - cameraChunk.x);
            int dz = std::abs(coord.y - cameraChunk.y);
            
            // Unload if outside view distance + 1 buffer
            if (dx > viewDist + 1 || dz > viewDist + 1) {
                chunksToUnload.push_back(coord);
            }
        }
        
        // Unload chunks
        for (const auto& coord : chunksToUnload) {
            UnloadChunk(coord.x, coord.y);
        }
        
        // Load chunks
        for (const auto& coord : chunksToLoad) {
            LoadChunk(coord.x, coord.y);
        }
        
        // Rebuild object positions if chunks changed
        if (!chunksToLoad.empty() || !chunksToUnload.empty()) {
            RebuildObjectPositions();
            GEN_DEBUG("Chunks updated: {} loaded, {} unloaded, {} total", 
                chunksToLoad.size(), chunksToUnload.size(), m_LoadedChunks.size());
        }
    }

    void ChunkManager::LoadChunk(int chunkX, int chunkZ) {
        glm::ivec2 coord(chunkX, chunkZ);
        
        auto chunk = std::make_unique<Chunk>(chunkX, chunkZ, m_Settings.chunkSize, m_Settings.cellSize);
        chunk->Generate(m_Settings.terrainSettings, m_Settings.seed);
        chunk->Upload(*m_Device);
        
        m_LoadedChunks[coord] = std::move(chunk);
    }

    void ChunkManager::UnloadChunk(int chunkX, int chunkZ) {
        glm::ivec2 coord(chunkX, chunkZ);
        
        auto it = m_LoadedChunks.find(coord);
        if (it != m_LoadedChunks.end()) {
            it->second->Destroy();
            m_LoadedChunks.erase(it);
        }
    }

    void ChunkManager::RebuildObjectPositions() {
        m_AllTreePositions.clear();
        m_AllRockPositions.clear();
        
        for (const auto& [coord, chunk] : m_LoadedChunks) {
            const auto& trees = chunk->GetTreePositions();
            const auto& rocks = chunk->GetRockPositions();
            
            m_AllTreePositions.insert(m_AllTreePositions.end(), trees.begin(), trees.end());
            m_AllRockPositions.insert(m_AllRockPositions.end(), rocks.begin(), rocks.end());
        }
    }

    void ChunkManager::Render(Renderer& renderer) {
        for (const auto& [coord, chunk] : m_LoadedChunks) {
            if (chunk->GetState() == ChunkState::Loaded && chunk->GetMesh()) {
                glm::vec3 worldPos = chunk->GetWorldPosition();
                glm::mat4 transform = glm::translate(glm::mat4(1.0f), worldPos);
                renderer.Draw(*chunk->GetMesh(), transform);
            }
        }
    }

    float ChunkManager::GetHeightAt(float worldX, float worldZ) const {
        Chunk* chunk = GetChunkAt(worldX, worldZ);
        if (chunk) {
            glm::vec3 chunkPos = chunk->GetWorldPosition();
            float localX = worldX - chunkPos.x;
            float localZ = worldZ - chunkPos.z;
            return chunk->GetHeightAt(localX, localZ);
        }
        return 0.0f;
    }

    Chunk* ChunkManager::GetChunkAt(float worldX, float worldZ) const {
        glm::ivec2 coord = WorldToChunkCoord(worldX, worldZ);
        return GetChunkByCoord(coord.x, coord.y);
    }

    Chunk* ChunkManager::GetChunkByCoord(int chunkX, int chunkZ) const {
        glm::ivec2 coord(chunkX, chunkZ);
        auto it = m_LoadedChunks.find(coord);
        if (it != m_LoadedChunks.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    void ChunkManager::SetViewDistance(int distance) {
        m_Settings.viewDistance = distance;
        // Force update on next frame
        m_LastCameraChunk = glm::ivec2(INT_MAX, INT_MAX);
    }

}
