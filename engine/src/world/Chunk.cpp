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

    std::shared_ptr<Mesh> Chunk::GenerateTerrainMesh(float offsetX, float offsetZ, uint32_t worldSeed)
    {
        const auto &settings = m_TerrainGenerator.GetSettings();
        SimplexNoise noise(worldSeed);

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        int width = settings.width + 1;
        int depth = settings.depth + 1;

        // Generate heightmap with world-space noise sampling
        std::vector<float> heightmap(width * depth);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                float localX = x * settings.cellSize;
                float localZ = z * settings.cellSize;
                float worldX = offsetX + localX;
                float worldZ = offsetZ + localZ;

                float noiseX = worldX * settings.noiseScale;
                float noiseZ = worldZ * settings.noiseScale;

                float height;
                if (settings.useWarp && settings.warpLevels > 0)
                {
                    height = noise.MultiWarpedFBM(noiseX, noiseZ,
                                                  settings.octaves, settings.persistence, settings.lacunarity,
                                                  settings.warpStrength, settings.warpScale, settings.warpLevels);
                }
                else
                {
                    height = noise.FBM(noiseX, noiseZ,
                                       settings.octaves, settings.persistence, settings.lacunarity);
                }

                // Blend ridge noise for mountain ranges
                if (settings.useRidgeNoise)
                {
                    float ridgeCoordX = noiseX;
                    float ridgeCoordZ = noiseZ;

                    if (settings.useWarp && settings.warpLevels > 0 && settings.warpStrength > 0.0f)
                    {
                        float wx = ridgeCoordX;
                        float wz = ridgeCoordZ;

                        for (int level = 0; level < settings.warpLevels; level++)
                        {
                            float offsetX = 5.2f + level * 17.1f;
                            float offsetZ = 1.3f + level * 31.7f;
                            float offsetX2 = 9.7f + level * 23.5f;
                            float offsetZ2 = 2.8f + level * 13.9f;

                            float levelWarpStrength = settings.warpStrength / (1.0f + level * 0.5f);
                            float levelWarpScale = settings.warpScale * (1.0f + level * 0.3f);

                            float warpOffsetX = noise.FBM(wx * levelWarpScale + offsetX, wz * levelWarpScale + offsetZ, 2, 0.5f, 2.0f) * levelWarpStrength;
                            float warpOffsetZ = noise.FBM(wx * levelWarpScale + offsetX2, wz * levelWarpScale + offsetZ2, 2, 0.5f, 2.0f) * levelWarpStrength;

                            wx += warpOffsetX;
                            wz += warpOffsetZ;
                        }

                        ridgeCoordX = wx;
                        ridgeCoordZ = wz;
                    }

                    float ridgeNoiseX = ridgeCoordX * settings.ridgeScale;
                    float ridgeNoiseZ = ridgeCoordZ * settings.ridgeScale;
                    float ridgeNoise = noise.RidgeNoise(ridgeNoiseX, ridgeNoiseZ,
                                                        settings.octaves,
                                                        settings.persistence,
                                                        settings.lacunarity);
                    ridgeNoise = std::pow(ridgeNoise, settings.ridgePower);

                    // Calculate uplift mask - determines where mountains appear
                    float upliftMask = 1.0f;
                    if (settings.useUpliftMask)
                    {
                        float upliftNoiseX = worldX * settings.upliftScale;
                        float upliftNoiseZ = worldZ * settings.upliftScale;
                        float upliftNoise = noise.FBM(upliftNoiseX, upliftNoiseZ, 2, 0.5f, 2.0f);
                        upliftNoise = (upliftNoise + 1.0f) * 0.5f; // Map to [0, 1]

                        // Smoothstep for gradual transition from plains to mountains
                        float t = (upliftNoise - settings.upliftThresholdLow) /
                                  (settings.upliftThresholdHigh - settings.upliftThresholdLow);
                        t = std::clamp(t, 0.0f, 1.0f);
                        upliftMask = t * t * (3.0f - 2.0f * t); // Smoothstep
                        upliftMask = std::pow(upliftMask, settings.upliftPower);
                    }

                    // Apply uplift mask to ridge contribution
                    float ridgeContribution = ridgeNoise * settings.ridgeWeight * upliftMask;
                    float baseWeight = 1.0f - (settings.ridgeWeight * upliftMask);
                    height = height * baseWeight + ridgeContribution;

                    // Ridge peak sharpening - extra boost at peaks
                    height += std::pow(ridgeNoise, 4.0f) * settings.peakBoost * upliftMask;
                }

                // Normalize to [0,1]
                height = (height + 1.0f) * 0.5f;

                // Height-dependent shaping: sharp tops, soft bases
                // lerp(1.0, 0.6, heightNorm) softens lower elevations
                float heightNorm = std::clamp(height, 0.0f, 1.0f);
                float shapeFactor = 1.0f - 0.4f * heightNorm; // lerp(1.0, 0.6, h)
                height *= shapeFactor;

                heightmap[z * width + x] = settings.baseHeight + height * settings.heightScale;
            }
        }

        // Apply erosion effects
        if (settings.useErosion)
        {
            // Cheap erosion: slope-based shaping and valley deepening
            std::vector<float> erodedHeightmap = heightmap;

            for (int z = 1; z < depth - 1; z++)
            {
                for (int x = 1; x < width - 1; x++)
                {
                    int idx = z * width + x;
                    float h = heightmap[idx];

                    // Calculate slope from neighbors
                    float hL = heightmap[idx - 1];
                    float hR = heightmap[idx + 1];
                    float hU = heightmap[(z - 1) * width + x];
                    float hD = heightmap[(z + 1) * width + x];

                    float slopeX = (hR - hL) / (2.0f * settings.cellSize);
                    float slopeZ = (hD - hU) / (2.0f * settings.cellSize);
                    float slope = std::sqrt(slopeX * slopeX + slopeZ * slopeZ);

                    // Slope erosion: steep areas erode faster
                    if (slope > settings.slopeThreshold)
                    {
                        float erosionFactor = 1.0f - settings.slopeErosionStrength *
                                                         std::min(1.0f, (slope - settings.slopeThreshold) / settings.slopeThreshold);
                        h *= erosionFactor;
                    }

                    // Valley deepening: areas lower than neighbors carve deeper
                    float avgNeighbor = (hL + hR + hU + hD) * 0.25f;
                    if (h < avgNeighbor)
                    {
                        float valleyFactor = (avgNeighbor - h) / settings.heightScale;
                        h -= valleyFactor * settings.valleyDepth * settings.heightScale;
                    }

                    erodedHeightmap[idx] = h;
                }
            }

            heightmap = erodedHeightmap;

            // Hydraulic erosion: particle-based simulation
            if (settings.useHydraulicErosion && settings.erosionIterations > 0)
            {
                std::mt19937 rng(worldSeed + static_cast<uint32_t>(offsetX * 1000 + offsetZ));
                std::uniform_real_distribution<float> distX(1.0f, static_cast<float>(width - 2));
                std::uniform_real_distribution<float> distZ(1.0f, static_cast<float>(depth - 2));

                for (int iter = 0; iter < settings.erosionIterations; iter++)
                {
                    // Spawn water droplet at random position
                    float dropX = distX(rng);
                    float dropZ = distZ(rng);
                    float dirX = 0.0f;
                    float dirZ = 0.0f;
                    float speed = 1.0f;
                    float water = 1.0f;
                    float sediment = 0.0f;

                    for (int step = 0; step < 64; step++)
                    {
                        int cellX = static_cast<int>(dropX);
                        int cellZ = static_cast<int>(dropZ);

                        if (cellX < 1 || cellX >= width - 1 || cellZ < 1 || cellZ >= depth - 1)
                            break;

                        int idx = cellZ * width + cellX;

                        // Calculate gradient
                        float hL = heightmap[idx - 1];
                        float hR = heightmap[idx + 1];
                        float hU = heightmap[(cellZ - 1) * width + cellX];
                        float hD = heightmap[(cellZ + 1) * width + cellX];
                        float hC = heightmap[idx];

                        float gradX = (hR - hL) * 0.5f;
                        float gradZ = (hD - hU) * 0.5f;

                        // Update direction with inertia
                        dirX = dirX * settings.erosionInertia - gradX * (1.0f - settings.erosionInertia);
                        dirZ = dirZ * settings.erosionInertia - gradZ * (1.0f - settings.erosionInertia);

                        // Normalize direction
                        float len = std::sqrt(dirX * dirX + dirZ * dirZ);
                        if (len < 0.0001f)
                            break;
                        dirX /= len;
                        dirZ /= len;

                        // Move droplet
                        float newX = dropX + dirX;
                        float newZ = dropZ + dirZ;

                        int newCellX = static_cast<int>(newX);
                        int newCellZ = static_cast<int>(newZ);
                        if (newCellX < 1 || newCellX >= width - 1 || newCellZ < 1 || newCellZ >= depth - 1)
                            break;

                        float newH = heightmap[newCellZ * width + newCellX];
                        float deltaH = newH - hC;

                        // Calculate sediment capacity
                        float capacity = std::max(-deltaH, 0.01f) * speed * water * settings.erosionCapacity;

                        if (sediment > capacity || deltaH > 0)
                        {
                            // Deposit sediment
                            float depositAmount = (deltaH > 0) ? std::min(deltaH, sediment) : (sediment - capacity) * settings.erosionDeposition;
                            sediment -= depositAmount;
                            heightmap[idx] += depositAmount;
                        }
                        else
                        {
                            // Erode terrain
                            float erodeAmount = std::min((capacity - sediment) * 0.3f, -deltaH);
                            sediment += erodeAmount;
                            heightmap[idx] -= erodeAmount;
                        }

                        // Update droplet state
                        dropX = newX;
                        dropZ = newZ;
                        speed = std::sqrt(std::max(0.0f, speed * speed + deltaH));
                        water *= (1.0f - settings.erosionEvaporation);

                        if (water < 0.01f)
                            break;
                    }
                }
            }
        }

        // Height range for color normalization
        float minHeight = settings.baseHeight;
        float maxHeight = settings.baseHeight + settings.heightScale;
        float heightRange = maxHeight - minHeight;

        // Generate mesh with flat shading (per-triangle colors/normals)
        for (int z = 0; z < settings.depth; z++)
        {
            for (int x = 0; x < settings.width; x++)
            {
                float x0 = x * settings.cellSize;
                float x1 = (x + 1) * settings.cellSize;
                float z0 = z * settings.cellSize;
                float z1 = (z + 1) * settings.cellSize;

                float h00 = heightmap[z * width + x];
                float h10 = heightmap[z * width + (x + 1)];
                float h01 = heightmap[(z + 1) * width + x];
                float h11 = heightmap[(z + 1) * width + (x + 1)];

                glm::vec3 p00(x0, h00, z0);
                glm::vec3 p10(x1, h10, z0);
                glm::vec3 p01(x0, h01, z1);
                glm::vec3 p11(x1, h11, z1);

                // Triangle 1: p00, p01, p10 (CCW)
                glm::vec3 normal1 = glm::normalize(glm::cross(p01 - p00, p10 - p00));
                float avgH1 = (h00 + h10 + h01) / 3.0f;
                float normH1 = glm::clamp((avgH1 - minHeight) / heightRange, 0.0f, 1.0f);
                glm::vec3 color1 = TerrainGenerator::GetHeightColor(normH1, settings);

                uint32_t baseIdx = static_cast<uint32_t>(vertices.size());
                vertices.push_back({p00, normal1, color1});
                vertices.push_back({p01, normal1, color1});
                vertices.push_back({p10, normal1, color1});
                indices.push_back(baseIdx);
                indices.push_back(baseIdx + 1);
                indices.push_back(baseIdx + 2);

                // Triangle 2: p10, p01, p11 (CCW)
                glm::vec3 normal2 = glm::normalize(glm::cross(p01 - p10, p11 - p10));
                float avgH2 = (h10 + h11 + h01) / 3.0f;
                float normH2 = glm::clamp((avgH2 - minHeight) / heightRange, 0.0f, 1.0f);
                glm::vec3 color2 = TerrainGenerator::GetHeightColor(normH2, settings);

                baseIdx = static_cast<uint32_t>(vertices.size());
                vertices.push_back({p10, normal2, color2});
                vertices.push_back({p01, normal2, color2});
                vertices.push_back({p11, normal2, color2});
                indices.push_back(baseIdx);
                indices.push_back(baseIdx + 1);
                indices.push_back(baseIdx + 2);
            }
        }

        return std::make_shared<Mesh>(vertices, indices);
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
        const auto &settings = m_TerrainGenerator.GetSettings();
        glm::vec3 worldPos = GetWorldPosition();

        float worldX = worldPos.x + localX;
        float worldZ = worldPos.z + localZ;

        SimplexNoise noise(settings.seed);
        float noiseX = worldX * settings.noiseScale;
        float noiseZ = worldZ * settings.noiseScale;

        float height;
        if (settings.useWarp && settings.warpLevels > 0)
        {
            height = noise.MultiWarpedFBM(noiseX, noiseZ,
                                          settings.octaves, settings.persistence, settings.lacunarity,
                                          settings.warpStrength, settings.warpScale, settings.warpLevels);
        }
        else
        {
            height = noise.FBM(noiseX, noiseZ,
                               settings.octaves, settings.persistence, settings.lacunarity);
        }

        height = (height + 1.0f) * 0.5f;
        return settings.baseHeight + height * settings.heightScale;
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
