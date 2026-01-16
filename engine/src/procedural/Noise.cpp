#include "genesis/procedural/Noise.h"
#include <algorithm>
#include <numeric>
#include <random>
#include <cmath>

namespace Genesis {

    SimplexNoise::SimplexNoise(uint32_t seed) {
        SetSeed(seed);
    }

    void SimplexNoise::SetSeed(uint32_t seed) {
        m_Permutation.resize(512);
        
        // Initialize with values 0-255
        std::vector<int> p(256);
        std::iota(p.begin(), p.end(), 0);
        
        // Shuffle using seed
        std::mt19937 rng(seed);
        std::shuffle(p.begin(), p.end(), rng);
        
        // Duplicate for overflow handling
        for (int i = 0; i < 256; i++) {
            m_Permutation[i] = p[i];
            m_Permutation[256 + i] = p[i];
        }
    }

    float SimplexNoise::Grad(int hash, float x, float y) const {
        int h = hash & 7;
        float u = h < 4 ? x : y;
        float v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
    }

    float SimplexNoise::Grad(int hash, float x, float y, float z) const {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }

    float SimplexNoise::Noise(float x, float y) const {
        // Skew factors for 2D
        const float F2 = 0.5f * (std::sqrt(3.0f) - 1.0f);
        const float G2 = (3.0f - std::sqrt(3.0f)) / 6.0f;

        float s = (x + y) * F2;
        int i = static_cast<int>(std::floor(x + s));
        int j = static_cast<int>(std::floor(y + s));

        float t = (i + j) * G2;
        float X0 = i - t;
        float Y0 = j - t;
        float x0 = x - X0;
        float y0 = y - Y0;

        int i1, j1;
        if (x0 > y0) { i1 = 1; j1 = 0; }
        else { i1 = 0; j1 = 1; }

        float x1 = x0 - i1 + G2;
        float y1 = y0 - j1 + G2;
        float x2 = x0 - 1.0f + 2.0f * G2;
        float y2 = y0 - 1.0f + 2.0f * G2;

        int ii = i & 255;
        int jj = j & 255;

        float n0, n1, n2;

        float t0 = 0.5f - x0 * x0 - y0 * y0;
        if (t0 < 0) n0 = 0.0f;
        else {
            t0 *= t0;
            n0 = t0 * t0 * Grad(m_Permutation[ii + m_Permutation[jj]], x0, y0);
        }

        float t1 = 0.5f - x1 * x1 - y1 * y1;
        if (t1 < 0) n1 = 0.0f;
        else {
            t1 *= t1;
            n1 = t1 * t1 * Grad(m_Permutation[ii + i1 + m_Permutation[jj + j1]], x1, y1);
        }

        float t2 = 0.5f - x2 * x2 - y2 * y2;
        if (t2 < 0) n2 = 0.0f;
        else {
            t2 *= t2;
            n2 = t2 * t2 * Grad(m_Permutation[ii + 1 + m_Permutation[jj + 1]], x2, y2);
        }

        return 70.0f * (n0 + n1 + n2);
    }

    float SimplexNoise::Noise(float x, float y, float z) const {
        // Skew factors for 3D
        const float F3 = 1.0f / 3.0f;
        const float G3 = 1.0f / 6.0f;

        float s = (x + y + z) * F3;
        int i = static_cast<int>(std::floor(x + s));
        int j = static_cast<int>(std::floor(y + s));
        int k = static_cast<int>(std::floor(z + s));

        float t = (i + j + k) * G3;
        float X0 = i - t;
        float Y0 = j - t;
        float Z0 = k - t;
        float x0 = x - X0;
        float y0 = y - Y0;
        float z0 = z - Z0;

        int i1, j1, k1;
        int i2, j2, k2;

        if (x0 >= y0) {
            if (y0 >= z0) { i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; }
            else if (x0 >= z0) { i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; }
            else { i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; }
        } else {
            if (y0 < z0) { i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; }
            else if (x0 < z0) { i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; }
            else { i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; }
        }

        float x1 = x0 - i1 + G3;
        float y1 = y0 - j1 + G3;
        float z1 = z0 - k1 + G3;
        float x2 = x0 - i2 + 2.0f * G3;
        float y2 = y0 - j2 + 2.0f * G3;
        float z2 = z0 - k2 + 2.0f * G3;
        float x3 = x0 - 1.0f + 3.0f * G3;
        float y3 = y0 - 1.0f + 3.0f * G3;
        float z3 = z0 - 1.0f + 3.0f * G3;

        int ii = i & 255;
        int jj = j & 255;
        int kk = k & 255;

        float n0, n1, n2, n3;

        float t0 = 0.6f - x0*x0 - y0*y0 - z0*z0;
        if (t0 < 0) n0 = 0.0f;
        else {
            t0 *= t0;
            n0 = t0 * t0 * Grad(m_Permutation[ii + m_Permutation[jj + m_Permutation[kk]]], x0, y0, z0);
        }

        float t1 = 0.6f - x1*x1 - y1*y1 - z1*z1;
        if (t1 < 0) n1 = 0.0f;
        else {
            t1 *= t1;
            n1 = t1 * t1 * Grad(m_Permutation[ii + i1 + m_Permutation[jj + j1 + m_Permutation[kk + k1]]], x1, y1, z1);
        }

        float t2 = 0.6f - x2*x2 - y2*y2 - z2*z2;
        if (t2 < 0) n2 = 0.0f;
        else {
            t2 *= t2;
            n2 = t2 * t2 * Grad(m_Permutation[ii + i2 + m_Permutation[jj + j2 + m_Permutation[kk + k2]]], x2, y2, z2);
        }

        float t3 = 0.6f - x3*x3 - y3*y3 - z3*z3;
        if (t3 < 0) n3 = 0.0f;
        else {
            t3 *= t3;
            n3 = t3 * t3 * Grad(m_Permutation[ii + 1 + m_Permutation[jj + 1 + m_Permutation[kk + 1]]], x3, y3, z3);
        }

        return 32.0f * (n0 + n1 + n2 + n3);
    }

    float SimplexNoise::FBM(float x, float y, int octaves, float persistence, float lacunarity) const {
        float total = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; i++) {
            total += Noise(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return total / maxValue;
    }

    float SimplexNoise::FBM(float x, float y, float z, int octaves, float persistence, float lacunarity) const {
        float total = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; i++) {
            total += Noise(x * frequency, y * frequency, z * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return total / maxValue;
    }

    float SimplexNoise::RidgeNoise(float x, float y, int octaves, float persistence, float lacunarity) const {
        float total = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; i++) {
            float n = Noise(x * frequency, y * frequency);
            n = 1.0f - std::abs(n);  // Invert to create ridges
            n = n * n;  // Square to sharpen ridges
            total += n * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return total / maxValue;
    }

}
