#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Genesis {

    // Simplex noise implementation for procedural generation
    class SimplexNoise {
    public:
        SimplexNoise(uint32_t seed = 0);

        // 2D noise [-1, 1]
        float Noise(float x, float y) const;
        
        // 3D noise [-1, 1]
        float Noise(float x, float y, float z) const;

        // Fractal Brownian Motion (layered noise)
        float FBM(float x, float y, int octaves, float persistence = 0.5f, float lacunarity = 2.0f) const;
        float FBM(float x, float y, float z, int octaves, float persistence = 0.5f, float lacunarity = 2.0f) const;

        // Ridge noise (inverted absolute value for mountain ridges)
        float RidgeNoise(float x, float y, int octaves, float persistence = 0.5f, float lacunarity = 2.0f) const;

        void SetSeed(uint32_t seed);

    private:
        float Grad(int hash, float x, float y) const;
        float Grad(int hash, float x, float y, float z) const;
        
        std::vector<int> m_Permutation;
    };

}
