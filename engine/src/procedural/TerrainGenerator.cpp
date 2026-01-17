#include "genesis/procedural/TerrainGenerator.h"
#include <algorithm>
#include <cmath>
#include <random>

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

    float TerrainGenerator::SampleRawHeight(float worldX, float worldZ) const
    {
        float noiseX = worldX * m_Settings.noiseScale;
        float noiseZ = worldZ * m_Settings.noiseScale;

        // Layer 1: Base terrain noise (unwarped FBM for micro-detail)
        // Warping is applied only to ridge noise for macro features
        float baseNoise = m_Noise.FBM(noiseX, noiseZ,
                                      m_Settings.octaves,
                                      m_Settings.persistence,
                                      m_Settings.lacunarity);

        float height = baseNoise;

        // Layer 2: Ridge noise for mountains (optional)
        if (m_Settings.useRidgeNoise)
        {
            float ridgeCoordX = noiseX;
            float ridgeCoordZ = noiseZ;

            // Apply domain warping ONLY to ridge coordinates (macro features)
            if (m_Settings.useWarp && m_Settings.warpLevels > 0 && m_Settings.warpStrength > 0.0f)
            {
                float wx = ridgeCoordX;
                float wz = ridgeCoordZ;

                for (int level = 0; level < m_Settings.warpLevels; level++)
                {
                    float offsetX = 5.2f + level * 17.1f;
                    float offsetZ = 1.3f + level * 31.7f;
                    float offsetX2 = 9.7f + level * 23.5f;
                    float offsetZ2 = 2.8f + level * 13.9f;

                    float levelWarpStrength = m_Settings.warpStrength / (1.0f + level * 0.5f);
                    float levelWarpScale = m_Settings.warpScale * (1.0f + level * 0.3f);

                    float warpOffsetX = m_Noise.FBM(wx * levelWarpScale + offsetX, wz * levelWarpScale + offsetZ, 2, 0.5f, 2.0f) * levelWarpStrength;
                    float warpOffsetZ = m_Noise.FBM(wx * levelWarpScale + offsetX2, wz * levelWarpScale + offsetZ2, 2, 0.5f, 2.0f) * levelWarpStrength;

                    wx += warpOffsetX;
                    wz += warpOffsetZ;
                }

                ridgeCoordX = wx;
                ridgeCoordZ = wz;
            }

            // Ridge noise uses DECOUPLED spectrum (fewer octaves for longer, cleaner ridges)
            constexpr int RIDGE_OCTAVES = 3;
            constexpr float RIDGE_PERSISTENCE = 0.5f;
            constexpr float RIDGE_LACUNARITY = 2.0f;

            float ridgeNoiseX = ridgeCoordX * m_Settings.ridgeScale;
            float ridgeNoiseZ = ridgeCoordZ * m_Settings.ridgeScale;
            float ridgeNoise = m_Noise.RidgeNoise(ridgeNoiseX, ridgeNoiseZ,
                                                  RIDGE_OCTAVES,
                                                  RIDGE_PERSISTENCE,
                                                  RIDGE_LACUNARITY);

            // Apply power function to sharpen ridge peaks
            ridgeNoise = std::pow(ridgeNoise, m_Settings.ridgePower);

            // Layer 3: Tectonic uplift mask (determines WHERE mountains appear)
            float upliftMask = 1.0f;
            if (m_Settings.useUpliftMask)
            {
                float upliftNoiseX = worldX * m_Settings.upliftScale;
                float upliftNoiseZ = worldZ * m_Settings.upliftScale;
                float upliftNoise = m_Noise.FBM(upliftNoiseX, upliftNoiseZ, 2, 0.5f, 2.0f);
                upliftNoise = (upliftNoise + 1.0f) * 0.5f; // Map to [0, 1]

                // Smoothstep for gradual transition from plains to mountains
                float t = (upliftNoise - m_Settings.upliftThresholdLow) /
                          (m_Settings.upliftThresholdHigh - m_Settings.upliftThresholdLow);
                t = std::clamp(t, 0.0f, 1.0f);
                upliftMask = t * t * (3.0f - 2.0f * t); // Smoothstep
                upliftMask = std::pow(upliftMask, m_Settings.upliftPower);
            }

            // Blend base terrain with mountains based on uplift mask
            float ridgeContribution = ridgeNoise * m_Settings.ridgeWeight * upliftMask;
            float baseWeight = 1.0f - (m_Settings.ridgeWeight * upliftMask);
            height = baseNoise * baseWeight + ridgeContribution;
        }

        // Map from [-1, 1] to [0, 1] and convert to world height
        // NO shaping here - shaping is applied ONCE after erosion in ApplyPeakShaping
        height = (height + 1.0f) * 0.5f;
        return m_Settings.baseHeight + height * m_Settings.heightScale;
    }

    void TerrainGenerator::ApplySlopeErosion(std::vector<float> &heightmap, int width, int depth) const
    {
        // Simple smoothing-based erosion that doesn't create artifacts
        // Uses Laplacian smoothing weighted by slope

        std::vector<float> smoothed(heightmap.size());

        // Multiple passes for gradual erosion
        int passes = static_cast<int>(m_Settings.slopeErosionStrength * 3.0f) + 1;

        for (int pass = 0; pass < passes; pass++)
        {
            // Copy current state
            for (size_t i = 0; i < heightmap.size(); i++)
            {
                smoothed[i] = heightmap[i];
            }

            for (int z = 1; z < depth - 1; z++)
            {
                for (int x = 1; x < width - 1; x++)
                {
                    int idx = z * width + x;
                    float h = heightmap[idx];

                    // Get neighbors
                    float hL = heightmap[idx - 1];
                    float hR = heightmap[idx + 1];
                    float hU = heightmap[(z - 1) * width + x];
                    float hD = heightmap[(z + 1) * width + x];

                    // Calculate slope magnitude
                    float slopeX = (hR - hL) / (2.0f * m_Settings.cellSize);
                    float slopeZ = (hD - hU) / (2.0f * m_Settings.cellSize);
                    float slope = std::sqrt(slopeX * slopeX + slopeZ * slopeZ);

                    // Average of neighbors (Laplacian target)
                    float avg = (hL + hR + hU + hD) * 0.25f;

                    // Erosion factor based on slope (steep areas erode more)
                    float slopeFactor = std::min(1.0f, slope / 2.0f);

                    // Blend toward average proportional to slope
                    // Only erode downward (don't raise cells)
                    if (avg < h)
                    {
                        float erosionRate = slopeFactor * m_Settings.slopeErosionStrength * 0.15f;
                        smoothed[idx] = h + (avg - h) * erosionRate;
                    }

                    // Optional valley deepening (very gentle)
                    if (m_Settings.valleyDepth > 0.0f && h < avg)
                    {
                        // Already in a valley - deepen slightly
                        float valleyFactor = std::min(0.1f, (avg - h) / m_Settings.heightScale);
                        smoothed[idx] -= valleyFactor * m_Settings.valleyDepth * 0.5f;
                    }
                }
            }

            // Copy back
            for (size_t i = 0; i < heightmap.size(); i++)
            {
                heightmap[i] = smoothed[i];
            }
        }
    }

    float TerrainGenerator::SampleHeightBilinear(const std::vector<float> &heightmap, int width, float x, float z) const
    {
        int x0 = static_cast<int>(std::floor(x));
        int z0 = static_cast<int>(std::floor(z));
        int x1 = x0 + 1;
        int z1 = z0 + 1;

        float fx = x - x0;
        float fz = z - z0;

        float h00 = heightmap[z0 * width + x0];
        float h10 = heightmap[z0 * width + x1];
        float h01 = heightmap[z1 * width + x0];
        float h11 = heightmap[z1 * width + x1];

        float h0 = h00 * (1.0f - fx) + h10 * fx;
        float h1 = h01 * (1.0f - fx) + h11 * fx;
        return h0 * (1.0f - fz) + h1 * fz;
    }

    glm::vec2 TerrainGenerator::SampleGradientBilinear(const std::vector<float> &heightmap, int width, float x, float z) const
    {
        constexpr float EPSILON = 0.5f;
        float hL = SampleHeightBilinear(heightmap, width, x - EPSILON, z);
        float hR = SampleHeightBilinear(heightmap, width, x + EPSILON, z);
        float hU = SampleHeightBilinear(heightmap, width, x, z - EPSILON);
        float hD = SampleHeightBilinear(heightmap, width, x, z + EPSILON);
        return glm::vec2((hR - hL) / (2.0f * EPSILON), (hD - hU) / (2.0f * EPSILON));
    }

    void TerrainGenerator::ApplyHydraulicErosion(std::vector<float> &heightmap, int width, int depth, float offsetX, float offsetZ) const
    {
        // Create deterministic seed based on chunk position using integer grid coordinates
        int32_t chunkGridX = static_cast<int32_t>(std::floor(offsetX / (m_Settings.width * m_Settings.cellSize)));
        int32_t chunkGridZ = static_cast<int32_t>(std::floor(offsetZ / (m_Settings.depth * m_Settings.cellSize)));
        uint32_t chunkSeed = m_Settings.seed ^ (static_cast<uint32_t>(chunkGridX * 198491317) ^ static_cast<uint32_t>(chunkGridZ * 6542989));

        std::mt19937 rng(chunkSeed);
        std::uniform_real_distribution<float> distX(2.0f, static_cast<float>(width - 3));
        std::uniform_real_distribution<float> distZ(2.0f, static_cast<float>(depth - 3));

        for (int iter = 0; iter < m_Settings.erosionIterations; iter++)
        {
            // Spawn water droplet at random position
            float dropX = distX(rng);
            float dropZ = distZ(rng);
            float dirX = 0.0f;
            float dirZ = 0.0f;
            float speed = 1.0f;
            float water = 1.0f;
            float sediment = 0.0f;

            constexpr int MAX_DROPLET_STEPS = 64;
            for (int step = 0; step < MAX_DROPLET_STEPS; step++)
            {
                // Bounds check with margin for bilinear sampling
                if (dropX < 2.0f || dropX >= width - 2 || dropZ < 2.0f || dropZ >= depth - 2)
                    break;

                // Bilinear height and gradient sampling (eliminates grid bias)
                float hC = SampleHeightBilinear(heightmap, width, dropX, dropZ);
                glm::vec2 grad = SampleGradientBilinear(heightmap, width, dropX, dropZ);

                // Update direction with inertia
                dirX = dirX * m_Settings.erosionInertia - grad.x * (1.0f - m_Settings.erosionInertia);
                dirZ = dirZ * m_Settings.erosionInertia - grad.y * (1.0f - m_Settings.erosionInertia);

                // Normalize direction
                float len = std::sqrt(dirX * dirX + dirZ * dirZ);
                if (len < 0.0001f)
                    break;
                dirX /= len;
                dirZ /= len;

                // Move droplet
                float newX = dropX + dirX;
                float newZ = dropZ + dirZ;

                if (newX < 2.0f || newX >= width - 2 || newZ < 2.0f || newZ >= depth - 2)
                    break;

                float newH = SampleHeightBilinear(heightmap, width, newX, newZ);
                float deltaH = newH - hC;

                // Calculate sediment capacity
                float capacity = std::max(-deltaH, 0.01f) * speed * water * m_Settings.erosionCapacity;

                // Get cell index for erosion/deposition (use nearest cell)
                int cellX = static_cast<int>(std::round(dropX));
                int cellZ = static_cast<int>(std::round(dropZ));
                int idx = cellZ * width + cellX;

                if (sediment > capacity || deltaH > 0)
                {
                    // Deposit sediment
                    float depositAmount = (deltaH > 0) ? std::min(deltaH, sediment) : (sediment - capacity) * m_Settings.erosionDeposition;
                    sediment -= depositAmount;
                    heightmap[idx] += depositAmount;
                }
                else
                {
                    // Erode terrain
                    float erodeAmount = std::min((capacity - sediment) * 0.3f, -deltaH);
                    sediment += erodeAmount;
                    heightmap[idx] -= erodeAmount;
                }

                // Update droplet state
                dropX = newX;
                dropZ = newZ;
                speed = std::sqrt(std::max(0.0f, speed * speed + deltaH));
                water *= (1.0f - m_Settings.erosionEvaporation);

                if (water < 0.01f)
                    break;
            }
        }
    }

    void TerrainGenerator::ApplyPeakShaping(std::vector<float> &heightmap, int width, int depth) const
    {
        float minH = m_Settings.baseHeight;
        float maxH = m_Settings.baseHeight + m_Settings.heightScale;
        float range = maxH - minH;

        if (range <= 0.0f)
            return;

        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                int idx = z * width + x;
                float h = heightmap[idx];

                // Normalize height to [0,1]
                float heightNorm = std::clamp((h - minH) / range, 0.0f, 1.0f);

                // Soft bases, sharp peaks: multiply by lerp(1.0, 0.6, h)
                float shapeFactor = 1.0f - 0.4f * heightNorm;
                h = minH + (h - minH) * shapeFactor;

                // Extra peak sharpening
                h += std::pow(heightNorm, 4.0f) * m_Settings.peakBoost * range;

                heightmap[idx] = h;
            }
        }
    }

    std::vector<float> TerrainGenerator::GenerateHeightmap()
    {
        return GenerateHeightmapAtOffset(0.0f, 0.0f);
    }

    std::vector<float> TerrainGenerator::GenerateHeightmapAtOffset(float offsetX, float offsetZ)
    {
        int width = m_Settings.width + 1;
        int depth = m_Settings.depth + 1;

        // Border for erosion context - needs to be large enough for erosion to settle
        // but we'll blend results near edges to ensure seamless boundaries
        constexpr int BORDER = 8;
        constexpr int BLEND_ZONE = 6; // Cells from edge where we blend eroded -> raw
        int extWidth = width + 2 * BORDER;
        int extDepth = depth + 2 * BORDER;

        // Step 1: Generate raw heightmap (this is seamless across chunks)
        std::vector<float> rawHeightmap(width * depth);
        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                float worldX = offsetX + x * m_Settings.cellSize;
                float worldZ = offsetZ + z * m_Settings.cellSize;
                rawHeightmap[z * width + x] = SampleRawHeight(worldX, worldZ);
            }
        }

        // Step 2: Generate extended heightmap for erosion processing
        std::vector<float> extHeightmap(extWidth * extDepth);
        for (int z = 0; z < extDepth; z++)
        {
            for (int x = 0; x < extWidth; x++)
            {
                float localX = (x - BORDER) * m_Settings.cellSize;
                float localZ = (z - BORDER) * m_Settings.cellSize;
                float worldX = offsetX + localX;
                float worldZ = offsetZ + localZ;
                extHeightmap[z * extWidth + x] = SampleRawHeight(worldX, worldZ);
            }
        }

        // Step 3: Apply erosion effects on extended heightmap
        if (m_Settings.useErosion)
        {
            ApplySlopeErosion(extHeightmap, extWidth, extDepth);

            if (m_Settings.useHydraulicErosion && m_Settings.erosionIterations > 0)
            {
                ApplyHydraulicErosion(extHeightmap, extWidth, extDepth, offsetX, offsetZ);
            }
        }

        // Step 4: Extract eroded chunk and blend with raw near edges
        // This ensures chunk boundaries are seamless (raw terrain is seamless)
        m_CachedHeightmap.resize(width * depth);
        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                int extIdx = (z + BORDER) * extWidth + (x + BORDER);
                float erodedH = extHeightmap[extIdx];
                float rawH = rawHeightmap[z * width + x];

                // Calculate distance from nearest edge (in cells)
                int distFromEdge = std::min({x, z, (width - 1) - x, (depth - 1) - z});

                // Blend factor: 0 at edge (use raw), 1 in interior (use eroded)
                float blend = 1.0f;
                if (distFromEdge < BLEND_ZONE)
                {
                    // Smoothstep blend from raw at edge to eroded in interior
                    float t = static_cast<float>(distFromEdge) / static_cast<float>(BLEND_ZONE);
                    blend = t * t * (3.0f - 2.0f * t); // Smoothstep
                }

                m_CachedHeightmap[z * width + x] = rawH * (1.0f - blend) + erodedH * blend;
            }
        }

        // Step 5: Apply peak shaping (operates on final heightmap, is local so no seam issues)
        ApplyPeakShaping(m_CachedHeightmap, width, depth);

        // Store chunk origin for GetHeightAt queries
        m_ChunkOrigin = glm::vec2(offsetX, offsetZ);

        return m_CachedHeightmap;
    }

    float TerrainGenerator::GetHeightAt(float worldX, float worldZ) const
    {
        if (m_CachedHeightmap.empty())
        {
            return SampleRawHeight(worldX, worldZ);
        }

        // Convert world coordinates to local grid coordinates using stored chunk origin
        float localX = worldX - m_ChunkOrigin.x;
        float localZ = worldZ - m_ChunkOrigin.y;
        float gridX = localX / m_Settings.cellSize;
        float gridZ = localZ / m_Settings.cellSize;

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
            return deepWater;
        if (normalizedHeight < settings.waterLevel)
            return shallowWater;
        if (normalizedHeight < settings.sandLevel)
            return sand;
        if (normalizedHeight < settings.grassLevel * 0.7f)
            return grass;
        if (normalizedHeight < settings.grassLevel)
            return darkGrass;
        if (normalizedHeight < settings.rockLevel)
            return rock;
        return snow;
    }

    std::shared_ptr<Mesh> TerrainGenerator::BuildMeshFromHeightmap(const std::vector<float> &heightmap, float offsetX, float offsetZ) const
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        int width = m_Settings.width + 1;
        int depth = m_Settings.depth + 1;

        // Use consistent global height range for color normalization (not per-chunk min/max)
        // This ensures colors are consistent across chunk boundaries
        float minHeight = m_Settings.baseHeight;
        float maxHeight = m_Settings.baseHeight + m_Settings.heightScale;
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
                    float x0 = x * m_Settings.cellSize;
                    float x1 = (x + 1) * m_Settings.cellSize;
                    float z0 = z * m_Settings.cellSize;
                    float z1 = (z + 1) * m_Settings.cellSize;

                    float h00 = heightmap[z * width + x];
                    float h10 = heightmap[z * width + (x + 1)];
                    float h01 = heightmap[(z + 1) * width + x];
                    float h11 = heightmap[(z + 1) * width + (x + 1)];

                    glm::vec3 p00(x0, h00, z0);
                    glm::vec3 p10(x1, h10, z0);
                    glm::vec3 p01(x0, h01, z1);
                    glm::vec3 p11(x1, h11, z1);

                    // Triangle 1: p00, p01, p10 (CCW winding)
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

                    // Triangle 2: p10, p01, p11 (CCW winding)
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
            for (int z = 0; z < depth; z++)
            {
                for (int x = 0; x < width; x++)
                {
                    float worldX = x * m_Settings.cellSize;
                    float worldZ = z * m_Settings.cellSize;
                    float height = heightmap[z * width + x];

                    float normH = (height - minHeight) / heightRange;
                    glm::vec3 color = m_Settings.useHeightColors ? GetHeightColor(normH, m_Settings) : glm::vec3(0.34f, 0.55f, 0.25f);

                    vertices.push_back({glm::vec3(worldX, height, worldZ),
                                        glm::vec3(0.0f, 1.0f, 0.0f),
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

                    indices.push_back(i00);
                    indices.push_back(i01);
                    indices.push_back(i10);

                    glm::vec3 n1 = glm::cross(
                        vertices[i01].Position - vertices[i00].Position,
                        vertices[i10].Position - vertices[i00].Position);
                    normals[i00] += n1;
                    normals[i01] += n1;
                    normals[i10] += n1;

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

            for (size_t i = 0; i < vertices.size(); i++)
            {
                vertices[i].Normal = glm::normalize(normals[i]);
            }
        }

        return std::make_shared<Mesh>(vertices, indices);
    }

    std::shared_ptr<Mesh> TerrainGenerator::Generate()
    {
        GenerateHeightmap();
        return BuildMeshFromHeightmap(m_CachedHeightmap, 0.0f, 0.0f);
    }

    std::shared_ptr<Mesh> TerrainGenerator::GenerateAtOffset(float offsetX, float offsetZ)
    {
        GenerateHeightmapAtOffset(offsetX, offsetZ);
        return BuildMeshFromHeightmap(m_CachedHeightmap, offsetX, offsetZ);
    }

}
