#include "genesis/world/Chunk.h"
#include "genesis/renderer/VulkanDevice.h"
#include "genesis/procedural/Water.h"
#include "genesis/procedural/Noise.h"
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

    void Chunk::Generate(const TerrainSettings &settings, uint32_t worldSeed, float seaLevel, bool computeHydrology)
    {
        GEN_DEBUG("Chunk::Generate - heightScale: {}, noiseScale: {}, useWarp: {}, hydrology: {}",
                  settings.heightScale, settings.noiseScale, settings.useWarp, computeHydrology);

        // Configure terrain generator
        TerrainSettings chunkSettings = settings;
        chunkSettings.width = m_Size;
        chunkSettings.depth = m_Size;
        chunkSettings.cellSize = m_CellSize;
        chunkSettings.seed = worldSeed;
        m_TerrainGenerator.SetSettings(chunkSettings);

        glm::vec3 worldPos = GetWorldPosition();

        // Step 1: Generate heightmap first (needed by all subsequent systems)
        m_TerrainGenerator.GenerateHeightmapAtOffset(worldPos.x, worldPos.z);
        const auto &heightmap = m_TerrainGenerator.GetCachedHeightmap();

        // Step 2: Run hydrology pipeline only if requested (expensive for distant chunks)
        if (computeHydrology)
        {
            GenerateHydrology(seaLevel);
            GenerateClimateAndMaterials(seaLevel);
        }
        else
        {
            // For distant chunks, generate lightweight climate/biome data without full hydrology
            GenerateClimateAndBiomes(seaLevel);
        }

        // Step 3: Build terrain mesh (uses biome-based coloring)
        auto terrainMesh = GenerateTerrainMesh(worldPos.x, worldPos.z, worldSeed);

        if (terrainMesh)
        {
            m_Mesh = std::make_unique<Mesh>(terrainMesh->GetVertices(), terrainMesh->GetIndices());
        }

        // Initialize ocean mask and generate below-sea mask
        m_OceanMask.Initialize(m_Size, m_Size);
        if (!heightmap.empty())
        {
            m_OceanMask.GenerateBelowSeaMask(heightmap, seaLevel);
        }

        // Step 4: Generate water meshes for lakes and rivers only if hydrology computed
        if (computeHydrology)
        {
            GenerateWaterMeshes(seaLevel);
        }

        // Only generate ocean water plane if there's terrain below sea level
        // Check if any cells in the ocean mask are marked as below sea level
        if (m_OceanMask.HasAnyBelowSeaLevel())
        {
            GenerateWater(seaLevel);
        }

        // Generate object positions
        GenerateObjects(worldSeed, seaLevel);

        m_State = ChunkState::Loading;
    }

    std::shared_ptr<Mesh> Chunk::GenerateTerrainMesh(float offsetX, float offsetZ, [[maybe_unused]] uint32_t worldSeed)
    {
        // Delegate all terrain generation to TerrainGenerator (single source of truth)
        // Now uses biome classifier data for coloring if available
        return m_TerrainGenerator.GenerateAtOffset(offsetX, offsetZ, &m_MaterialBlender, &m_BiomeClassifier);
    }

    void Chunk::GenerateHydrology(float seaLevel)
    {
        const auto &heightmap = m_TerrainGenerator.GetCachedHeightmap();
        if (heightmap.empty())
        {
            GEN_WARN("Chunk::GenerateHydrology - No heightmap available");
            return;
        }

        int gridWidth = m_Size + 1;
        int gridDepth = m_Size + 1;

        // Step 1: Compute drainage graph (flow directions and accumulation)
        m_DrainageGraph.Compute(heightmap, gridWidth, gridDepth, m_CellSize, seaLevel);

        // Step 2: Generate rivers from drainage data
        m_RiverGenerator.Configure(0.5f, m_CellSize); // Default river strength
        m_RiverGenerator.Generate(m_DrainageGraph, heightmap, seaLevel);

        // Step 3: Generate lakes from drainage pits
        m_LakeGenerator.Generate(m_DrainageGraph, heightmap, seaLevel);

        // Step 4: Compute unified hydrology data
        m_HydrologyGenerator.Compute(m_DrainageGraph, m_RiverGenerator, m_LakeGenerator, heightmap, m_CellSize);
        m_HydrologyData = m_HydrologyGenerator.GetData();

        GEN_DEBUG("Chunk::GenerateHydrology - Rivers: {}, Lakes: {}",
                  m_RiverGenerator.GetNetwork().rivers.size(),
                  m_LakeGenerator.GetNetwork().lakes.size());
    }

    void Chunk::GenerateClimateAndMaterials(float seaLevel)
    {
        const auto &heightmap = m_TerrainGenerator.GetCachedHeightmap();
        const auto &settings = m_TerrainGenerator.GetSettings();

        if (heightmap.empty())
        {
            GEN_WARN("Chunk::GenerateClimateAndMaterials - No heightmap available");
            return;
        }

        glm::vec3 worldPos = GetWorldPosition();

        // Initialize climate generator with noise
        SimplexNoise noise(settings.seed);
        ClimateSettings climateSettings;
        m_ClimateGenerator.Initialize(&noise, climateSettings);

        // Grid dimensions (heightmap is (width+1) x (depth+1))
        int gridWidth = m_Size + 1;
        int gridDepth = m_Size + 1;

        // Use simplified climate generation for biome classification
        // This ensures seamless chunk boundaries (no chunk-local rain shadow effects)
        m_ClimateGenerator.Generate(
            heightmap,
            gridWidth,
            gridDepth,
            seaLevel,
            settings.heightScale,
            m_CellSize,
            worldPos.x,
            worldPos.z);

        // Classify biomes based on climate data
        m_BiomeClassifier.Classify(m_ClimateGenerator.GetData());

        // Compute material weights from climate and hydrology
        m_MaterialBlender.Compute(
            heightmap,
            m_HydrologyData,
            m_ClimateGenerator.GetData(),
            seaLevel,
            settings.heightScale);
    }

    void Chunk::GenerateClimateAndBiomes(float seaLevel)
    {
        // Lightweight version of GenerateClimateAndMaterials for distant chunks
        // Generates climate and biome data without full hydrology computation
        const auto &heightmap = m_TerrainGenerator.GetCachedHeightmap();
        const auto &settings = m_TerrainGenerator.GetSettings();

        if (heightmap.empty())
        {
            GEN_WARN("Chunk::GenerateClimateAndBiomes - No heightmap available");
            return;
        }

        glm::vec3 worldPos = GetWorldPosition();

        // Initialize climate generator with noise
        SimplexNoise noise(settings.seed);
        ClimateSettings climateSettings;
        m_ClimateGenerator.Initialize(&noise, climateSettings);

        // Grid dimensions (heightmap is (width+1) x (depth+1))
        int gridWidth = m_Size + 1;
        int gridDepth = m_Size + 1;

        // Generate climate fields with explicit grid dimensions (no hydrology)
        m_ClimateGenerator.Generate(
            heightmap,
            gridWidth,
            gridDepth,
            seaLevel,
            settings.heightScale,
            m_CellSize,
            worldPos.x,
            worldPos.z);

        // Classify biomes based on climate data
        m_BiomeClassifier.Classify(m_ClimateGenerator.GetData());
    }

    void Chunk::GenerateWaterMeshes([[maybe_unused]] float seaLevel)
    {
        const auto &heightmap = m_TerrainGenerator.GetCachedHeightmap();

        // Generate lake meshes in local coordinates (chunk transform handles world position)
        const auto &lakeNetwork = m_LakeGenerator.GetNetwork();
        if (!lakeNetwork.lakes.empty())
        {
            // Use zero offset - meshes should be in local chunk space
            glm::vec3 localOffset(0.0f);
            auto lakeMeshes = m_LakeMeshGenerator.Generate(lakeNetwork, heightmap, m_CellSize, localOffset);
            auto combinedLakeMesh = m_LakeMeshGenerator.CreateCombinedMesh(lakeMeshes);
            if (combinedLakeMesh && !combinedLakeMesh->GetVertices().empty())
            {
                m_LakeMesh = std::make_unique<Mesh>(combinedLakeMesh->GetVertices(), combinedLakeMesh->GetIndices());
                m_HasLakes = true;
            }
        }

        // Generate river meshes in local coordinates (chunk transform handles world position)
        const auto &riverNetwork = m_RiverGenerator.GetNetwork();
        if (!riverNetwork.rivers.empty())
        {
            // Use zero offset - meshes should be in local chunk space
            glm::vec3 localOffset(0.0f);
            auto riverMesh = m_RiverMeshGenerator.GenerateCombinedMesh(riverNetwork, heightmap, m_CellSize, localOffset);
            if (riverMesh && !riverMesh->GetVertices().empty())
            {
                m_RiverMesh = std::make_unique<Mesh>(riverMesh->GetVertices(), riverMesh->GetIndices());
                m_HasRivers = true;
            }
        }

        GEN_DEBUG("Chunk::GenerateWaterMeshes - HasLakes: {}, HasRivers: {}", m_HasLakes, m_HasRivers);
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

        // Normalized height thresholds for object placement
        constexpr float SHORE_LEVEL = 0.25f; // Below this is beach/shore
        constexpr float TREELINE = 0.8f;     // Above this is alpine/rock

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

            // Trees in vegetation zones (between shore and treeline)
            if (normalizedHeight > SHORE_LEVEL && normalizedHeight < TREELINE)
            {
                if (randomValue < 0.01f)
                {
                    m_TreePositions.push_back(glm::vec3(worldX, height, worldZ));
                }
            }

            // Rocks everywhere above water
            randomValue = distProb(rng);
            if (normalizedHeight > 0.1f)
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
        // Generate water mesh only for cells that are actually below sea level
        const auto &heightmap = m_TerrainGenerator.GetCachedHeightmap();
        if (heightmap.empty())
        {
            m_HasWater = false;
            return;
        }

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        glm::vec3 waterColor(0.1f, 0.4f, 0.6f);
        int gridWidth = m_Size + 1;

        // Generate a quad for each cell that is below sea level
        for (int z = 0; z < m_Size; z++)
        {
            for (int x = 0; x < m_Size; x++)
            {
                // Check if cell center is below sea level
                size_t idx = static_cast<size_t>(z) * gridWidth + x;
                float cellHeight = heightmap[idx];

                if (cellHeight >= seaLevel)
                {
                    continue;
                }

                // Cell corners in local space
                float x0 = x * m_CellSize;
                float x1 = (x + 1) * m_CellSize;
                float z0 = z * m_CellSize;
                float z1 = (z + 1) * m_CellSize;

                // Create quad vertices
                uint32_t baseIdx = static_cast<uint32_t>(vertices.size());

                Vertex v0, v1, v2, v3;
                v0.Position = glm::vec3(x0, seaLevel, z0);
                v0.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                v0.Color = waterColor;
                v0.TexCoord = glm::vec2(0.0f, 0.0f);

                v1.Position = glm::vec3(x1, seaLevel, z0);
                v1.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                v1.Color = waterColor;
                v1.TexCoord = glm::vec2(1.0f, 0.0f);

                v2.Position = glm::vec3(x1, seaLevel, z1);
                v2.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                v2.Color = waterColor;
                v2.TexCoord = glm::vec2(1.0f, 1.0f);

                v3.Position = glm::vec3(x0, seaLevel, z1);
                v3.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                v3.Color = waterColor;
                v3.TexCoord = glm::vec2(0.0f, 1.0f);

                vertices.push_back(v0);
                vertices.push_back(v1);
                vertices.push_back(v2);
                vertices.push_back(v3);

                // Two triangles per quad (CCW winding)
                indices.push_back(baseIdx + 0);
                indices.push_back(baseIdx + 3);
                indices.push_back(baseIdx + 1);

                indices.push_back(baseIdx + 1);
                indices.push_back(baseIdx + 3);
                indices.push_back(baseIdx + 2);
            }
        }

        if (vertices.empty())
        {
            m_HasWater = false;
            return;
        }

        m_WaterMesh = std::make_unique<Mesh>(vertices, indices);
        m_HasWater = true;
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
        // Generate water mesh only for cells that are actually below sea level
        // This creates per-cell water quads, not a full plane

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        glm::vec3 oceanColor(0.1f, 0.4f, 0.6f);
        int cellCount = 0;

        // Generate a quad for each cell that is below sea level (ocean or inland lake)
        for (int z = 0; z < m_Size; z++)
        {
            for (int x = 0; x < m_Size; x++)
            {
                if (!m_OceanMask.IsBelowSeaLevel(x, z))
                {
                    continue;
                }

                // Cell corners in local space
                float x0 = x * m_CellSize;
                float x1 = (x + 1) * m_CellSize;
                float z0 = z * m_CellSize;
                float z1 = (z + 1) * m_CellSize;

                // Determine color - ocean vs inland lake
                bool isOcean = m_OceanMask.IsOcean(x, z);
                glm::vec3 color = isOcean ? oceanColor : glm::vec3(0.15f, 0.45f, 0.55f);

                // Create quad vertices
                uint32_t baseIdx = static_cast<uint32_t>(vertices.size());

                Vertex v0, v1, v2, v3;
                v0.Position = glm::vec3(x0, seaLevel, z0);
                v0.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                v0.Color = color;
                v0.TexCoord = glm::vec2(0.0f, 0.0f);

                v1.Position = glm::vec3(x1, seaLevel, z0);
                v1.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                v1.Color = color;
                v1.TexCoord = glm::vec2(1.0f, 0.0f);

                v2.Position = glm::vec3(x1, seaLevel, z1);
                v2.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                v2.Color = color;
                v2.TexCoord = glm::vec2(1.0f, 1.0f);

                v3.Position = glm::vec3(x0, seaLevel, z1);
                v3.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                v3.Color = color;
                v3.TexCoord = glm::vec2(0.0f, 1.0f);

                vertices.push_back(v0);
                vertices.push_back(v1);
                vertices.push_back(v2);
                vertices.push_back(v3);

                // Two triangles per quad (CCW winding)
                indices.push_back(baseIdx + 0);
                indices.push_back(baseIdx + 3);
                indices.push_back(baseIdx + 1);

                indices.push_back(baseIdx + 1);
                indices.push_back(baseIdx + 3);
                indices.push_back(baseIdx + 2);

                cellCount++;
            }
        }

        if (vertices.empty())
        {
            m_HasWater = false;
            return;
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

            // Upload lake mesh
            if (m_LakeMesh && m_HasLakes)
            {
                auto cpuLake = std::move(m_LakeMesh);
                m_LakeMesh = std::make_unique<Mesh>(device, cpuLake->GetVertices(), cpuLake->GetIndices());
            }

            // Upload river mesh
            if (m_RiverMesh && m_HasRivers)
            {
                auto cpuRiver = std::move(m_RiverMesh);
                m_RiverMesh = std::make_unique<Mesh>(device, cpuRiver->GetVertices(), cpuRiver->GetIndices());
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
            if (m_LakeMesh)
            {
                m_LakeMesh->Shutdown();
                m_LakeMesh.reset();
            }
            if (m_RiverMesh)
            {
                m_RiverMesh->Shutdown();
                m_RiverMesh.reset();
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
        if (m_LakeMesh)
        {
            if (m_State == ChunkState::Loaded)
            {
                m_LakeMesh->Shutdown();
            }
            m_LakeMesh.reset();
        }
        if (m_RiverMesh)
        {
            if (m_State == ChunkState::Loaded)
            {
                m_RiverMesh->Shutdown();
            }
            m_RiverMesh.reset();
        }
        m_TreePositions.clear();
        m_RockPositions.clear();
        m_HasLakes = false;
        m_HasRivers = false;
        m_HasWater = false;
        m_State = ChunkState::Unloaded;
    }

}
