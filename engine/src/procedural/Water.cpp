#include "genesis/procedural/Water.h"
#include "genesis/renderer/Mesh.h"

namespace Genesis {

    std::shared_ptr<Mesh> WaterGenerator::GenerateWaterPlane(
        float chunkX, float chunkZ,
        float chunkSize,
        int subdivisions,
        float seaLevel) {
        
        return GeneratePlane(
            chunkX + chunkSize * 0.5f,
            chunkZ + chunkSize * 0.5f,
            chunkSize,
            chunkSize,
            subdivisions,
            seaLevel);
    }

    std::shared_ptr<Mesh> WaterGenerator::GeneratePlane(
        float centerX, float centerZ,
        float width, float depth,
        int subdivisions,
        float height) {
        
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        
        float halfWidth = width * 0.5f;
        float halfDepth = depth * 0.5f;
        float cellWidth = width / subdivisions;
        float cellDepth = depth / subdivisions;
        
        // Water color (blueish)
        glm::vec3 waterColor(0.1f, 0.4f, 0.6f);
        
        // Generate vertices
        for (int z = 0; z <= subdivisions; z++) {
            for (int x = 0; x <= subdivisions; x++) {
                Vertex v;
                v.Position = glm::vec3(
                    centerX - halfWidth + x * cellWidth,
                    height,
                    centerZ - halfDepth + z * cellDepth);
                v.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                v.Color = waterColor;
                v.TexCoord = glm::vec2(
                    static_cast<float>(x) / subdivisions,
                    static_cast<float>(z) / subdivisions);
                vertices.push_back(v);
            }
        }
        
        // Generate indices (CCW winding order)
        for (int z = 0; z < subdivisions; z++) {
            for (int x = 0; x < subdivisions; x++) {
                int topLeft = z * (subdivisions + 1) + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * (subdivisions + 1) + x;
                int bottomRight = bottomLeft + 1;
                
                // First triangle (CCW)
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);
                
                // Second triangle (CCW)
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }
        
        return std::make_shared<Mesh>(vertices, indices);
    }

} // namespace Genesis
