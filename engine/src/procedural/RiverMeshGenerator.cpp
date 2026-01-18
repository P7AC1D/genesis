#include "genesis/procedural/RiverMeshGenerator.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

namespace Genesis
{

    std::vector<RiverMeshData> RiverMeshGenerator::Generate(const RiverNetwork &network,
                                                            const std::vector<float> &heightmap,
                                                            float cellSize,
                                                            const glm::vec3 &chunkOffset) const
    {
        std::vector<RiverMeshData> riverMeshes;
        riverMeshes.reserve(network.rivers.size());

        for (size_t i = 0; i < network.rivers.size(); i++)
        {
            const auto &path = network.rivers[i];

            // Skip very short rivers
            if (path.segmentIndices.size() < 2)
            {
                continue;
            }

            RiverMeshData meshData = GenerateRiverMesh(path, network, heightmap, cellSize, chunkOffset);
            meshData.riverIndex = i;
            riverMeshes.push_back(std::move(meshData));
        }

        return riverMeshes;
    }

    std::unique_ptr<Mesh> RiverMeshGenerator::GenerateCombinedMesh(const RiverNetwork &network,
                                                                   const std::vector<float> &heightmap,
                                                                   float cellSize,
                                                                   const glm::vec3 &chunkOffset) const
    {
        std::vector<Vertex> allVertices;
        std::vector<uint32_t> allIndices;

        // Process all river segments directly for efficiency
        for (size_t i = 0; i < network.segments.size(); i++)
        {
            const auto &segment = network.segments[i];

            // Only process rivers and streams (not none/lake/ocean)
            if (segment.type != WaterType::Stream && segment.type != WaterType::River)
            {
                continue;
            }

            // Find next segment for smooth connection
            const RiverSegment *nextSegment = nullptr;
            if (segment.downstreamIndex >= 0 && segment.downstreamIndex < static_cast<int>(network.segments.size()))
            {
                nextSegment = &network.segments[segment.downstreamIndex];
            }

            GenerateRibbonSegment(segment, nextSegment, heightmap, network.width,
                                  cellSize, chunkOffset, allVertices, allIndices);
        }

        if (allVertices.empty())
        {
            return nullptr;
        }

        return std::make_unique<Mesh>(allVertices, allIndices);
    }

    RiverMeshData RiverMeshGenerator::GenerateRiverMesh(const RiverPath &path,
                                                        const RiverNetwork &network,
                                                        const std::vector<float> &heightmap,
                                                        float cellSize,
                                                        const glm::vec3 &chunkOffset) const
    {
        RiverMeshData meshData;

        if (path.segmentIndices.empty())
        {
            return meshData;
        }

        // Track start/end points
        const auto &firstSeg = network.segments[path.segmentIndices.front()];
        const auto &lastSeg = network.segments[path.segmentIndices.back()];

        meshData.startPoint = chunkOffset + glm::vec3(
                                                firstSeg.cell.x * cellSize,
                                                firstSeg.surfaceHeight,
                                                firstSeg.cell.y * cellSize);

        meshData.endPoint = chunkOffset + glm::vec3(
                                              lastSeg.cell.x * cellSize,
                                              lastSeg.surfaceHeight,
                                              lastSeg.cell.y * cellSize);

        // Calculate average width
        float totalWidth = 0.0f;
        for (size_t idx : path.segmentIndices)
        {
            totalWidth += network.segments[idx].width;
        }
        meshData.averageWidth = totalWidth / path.segmentIndices.size();

        // Generate ribbon vertices for each segment
        for (size_t i = 0; i < path.segmentIndices.size(); i++)
        {
            const auto &segment = network.segments[path.segmentIndices[i]];

            // Get next segment for smooth connection
            const RiverSegment *nextSegment = nullptr;
            if (i + 1 < path.segmentIndices.size())
            {
                nextSegment = &network.segments[path.segmentIndices[i + 1]];
            }

            GenerateRibbonSegment(segment, nextSegment, heightmap, network.width,
                                  cellSize, chunkOffset, meshData.vertices, meshData.indices);
        }

        return meshData;
    }

    void RiverMeshGenerator::GenerateRibbonSegment(const RiverSegment &segment,
                                                   const RiverSegment *nextSegment,
                                                   const std::vector<float> &heightmap,
                                                   int gridWidth,
                                                   float cellSize,
                                                   const glm::vec3 &chunkOffset,
                                                   std::vector<Vertex> &vertices,
                                                   std::vector<uint32_t> &indices) const
    {
        // Current position
        float cx = (segment.cell.x + 0.5f) * cellSize;
        float cz = (segment.cell.y + 0.5f) * cellSize;
        float cy = segment.surfaceHeight + m_Settings.surfaceOffset;

        // Next position (for direction calculation)
        float nx, nz;
        if (nextSegment)
        {
            nx = (nextSegment->cell.x + 0.5f) * cellSize;
            nz = (nextSegment->cell.y + 0.5f) * cellSize;
        }
        else
        {
            // Use downstream direction from current cell
            nx = cx + cellSize;
            nz = cz;
        }

        // Direction vector
        glm::vec2 dir = glm::normalize(glm::vec2(nx - cx, nz - cz));

        // Perpendicular vector for ribbon width
        glm::vec2 perp(-dir.y, dir.x);

        float halfWidth = segment.width * 0.5f;

        // Compute color based on river properties
        float slope = 0.0f; // Could compute from height difference
        glm::vec3 color = ComputeFlowColor(segment.width, segment.depth, slope);

        // Create ribbon vertices (4 vertices per segment for quad)
        uint32_t baseIndex = static_cast<uint32_t>(vertices.size());

        // Left edge - current
        Vertex vl0;
        vl0.Position = glm::vec3(chunkOffset.x + cx - perp.x * halfWidth,
                                 cy,
                                 chunkOffset.z + cz - perp.y * halfWidth);
        vl0.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vl0.Color = color;
        vl0.TexCoord = glm::vec2(0.0f, 0.0f);

        // Right edge - current
        Vertex vr0;
        vr0.Position = glm::vec3(chunkOffset.x + cx + perp.x * halfWidth,
                                 cy,
                                 chunkOffset.z + cz + perp.y * halfWidth);
        vr0.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vr0.Color = color;
        vr0.TexCoord = glm::vec2(1.0f, 0.0f);

        // Next position height
        float ny = nextSegment ? (nextSegment->surfaceHeight + m_Settings.surfaceOffset) : cy;

        // Left edge - next
        Vertex vl1;
        vl1.Position = glm::vec3(chunkOffset.x + nx - perp.x * halfWidth,
                                 ny,
                                 chunkOffset.z + nz - perp.y * halfWidth);
        vl1.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vl1.Color = color;
        vl1.TexCoord = glm::vec2(0.0f, 1.0f);

        // Right edge - next
        Vertex vr1;
        vr1.Position = glm::vec3(chunkOffset.x + nx + perp.x * halfWidth,
                                 ny,
                                 chunkOffset.z + nz + perp.y * halfWidth);
        vr1.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vr1.Color = color;
        vr1.TexCoord = glm::vec2(1.0f, 1.0f);

        vertices.push_back(vl0);
        vertices.push_back(vr0);
        vertices.push_back(vl1);
        vertices.push_back(vr1);

        // Two triangles per quad (CCW winding)
        // Triangle 1: vl0, vl1, vr0
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 1);

        // Triangle 2: vr0, vl1, vr1
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);
    }

    glm::vec2 RiverMeshGenerator::ComputePerpendicular(const glm::ivec2 &current,
                                                       const glm::ivec2 &next) const
    {
        glm::vec2 dir = glm::normalize(glm::vec2(next - current));
        return glm::vec2(-dir.y, dir.x);
    }

    glm::vec3 RiverMeshGenerator::ComputeFlowColor(float width, float depth, float slope) const
    {
        // Blend between shallow and deep based on depth
        float depthFactor = std::min(depth / 5.0f, 1.0f);
        glm::vec3 color = glm::mix(m_Settings.shallowColor, m_Settings.deepColor, depthFactor);

        // Add foam on steep sections
        if (slope > m_Settings.foamThreshold)
        {
            float foamAmount = (slope - m_Settings.foamThreshold) / (1.0f - m_Settings.foamThreshold);
            color = glm::mix(color, m_Settings.foamColor, foamAmount * 0.5f);
        }

        return color;
    }

    float RiverMeshGenerator::SampleHeight(const std::vector<float> &heightmap,
                                           int gridWidth,
                                           float cellSize,
                                           float worldX,
                                           float worldZ,
                                           const glm::vec3 &chunkOffset) const
    {
        float localX = worldX - chunkOffset.x;
        float localZ = worldZ - chunkOffset.z;

        int cellX = static_cast<int>(localX / cellSize);
        int cellZ = static_cast<int>(localZ / cellSize);

        if (cellX < 0 || cellX >= gridWidth || cellZ < 0)
        {
            return 0.0f;
        }

        size_t idx = static_cast<size_t>(cellZ) * gridWidth + cellX;
        if (idx >= heightmap.size())
        {
            return 0.0f;
        }

        return heightmap[idx];
    }

    std::unique_ptr<Mesh> RiverMeshGenerator::CreateMesh(const RiverMeshData &data) const
    {
        if (data.vertices.empty() || data.indices.empty())
        {
            return nullptr;
        }

        return std::make_unique<Mesh>(data.vertices, data.indices);
    }

}
