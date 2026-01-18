#pragma once

#include "genesis/renderer/Mesh.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace Genesis
{

    class VulkanDevice;
    class VulkanBuffer;

    /**
     * Section 30.3: Instanced Rendering
     *
     * Supports efficient rendering of many identical objects
     * (trees, rocks, vegetation) using GPU instancing.
     */

    /**
     * Per-instance data for instanced rendering.
     */
    struct InstanceData
    {
        glm::vec4 PositionAndScale; // xyz = position, w = uniform scale
        glm::vec4 RotationAndTint;  // xyz = euler rotation (radians), w = tint factor

        static std::vector<VkVertexInputBindingDescription> GetBindingDescription();
        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
    };

    /**
     * Batch of instances for a single mesh type.
     */
    struct InstanceBatch
    {
        std::vector<InstanceData> instances;
        Mesh *mesh = nullptr; // Shared mesh for all instances
        uint32_t instanceCount = 0;
    };

    /**
     * Manages instanced rendering for vegetation and props.
     */
    class InstancedRenderer
    {
    public:
        InstancedRenderer() = default;
        ~InstancedRenderer();

        /**
         * Initialize the instanced renderer.
         */
        void Init(VulkanDevice &device, VkDescriptorSetLayout descriptorSetLayout,
                  VkRenderPass renderPass);

        /**
         * Shutdown and release resources.
         */
        void Shutdown();

        /**
         * Begin a new frame - clear instance data.
         */
        void BeginFrame();

        /**
         * Add instances for rendering.
         *
         * @param mesh      Shared mesh for all instances
         * @param positions Instance positions (world space)
         * @param scale     Uniform scale for all instances
         */
        void AddInstances(Mesh *mesh,
                          const std::vector<glm::vec3> &positions,
                          float scale = 1.0f);

        /**
         * Add instances with individual transforms.
         *
         * @param mesh      Shared mesh
         * @param instances Per-instance transform data
         */
        void AddInstances(Mesh *mesh,
                          const std::vector<InstanceData> &instances);

        /**
         * Upload instance data to GPU.
         * Call after all AddInstances calls and before Draw.
         */
        void UploadInstances();

        /**
         * Draw all instanced batches.
         *
         * @param commandBuffer Active command buffer
         * @param viewProjection Combined view-projection matrix
         */
        void Draw(VkCommandBuffer commandBuffer,
                  VkDescriptorSet descriptorSet,
                  VkPipelineLayout pipelineLayout);

        /**
         * Get total instance count across all batches.
         */
        uint32_t GetTotalInstanceCount() const;

        /**
         * Get number of draw calls.
         */
        uint32_t GetDrawCallCount() const { return static_cast<uint32_t>(m_Batches.size()); }

    private:
        void CreateInstanceBuffer();
        void CreatePipeline(VkDescriptorSetLayout descriptorSetLayout, VkRenderPass renderPass);

        VulkanDevice *m_Device = nullptr;
        std::unique_ptr<VulkanBuffer> m_InstanceBuffer;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_Pipeline = VK_NULL_HANDLE;

        std::vector<InstanceBatch> m_Batches;
        std::vector<InstanceData> m_AllInstances;
        size_t m_InstanceBufferCapacity = 0;

        static constexpr size_t INITIAL_INSTANCE_CAPACITY = 10000;
        static constexpr size_t MAX_INSTANCES = 100000;
    };

    /**
     * Section 30.3: Vegetation spawning helper.
     *
     * Uses biome data to determine vegetation density and types.
     */
    struct VegetationInstance
    {
        glm::vec3 position;
        float scale;
        float rotation; // Y-axis rotation
        int meshType;   // 0 = tree, 1 = rock, 2+ = future vegetation
    };

    /**
     * Spawns vegetation based on biome and terrain data.
     */
    class VegetationSpawner
    {
    public:
        struct SpawnSettings
        {
            // Density per biome (instances per unit area)
            float forestTreeDensity = 0.02f;
            float grasslandTreeDensity = 0.002f;
            float desertRockDensity = 0.01f;
            float mountainRockDensity = 0.015f;

            // Scale ranges
            glm::vec2 treeScaleRange{0.8f, 1.2f};
            glm::vec2 rockScaleRange{0.5f, 1.5f};

            // Placement rules
            float minSpacing = 2.0f;       // Minimum distance between vegetation
            float maxSlopeForTrees = 0.5f; // Trees don't grow on steep slopes
            float minHeightAboveWater = 1.0f;
        };

        VegetationSpawner() = default;

        /**
         * Configure spawning settings.
         */
        void SetSettings(const SpawnSettings &settings) { m_Settings = settings; }
        const SpawnSettings &GetSettings() const { return m_Settings; }

        /**
         * Spawn vegetation for a chunk.
         *
         * @param heightmap   Terrain heights
         * @param slopeMap    Terrain slopes (optional)
         * @param biomeWeights Biome weights per cell (optional)
         * @param width       Grid width
         * @param depth       Grid depth
         * @param cellSize    World units per cell
         * @param chunkOffset Chunk world offset
         * @param seaLevel    Sea level height
         * @param seed        Random seed
         * @return Vector of vegetation instances
         */
        std::vector<VegetationInstance> Spawn(const std::vector<float> &heightmap,
                                              const std::vector<float> *slopeMap,
                                              int width, int depth,
                                              float cellSize,
                                              const glm::vec3 &chunkOffset,
                                              float seaLevel,
                                              uint32_t seed) const;

    private:
        SpawnSettings m_Settings;
    };

}
