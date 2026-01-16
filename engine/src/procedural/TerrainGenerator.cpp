#include "genesis/procedural/TerrainGenerator.h"
#include <algorithm>

namespace Genesis
{

    TerrainGenerator::TerrainGenerator()
        : m_Noise(m_Settings.seed)
    {
    }

    TerrainGenerator::TerrainGenerator(const TerrainSettings &settings)
        : m_Settings(settings), m_Noise(settings.seed)
    {
    }

    void TerrainGenerator::SetSettings(const TerrainSettings &settings)
    {
        m_Settings = settings;
        m_Noise.SetSeed(settings.seed);
        m_CachedHeightmap.clear();
    }

    float TerrainGenerator::SampleHeight(float x, float z) const
    {
        float noiseX = x * m_Settings.noiseScale;
        float noiseZ = z * m_Settings.noiseScale;

        // Get base terrain noise
        float baseNoise = m_Noise.FBM(noiseX, noiseZ,
                                      m_Settings.octaves,
                                      m_Settings.persistence,
                                      m_Settings.lacunarity);

        float height = baseNoise;

        if (m_Settings.useRidgeNoise)
        {
            // Get ridge noise for sharp mountain crests
            float ridgeNoiseX = noiseX * m_Settings.ridgeScale;
            float ridgeNoiseZ = noiseZ * m_Settings.ridgeScale;
            float ridgeNoise = m_Noise.RidgeNoise(ridgeNoiseX, ridgeNoiseZ,
                                                  m_Settings.octaves,
                                                  m_Settings.persistence,
                                                  m_Settings.lacunarity);

            // Apply power function to sharpen ridge peaks
            ridgeNoise = std::pow(ridgeNoise, m_Settings.ridgePower);

            // Calculate uplift mask - determines where mountains appear
            float upliftMask = 1.0f;
            if (m_Settings.useUpliftMask)
            {
                float upliftNoiseX = x * m_Settings.upliftScale;
                float upliftNoiseZ = z * m_Settings.upliftScale;
                float upliftNoise = m_Noise.FBM(upliftNoiseX, upliftNoiseZ, 2, 0.5f, 2.0f);
                upliftNoise = (upliftNoise + 1.0f) * 0.5f; // Map to [0, 1]

                // Smoothstep for gradual transition from plains to mountains
                float t = (upliftNoise - m_Settings.upliftThresholdLow) /
                          (m_Settings.upliftThresholdHigh - m_Settings.upliftThresholdLow);
                t = std::clamp(t, 0.0f, 1.0f);
                upliftMask = t * t * (3.0f - 2.0f * t); // Smoothstep
                upliftMask = std::pow(upliftMask, m_Settings.upliftPower);
            }

            // Apply uplift mask to ridge contribution
            float ridgeContribution = ridgeNoise * m_Settings.ridgeWeight * upliftMask;
            float baseWeight = 1.0f - (m_Settings.ridgeWeight * upliftMask);
            height = baseNoise * baseWeight + ridgeContribution;
        }

        // Map from [-1, 1] to [0, 1]
        height = (height + 1.0f) * 0.5f;

        return m_Settings.baseHeight + height * m_Settings.heightScale;
    }

    std::vector<float> TerrainGenerator::GenerateHeightmap()
    {
        int width = m_Settings.width + 1;
        int depth = m_Settings.depth + 1;

        m_CachedHeightmap.resize(width * depth);

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                float worldX = x * m_Settings.cellSize;
                float worldZ = z * m_Settings.cellSize;
                m_CachedHeightmap[z * width + x] = SampleHeight(worldX, worldZ);
            }
        }

        return m_CachedHeightmap;
    }

    float TerrainGenerator::GetHeightAt(float worldX, float worldZ) const
    {
        if (m_CachedHeightmap.empty())
        {
            return SampleHeight(worldX, worldZ);
        }

        // Bilinear interpolation from cached heightmap
        float gridX = worldX / m_Settings.cellSize;
        float gridZ = worldZ / m_Settings.cellSize;

        int x0 = static_cast<int>(std::floor(gridX));
        int z0 = static_cast<int>(std::floor(gridZ));
        int x1 = x0 + 1;
        int z1 = z0 + 1;

        int width = m_Settings.width + 1;
        int depth = m_Settings.depth + 1;

        x0 = std::clamp(x0, 0, width - 1);
        x1 = std::clamp(x1, 0, width - 1);
        z0 = std::clamp(z0, 0, depth - 1);
        z1 = std::clamp(z1, 0, depth - 1);

        float fx = gridX - std::floor(gridX);
        float fz = gridZ - std::floor(gridZ);

        float h00 = m_CachedHeightmap[z0 * width + x0];
        float h10 = m_CachedHeightmap[z0 * width + x1];
        float h01 = m_CachedHeightmap[z1 * width + x0];
        float h11 = m_CachedHeightmap[z1 * width + x1];

        float h0 = h00 * (1 - fx) + h10 * fx;
        float h1 = h01 * (1 - fx) + h11 * fx;

        return h0 * (1 - fz) + h1 * fz;
    }

    glm::vec3 TerrainGenerator::GetHeightColor(float normalizedHeight, const TerrainSettings &settings)
    {
        // Low-poly color palette
        const glm::vec3 deepWater(0.1f, 0.3f, 0.5f);
        const glm::vec3 shallowWater(0.2f, 0.5f, 0.7f);
        const glm::vec3 sand(0.76f, 0.70f, 0.50f);
        const glm::vec3 grass(0.34f, 0.55f, 0.25f);
        const glm::vec3 darkGrass(0.24f, 0.42f, 0.18f);
        const glm::vec3 rock(0.5f, 0.5f, 0.5f);
        const glm::vec3 snow(0.95f, 0.95f, 0.97f);

        if (normalizedHeight < settings.waterLevel * 0.5f)
        {
            return deepWater;
        }
        else if (normalizedHeight < settings.waterLevel)
        {
            return shallowWater;
        }
        else if (normalizedHeight < settings.sandLevel)
        {
            return sand;
        }
        else if (normalizedHeight < settings.grassLevel * 0.7f)
        {
            return grass;
        }
        else if (normalizedHeight < settings.grassLevel)
        {
            return darkGrass;
        }
        else if (normalizedHeight < settings.rockLevel)
        {
            return rock;
        }
        else
        {
            return snow;
        }
    }

    std::shared_ptr<Mesh> TerrainGenerator::Generate()
    {
        GenerateHeightmap();

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        int width = m_Settings.width + 1;
        int depth = m_Settings.depth + 1;

        // Find min/max height for normalization
        float minHeight = *std::min_element(m_CachedHeightmap.begin(), m_CachedHeightmap.end());
        float maxHeight = *std::max_element(m_CachedHeightmap.begin(), m_CachedHeightmap.end());
        float heightRange = maxHeight - minHeight;
        if (heightRange < 0.001f)
            heightRange = 1.0f;

        if (m_Settings.flatShading)
        {
            // Flat shading: each triangle has its own vertices with face normal
            for (int z = 0; z < m_Settings.depth; z++)
            {
                for (int x = 0; x < m_Settings.width; x++)
                {
                    // Get the 4 corners of this cell
                    float x0 = x * m_Settings.cellSize;
                    float x1 = (x + 1) * m_Settings.cellSize;
                    float z0 = z * m_Settings.cellSize;
                    float z1 = (z + 1) * m_Settings.cellSize;

                    float h00 = m_CachedHeightmap[z * width + x];
                    float h10 = m_CachedHeightmap[z * width + (x + 1)];
                    float h01 = m_CachedHeightmap[(z + 1) * width + x];
                    float h11 = m_CachedHeightmap[(z + 1) * width + (x + 1)];

                    glm::vec3 p00(x0, h00, z0);
                    glm::vec3 p10(x1, h10, z0);
                    glm::vec3 p01(x0, h01, z1);
                    glm::vec3 p11(x1, h11, z1);

                    // Triangle 1: p00, p01, p10 (CCW winding for upward normal)
                    glm::vec3 normal1 = glm::normalize(glm::cross(p01 - p00, p10 - p00));
                    float avgH1 = (h00 + h10 + h01) / 3.0f;
                    float normH1 = (avgH1 - minHeight) / heightRange;
                    glm::vec3 color1 = m_Settings.useHeightColors ? GetHeightColor(normH1, m_Settings) : glm::vec3(0.34f, 0.55f, 0.25f);

                    uint32_t baseIdx = static_cast<uint32_t>(vertices.size());
                    vertices.push_back({p00, normal1, color1});
                    vertices.push_back({p01, normal1, color1});
                    vertices.push_back({p10, normal1, color1});
                    indices.push_back(baseIdx);
                    indices.push_back(baseIdx + 1);
                    indices.push_back(baseIdx + 2);

                    // Triangle 2: p10, p01, p11 (CCW winding for upward normal)
                    glm::vec3 normal2 = glm::normalize(glm::cross(p01 - p10, p11 - p10));
                    float avgH2 = (h10 + h11 + h01) / 3.0f;
                    float normH2 = (avgH2 - minHeight) / heightRange;
                    glm::vec3 color2 = m_Settings.useHeightColors ? GetHeightColor(normH2, m_Settings) : glm::vec3(0.34f, 0.55f, 0.25f);

                    baseIdx = static_cast<uint32_t>(vertices.size());
                    vertices.push_back({p10, normal2, color2});
                    vertices.push_back({p01, normal2, color2});
                    vertices.push_back({p11, normal2, color2});
                    indices.push_back(baseIdx);
                    indices.push_back(baseIdx + 1);
                    indices.push_back(baseIdx + 2);
                }
            }
        }
        else
        {
            // Smooth shading: shared vertices with averaged normals
            // First pass: create vertices with positions and colors
            for (int z = 0; z < depth; z++)
            {
                for (int x = 0; x < width; x++)
                {
                    float worldX = x * m_Settings.cellSize;
                    float worldZ = z * m_Settings.cellSize;
                    float height = m_CachedHeightmap[z * width + x];

                    float normH = (height - minHeight) / heightRange;
                    glm::vec3 color = m_Settings.useHeightColors ? GetHeightColor(normH, m_Settings) : glm::vec3(0.34f, 0.55f, 0.25f);

                    vertices.push_back({glm::vec3(worldX, height, worldZ),
                                        glm::vec3(0.0f, 1.0f, 0.0f), // Will be recalculated
                                        color});
                }
            }

            // Generate indices and calculate normals
            std::vector<glm::vec3> normals(vertices.size(), glm::vec3(0.0f));

            for (int z = 0; z < m_Settings.depth; z++)
            {
                for (int x = 0; x < m_Settings.width; x++)
                {
                    uint32_t i00 = z * width + x;
                    uint32_t i10 = z * width + (x + 1);
                    uint32_t i01 = (z + 1) * width + x;
                    uint32_t i11 = (z + 1) * width + (x + 1);

                    // Triangle 1 (CCW: i00, i01, i10)
                    indices.push_back(i00);
                    indices.push_back(i01);
                    indices.push_back(i10);

                    glm::vec3 n1 = glm::cross(
                        vertices[i01].Position - vertices[i00].Position,
                        vertices[i10].Position - vertices[i00].Position);
                    normals[i00] += n1;
                    normals[i01] += n1;
                    normals[i10] += n1;

                    // Triangle 2 (CCW: i10, i01, i11)
                    indices.push_back(i10);
                    indices.push_back(i01);
                    indices.push_back(i11);

                    glm::vec3 n2 = glm::cross(
                        vertices[i01].Position - vertices[i10].Position,
                        vertices[i11].Position - vertices[i10].Position);
                    normals[i10] += n2;
                    normals[i01] += n2;
                    normals[i11] += n2;
                }
            }

            // Normalize all normals
            for (size_t i = 0; i < vertices.size(); i++)
            {
                vertices[i].Normal = glm::normalize(normals[i]);
            }
        }

        return std::make_shared<Mesh>(vertices, indices);
    }

}
