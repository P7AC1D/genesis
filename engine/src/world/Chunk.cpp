#include "genesis/world/Chunk.h"
#include "genesis/renderer/VulkanDevice.h"
#include <random>

namespace Genesis {

    Chunk::Chunk(int x, int z, int size, float cellSize)
        : m_ChunkX(x), m_ChunkZ(z), m_Size(size), m_CellSize(cellSize) {
    }

    Chunk::~Chunk() {
        Destroy();
    }

    glm::vec3 Chunk::GetWorldPosition() const {
        float worldX = m_ChunkX * m_Size * m_CellSize;
        float worldZ = m_ChunkZ * m_Size * m_CellSize;
        return glm::vec3(worldX, 0.0f, worldZ);
    }

    bool Chunk::ContainsWorldPosition(float worldX, float worldZ) const {
        glm::vec3 origin = GetWorldPosition();
        float chunkWorldSize = m_Size * m_CellSize;
        
        return worldX >= origin.x && worldX < origin.x + chunkWorldSize &&
               worldZ >= origin.z && worldZ < origin.z + chunkWorldSize;
    }

    void Chunk::Generate(const TerrainSettings& baseSettings, uint32_t worldSeed) {
        // Create chunk-specific terrain settings
        TerrainSettings settings = baseSettings;
        settings.width = m_Size;
        settings.depth = m_Size;
        settings.cellSize = m_CellSize;
        
        // Keep world seed for terrain - must be same across all chunks for seamless noise
        settings.seed = worldSeed;
        
        m_TerrainGenerator.SetSettings(settings);
        
        // Generate with world-space offset for seamless noise
        glm::vec3 worldPos = GetWorldPosition();
        
        // Generate the mesh - vertices in local space, noise sampled from world space
        auto terrainMesh = GenerateWithWorldOffset(worldPos.x, worldPos.z, worldSeed);
        
        if (terrainMesh) {
            m_Mesh = std::make_unique<Mesh>(terrainMesh->GetVertices(), terrainMesh->GetIndices());
        }
        
        // Generate object positions (uses chunk-specific seed derived from world seed)
        GenerateObjects(worldSeed);
        
        m_State = ChunkState::Loading;
    }

    std::shared_ptr<Mesh> Chunk::GenerateWithWorldOffset(float offsetX, float offsetZ, uint32_t worldSeed) {
        const auto& settings = m_TerrainGenerator.GetSettings();
        SimplexNoise noise(worldSeed);  // Use world seed, not chunk seed
        
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        
        int width = settings.width + 1;
        int depth = settings.depth + 1;
        
        // Generate heightmap with world-space noise sampling
        std::vector<float> heightmap(width * depth);
        
        for (int z = 0; z < depth; z++) {
            for (int x = 0; x < width; x++) {
                // Local position
                float localX = x * settings.cellSize;
                float localZ = z * settings.cellSize;
                
                // World position for noise sampling
                float worldX = offsetX + localX;
                float worldZ = offsetZ + localZ;
                
                // Sample noise at world coordinates
                float noiseX = worldX * settings.noiseScale;
                float noiseZ = worldZ * settings.noiseScale;
                
                float height = noise.FBM(noiseX, noiseZ, 
                    settings.octaves, settings.persistence, settings.lacunarity);
                
                height = (height + 1.0f) * 0.5f;
                height = settings.baseHeight + height * settings.heightScale;
                
                heightmap[z * width + x] = height;
            }
        }
        
        // Use absolute height range based on settings, not per-chunk min/max
        // This ensures consistent coloring across all chunks
        float minHeight = settings.baseHeight;
        float maxHeight = settings.baseHeight + settings.heightScale;
        float heightRange = settings.heightScale;
        
        // Generate mesh with flat shading (same as TerrainGenerator)
        for (int z = 0; z < settings.depth; z++) {
            for (int x = 0; x < settings.width; x++) {
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
                float normH1 = (avgH1 - minHeight) / heightRange;
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
                float normH2 = (avgH2 - minHeight) / heightRange;
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
        
        // Store heightmap for later queries
        m_TerrainGenerator.SetSettings(m_TerrainGenerator.GetSettings()); // Reset
        
        return std::make_shared<Mesh>(vertices, indices);
    }

    void Chunk::GenerateObjects(uint32_t worldSeed) {
        const auto& settings = m_TerrainGenerator.GetSettings();
        glm::vec3 worldPos = GetWorldPosition();
        float chunkWorldSize = m_Size * m_CellSize;
        
        // Seed RNG based on chunk position
        uint32_t chunkSeed = worldSeed ^ (static_cast<uint32_t>(m_ChunkX * 198491317) ^ 
                                          static_cast<uint32_t>(m_ChunkZ * 6542989));
        std::mt19937 rng(chunkSeed);
        std::uniform_real_distribution<float> distPos(0.0f, chunkWorldSize);
        
        // Trees - about 1 per 100 square units
        int numTrees = static_cast<int>((chunkWorldSize * chunkWorldSize) / 100.0f);
        m_TreePositions.reserve(numTrees);
        
        for (int i = 0; i < numTrees; i++) {
            float localX = distPos(rng);
            float localZ = distPos(rng);
            float height = GetHeightAtLocal(localX, localZ);
            
            float normalizedHeight = height / settings.heightScale;
            if (normalizedHeight > settings.sandLevel && normalizedHeight < settings.rockLevel) {
                m_TreePositions.push_back(glm::vec3(
                    worldPos.x + localX,
                    height,
                    worldPos.z + localZ
                ));
            }
        }
        
        // Rocks - about 1 per 150 square units
        int numRocks = static_cast<int>((chunkWorldSize * chunkWorldSize) / 150.0f);
        m_RockPositions.reserve(numRocks);
        
        for (int i = 0; i < numRocks; i++) {
            float localX = distPos(rng);
            float localZ = distPos(rng);
            float height = GetHeightAtLocal(localX, localZ);
            
            float normalizedHeight = height / settings.heightScale;
            if (normalizedHeight > settings.waterLevel * 0.5f) {
                m_RockPositions.push_back(glm::vec3(
                    worldPos.x + localX,
                    height + 0.1f,
                    worldPos.z + localZ
                ));
            }
        }
    }

    float Chunk::GetHeightAtLocal(float localX, float localZ) const {
        const auto& settings = m_TerrainGenerator.GetSettings();
        glm::vec3 worldPos = GetWorldPosition();
        
        // Sample noise at world coordinates
        float worldX = worldPos.x + localX;
        float worldZ = worldPos.z + localZ;
        
        SimplexNoise noise(settings.seed);
        float noiseX = worldX * settings.noiseScale;
        float noiseZ = worldZ * settings.noiseScale;
        
        float height = noise.FBM(noiseX, noiseZ, 
            settings.octaves, settings.persistence, settings.lacunarity);
        
        height = (height + 1.0f) * 0.5f;
        return settings.baseHeight + height * settings.heightScale;
    }

    float Chunk::GetHeightAt(float localX, float localZ) const {
        return GetHeightAtLocal(localX, localZ);
    }

    void Chunk::Upload(VulkanDevice& device) {
        if (m_Mesh && m_State == ChunkState::Loading) {
            // Create GPU buffers
            auto cpuMesh = std::move(m_Mesh);
            m_Mesh = std::make_unique<Mesh>(device, cpuMesh->GetVertices(), cpuMesh->GetIndices());
            m_State = ChunkState::Loaded;
        }
    }

    void Chunk::Unload() {
        if (m_Mesh && m_State == ChunkState::Loaded) {
            m_Mesh->Shutdown();
            m_Mesh.reset();
            m_State = ChunkState::Unloaded;
        }
    }

    void Chunk::Destroy() {
        if (m_Mesh) {
            if (m_State == ChunkState::Loaded) {
                m_Mesh->Shutdown();
            }
            m_Mesh.reset();
        }
        m_TreePositions.clear();
        m_RockPositions.clear();
        m_State = ChunkState::Unloaded;
    }

}
