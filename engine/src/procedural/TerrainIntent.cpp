#include "genesis/procedural/TerrainIntent.h"
#include <algorithm>
#include <cctype>

namespace Genesis
{

    namespace
    {
        // Built-in presets representing realistic terrain archetypes
        const std::vector<TerrainPreset> s_Presets = {
            {"Alpine Young",
             "Sharp, dramatic peaks with active erosion. Fresh mountain ranges with deep valleys.",
             {.continentalScale = 0.8f,
              .elevationRange = 0.9f,
              .mountainCoverage = 0.7f,
              .mountainSharpness = 0.8f,
              .ruggedness = 0.55f,
              .erosionAge = 0.2f,
              .riverStrength = 0.6f,
              .chaos = 0.3f}},
            {"Ancient Highlands",
             "Weathered, rounded mountains worn down over millennia. Gentle slopes with mature river systems.",
             {.continentalScale = 0.7f,
              .elevationRange = 0.6f,
              .mountainCoverage = 0.5f,
              .mountainSharpness = 0.3f,
              .ruggedness = 0.4f,
              .erosionAge = 0.85f,
              .riverStrength = 0.4f,
              .chaos = 0.2f}},
            {"Arid Plateaus",
             "High flat mesas with dramatic cliff edges. Sparse erosion in dry conditions.",
             {.continentalScale = 0.6f,
              .elevationRange = 0.5f,
              .mountainCoverage = 0.3f,
              .mountainSharpness = 0.6f,
              .ruggedness = 0.3f,
              .erosionAge = 0.6f,
              .riverStrength = 0.2f,
              .chaos = 0.4f}},
            {"Volcanic Ranges",
             "Steep, dramatic peaks with chaotic formations. Active geological features.",
             {.continentalScale = 0.5f,
              .elevationRange = 0.95f,
              .mountainCoverage = 0.6f,
              .mountainSharpness = 0.85f,
              .ruggedness = 0.75f,
              .erosionAge = 0.15f,
              .riverStrength = 0.3f,
              .chaos = 0.7f}},
            {"Rolling Temperate",
             "Gentle hills and valleys. Lush, eroded landscape typical of temperate regions.",
             {.continentalScale = 0.6f,
              .elevationRange = 0.35f,
              .mountainCoverage = 0.2f,
              .mountainSharpness = 0.25f,
              .ruggedness = 0.35f,
              .erosionAge = 0.7f,
              .riverStrength = 0.55f,
              .chaos = 0.25f}},
            {"Coastal Fjords",
             "Deep valleys carved by glaciers, steep cliffs meeting water.",
             {.continentalScale = 0.55f,
              .elevationRange = 0.8f,
              .mountainCoverage = 0.55f,
              .mountainSharpness = 0.7f,
              .ruggedness = 0.5f,
              .erosionAge = 0.5f,
              .riverStrength = 0.75f,
              .chaos = 0.35f}},
            {"Flat Plains",
             "Minimal elevation change. Wide open spaces with subtle undulation.",
             {.continentalScale = 0.8f,
              .elevationRange = 0.15f,
              .mountainCoverage = 0.02f,
              .mountainSharpness = 0.2f,
              .ruggedness = 0.2f,
              .erosionAge = 0.8f,
              .riverStrength = 0.3f,
              .chaos = 0.15f}},
            {"Custom",
             "User-defined terrain parameters.",
             {.continentalScale = 0.5f,
              .elevationRange = 0.5f,
              .mountainCoverage = 0.5f,
              .mountainSharpness = 0.5f,
              .ruggedness = 0.5f,
              .erosionAge = 0.5f,
              .riverStrength = 0.5f,
              .chaos = 0.3f}}};
    }

    TerrainSettings TerrainIntentMapper::DeriveSettings(const TerrainIntent &intent)
    {
        TerrainSettings settings{};

        DeriveWorldScale(intent, settings);
        DeriveContinentalField(intent, settings);
        DeriveNoiseSpectrum(intent, settings);
        DeriveMountains(intent, settings);
        DeriveTectonicUplift(intent, settings);
        DeriveDomainWarping(intent, settings);
        DeriveErosion(intent, settings);
        EnforceInvariants(settings);

        return settings;
    }

    const std::vector<TerrainPreset> &TerrainIntentMapper::GetPresets()
    {
        return s_Presets;
    }

    const TerrainPreset *TerrainIntentMapper::FindPreset(const std::string &name)
    {
        auto toLower = [](std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });
            return s;
        };

        std::string lowerName = toLower(name);
        for (const auto &preset : s_Presets)
        {
            if (toLower(preset.name) == lowerName)
            {
                return &preset;
            }
        }
        return nullptr;
    }

    void TerrainIntentMapper::DeriveWorldScale(const TerrainIntent &intent, TerrainSettings &settings)
    {
        // Continental scale: larger values = bigger landmasses = lower noise frequency
        // Range: 0.02 (small features) to 0.0015 (continental scale)
        settings.noiseScale = Lerp(0.02f, 0.0015f, intent.continentalScale);

        // Elevation range: flat (6 units) to extreme (40 units)
        settings.heightScale = Lerp(6.0f, 40.0f, intent.elevationRange);

        // Base height: neutral offset (amplitude already handled by heightScale)
        settings.baseHeight = 0.0f;
    }

    void TerrainIntentMapper::DeriveContinentalField(const TerrainIntent &intent, TerrainSettings &settings)
    {
        // Enable continental field for geological terrain generation
        settings.useContinentalField = true;

        // Continental frequency: larger continentalScale = lower frequency = bigger landmasses
        // Range: 0.0006 (small archipelagos) to 0.00015 (massive continents)
        settings.continentalFrequency = Lerp(0.0006f, 0.00015f, intent.continentalScale);

        // Ocean threshold: determines land/ocean ratio
        // Larger landmasses (high scale) = slightly lower threshold (more land area)
        settings.oceanThreshold = Lerp(0.48f, 0.42f, intent.continentalScale);

        // Coastline blend epsilon: more chaos = rougher, more irregular coastlines
        settings.coastlineBlend = Lerp(0.03f, 0.08f, intent.chaos);

        // Ocean depth scales with elevation range
        // elevationRange 0 = shallow seas (30), elevationRange 1 = deep oceans (80)
        settings.oceanDepth = Lerp(30.0f, 80.0f, intent.elevationRange);

        // Ocean floor variation: ruggedness affects underwater terrain
        settings.oceanFloorVariation = Lerp(0.2f, 0.4f, intent.ruggedness);
    }

    void TerrainIntentMapper::DeriveNoiseSpectrum(const TerrainIntent &intent, TerrainSettings &settings)
    {
        // Ruggedness controls the noise spectrum:
        //   - Octaves: more = finer detail
        //   - Persistence: higher = rougher terrain
        //   - Lacunarity: higher = more small-scale detail per octave
        //
        // STABILITY CONSTRAINT: persistence * lacunarity < 1.0
        // Otherwise noise amplitude grows with each octave instead of shrinking.
        // With lacunarity range [1.8, 2.4], safe persistence max is ~0.42

        settings.octaves = static_cast<int>(Lerp(3.0f, 6.0f, intent.ruggedness));
        settings.persistence = Lerp(0.35f, 0.42f, intent.ruggedness);
        settings.lacunarity = Lerp(1.8f, 2.4f, intent.ruggedness);
    }

    void TerrainIntentMapper::DeriveMountains(const TerrainIntent &intent, TerrainSettings &settings)
    {
        // Enable ridge noise only when there's meaningful mountain coverage
        // Use small epsilon (0.04) to avoid floating-point edge cases at exactly 0.05
        settings.useRidgeNoise = intent.mountainCoverage > 0.04f;

        // Ridge weight directly maps from coverage
        settings.ridgeWeight = Saturate(intent.mountainCoverage);

        // Sharpness controls the ridge power exponent
        // Low sharpness = rounded mountains (1.4)
        // High sharpness = jagged peaks (3.8)
        settings.ridgePower = Lerp(1.4f, 3.8f, intent.mountainSharpness);

        // Peak boost adds extra definition to summits (reduced to prevent spikes)
        settings.peakBoost = intent.mountainSharpness * 0.4f;

        // Ridge scale: larger continental scale = larger mountain features
        settings.ridgeScale = Lerp(0.6f, 1.5f, intent.continentalScale);
    }

    void TerrainIntentMapper::DeriveTectonicUplift(const TerrainIntent &intent, TerrainSettings &settings)
    {
        // Always enable uplift mask for realistic mountain distribution
        settings.useUpliftMask = true;

        // Uplift scale: larger continental scale = larger tectonic regions
        settings.upliftScale = Lerp(0.015f, 0.003f, intent.continentalScale);

        // Thresholds control where mountains vs plains appear
        // Lower coverage = higher threshold (more area is plains)
        settings.upliftThresholdLow = Lerp(0.25f, 0.45f, 1.0f - intent.mountainCoverage);
        settings.upliftThresholdHigh = settings.upliftThresholdLow + 0.25f;

        // Sharpness affects transition abruptness
        settings.upliftPower = Lerp(0.9f, 2.5f, intent.mountainSharpness);
    }

    void TerrainIntentMapper::DeriveDomainWarping(const TerrainIntent &intent, TerrainSettings &settings)
    {
        // Always enable warping for organic terrain
        settings.useWarp = true;

        // Chaos controls warp intensity
        // Keep warp strength VERY LOW to avoid coordinate folding artifacts (spikes)
        // Multi-level warping compounds, so we need conservative values
        // Safe cumulative warp should be < 0.25 in noise space
        settings.warpStrength = Lerp(0.03f, 0.15f, intent.chaos);
        settings.warpScale = Lerp(0.3f, 0.6f, intent.chaos);
        settings.warpLevels = static_cast<int>(Lerp(1.0f, 2.0f, intent.chaos));
    }

    void TerrainIntentMapper::DeriveErosion(const TerrainIntent &intent, TerrainSettings &settings)
    {
        // Always enable basic erosion
        settings.useErosion = true;

        // Erosion age: young terrain has strong slopes, old terrain is weathered
        // Invert age for slope erosion strength (young = steep, old = gentle)
        settings.slopeErosionStrength = Lerp(0.9f, 0.15f, intent.erosionAge);

        // Slope threshold: young terrain erodes at lower angles
        settings.slopeThreshold = Lerp(0.25f, 1.2f, intent.erosionAge);

        // Valley depth from river strength
        settings.valleyDepth = Lerp(0.15f, 0.6f, intent.riverStrength);

        // Hydraulic erosion only for more aged terrain (takes time to develop)
        settings.useHydraulicErosion = intent.erosionAge > 0.3f;

        // More iterations for older terrain (more time for water to carve)
        settings.erosionIterations = static_cast<int>(Lerp(80.0f, 300.0f, intent.erosionAge));

        // Sediment capacity scales with river strength
        settings.erosionCapacity = Lerp(3.0f, 8.0f, intent.riverStrength);

        // Deposition rate increases with age (sediment accumulates)
        settings.erosionDeposition = Lerp(0.3f, 0.7f, intent.erosionAge);

        // Evaporation rate decreases with age (mature drainage patterns)
        settings.erosionEvaporation = Lerp(0.02f, 0.005f, intent.erosionAge);

        // Inertia slightly increases with age (established flow patterns)
        settings.erosionInertia = Lerp(0.03f, 0.08f, intent.erosionAge);
    }

    void TerrainIntentMapper::EnforceInvariants(TerrainSettings &settings)
    {
        // Ensure uplift thresholds are properly ordered
        settings.upliftThresholdHigh = std::max(settings.upliftThresholdHigh,
                                                settings.upliftThresholdLow + 0.1f);

        // Ensure persistence doesn't exceed stable limit relative to lacunarity
        // This prevents noise from blowing up at high octaves
        // Invariants must only restrict, never expand
        float maxSafePersistence = 0.9f / settings.lacunarity;
        settings.persistence = std::min(settings.persistence, maxSafePersistence);

        // Clamp warp levels to safe range
        settings.warpLevels = std::clamp(settings.warpLevels, 1, 4);

        // Ensure erosion iterations are reasonable
        settings.erosionIterations = std::clamp(settings.erosionIterations, 10, 500);

        // Clamp all normalized values
        settings.ridgeWeight = Saturate(settings.ridgeWeight);
        settings.peakBoost = Saturate(settings.peakBoost);
        settings.slopeErosionStrength = Saturate(settings.slopeErosionStrength);
        settings.valleyDepth = Saturate(settings.valleyDepth);
        settings.erosionDeposition = Saturate(settings.erosionDeposition);
    }

}
