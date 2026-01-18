#pragma once

#include "genesis/procedural/LakeGenerator.h"
#include "genesis/renderer/Mesh.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Genesis
{

    /**
     * Section 30.2: Lake Mesh Generation
     *
     * Generates polygonal water surface meshes for lakes.
     * Lakes are rendered as flat meshes at their surface height.
     */
    struct LakeMeshSettings
    {
        // Mesh quality
        int subdivisions = 8;        // Subdivisions for lake surface
        float edgeSoftness = 0.5f;   // Edge smoothing factor
        float waveAmplitude = 0.05f; // Small wave displacement

        // Colors
        glm::vec3 shallowColor{0.15f, 0.50f, 0.55f};
        glm::vec3 deepColor{0.05f, 0.20f, 0.35f};
        float colorDepthScale = 10.0f; // Depth for full deep color
    };

    /**
     * Generated lake mesh data.
     */
    struct LakeMeshData
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        glm::vec3 center;    // Lake center in world space
        float surfaceHeight; // Water surface Y position
        int lakeIndex;       // Index into LakeNetwork::lakes
    };

    /**
     * Generates renderable meshes for lakes.
     */
    class LakeMeshGenerator
    {
    public:
        LakeMeshGenerator() = default;
        ~LakeMeshGenerator() = default;

        /**
         * Configure mesh generation settings.
         */
        void SetSettings(const LakeMeshSettings &settings) { m_Settings = settings; }
        const LakeMeshSettings &GetSettings() const { return m_Settings; }

        /**
         * Generate meshes for all lakes in a lake network.
         *
         * @param network     Lake network data
         * @param heightmap   Terrain heights (for depth coloring)
         * @param cellSize    World units per cell
         * @param chunkOffset Chunk world offset
         * @return Vector of lake mesh data
         */
        std::vector<LakeMeshData> Generate(const LakeNetwork &network,
                                           const std::vector<float> &heightmap,
                                           float cellSize,
                                           const glm::vec3 &chunkOffset) const;

        /**
         * Generate a single lake mesh.
         *
         * @param basin       Lake basin data
         * @param network     Lake network (for lake depth lookup)
         * @param heightmap   Terrain heights
         * @param cellSize    World units per cell
         * @param chunkOffset Chunk world offset
         * @return Lake mesh data
         */
        LakeMeshData GenerateLakeMesh(const LakeBasin &basin,
                                      const LakeNetwork &network,
                                      const std::vector<float> &heightmap,
                                      float cellSize,
                                      const glm::vec3 &chunkOffset) const;

        /**
         * Create a Mesh object from lake mesh data.
         */
        std::unique_ptr<Mesh> CreateMesh(const LakeMeshData &data) const;

        /**
         * Create a combined mesh from all lakes.
         */
        std::unique_ptr<Mesh> CreateCombinedMesh(const std::vector<LakeMeshData> &lakes) const;

    private:
        /**
         * Compute lake boundary polygon.
         */
        std::vector<glm::vec2> ComputeBoundaryPolygon(const LakeBasin &basin,
                                                      float cellSize) const;

        /**
         * Triangulate a polygon using ear clipping.
         */
        void TriangulatePolygon(const std::vector<glm::vec2> &polygon,
                                std::vector<uint32_t> &indices,
                                size_t vertexOffset) const;

        /**
         * Compute depth-based color.
         */
        glm::vec3 ComputeDepthColor(float depth) const;

        LakeMeshSettings m_Settings;
    };

}
