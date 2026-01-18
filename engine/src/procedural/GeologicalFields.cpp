#include "genesis/procedural/GeologicalFields.h"
#include "genesis/procedural/TerrainGenerator.h"
#include <algorithm>
#include <cmath>

namespace Genesis
{

    GeologicalFieldGenerator::GeologicalFieldGenerator()
        : m_Seed(12345), m_Noise(12345)
    {
        DeriveFieldSettings(m_Intent);
    }

    GeologicalFieldGenerator::GeologicalFieldGenerator(const TerrainIntent &intent, uint32_t seed)
        : m_Intent(intent), m_Seed(seed), m_Noise(seed)
    {
        DeriveFieldSettings(intent);
    }

    void GeologicalFieldGenerator::Configure(const TerrainIntent &intent, uint32_t seed)
    {
        m_Intent = intent;
        m_Seed = seed;
        m_Noise.SetSeed(seed);
        DeriveFieldSettings(intent);
    }

    void GeologicalFieldGenerator::SetFieldSettings(const GeologicalFieldSettings &settings)
    {
        m_FieldSettings = settings;
    }

    void GeologicalFieldGenerator::DeriveFieldSettings(const TerrainIntent &intent)
    {
        // Continental frequency: larger continentalScale = lower frequency = bigger landmasses
        // Range: 0.0006 (small islands) to 0.00015 (massive continents)
        m_FieldSettings.continentalFrequency = Lerp(0.0006f, 0.00015f, intent.continentalScale);

        // Ocean threshold stays relatively constant but can shift slightly with scale
        // Larger landmasses (high scale) = slightly lower threshold (more land area)
        m_FieldSettings.oceanThreshold = Lerp(0.48f, 0.42f, intent.continentalScale);

        // Coastline blend epsilon: more chaos = rougher coastlines
        m_FieldSettings.coastlineBlend = Lerp(0.03f, 0.08f, intent.chaos);

        // Ocean depth scales with elevation range
        // elevationRange 0 = shallow seas (30-50), elevationRange 1 = deep oceans (50-80)
        m_FieldSettings.oceanDepthMin = Lerp(20.0f, 40.0f, intent.elevationRange);
        m_FieldSettings.oceanDepthMax = Lerp(40.0f, 80.0f, intent.elevationRange);
        m_FieldSettings.oceanFloorVariation = Lerp(0.2f, 0.4f, intent.ruggedness);

        // Elevation amplitude field: ties to continental scale for coherent regions
        m_FieldSettings.elevationFieldFrequency = Lerp(0.001f, 0.0004f, intent.continentalScale);

        // Erosion age spatial variation
        m_FieldSettings.useSpatialErosion = true;
        m_FieldSettings.erosionFieldFrequency = Lerp(0.0008f, 0.0003f, intent.continentalScale);
        m_FieldSettings.erosionAgeBase = intent.erosionAge;
        // More chaos = more erosion age variation across the landscape
        m_FieldSettings.erosionAgeVariation = Lerp(0.15f, 0.4f, intent.chaos);
    }

    GeologicalFields GeologicalFieldGenerator::SampleFields(float worldX, float worldZ) const
    {
        GeologicalFields fields;

        // ========================================
        // Layer 1: Continental Field
        // ========================================
        // Low-frequency FBM determines macro topology (land vs ocean)
        float contX = worldX * m_FieldSettings.continentalFrequency;
        float contZ = worldZ * m_FieldSettings.continentalFrequency;

        float continentalNoise = m_Noise.FBM(contX, contZ,
                                             static_cast<int>(m_FieldSettings.continentalOctaves),
                                             0.5f, 2.0f);

        // Map from [-1, 1] to [0, 1] for threshold comparison
        fields.continental = (continentalNoise + 1.0f) * 0.5f;

        // Compute ocean mask via smoothstep
        fields.oceanMask = ComputeOceanMask(fields.continental);

        // ========================================
        // Layer 2: Elevation Amplitude Field
        // ========================================
        // Controls local height range variation
        float elevX = worldX * m_FieldSettings.elevationFieldFrequency;
        float elevZ = worldZ * m_FieldSettings.elevationFieldFrequency;

        // Use offset to decorrelate from continental field
        float elevNoise = m_Noise.FBM(elevX + 100.0f, elevZ + 200.0f,
                                      static_cast<int>(m_FieldSettings.elevationFieldOctaves),
                                      0.5f, 2.0f);

        // Map to [0.3, 1.0] - never fully flat
        fields.elevationAmplitude = 0.3f + 0.7f * ((elevNoise + 1.0f) * 0.5f);

        // Reduce amplitude in ocean areas (ocean floors are relatively flat)
        fields.elevationAmplitude *= (1.0f - fields.oceanMask * 0.6f);

        // ========================================
        // Layer 3: Uplift Mask (Mountain Eligibility)
        // ========================================
        // This field determines WHERE mountains can appear
        // Uses settings from TerrainSettings if field settings are zero
        float upliftFreq = m_FieldSettings.upliftFrequency;
        if (upliftFreq <= 0.0f)
        {
            // Default: scale with continental frequency but at higher detail
            upliftFreq = m_FieldSettings.continentalFrequency * 10.0f;
        }

        float upliftX = worldX * upliftFreq;
        float upliftZ = worldZ * upliftFreq;

        // Offset to decorrelate from other fields
        float upliftNoise = m_Noise.FBM(upliftX + 500.0f, upliftZ + 700.0f, 2, 0.5f, 2.0f);
        upliftNoise = (upliftNoise + 1.0f) * 0.5f; // Map to [0, 1]

        // Smoothstep for gradual transition
        float threshLow = m_FieldSettings.upliftThresholdLow;
        float threshHigh = m_FieldSettings.upliftThresholdHigh;
        if (threshLow <= 0.0f)
            threshLow = 0.4f;
        if (threshHigh <= 0.0f)
            threshHigh = 0.7f;

        float t = (upliftNoise - threshLow) / (threshHigh - threshLow);
        t = std::clamp(t, 0.0f, 1.0f);
        fields.upliftMask = t * t * (3.0f - 2.0f * t); // Smoothstep

        // Mountains don't exist in deep ocean
        fields.upliftMask *= (1.0f - fields.oceanMask);

        // ========================================
        // Layer 4: Ridge Value (placeholder - computed in TerrainGenerator)
        // ========================================
        // Ridge noise is computed separately in TerrainGenerator because it uses
        // domain warping and the mechanical ridge settings. This field will be
        // populated by the TerrainGenerator when needed.
        fields.ridgeValue = 0.0f;

        // ========================================
        // Layer 5: Erosion Age Field
        // ========================================
        if (m_FieldSettings.useSpatialErosion)
        {
            float erosionX = worldX * m_FieldSettings.erosionFieldFrequency;
            float erosionZ = worldZ * m_FieldSettings.erosionFieldFrequency;

            // Offset to decorrelate
            float erosionNoise = m_Noise.FBM(erosionX + 300.0f, erosionZ + 400.0f, 2, 0.5f, 2.0f);
            erosionNoise = erosionNoise * m_FieldSettings.erosionAgeVariation;

            fields.erosionAge = std::clamp(
                m_FieldSettings.erosionAgeBase + erosionNoise,
                0.0f, 1.0f);
        }
        else
        {
            fields.erosionAge = m_FieldSettings.erosionAgeBase;
        }

        return fields;
    }

    float GeologicalFieldGenerator::SampleContinentalField(float worldX, float worldZ) const
    {
        float contX = worldX * m_FieldSettings.continentalFrequency;
        float contZ = worldZ * m_FieldSettings.continentalFrequency;

        float continentalNoise = m_Noise.FBM(contX, contZ,
                                             static_cast<int>(m_FieldSettings.continentalOctaves),
                                             0.5f, 2.0f);

        return (continentalNoise + 1.0f) * 0.5f;
    }

    float GeologicalFieldGenerator::ComputeOceanMask(float continentalValue) const
    {
        // Smoothstep: values below (threshold - epsilon) = 1.0 (ocean)
        //             values above (threshold + epsilon) = 0.0 (land)
        float threshold = m_FieldSettings.oceanThreshold;
        float epsilon = m_FieldSettings.coastlineBlend;

        return Smoothstep(threshold + epsilon, threshold - epsilon, continentalValue);
    }

    float GeologicalFieldGenerator::ComputeOceanDepth(float worldX, float worldZ, float oceanMask) const
    {
        if (oceanMask <= 0.0f)
            return 0.0f;

        // Base ocean depth interpolated by elevation range (already baked into settings)
        float baseDepth = Lerp(m_FieldSettings.oceanDepthMin,
                               m_FieldSettings.oceanDepthMax,
                               m_Intent.elevationRange);

        // Add variation to ocean floor
        if (m_FieldSettings.oceanFloorVariation > 0.0f)
        {
            float varX = worldX * m_FieldSettings.continentalFrequency * 5.0f;
            float varZ = worldZ * m_FieldSettings.continentalFrequency * 5.0f;

            float variation = m_Noise.FBM(varX + 800.0f, varZ + 900.0f, 2, 0.5f, 2.0f);
            baseDepth += variation * baseDepth * m_FieldSettings.oceanFloorVariation;
        }

        // Depth fades in with ocean mask (shallow near coast, deep in open ocean)
        return baseDepth * oceanMask * oceanMask; // Squared for natural depth curve
    }

    float GeologicalFieldGenerator::ComputeBaseHeight(const GeologicalFields &fields,
                                                      float baseNoise,
                                                      float ridgeNoise,
                                                      const TerrainSettings &settings) const
    {
        // Start with base terrain noise, scaled by elevation amplitude field
        float height = baseNoise * fields.elevationAmplitude;

        // Add ridge contribution modulated by uplift mask
        if (settings.useRidgeNoise && fields.upliftMask > 0.0f)
        {
            // Ridge contribution scaled by both uplift mask and ridge weight
            float ridgeContribution = ridgeNoise * settings.ridgeWeight * fields.upliftMask;

            // Blend: reduce base noise where ridges dominate
            float baseWeight = 1.0f - (settings.ridgeWeight * fields.upliftMask);
            height = baseNoise * baseWeight * fields.elevationAmplitude + ridgeContribution;
        }

        // Map from [-1, 1] range to [0, 1]
        height = (height + 1.0f) * 0.5f;

        // Apply height scale
        float worldHeight = settings.baseHeight + height * settings.heightScale;

        // Apply ocean depth bias
        // Ocean areas are pushed down below sea level
        if (fields.oceanMask > 0.0f)
        {
            float oceanDepth = ComputeOceanDepth(0.0f, 0.0f, fields.oceanMask);
            worldHeight -= oceanDepth;
        }

        return worldHeight;
    }

    float GeologicalFieldGenerator::Smoothstep(float edge0, float edge1, float x)
    {
        float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

}
