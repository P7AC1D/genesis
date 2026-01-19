#include "genesis/world/ChunkManager.h"
#include "genesis/renderer/Renderer.h"
#include "genesis/renderer/VulkanDevice.h"
#include "genesis/core/Log.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace Genesis
{

    ChunkManager::ChunkManager() = default;

    ChunkManager::~ChunkManager()
    {
        Shutdown();
    }

    void ChunkManager::Initialize(VulkanDevice &device, const WorldSettings &settings)
    {
        m_Device = &device;
        m_Settings = settings;

        // Apply default terrain settings if not configured
        if (m_Settings.terrainSettings.heightScale == 0)
        {
            m_Settings.terrainSettings.heightScale = 8.0f;
            m_Settings.terrainSettings.noiseScale = 0.04f;
            m_Settings.terrainSettings.octaves = 5;
            m_Settings.terrainSettings.persistence = 0.5f;
            m_Settings.terrainSettings.lacunarity = 2.0f;
            m_Settings.terrainSettings.flatShading = true;
            m_Settings.terrainSettings.useBiomeColors = true;
        }

        GEN_INFO("ChunkManager initialized (chunk size: {}, view distance: {})",
                 m_Settings.chunkSize, m_Settings.viewDistance);
    }

    void ChunkManager::Shutdown()
    {
        if (m_Device)
        {
            m_Device->WaitIdle();
        }

        for (auto &[coord, chunk] : m_LoadedChunks)
        {
            chunk->Destroy();
        }
        m_LoadedChunks.clear();
        m_AllTreePositions.clear();
        m_AllRockPositions.clear();
        m_Device = nullptr;
    }

    glm::ivec2 ChunkManager::WorldToChunkCoord(float worldX, float worldZ) const
    {
        float chunkWorldSize = m_Settings.chunkSize * m_Settings.cellSize;
        int chunkX = static_cast<int>(std::floor(worldX / chunkWorldSize));
        int chunkZ = static_cast<int>(std::floor(worldZ / chunkWorldSize));
        return glm::ivec2(chunkX, chunkZ);
    }

    glm::vec3 ChunkManager::ChunkCoordToWorld(int chunkX, int chunkZ) const
    {
        float chunkWorldSize = m_Settings.chunkSize * m_Settings.cellSize;
        return glm::vec3(chunkX * chunkWorldSize, 0.0f, chunkZ * chunkWorldSize);
    }

    void ChunkManager::Update(const glm::vec3 &cameraPosition)
    {
        if (!m_Device)
            return;

        // Process deferred regeneration at frame start (before rendering)
        if (m_NeedsRegeneration)
        {
            PerformRegeneration();
            m_NeedsRegeneration = false;
        }

        glm::ivec2 cameraChunk = WorldToChunkCoord(cameraPosition.x, cameraPosition.z);

        if (cameraChunk == m_LastCameraChunk)
            return;
        m_LastCameraChunk = cameraChunk;

        std::vector<glm::ivec2> chunksToLoad;
        int viewDist = m_Settings.viewDistance;

        for (int z = -viewDist; z <= viewDist; z++)
        {
            for (int x = -viewDist; x <= viewDist; x++)
            {
                glm::ivec2 coord(cameraChunk.x + x, cameraChunk.y + z);

                if (m_LoadedChunks.find(coord) == m_LoadedChunks.end())
                {
                    chunksToLoad.push_back(coord);
                }
            }
        }

        std::vector<glm::ivec2> chunksToUnload;
        for (const auto &[coord, chunk] : m_LoadedChunks)
        {
            int dx = std::abs(coord.x - cameraChunk.x);
            int dz = std::abs(coord.y - cameraChunk.y);

            if (dx > viewDist + 1 || dz > viewDist + 1)
            {
                chunksToUnload.push_back(coord);
            }
        }

        if (!chunksToUnload.empty())
        {
            m_Device->WaitIdle();
        }

        for (const auto &coord : chunksToUnload)
        {
            UnloadChunk(coord.x, coord.y);
        }

        for (const auto &coord : chunksToLoad)
        {
            LoadChunk(coord.x, coord.y);
        }

        if (!chunksToLoad.empty() || !chunksToUnload.empty())
        {
            RebuildObjectPositions();
            GEN_DEBUG("Chunks updated: {} loaded, {} unloaded, {} total",
                      chunksToLoad.size(), chunksToUnload.size(), m_LoadedChunks.size());
        }
    }

    void ChunkManager::LoadChunk(int chunkX, int chunkZ)
    {
        glm::ivec2 coord(chunkX, chunkZ);

        auto chunk = std::make_unique<Chunk>(chunkX, chunkZ, m_Settings.chunkSize, m_Settings.cellSize);
        float seaLevel = m_Settings.waterEnabled ? m_Settings.seaLevel : -1000.0f;

        // Only compute full hydrology for chunks within hydrologyDistance
        int dx = std::abs(chunkX - m_LastCameraChunk.x);
        int dz = std::abs(chunkZ - m_LastCameraChunk.y);
        bool computeHydrology = (dx <= m_Settings.hydrologyDistance && dz <= m_Settings.hydrologyDistance);

        GEN_DEBUG("LoadChunk ({}, {}) - hydrology: {}",
                  chunkX, chunkZ, computeHydrology);

        chunk->Generate(m_Settings.terrainSettings, m_Settings.seed, seaLevel, computeHydrology);
        chunk->Upload(*m_Device);

        m_LoadedChunks[coord] = std::move(chunk);
    }

    void ChunkManager::UnloadChunk(int chunkX, int chunkZ)
    {
        glm::ivec2 coord(chunkX, chunkZ);

        auto it = m_LoadedChunks.find(coord);
        if (it != m_LoadedChunks.end())
        {
            it->second->Destroy();
            m_LoadedChunks.erase(it);
        }
    }

    void ChunkManager::RebuildObjectPositions()
    {
        m_AllTreePositions.clear();
        m_AllRockPositions.clear();

        for (const auto &[coord, chunk] : m_LoadedChunks)
        {
            const auto &trees = chunk->GetTreePositions();
            const auto &rocks = chunk->GetRockPositions();

            m_AllTreePositions.insert(m_AllTreePositions.end(), trees.begin(), trees.end());
            m_AllRockPositions.insert(m_AllRockPositions.end(), rocks.begin(), rocks.end());
        }
    }

    void ChunkManager::Render(Renderer &renderer)
    {
        for (const auto &[coord, chunk] : m_LoadedChunks)
        {
            if (chunk->GetState() == ChunkState::Loaded && chunk->GetMesh())
            {
                glm::vec3 worldPos = chunk->GetWorldPosition();
                glm::mat4 transform = glm::translate(glm::mat4(1.0f), worldPos);
                renderer.Draw(*chunk->GetMesh(), transform);
            }
        }

        if (m_Settings.waterEnabled)
        {
            // Render ocean water planes
            for (const auto &[coord, chunk] : m_LoadedChunks)
            {
                if (chunk->GetState() == ChunkState::Loaded && chunk->HasWater() && chunk->GetWaterMesh())
                {
                    glm::vec3 worldPos = chunk->GetWorldPosition();
                    glm::mat4 transform = glm::translate(glm::mat4(1.0f), worldPos);
                    renderer.DrawWater(*chunk->GetWaterMesh(), transform);
                }
            }

            // Render lake meshes
            for (const auto &[coord, chunk] : m_LoadedChunks)
            {
                if (chunk->GetState() == ChunkState::Loaded && chunk->HasLakes() && chunk->GetLakeMesh())
                {
                    glm::vec3 worldPos = chunk->GetWorldPosition();
                    glm::mat4 transform = glm::translate(glm::mat4(1.0f), worldPos);
                    renderer.DrawWater(*chunk->GetLakeMesh(), transform);
                }
            }

            // Render river meshes
            for (const auto &[coord, chunk] : m_LoadedChunks)
            {
                if (chunk->GetState() == ChunkState::Loaded && chunk->HasRivers() && chunk->GetRiverMesh())
                {
                    glm::vec3 worldPos = chunk->GetWorldPosition();
                    glm::mat4 transform = glm::translate(glm::mat4(1.0f), worldPos);
                    renderer.DrawWater(*chunk->GetRiverMesh(), transform);
                }
            }
        }
    }

    float ChunkManager::GetHeightAt(float worldX, float worldZ) const
    {
        Chunk *chunk = GetChunkAt(worldX, worldZ);
        if (chunk)
        {
            glm::vec3 chunkPos = chunk->GetWorldPosition();
            float localX = worldX - chunkPos.x;
            float localZ = worldZ - chunkPos.z;
            return chunk->GetHeightAt(localX, localZ);
        }
        return 0.0f;
    }

    Chunk *ChunkManager::GetChunkAt(float worldX, float worldZ) const
    {
        glm::ivec2 coord = WorldToChunkCoord(worldX, worldZ);
        return GetChunkByCoord(coord.x, coord.y);
    }

    Chunk *ChunkManager::GetChunkByCoord(int chunkX, int chunkZ) const
    {
        glm::ivec2 coord(chunkX, chunkZ);
        auto it = m_LoadedChunks.find(coord);
        if (it != m_LoadedChunks.end())
        {
            return it->second.get();
        }
        return nullptr;
    }

    void ChunkManager::SetViewDistance(int distance)
    {
        m_Settings.viewDistance = distance;
        m_LastCameraChunk = glm::ivec2(INT_MAX, INT_MAX);
    }

    void ChunkManager::RegenerateAllChunks()
    {
        // Defer regeneration to next Update() call to avoid Vulkan sync issues
        GEN_INFO("RegenerateAllChunks: scheduling deferred regeneration");
        GEN_INFO("New settings: heightScale={}, noiseScale={}, octaves={}",
                 m_Settings.terrainSettings.heightScale,
                 m_Settings.terrainSettings.noiseScale,
                 m_Settings.terrainSettings.octaves);
        m_NeedsRegeneration = true;
    }

    void ChunkManager::PerformRegeneration()
    {
        if (!m_Device)
        {
            GEN_ERROR("PerformRegeneration: m_Device is null!");
            return;
        }

        GEN_INFO("PerformRegeneration: starting...");

        // Recompute absolute sea level from normalized value
        m_Settings.ComputeSeaLevel();
        GEN_INFO("Sea level: {} (normalized: {})", m_Settings.seaLevel, m_Settings.seaLevelNormalized);

        m_Device->WaitIdle();

        std::vector<glm::ivec2> chunksToRegenerate;
        for (const auto &[coord, chunk] : m_LoadedChunks)
        {
            chunksToRegenerate.push_back(coord);
        }

        GEN_INFO("Unloading {} chunks...", chunksToRegenerate.size());
        for (const auto &coord : chunksToRegenerate)
        {
            UnloadChunk(coord.x, coord.y);
        }

        GEN_INFO("Reloading {} chunks...", chunksToRegenerate.size());
        for (const auto &coord : chunksToRegenerate)
        {
            LoadChunk(coord.x, coord.y);
        }

        // Perform ocean flood fill across all loaded chunks
        if (m_Settings.useOceanMask && m_Settings.waterEnabled)
        {
            PerformOceanFloodFill();
        }

        RebuildObjectPositions();
        GEN_INFO("Regenerated {} chunks with updated settings", chunksToRegenerate.size());
    }

    void ChunkManager::UpdateTerrainSettings(const TerrainSettings &settings)
    {
        m_Settings.terrainSettings = settings;
        m_Settings.ComputeSeaLevel(); // Recompute when terrain settings change
        RegenerateAllChunks();
    }

    void ChunkManager::UpdateWorldSettings(float seaLevel, bool waterEnabled)
    {
        m_Settings.seaLevel = seaLevel;
        // Update normalized value to match absolute value
        if (m_Settings.terrainSettings.heightScale > 0)
        {
            m_Settings.seaLevelNormalized = (seaLevel - m_Settings.terrainSettings.baseHeight) / m_Settings.terrainSettings.heightScale;
        }
        m_Settings.waterEnabled = waterEnabled;
        RegenerateAllChunks();
    }

    void ChunkManager::UpdateSeaLevelNormalized(float seaLevelNormalized)
    {
        m_Settings.seaLevelNormalized = seaLevelNormalized;
        m_Settings.ComputeSeaLevel();
        RegenerateAllChunks();
    }

    bool ChunkManager::IsChunkAtWorldBoundary(int chunkX, int chunkZ, ChunkEdge edge) const
    {
        // For an infinite world, chunks at the boundary of currently loaded chunks
        // are considered "at world boundary" for flood fill purposes.
        // This allows ocean to flood in from edges of the visible world.

        int viewDist = m_Settings.viewDistance;

        switch (edge)
        {
        case ChunkEdge::NegativeX:
            return (chunkX <= m_LastCameraChunk.x - viewDist);
        case ChunkEdge::PositiveX:
            return (chunkX >= m_LastCameraChunk.x + viewDist);
        case ChunkEdge::NegativeZ:
            return (chunkZ <= m_LastCameraChunk.y - viewDist);
        case ChunkEdge::PositiveZ:
            return (chunkZ >= m_LastCameraChunk.y + viewDist);
        }
        return false;
    }

    void ChunkManager::PerformOceanFloodFill()
    {
        GEN_DEBUG("PerformOceanFloodFill: starting ocean mask computation");

        // Step 1: Run initial flood fill on all chunks from world boundaries
        for (auto &[coord, chunk] : m_LoadedChunks)
        {
            int cx = coord.x;
            int cz = coord.y;

            auto isAtBoundary = [this, cx, cz](ChunkEdge edge)
            {
                return IsChunkAtWorldBoundary(cx, cz, edge);
            };

            chunk->GetOceanMask().FloodFillFromBoundary(isAtBoundary, nullptr);
        }

        // Step 2: Propagate ocean connectivity between neighboring chunks
        // Repeat until no changes occur (convergence)
        bool changed = true;
        int iterations = 0;
        const int maxIterations = 10; // Safety limit

        while (changed && iterations < maxIterations)
        {
            changed = false;
            iterations++;

            for (auto &[coord, chunk] : m_LoadedChunks)
            {
                OceanMask &mask = chunk->GetOceanMask();
                const ChunkOceanBoundary &boundary = mask.GetBoundary();

                // Check each neighbor and propagate
                // -X neighbor
                if (auto *neighbor = GetChunkByCoord(coord.x - 1, coord.y))
                {
                    const auto &neighborBoundary = neighbor->GetOceanMask().GetBoundary();
                    for (int z = 0; z < mask.GetDepth(); z++)
                    {
                        if (z < static_cast<int>(neighborBoundary.posX.size()) &&
                            neighborBoundary.posX[z] && !mask.IsOcean(0, z) && mask.IsBelowSeaLevel(0, z))
                        {
                            mask.PropagateFromNeighbor(ChunkEdge::NegativeX, neighborBoundary.posX);
                            changed = true;
                            break;
                        }
                    }
                }

                // +X neighbor
                if (auto *neighbor = GetChunkByCoord(coord.x + 1, coord.y))
                {
                    const auto &neighborBoundary = neighbor->GetOceanMask().GetBoundary();
                    for (int z = 0; z < mask.GetDepth(); z++)
                    {
                        if (z < static_cast<int>(neighborBoundary.negX.size()) &&
                            neighborBoundary.negX[z] && !mask.IsOcean(mask.GetWidth() - 1, z) &&
                            mask.IsBelowSeaLevel(mask.GetWidth() - 1, z))
                        {
                            mask.PropagateFromNeighbor(ChunkEdge::PositiveX, neighborBoundary.negX);
                            changed = true;
                            break;
                        }
                    }
                }

                // -Z neighbor
                if (auto *neighbor = GetChunkByCoord(coord.x, coord.y - 1))
                {
                    const auto &neighborBoundary = neighbor->GetOceanMask().GetBoundary();
                    for (int x = 0; x < mask.GetWidth(); x++)
                    {
                        if (x < static_cast<int>(neighborBoundary.posZ.size()) &&
                            neighborBoundary.posZ[x] && !mask.IsOcean(x, 0) && mask.IsBelowSeaLevel(x, 0))
                        {
                            mask.PropagateFromNeighbor(ChunkEdge::NegativeZ, neighborBoundary.posZ);
                            changed = true;
                            break;
                        }
                    }
                }

                // +Z neighbor
                if (auto *neighbor = GetChunkByCoord(coord.x, coord.y + 1))
                {
                    const auto &neighborBoundary = neighbor->GetOceanMask().GetBoundary();
                    for (int x = 0; x < mask.GetWidth(); x++)
                    {
                        if (x < static_cast<int>(neighborBoundary.negZ.size()) &&
                            neighborBoundary.negZ[x] && !mask.IsOcean(x, mask.GetDepth() - 1) &&
                            mask.IsBelowSeaLevel(x, mask.GetDepth() - 1))
                        {
                            mask.PropagateFromNeighbor(ChunkEdge::PositiveZ, neighborBoundary.negZ);
                            changed = true;
                            break;
                        }
                    }
                }
            }
        }

        GEN_DEBUG("PerformOceanFloodFill: converged after {} iterations", iterations);

        // Step 3: Regenerate water meshes using ocean mask
        for (auto &[coord, chunk] : m_LoadedChunks)
        {
            chunk->RegenerateWater(m_Settings.seaLevel, m_Settings.useOceanMask);

            // Re-upload water mesh if chunk is loaded
            if (chunk->GetState() == ChunkState::Loaded && chunk->HasWater() && chunk->GetWaterMesh())
            {
                // The mesh needs to be uploaded to GPU
                auto cpuWater = std::make_unique<Mesh>(
                    chunk->GetWaterMesh()->GetVertices(),
                    chunk->GetWaterMesh()->GetIndices());
                chunk->m_WaterMesh->Shutdown();
                chunk->m_WaterMesh = std::make_unique<Mesh>(*m_Device, cpuWater->GetVertices(), cpuWater->GetIndices());
            }
        }
    }

}
