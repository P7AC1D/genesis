#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Genesis
{

    class Mesh;

    // Water settings for rendering
    struct WaterSettings
    {
        // Sea level can be specified as normalized (0-1) or absolute
        // seaLevel = baseHeight + heightScale * seaLevelNormalized
        float seaLevelNormalized = 0.45f; // Typical value: 45% of height range
        float seaLevel = 0.0f;            // Computed absolute sea level (world units)

        // Visual settings
        glm::vec3 shallowColor = glm::vec3(0.1f, 0.5f, 0.6f);
        glm::vec3 deepColor = glm::vec3(0.0f, 0.2f, 0.4f);
        float transparency = 0.7f;
        float waveHeight = 0.15f;
        float waveSpeed = 1.0f;
        bool enabled = true;

        // Ocean vs inland lake distinction
        bool useOceanMask = true;      // If true, only render water where ocean-connected
        bool renderInlandLakes = true; // If true, also render inland lakes (different style possible)

        // Compute absolute sea level from terrain settings
        void ComputeSeaLevel(float baseHeight, float heightScale)
        {
            seaLevel = baseHeight + heightScale * seaLevelNormalized;
        }
    };

    // Generates water plane meshes
    class WaterGenerator
    {
    public:
        // Generate a water plane for a chunk
        // chunkX, chunkZ: chunk coordinates in world space
        // chunkSize: size of one chunk
        // subdivisions: mesh detail level
        static std::shared_ptr<Mesh> GenerateWaterPlane(
            float chunkX, float chunkZ,
            float chunkSize,
            int subdivisions = 8,
            float seaLevel = 0.0f);

        // Generate a single large water plane
        static std::shared_ptr<Mesh> GeneratePlane(
            float centerX, float centerZ,
            float width, float depth,
            int subdivisions = 16,
            float height = 0.0f);
    };

} // namespace Genesis
