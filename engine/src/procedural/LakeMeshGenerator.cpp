#include "genesis/procedural/LakeMeshGenerator.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <set>

namespace Genesis
{

    std::vector<LakeMeshData> LakeMeshGenerator::Generate(const LakeNetwork &network,
                                                          const std::vector<float> &heightmap,
                                                          float cellSize,
                                                          const glm::vec3 &chunkOffset) const
    {
        std::vector<LakeMeshData> lakeMeshes;
        lakeMeshes.reserve(network.lakes.size());

        for (size_t i = 0; i < network.lakes.size(); i++)
        {
            const auto &basin = network.lakes[i];

            // Skip very small lakes
            if (basin.cells.size() < 3)
            {
                continue;
            }

            LakeMeshData meshData = GenerateLakeMesh(basin, network, heightmap, cellSize, chunkOffset);
            meshData.lakeIndex = static_cast<int>(i);
            lakeMeshes.push_back(std::move(meshData));
        }

        return lakeMeshes;
    }

    LakeMeshData LakeMeshGenerator::GenerateLakeMesh(const LakeBasin &basin,
                                                     const LakeNetwork &network,
                                                     const std::vector<float> &heightmap,
                                                     float cellSize,
                                                     const glm::vec3 &chunkOffset) const
    {
        LakeMeshData meshData;
        meshData.surfaceHeight = basin.surfaceHeight;

        // Compute center
        glm::vec2 center{0.0f};
        for (const auto &cell : basin.cells)
        {
            center += glm::vec2(cell.x, cell.y);
        }
        center /= static_cast<float>(basin.cells.size());
        meshData.center = chunkOffset + glm::vec3(center.x * cellSize, basin.surfaceHeight, center.y * cellSize);

        // Generate vertices for each lake cell
        // Use a simple grid approach - one quad per cell
        size_t baseVertex = 0;

        for (const auto &cell : basin.cells)
        {
            if (!network.InBounds(cell.x, cell.y))
            {
                continue;
            }

            size_t idx = network.Index(cell.x, cell.y);
            float depth = network.cellLakeDepth[idx];
            glm::vec3 color = ComputeDepthColor(depth);

            // Cell corners in world space
            float x0 = chunkOffset.x + cell.x * cellSize;
            float x1 = chunkOffset.x + (cell.x + 1) * cellSize;
            float z0 = chunkOffset.z + cell.y * cellSize;
            float z1 = chunkOffset.z + (cell.y + 1) * cellSize;
            float y = basin.surfaceHeight;

            // Create quad vertices
            Vertex v0, v1, v2, v3;

            v0.Position = glm::vec3(x0, y, z0);
            v0.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            v0.Color = color;
            v0.TexCoord = glm::vec2(0.0f, 0.0f);

            v1.Position = glm::vec3(x1, y, z0);
            v1.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            v1.Color = color;
            v1.TexCoord = glm::vec2(1.0f, 0.0f);

            v2.Position = glm::vec3(x1, y, z1);
            v2.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            v2.Color = color;
            v2.TexCoord = glm::vec2(1.0f, 1.0f);

            v3.Position = glm::vec3(x0, y, z1);
            v3.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            v3.Color = color;
            v3.TexCoord = glm::vec2(0.0f, 1.0f);

            meshData.vertices.push_back(v0);
            meshData.vertices.push_back(v1);
            meshData.vertices.push_back(v2);
            meshData.vertices.push_back(v3);

            // Two triangles per quad (CCW winding)
            uint32_t base = static_cast<uint32_t>(baseVertex);
            meshData.indices.push_back(base + 0);
            meshData.indices.push_back(base + 3);
            meshData.indices.push_back(base + 1);

            meshData.indices.push_back(base + 1);
            meshData.indices.push_back(base + 3);
            meshData.indices.push_back(base + 2);

            baseVertex += 4;
        }

        return meshData;
    }

    std::unique_ptr<Mesh> LakeMeshGenerator::CreateMesh(const LakeMeshData &data) const
    {
        if (data.vertices.empty() || data.indices.empty())
        {
            return nullptr;
        }

        return std::make_unique<Mesh>(data.vertices, data.indices);
    }

    std::unique_ptr<Mesh> LakeMeshGenerator::CreateCombinedMesh(const std::vector<LakeMeshData> &lakes) const
    {
        if (lakes.empty())
        {
            return nullptr;
        }

        std::vector<Vertex> allVertices;
        std::vector<uint32_t> allIndices;

        size_t totalVertices = 0;
        size_t totalIndices = 0;

        for (const auto &lake : lakes)
        {
            totalVertices += lake.vertices.size();
            totalIndices += lake.indices.size();
        }

        allVertices.reserve(totalVertices);
        allIndices.reserve(totalIndices);

        uint32_t vertexOffset = 0;

        for (const auto &lake : lakes)
        {
            allVertices.insert(allVertices.end(), lake.vertices.begin(), lake.vertices.end());

            for (uint32_t idx : lake.indices)
            {
                allIndices.push_back(idx + vertexOffset);
            }

            vertexOffset += static_cast<uint32_t>(lake.vertices.size());
        }

        if (allVertices.empty())
        {
            return nullptr;
        }

        return std::make_unique<Mesh>(allVertices, allIndices);
    }

    std::vector<glm::vec2> LakeMeshGenerator::ComputeBoundaryPolygon(const LakeBasin &basin,
                                                                     float cellSize) const
    {
        // Simple boundary extraction - find cells with non-lake neighbors
        std::vector<glm::vec2> boundary;

        // Create a set for quick lookup
        std::set<std::pair<int, int>> lakeSet;
        for (const auto &cell : basin.cells)
        {
            lakeSet.insert({cell.x, cell.y});
        }

        // Find boundary cells
        for (const auto &cell : basin.cells)
        {
            bool isBoundary = false;
            const int dx[] = {-1, 1, 0, 0};
            const int dy[] = {0, 0, -1, 1};

            for (int d = 0; d < 4; d++)
            {
                if (lakeSet.find({cell.x + dx[d], cell.y + dy[d]}) == lakeSet.end())
                {
                    isBoundary = true;
                    break;
                }
            }

            if (isBoundary)
            {
                boundary.push_back(glm::vec2((cell.x + 0.5f) * cellSize, (cell.y + 0.5f) * cellSize));
            }
        }

        return boundary;
    }

    void LakeMeshGenerator::TriangulatePolygon(const std::vector<glm::vec2> &polygon,
                                               std::vector<uint32_t> &indices,
                                               size_t vertexOffset) const
    {
        // Simple fan triangulation (works for convex polygons)
        if (polygon.size() < 3)
        {
            return;
        }

        for (size_t i = 1; i < polygon.size() - 1; i++)
        {
            indices.push_back(static_cast<uint32_t>(vertexOffset));
            indices.push_back(static_cast<uint32_t>(vertexOffset + i));
            indices.push_back(static_cast<uint32_t>(vertexOffset + i + 1));
        }
    }

    glm::vec3 LakeMeshGenerator::ComputeDepthColor(float depth) const
    {
        float t = std::min(depth / m_Settings.colorDepthScale, 1.0f);
        return glm::mix(m_Settings.shallowColor, m_Settings.deepColor, t);
    }

}
