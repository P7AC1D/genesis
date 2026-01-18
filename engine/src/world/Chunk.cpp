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

        // Initialize ocean mask and generate below-sea mask
        m_OceanMask.Initialize(m_Size, m_Size);
        const auto &heightmap = m_TerrainGenerator.GetCachedHeightmap();
        if (!heightmap.empty())
        {
            m_OceanMask.GenerateBelowSeaMask(heightmap, seaLevel);
        }

        // Generate water plane (will be updated after flood fill in ChunkManager)
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

    void Chunk::RegenerateWater(float seaLevel, bool useOceanMask)
    {
        // Clean up existing water mesh
        if (m_WaterMesh)
        {
            if (m_State == ChunkState::Loaded)
            {
                m_WaterMesh->Shutdown();
            }
            m_WaterMesh.reset();
            m_HasWater = false;
        }

        if (useOceanMask && m_OceanMask.IsFloodFillComplete())
        {
            GenerateWaterWithOceanMask(seaLevel);
        }
        else
        {
            GenerateWater(seaLevel);
        }
    }

    void Chunk::GenerateWaterWithOceanMask(float seaLevel)
    {
        // Generate water mesh only where ocean mask is true
        // This creates water only for ocean-connected cells, not inland lakes

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        glm::vec3 waterColor(0.1f, 0.4f, 0.6f);
        glm::vec3 lakeColor(0.15f, 0.45f, 0.55f); // Slightly different for lakes

        // Check if there's any ocean in this chunk
        bool hasOcean = false;
        for (int z = 0; z < m_Size && !hasOcean; z++)
        {
            for (int x = 0; x < m_Size && !hasOcean; x++)
            {
                if (m_OceanMask.IsOcean(x, z))
                {
                    hasOcean = true;
                }
            }
        }

        if (!hasOcean)
        {
            // No ocean in this chunk, optionally render lakes or skip water entirely
            m_HasWater = false;
            return;
        }

        // For simplicity, generate a full water plane for chunks with any ocean
        // A more sophisticated approach would generate only ocean cells
        float chunkWorldSize = m_Size * m_CellSize;
        int subdivisions = 8;
        float halfSize = chunkWorldSize * 0.5f;
        float cellWidth = chunkWorldSize / subdivisions;
        float cellDepth = chunkWorldSize / subdivisions;

        // Generate vertices
        for (int z = 0; z <= subdivisions; z++)
        {
            for (int x = 0; x <= subdivisions; x++)
            {
                Vertex v;
                v.Position = glm::vec3(
                    -halfSize + x * cellWidth + halfSize,
                    seaLevel,
                    -halfSize + z * cellDepth + halfSize);
                v.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                v.Color = waterColor;
                v.TexCoord = glm::vec2(
                    static_cast<float>(x) / subdivisions,
                    static_cast<float>(z) / subdivisions);
                vertices.push_back(v);
            }
        }

        // Generate indices (CCW winding order)
        for (int z = 0; z < subdivisions; z++)
        {
            for (int x = 0; x < subdivisions; x++)
            {
                int topLeft = z * (subdivisions + 1) + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * (subdivisions + 1) + x;
                int bottomRight = bottomLeft + 1;

                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }

        m_WaterMesh = std::make_unique<Mesh>(vertices, indices);
        m_HasWater = true;
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
