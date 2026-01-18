#pragma once

#include "genesis/procedural/RiverGenerator.h"
#include "genesis/renderer/Mesh.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Genesis
{

    /**
     * Section 30.2: River Mesh Generation
     *
     * Generates ribbon meshes for rivers and streams.
     * Rivers follow flow paths with width based on flow accumulation.
     */
    struct RiverMeshSettings
    {
        // Mesh quality
        int segmentsPerCell = 2;      // Subdivisions per river cell
        float edgeSoftness = 0.3f;    // Edge smoothing
        float bankHeight = 0.1f;      // Height of river banks above surface
        float surfaceOffset = -0.05f; // Water surface below terrain

        // Colors
        glm::vec3 shallowColor{0.15f, 0.45f, 0.55f};
        glm::vec3 deepColor{0.08f, 0.25f, 0.40f};
        glm::vec3 foamColor{0.7f, 0.8f, 0.9f}; // Rapids/foam color

        // Flow effects
        float flowSpeedScale = 0.5f; // Flow animation speed multiplier
        float foamThreshold = 0.7f;  // Slope threshold for foam
    };

    /**
     * Generated river mesh data.
     */
    struct RiverMeshData
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        glm::vec3 startPoint; // River start in world space
        glm::vec3 endPoint;   // River end in world space
        float averageWidth;   // Average river width
        size_t riverIndex;    // Index into RiverNetwork::rivers
    };

    /**
     * Generates renderable meshes for rivers.
     */
    class RiverMeshGenerator
    {
    public:
        RiverMeshGenerator() = default;
        ~RiverMeshGenerator() = default;

        /**
         * Configure mesh generation settings.
         */
        void SetSettings(const RiverMeshSettings &settings) { m_Settings = settings; }
        const RiverMeshSettings &GetSettings() const { return m_Settings; }

        /**
         * Generate meshes for all rivers in a river network.
         *
         * @param network     River network data
         * @param heightmap   Terrain heights
         * @param cellSize    World units per cell
         * @param chunkOffset Chunk world offset
         * @return Vector of river mesh data
         */
        std::vector<RiverMeshData> Generate(const RiverNetwork &network,
                                            const std::vector<float> &heightmap,
                                            float cellSize,
                                            const glm::vec3 &chunkOffset) const;

        /**
         * Generate mesh for all river segments (combined).
         * More efficient than individual river meshes.
         *
         * @param network     River network data
         * @param heightmap   Terrain heights
         * @param cellSize    World units per cell
         * @param chunkOffset Chunk world offset
         * @return Combined river mesh
         */
        std::unique_ptr<Mesh> GenerateCombinedMesh(const RiverNetwork &network,
                                                   const std::vector<float> &heightmap,
                                                   float cellSize,
                                                   const glm::vec3 &chunkOffset) const;

        /**
         * Generate ribbon mesh for a single river path.
         */
        RiverMeshData GenerateRiverMesh(const RiverPath &path,
                                        const RiverNetwork &network,
                                        const std::vector<float> &heightmap,
                                        float cellSize,
                                        const glm::vec3 &chunkOffset) const;

        /**
         * Create a Mesh object from river mesh data.
         */
        std::unique_ptr<Mesh> CreateMesh(const RiverMeshData &data) const;

    private:
        /**
         * Generate ribbon vertices for a river segment.
         *
         * @param segment     River segment
         * @param nextSegment Next segment (for smooth connection), or nullptr
         * @param heightmap   Terrain heights
         * @param cellSize    World units per cell
         * @param chunkOffset Chunk world offset
         * @param vertices    Output vertices
         * @param indices     Output indices
         */
        void GenerateRibbonSegment(const RiverSegment &segment,
                                   const RiverSegment *nextSegment,
                                   const std::vector<float> &heightmap,
                                   int gridWidth,
                                   float cellSize,
                                   const glm::vec3 &chunkOffset,
                                   std::vector<Vertex> &vertices,
                                   std::vector<uint32_t> &indices) const;

        /**
         * Compute perpendicular direction for ribbon width.
         */
        glm::vec2 ComputePerpendicular(const glm::ivec2 &current,
                                       const glm::ivec2 &next) const;

        /**
         * Compute flow-based color.
         */
        glm::vec3 ComputeFlowColor(float width, float depth, float slope) const;

        /**
         * Sample terrain height at world position.
         */
        float SampleHeight(const std::vector<float> &heightmap,
                           int gridWidth,
                           float cellSize,
                           float worldX,
                           float worldZ,
                           const glm::vec3 &chunkOffset) const;

        RiverMeshSettings m_Settings;
    };

}
