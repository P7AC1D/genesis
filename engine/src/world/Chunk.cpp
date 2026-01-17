#include "genesis/world/Chunk.h"
#include "genesis/renderer/VulkanDevice.h"
#include "genesis/procedural/Water.h"
#include "genesis/core/Log.h"
#include <random>
#include <cmath>

namespace Genesis
{

    Chunk::Chunk(int x, int z, int size, float cellSize)
        : m_ChunkX(x), m_ChunkZ(z), m_Size(size), m_CellSize(cellSize)
    {
    }

    Chunk::~Chunk()
    {
        Destroy();
    }

    glm::vec3 Chunk::GetWorldPosition() const
    {
        float worldX = m_ChunkX * m_Size * m_CellSize;
        float worldZ = m_ChunkZ * m_Size * m_CellSize;
        return glm::vec3(worldX, 0.0f, worldZ);
    }

    bool Chunk::ContainsWorldPosition(float worldX, float worldZ) const
    {
        glm::vec3 origin = GetWorldPosition();
        float chunkWorldSize = m_Size * m_CellSize;

        return worldX >= origin.x && worldX < origin.x + chunkWorldSize &&
               worldZ >= origin.z && worldZ < origin.z + chunkWorldSize;
    }

    void Chunk::Generate(const TerrainSettings &settings, uint32_t worldSeed, float seaLevel)
    {
        GEN_DEBUG("Chunk::Generate - heightScale: {}, noiseScale: {}, useWarp: {}",
                  settings.heightScale, settings.noiseScale, settings.useWarp);

        // Configure terrain generator
        TerrainSettings chunkSettings = settings;
        chunkSettings.width = m_Size;
        chunkSettings.depth = m_Size;
        chunkSettings.cellSize = m_CellSize;
        chunkSettings.seed = worldSeed;
        m_TerrainGenerator.SetSettings(chunkSettings);

        glm::vec3 worldPos = GetWorldPosition();

        // Generate terrain mesh
        auto terrainMesh = GenerateTerrainMesh(worldPos.x, worldPos.z, worldSeed);

        if (terrainMesh)
        {
            m_Mesh = std::make_unique<Mesh>(terrainMesh->GetVertices(), terrainMesh->GetIndices());
        }

        // Generate water plane
        GenerateWater(seaLevel);

        // Generate object positions
        GenerateObjects(worldSeed, seaLevel);

        m_State = ChunkState::Loading;
    }

    std::shared_ptr<Mesh> Chunk::GenerateTerrainMesh(float offsetX, float offsetZ, [[maybe_unused]] uint32_t worldSeed)
    {
        // Delegate all terrain generation to TerrainGenerator (single source of truth)
        return m_TerrainGenerator.GenerateAtOffset(offsetX, offsetZ);
    }

    void Chunk::GenerateObjects(uint32_t worldSeed, float seaLevel)
    {
        const auto &settings = m_TerrainGenerator.GetSettings();
        glm::vec3 worldPos = GetWorldPosition();
        float chunkWorldSize = m_Size * m_CellSize;

        uint32_t chunkSeed = worldSeed ^ (static_cast<uint32_t>(m_ChunkX * 198491317) ^
                                          static_cast<uint32_t>(m_ChunkZ * 6542989));
        std::mt19937 rng(chunkSeed);
        std::uniform_real_distribution<float> distPos(0.0f, chunkWorldSize);
        std::uniform_real_distribution<float> distProb(0.0f, 1.0f);

        float minHeight = settings.baseHeight;
        float maxHeight = settings.baseHeight + settings.heightScale;
        float heightRange = maxHeight - minHeight;

        int numSamples = static_cast<int>((chunkWorldSize * chunkWorldSize) / 25.0f);
        m_TreePositions.reserve(numSamples / 4);
        m_RockPositions.reserve(numSamples / 6);

        for (int i = 0; i < numSamples; i++)
        {
            float localX = distPos(rng);
            float localZ = distPos(rng);
            float height = GetHeightAtLocal(localX, localZ);

            if (height <= seaLevel + 0.5f)
                continue;

            float worldX = worldPos.x + localX;
            float worldZ = worldPos.z + localZ;
            float normalizedHeight = glm::clamp((height - minHeight) / heightRange, 0.0f, 1.0f);
            float randomValue = distProb(rng);

            // Trees in grass zone
            if (normalizedHeight > settings.sandLevel && normalizedHeight < settings.rockLevel)
            {
                if (randomValue < 0.01f)
                {
                    m_TreePositions.push_back(glm::vec3(worldX, height, worldZ));
                }
            }

            // Rocks everywhere above water
            randomValue = distProb(rng);
            if (normalizedHeight > settings.waterLevel * 0.5f)
            {
                if (randomValue < 0.007f)
                {
                    m_RockPositions.push_back(glm::vec3(worldX, height + 0.1f, worldZ));
                }
            }
        }
    }

    void Chunk::GenerateWater(float seaLevel)
    {
        float chunkWorldSize = m_Size * m_CellSize;

        auto waterMesh = WaterGenerator::GeneratePlane(
            chunkWorldSize * 0.5f,
            chunkWorldSize * 0.5f,
            chunkWorldSize,
            chunkWorldSize,
            8,
            seaLevel);

        if (waterMesh)
        {
            m_WaterMesh = std::make_unique<Mesh>(waterMesh->GetVertices(), waterMesh->GetIndices());
            m_HasWater = true;
        }
    }

    float Chunk::GetHeightAtLocal(float localX, float localZ) const
    {
        // Use cached heightmap from TerrainGenerator if available
        const auto &heightmap = m_TerrainGenerator.GetCachedHeightmap();
        if (!heightmap.empty())
        {
            return m_TerrainGenerator.GetHeightAt(localX, localZ);
        }

        // Fallback: sample directly from noise (before generation)
        glm::vec3 worldPos = GetWorldPosition();
        float worldX = worldPos.x + localX;
        float worldZ = worldPos.z + localZ;
        return m_TerrainGenerator.SampleRawHeight(worldX, worldZ);
    }

    float Chunk::GetHeightAt(float localX, float localZ) const
    {
        return GetHeightAtLocal(localX, localZ);
    }

    void Chunk::Upload(VulkanDevice &device)
    {
        if (m_Mesh && m_State == ChunkState::Loading)
        {
            auto cpuMesh = std::move(m_Mesh);
            m_Mesh = std::make_unique<Mesh>(device, cpuMesh->GetVertices(), cpuMesh->GetIndices());

            if (m_WaterMesh && m_HasWater)
            {
                auto cpuWater = std::move(m_WaterMesh);
                m_WaterMesh = std::make_unique<Mesh>(device, cpuWater->GetVertices(), cpuWater->GetIndices());
            }

            m_State = ChunkState::Loaded;
        }
    }

    void Chunk::Unload()
    {
        if (m_State == ChunkState::Loaded)
        {
            if (m_Mesh)
            {
                m_Mesh->Shutdown();
                m_Mesh.reset();
            }
            if (m_WaterMesh)
            {
                m_WaterMesh->Shutdown();
                m_WaterMesh.reset();
            }
            m_State = ChunkState::Unloaded;
        }
    }

    void Chunk::Destroy()
    {
        if (m_Mesh)
        {
            if (m_State == ChunkState::Loaded)
            {
                m_Mesh->Shutdown();
            }
            m_Mesh.reset();
        }
        if (m_WaterMesh)
        {
            if (m_State == ChunkState::Loaded)
            {
                m_WaterMesh->Shutdown();
            }
            m_WaterMesh.reset();
        }
        m_TreePositions.clear();
        m_RockPositions.clear();
        m_State = ChunkState::Unloaded;
    }

}
