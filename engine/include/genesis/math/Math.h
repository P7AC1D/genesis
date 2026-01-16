#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <random>
#include <cmath>

namespace Genesis {

    class Math {
    public:
        static constexpr float PI = 3.14159265358979323846f;
        static constexpr float TAU = PI * 2.0f;
        static constexpr float EPSILON = 1e-6f;

        static float Radians(float degrees) { return glm::radians(degrees); }
        static float Degrees(float radians) { return glm::degrees(radians); }

        static float Lerp(float a, float b, float t) { return a + (b - a) * t; }
        static glm::vec3 Lerp(const glm::vec3& a, const glm::vec3& b, float t) { return glm::mix(a, b, t); }

        static float Clamp(float value, float min, float max) { return glm::clamp(value, min, max); }
        
        static float SmoothStep(float edge0, float edge1, float x) {
            float t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        // Random number generation for procedural content
        static float Random() {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_real_distribution<float> dis(0.0f, 1.0f);
            return dis(gen);
        }

        static float Random(float min, float max) {
            return min + Random() * (max - min);
        }

        static int RandomInt(int min, int max) {
            return min + static_cast<int>(Random() * (max - min + 1));
        }

        static glm::vec3 RandomInUnitSphere() {
            float theta = Random() * TAU;
            float phi = acos(2.0f * Random() - 1.0f);
            return glm::vec3(
                sin(phi) * cos(theta),
                sin(phi) * sin(theta),
                cos(phi)
            );
        }

        // Perlin noise (simplified implementation for procedural terrain)
        static float PerlinNoise(float x, float y, int seed = 0);
        static float FBM(float x, float y, int octaves = 4, float persistence = 0.5f, int seed = 0);

        // Distance helpers
        static float Distance(const glm::vec3& a, const glm::vec3& b) { return glm::distance(a, b); }
        static float DistanceSquared(const glm::vec3& a, const glm::vec3& b) { return glm::distance2(a, b); }
    };

    // Procedural generation helpers
    namespace Procedural {
        
        // Heightmap generation for terrain
        float GenerateHeight(float x, float z, int seed = 0, float scale = 1.0f);
        
        // Color generation based on height (for low-poly terrain)
        glm::vec3 GetTerrainColor(float height, float maxHeight);
        
        // Tree/rock placement
        bool ShouldPlaceObject(float x, float z, float density, int seed = 0);
    }

}
