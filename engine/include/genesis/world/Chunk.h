#pragma once

#include "genesis/renderer/Mesh.h"
#include "genesis/procedural/TerrainGenerator.h"
#include "genesis/procedural/OceanMask.h"
#include "genesis/procedural/DrainageGraph.h"
#include "genesis/procedural/RiverGenerator.h"
#include "genesis/procedural/LakeGenerator.h"
#include "genesis/procedural/HydrologyData.h"
#include "genesis/procedural/ClimateGenerator.h"
#include "genesis/procedural/MaterialBlender.h"
#include "genesis/procedural/BiomeClassifier.h"
#include "genesis/procedural/LakeMeshGenerator.h"
#include "genesis/procedural/RiverMeshGenerator.h"
#include <glm/glm.hpp>
#include <memory>

namespace Genesis
{

    enum class ChunkState
    {
        Unloaded,
        Loading,
        Loaded,
        Unloading
    };

    class VulkanDevice;

    class Chunk
    {
    public:
        Chunk(int x, int z, int size, float cellSize);
        ~Chunk();

        // Generate terrain for this chunk
        // Set computeHydrology=false for distant chunks to skip expensive water calculations
        void Generate(const TerrainSettings &settings, uint32_t worldSeed, float seaLevel = 0.0f, bool computeHydrology = true);

        // Upload mesh to GPU
        void Upload(VulkanDevice &device);

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
        Mesh *GetMesh() const { return m_Mesh.get(); }
        Mesh *GetWaterMesh() const { return m_WaterMesh.get(); }
        bool HasWater() const { return m_HasWater; }

        // Get height at local position within chunk
        float GetHeightAt(float localX, float localZ) const;

        // Check if world position is within this chunk
        bool ContainsWorldPosition(float worldX, float worldZ) const;

        // Object positions (world coordinates)
        const std::vector<glm::vec3> &GetTreePositions() const { return m_TreePositions; }
        const std::vector<glm::vec3> &GetRockPositions() const { return m_RockPositions; }

        // Ocean mask access
        OceanMask &GetOceanMask() { return m_OceanMask; }
        const OceanMask &GetOceanMask() const { return m_OceanMask; }

        // Check if a cell is ocean (below sea level AND connected to world boundary)
        bool IsOceanAt(int cellX, int cellZ) const { return m_OceanMask.IsOcean(cellX, cellZ); }

        // Check if a cell is an inland lake (below sea level but NOT connected)
        bool IsInlandLakeAt(int cellX, int cellZ) const { return m_OceanMask.IsInlandLake(cellX, cellZ); }

        // Regenerate water mesh after ocean mask update
        void RegenerateWater(float seaLevel, bool useOceanMask);

        // Access to hydrology data for rendering
        const HydrologyData &GetHydrologyData() const { return m_HydrologyData; }
        const ClimateData &GetClimateData() const { return m_ClimateGenerator.GetData(); }
        const MaterialData &GetMaterialData() const { return m_MaterialBlender.GetData(); }
        const BiomeData &GetBiomeData() const { return m_BiomeClassifier.GetData(); }

        // Get biome weights at a cell position
        BiomeWeights GetBiomeWeights(int cellX, int cellZ) const { return m_BiomeClassifier.GetBiomeWeights(cellX, cellZ); }

        // Access to lake and river meshes
        Mesh *GetLakeMesh() const { return m_LakeMesh.get(); }
        Mesh *GetRiverMesh() const { return m_RiverMesh.get(); }
        bool HasLakes() const { return m_HasLakes; }
        bool HasRivers() const { return m_HasRivers; }

    private:
        std::shared_ptr<Mesh> GenerateTerrainMesh(float offsetX, float offsetZ, uint32_t worldSeed);
        void GenerateHydrology(float seaLevel);
        void GenerateClimateAndMaterials(float seaLevel);
        void GenerateClimateAndBiomes(float seaLevel); // Lightweight version without full hydrology
        void GenerateWaterMeshes(float seaLevel);
        void GenerateObjects(uint32_t worldSeed, float seaLevel);
        void GenerateWater(float seaLevel);
        void GenerateWaterWithOceanMask(float seaLevel);
        float GetHeightAtLocal(float localX, float localZ) const;

        int m_ChunkX; // Chunk coordinate (not world position)
        int m_ChunkZ;
        int m_Size; // Grid cells per side
        float m_CellSize;

        ChunkState m_State = ChunkState::Unloaded;

        std::unique_ptr<Mesh> m_Mesh;
        std::unique_ptr<Mesh> m_WaterMesh;
        std::unique_ptr<Mesh> m_LakeMesh;
        std::unique_ptr<Mesh> m_RiverMesh;
        bool m_HasWater = false;
        bool m_HasLakes = false;
        bool m_HasRivers = false;

        TerrainGenerator m_TerrainGenerator;
        OceanMask m_OceanMask;

        // Hydrology pipeline components
        DrainageGraph m_DrainageGraph;
        RiverGenerator m_RiverGenerator;
        LakeGenerator m_LakeGenerator;
        HydrologyGenerator m_HydrologyGenerator;
        HydrologyData m_HydrologyData;

        // Climate and material systems
        ClimateGenerator m_ClimateGenerator;
        MaterialBlender m_MaterialBlender;
        BiomeClassifier m_BiomeClassifier;

        // Mesh generators for water bodies
        LakeMeshGenerator m_LakeMeshGenerator;
        RiverMeshGenerator m_RiverMeshGenerator;

        // Object positions within this chunk (world coordinates)
        std::vector<glm::vec3> m_TreePositions;
        std::vector<glm::vec3> m_RockPositions;

        friend class ChunkManager;
    };

}
