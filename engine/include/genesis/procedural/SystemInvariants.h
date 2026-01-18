#pragma once

#include "genesis/core/Log.h"
#include <string>

namespace Genesis
{

    /**
     * Section 33: System Invariants (Must Hold)
     *
     * Defines architectural invariants for the terrain generation pipeline.
     * These invariants ensure correct data flow and separation of concerns.
     *
     * Invariant 1: Geometry precedes water
     *   - Heightmap must be generated before drainage/rivers/lakes
     *   - Water systems read from heightmap, never write to it
     *
     * Invariant 2: Water precedes biomes
     *   - Hydrology data must be computed before climate/biomes
     *   - Biomes use moisture from hydrology, not raw heightmap
     *
     * Invariant 3: Biomes do not alter height
     *   - BiomeClassifier only reads terrain/climate data
     *   - Biome output is classification only, never height modification
     *
     * Invariant 4: Materials are blended, never selected
     *   - MaterialBlender outputs normalized weights [0,1]
     *   - No hard material selection; all materials blend continuously
     *
     * Invariant 5: Intent never touches noise directly
     *   - TerrainIntent maps to TerrainSettings (parameters only)
     *   - Noise sampling is encapsulated in TerrainGenerator
     */

    /**
     * Pipeline stages in order of execution.
     */
    enum class PipelineStage : uint8_t
    {
        None = 0,
        HeightmapGeneration,  // Stage 1: Raw heightmap from noise
        ErosionProcessing,    // Stage 2: Erosion applied to heightmap
        DrainageComputation,  // Stage 3: Flow direction and accumulation
        RiverGeneration,      // Stage 4: River network from drainage
        LakeDetection,        // Stage 5: Lake basins from drainage
        HydrologyAggregation, // Stage 6: Unified hydrology data
        ClimateGeneration,    // Stage 7: Temperature, moisture, fertility
        BiomeClassification,  // Stage 8: Biome weights from climate
        MaterialBlending,     // Stage 9: Surface materials from all data
        MeshGeneration,       // Stage 10: Final mesh with materials

        Count
    };

    /**
     * Returns the display name for a pipeline stage.
     */
    inline const char *GetPipelineStageName(PipelineStage stage)
    {
        switch (stage)
        {
        case PipelineStage::None:
            return "None";
        case PipelineStage::HeightmapGeneration:
            return "Heightmap Generation";
        case PipelineStage::ErosionProcessing:
            return "Erosion Processing";
        case PipelineStage::DrainageComputation:
            return "Drainage Computation";
        case PipelineStage::RiverGeneration:
            return "River Generation";
        case PipelineStage::LakeDetection:
            return "Lake Detection";
        case PipelineStage::HydrologyAggregation:
            return "Hydrology Aggregation";
        case PipelineStage::ClimateGeneration:
            return "Climate Generation";
        case PipelineStage::BiomeClassification:
            return "Biome Classification";
        case PipelineStage::MaterialBlending:
            return "Material Blending";
        case PipelineStage::MeshGeneration:
            return "Mesh Generation";
        default:
            return "Unknown";
        }
    }

    /**
     * Validates that pipeline stages are executed in correct order.
     *
     * @param current   The stage about to execute
     * @param previous  The last completed stage
     * @return true if order is valid, false if invariant violated
     */
    inline bool ValidatePipelineOrder(PipelineStage current, PipelineStage previous)
    {
        // Stage numbers must be monotonically increasing
        return static_cast<uint8_t>(current) > static_cast<uint8_t>(previous);
    }

    /**
     * Checks if a stage can be executed given the current pipeline state.
     *
     * @param stage     The stage to check
     * @param completed Bitmask of completed stages
     * @return true if all prerequisites are met
     */
    inline bool CanExecuteStage(PipelineStage stage, uint16_t completedMask)
    {
        auto bit = [](PipelineStage s)
        {
            return static_cast<uint16_t>(1 << static_cast<uint8_t>(s));
        };

        switch (stage)
        {
        case PipelineStage::HeightmapGeneration:
            return true; // No prerequisites

        case PipelineStage::ErosionProcessing:
            return (completedMask & bit(PipelineStage::HeightmapGeneration)) != 0;

        case PipelineStage::DrainageComputation:
            // Requires heightmap (erosion is optional)
            return (completedMask & bit(PipelineStage::HeightmapGeneration)) != 0;

        case PipelineStage::RiverGeneration:
        case PipelineStage::LakeDetection:
            return (completedMask & bit(PipelineStage::DrainageComputation)) != 0;

        case PipelineStage::HydrologyAggregation:
            return (completedMask & bit(PipelineStage::DrainageComputation)) != 0;

        case PipelineStage::ClimateGeneration:
            return (completedMask & bit(PipelineStage::HeightmapGeneration)) != 0;

        case PipelineStage::BiomeClassification:
            return (completedMask & bit(PipelineStage::ClimateGeneration)) != 0;

        case PipelineStage::MaterialBlending:
            return (completedMask & bit(PipelineStage::HeightmapGeneration)) != 0 &&
                   (completedMask & bit(PipelineStage::ClimateGeneration)) != 0;

        case PipelineStage::MeshGeneration:
            return (completedMask & bit(PipelineStage::HeightmapGeneration)) != 0;

        default:
            return false;
        }
    }

    /**
     * Tracks pipeline execution state and validates invariants.
     */
    class PipelineValidator
    {
    public:
        PipelineValidator() = default;

        /**
         * Reset pipeline state for a new generation pass.
         */
        void Reset()
        {
            m_CompletedMask = 0;
            m_LastStage = PipelineStage::None;
            m_ViolationCount = 0;
        }

        /**
         * Mark a stage as starting execution.
         * Validates prerequisites and logs warnings on violations.
         *
         * @param stage The stage beginning execution
         * @return true if valid to proceed, false if invariant violated
         */
        bool BeginStage(PipelineStage stage)
        {
            if (!CanExecuteStage(stage, m_CompletedMask))
            {
                GEN_WARN("Pipeline invariant violation: {} executed without prerequisites",
                         GetPipelineStageName(stage));
                m_ViolationCount++;
                return false;
            }

            m_CurrentStage = stage;
            return true;
        }

        /**
         * Mark a stage as completed.
         */
        void EndStage(PipelineStage stage)
        {
            m_CompletedMask |= static_cast<uint16_t>(1 << static_cast<uint8_t>(stage));
            m_LastStage = stage;
            m_CurrentStage = PipelineStage::None;
        }

        /**
         * Check if a stage has been completed.
         */
        bool IsStageComplete(PipelineStage stage) const
        {
            return (m_CompletedMask & static_cast<uint16_t>(1 << static_cast<uint8_t>(stage))) != 0;
        }

        /**
         * Get the last completed stage.
         */
        PipelineStage GetLastStage() const { return m_LastStage; }

        /**
         * Get the current executing stage.
         */
        PipelineStage GetCurrentStage() const { return m_CurrentStage; }

        /**
         * Get the number of invariant violations detected.
         */
        int GetViolationCount() const { return m_ViolationCount; }

        /**
         * Get completed stages as a bitmask.
         */
        uint16_t GetCompletedMask() const { return m_CompletedMask; }

    private:
        uint16_t m_CompletedMask = 0;
        PipelineStage m_LastStage = PipelineStage::None;
        PipelineStage m_CurrentStage = PipelineStage::None;
        int m_ViolationCount = 0;
    };

    // ========================================================================
    // Invariant Assertion Macros
    // ========================================================================

    /**
     * Assert that geometry (heightmap) has been generated before water systems.
     */
#define GEN_INVARIANT_GEOMETRY_FIRST(validator)                               \
    do                                                                        \
    {                                                                         \
        if (!(validator).IsStageComplete(PipelineStage::HeightmapGeneration)) \
        {                                                                     \
            GEN_ERROR("Invariant violation: Geometry must precede water");    \
            GEN_ASSERT(false);                                                \
        }                                                                     \
    } while (0)

    /**
     * Assert that hydrology has been computed before biome classification.
     */
#define GEN_INVARIANT_WATER_BEFORE_BIOMES(validator)                                     \
    do                                                                                   \
    {                                                                                    \
        if (!(validator).IsStageComplete(PipelineStage::HydrologyAggregation))           \
        {                                                                                \
            GEN_WARN("Invariant: Water should precede biomes (hydrology not complete)"); \
        }                                                                                \
    } while (0)

    /**
     * Assert that material weights are properly normalized.
     */
#define GEN_INVARIANT_MATERIALS_BLENDED(weights)                                            \
    do                                                                                      \
    {                                                                                       \
        float sum = 0.0f;                                                                   \
        for (float w : (weights).weights)                                                   \
            sum += w;                                                                       \
        if (std::abs(sum - 1.0f) > 0.01f)                                                   \
        {                                                                                   \
            GEN_WARN("Invariant violation: Material weights not normalized (sum={})", sum); \
        }                                                                                   \
    } while (0)

    // ========================================================================
    // Section 31: Determinism & Chunking Guidelines
    // ========================================================================

    /**
     * Determinism requirements for procedural generation:
     *
     * 1. All noise sampling MUST use world-space coordinates
     *    - float worldX = offsetX + localX * cellSize;
     *    - Ensures identical results regardless of chunk boundaries
     *
     * 2. Hydrology uses padded borders
     *    - Extended border (BORDER = 8 cells) for erosion context
     *    - Blend zone (BLEND_ZONE = 6 cells) for seamless transitions
     *
     * 3. Chunk-local caches only
     *    - Each chunk owns its heightmap, drainage, hydrology data
     *    - No shared mutable state between chunks
     *
     * 4. No global mutable state
     *    - Noise permutation tables are per-instance
     *    - Random generators seeded from (worldSeed, chunkX, chunkZ)
     *
     * 5. Deterministic seed derivation:
     *    chunkSeed = worldSeed ^ (chunkX * 198491317) ^ (chunkZ * 6542989)
     */

    /**
     * Compute deterministic seed for a chunk.
     *
     * @param worldSeed Global world seed
     * @param chunkX    Chunk X coordinate
     * @param chunkZ    Chunk Z coordinate
     * @return Deterministic seed for this chunk
     */
    inline uint32_t ComputeChunkSeed(uint32_t worldSeed, int chunkX, int chunkZ)
    {
        return worldSeed ^
               (static_cast<uint32_t>(chunkX * 198491317) ^
                static_cast<uint32_t>(chunkZ * 6542989));
    }

    /**
     * Settings for chunk border handling.
     */
    struct ChunkBorderSettings
    {
        // Border cells for erosion context (must be >= erosion radius)
        static constexpr int BORDER = 8;

        // Blend zone for seamless chunk transitions
        static constexpr int BLEND_ZONE = 6;

        // Minimum overlap for hydrology propagation
        static constexpr int HYDROLOGY_OVERLAP = 4;
    };

}
