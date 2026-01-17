#pragma once

#include "genesis/procedural/TerrainGenerator.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace Genesis
{

    /**
     * TerrainIntent encapsulates high-level, human-understandable terrain concepts.
     *
     * Instead of exposing ~30 mechanical parameters, we expose 8 orthogonal axes
     * that map to how artists and designers actually think about terrain:
     *
     *   - Continental Scale: Size of major landmasses and mountain belts
     *   - Elevation Range: Absolute height contrast (flat vs extreme relief)
     *   - Mountain Coverage: How much of the world is mountainous
     *   - Mountain Sharpness: Rounded vs jagged peaks
     *   - Ruggedness: Small-scale surface roughness
     *   - Erosion Age: Degree of weathering (young vs ancient)
     *   - River Strength: Valley carving dominance
     *   - Chaos: Breaks symmetry and predictability
     *
     * All parameters are normalized to [0, 1] for intuitive slider control.
     * The derivation layer maps these to valid, physically-coherent terrain settings.
     */
    struct TerrainIntent
    {
        float continentalScale = 0.5f;  // 0..1: Small islands → Large continents
        float elevationRange = 0.5f;    // 0..1: Flat → Extreme vertical relief
        float mountainCoverage = 0.5f;  // 0..1: % of world that is mountainous
        float mountainSharpness = 0.5f; // 0..1: Rounded → Jagged peaks
        float ruggedness = 0.5f;        // 0..1: Smooth → Noisy detail
        float erosionAge = 0.5f;        // 0..1: Young → Ancient terrain
        float riverStrength = 0.5f;     // 0..1: Weak streams → Dominant rivers
        float chaos = 0.3f;             // 0..1: Predictable → Wild

        bool operator==(const TerrainIntent &other) const
        {
            constexpr float epsilon = 0.0001f;
            return std::abs(continentalScale - other.continentalScale) < epsilon &&
                   std::abs(elevationRange - other.elevationRange) < epsilon &&
                   std::abs(mountainCoverage - other.mountainCoverage) < epsilon &&
                   std::abs(mountainSharpness - other.mountainSharpness) < epsilon &&
                   std::abs(ruggedness - other.ruggedness) < epsilon &&
                   std::abs(erosionAge - other.erosionAge) < epsilon &&
                   std::abs(riverStrength - other.riverStrength) < epsilon &&
                   std::abs(chaos - other.chaos) < epsilon;
        }

        bool operator!=(const TerrainIntent &other) const { return !(*this == other); }
    };

    /**
     * Named preset with metadata for UI display.
     */
    struct TerrainPreset
    {
        std::string name;
        std::string description;
        TerrainIntent intent;
    };

    /**
     * Derives mechanical TerrainSettings from high-level TerrainIntent.
     *
     * This is the core mapping that ensures:
     *   - All generated configurations are physically valid
     *   - Parameters maintain correct relationships (invariants)
     *   - Users cannot accidentally create "alien terrain" states
     */
    class TerrainIntentMapper
    {
    public:
        /**
         * Maps a TerrainIntent to fully-derived TerrainSettings.
         * This eliminates invalid states and enforces parameter invariants.
         */
        static TerrainSettings DeriveSettings(const TerrainIntent &intent);

        /**
         * Returns built-in terrain presets for common terrain types.
         */
        static const std::vector<TerrainPreset> &GetPresets();

        /**
         * Finds a preset by name (case-insensitive).
         * Returns nullptr if not found.
         */
        static const TerrainPreset *FindPreset(const std::string &name);

    private:
        static float Lerp(float a, float b, float t) { return a + t * (b - a); }
        static float Saturate(float x) { return std::clamp(x, 0.0f, 1.0f); }
        static float Smoothstep(float t) { return t * t * (3.0f - 2.0f * t); }

        static void DeriveWorldScale(const TerrainIntent &intent, TerrainSettings &settings);
        static void DeriveNoiseSpectrum(const TerrainIntent &intent, TerrainSettings &settings);
        static void DeriveMountains(const TerrainIntent &intent, TerrainSettings &settings);
        static void DeriveTectonicUplift(const TerrainIntent &intent, TerrainSettings &settings);
        static void DeriveDomainWarping(const TerrainIntent &intent, TerrainSettings &settings);
        static void DeriveErosion(const TerrainIntent &intent, TerrainSettings &settings);
        static void EnforceInvariants(TerrainSettings &settings);
    };

}
