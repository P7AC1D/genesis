#include "genesis/renderer/InstancedRenderer.h"
#include "genesis/renderer/VulkanDevice.h"
#include "genesis/renderer/VulkanBuffer.h"
#include "genesis/core/Log.h"
#include <glm/glm.hpp>
#include <random>
#include <cmath>

namespace
{
    constexpr float TWO_PI = 6.28318530717958647692f;
}

namespace Genesis
{

    // ==================== InstanceData ====================

    std::vector<VkVertexInputBindingDescription> InstanceData::GetBindingDescription()
    {
        std::vector<VkVertexInputBindingDescription> bindings(1);

        // Instance data binding (binding = 1, after vertex data at binding 0)
        bindings[0].binding = 1;
        bindings[0].stride = sizeof(InstanceData);
        bindings[0].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

        return bindings;
    }

    std::vector<VkVertexInputAttributeDescription> InstanceData::GetAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attributes(2);

        // Position and scale (location 4 - after vertex attributes 0-3)
        attributes[0].binding = 1;
        attributes[0].location = 4;
        attributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributes[0].offset = offsetof(InstanceData, PositionAndScale);

        // Rotation and tint (location 5)
        attributes[1].binding = 1;
        attributes[1].location = 5;
        attributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributes[1].offset = offsetof(InstanceData, RotationAndTint);

        return attributes;
    }

    // ==================== InstancedRenderer ====================

    InstancedRenderer::~InstancedRenderer()
    {
        Shutdown();
    }

    void InstancedRenderer::Init(VulkanDevice &device, VkDescriptorSetLayout descriptorSetLayout,
                                 VkRenderPass renderPass)
    {
        m_Device = &device;
        CreateInstanceBuffer();
        // Note: Pipeline creation would require instanced shader variants
        // For now, we'll use the standard pipeline with manual instance handling
    }

    void InstancedRenderer::Shutdown()
    {
        if (m_Device)
        {
            if (m_Pipeline != VK_NULL_HANDLE)
            {
                vkDestroyPipeline(m_Device->GetDevice(), m_Pipeline, nullptr);
                m_Pipeline = VK_NULL_HANDLE;
            }

            if (m_InstanceBuffer)
            {
                m_InstanceBuffer->Shutdown();
                m_InstanceBuffer.reset();
            }

            m_Device = nullptr;
        }

        m_Batches.clear();
        m_AllInstances.clear();
    }

    void InstancedRenderer::BeginFrame()
    {
        m_Batches.clear();
        m_AllInstances.clear();
    }

    void InstancedRenderer::AddInstances(Mesh *mesh,
                                         const std::vector<glm::vec3> &positions,
                                         float scale)
    {
        if (!mesh || positions.empty())
        {
            return;
        }

        // Find or create batch for this mesh
        InstanceBatch *batch = nullptr;
        for (auto &b : m_Batches)
        {
            if (b.mesh == mesh)
            {
                batch = &b;
                break;
            }
        }

        if (!batch)
        {
            m_Batches.emplace_back();
            batch = &m_Batches.back();
            batch->mesh = mesh;
        }

        // Add instances
        for (const auto &pos : positions)
        {
            InstanceData instance;
            instance.PositionAndScale = glm::vec4(pos, scale);
            instance.RotationAndTint = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            batch->instances.push_back(instance);
        }

        batch->instanceCount = static_cast<uint32_t>(batch->instances.size());
    }

    void InstancedRenderer::AddInstances(Mesh *mesh,
                                         const std::vector<InstanceData> &instances)
    {
        if (!mesh || instances.empty())
        {
            return;
        }

        // Find or create batch for this mesh
        InstanceBatch *batch = nullptr;
        for (auto &b : m_Batches)
        {
            if (b.mesh == mesh)
            {
                batch = &b;
                break;
            }
        }

        if (!batch)
        {
            m_Batches.emplace_back();
            batch = &m_Batches.back();
            batch->mesh = mesh;
        }

        batch->instances.insert(batch->instances.end(), instances.begin(), instances.end());
        batch->instanceCount = static_cast<uint32_t>(batch->instances.size());
    }

    void InstancedRenderer::UploadInstances()
    {
        // Collect all instances
        m_AllInstances.clear();
        for (const auto &batch : m_Batches)
        {
            m_AllInstances.insert(m_AllInstances.end(),
                                  batch.instances.begin(),
                                  batch.instances.end());
        }

        if (m_AllInstances.empty() || !m_InstanceBuffer)
        {
            return;
        }

        // Resize buffer if needed
        size_t requiredSize = m_AllInstances.size() * sizeof(InstanceData);
        if (requiredSize > m_InstanceBufferCapacity)
        {
            m_InstanceBuffer->Shutdown();

            size_t newCapacity = std::max(requiredSize, m_InstanceBufferCapacity * 2);
            newCapacity = std::min(newCapacity, MAX_INSTANCES * sizeof(InstanceData));

            m_InstanceBuffer->Init(*m_Device, newCapacity,
                                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            m_InstanceBuffer->Map();
            m_InstanceBufferCapacity = newCapacity;
        }

        // Upload data
        m_InstanceBuffer->WriteToBuffer(m_AllInstances.data(), requiredSize);
    }

    void InstancedRenderer::Draw(VkCommandBuffer commandBuffer,
                                 VkDescriptorSet descriptorSet,
                                 VkPipelineLayout pipelineLayout)
    {
        if (m_AllInstances.empty() || !m_InstanceBuffer)
        {
            return;
        }

        // Note: For true instanced rendering, we would:
        // 1. Bind the instance buffer as vertex buffer binding 1
        // 2. Use vkCmdDrawIndexedInstanced with instance count
        // This requires shader support for instance attributes

        // Current implementation: draw each instance separately
        // (inefficient but works with existing shaders)
        size_t instanceOffset = 0;
        for (const auto &batch : m_Batches)
        {
            if (!batch.mesh || batch.instances.empty())
            {
                continue;
            }

            // For each instance, we would normally use push constants
            // to set the transform. This is a placeholder for full instancing.
            batch.mesh->Bind(commandBuffer);

            for (const auto &instance : batch.instances)
            {
                // In a full implementation, this would be a single
                // vkCmdDrawIndexed with instanceCount = batch.instanceCount
                batch.mesh->Draw(commandBuffer);
            }

            instanceOffset += batch.instances.size();
        }
    }

    uint32_t InstancedRenderer::GetTotalInstanceCount() const
    {
        uint32_t total = 0;
        for (const auto &batch : m_Batches)
        {
            total += batch.instanceCount;
        }
        return total;
    }

    void InstancedRenderer::CreateInstanceBuffer()
    {
        m_InstanceBuffer = std::make_unique<VulkanBuffer>();

        VkDeviceSize bufferSize = INITIAL_INSTANCE_CAPACITY * sizeof(InstanceData);
        m_InstanceBuffer->Init(*m_Device, bufferSize,
                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_InstanceBuffer->Map();
        m_InstanceBufferCapacity = bufferSize;
    }

    void InstancedRenderer::CreatePipeline(VkDescriptorSetLayout descriptorSetLayout,
                                           VkRenderPass renderPass)
    {
        // Pipeline creation for instanced rendering would go here
        // This requires instanced vertex shader variants
        // For now, we use the standard pipeline
    }

    // ==================== VegetationSpawner ====================

    std::vector<VegetationInstance> VegetationSpawner::Spawn(const std::vector<float> &heightmap,
                                                             const std::vector<float> *slopeMap,
                                                             int width, int depth,
                                                             float cellSize,
                                                             const glm::vec3 &chunkOffset,
                                                             float seaLevel,
                                                             uint32_t seed) const
    {
        std::vector<VegetationInstance> instances;

        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> posOffset(-0.4f, 0.4f);
        std::uniform_real_distribution<float> rotDist(0.0f, TWO_PI);
        std::uniform_real_distribution<float> scaleDist(0.0f, 1.0f);
        std::uniform_real_distribution<float> probDist(0.0f, 1.0f);

        float chunkArea = width * depth * cellSize * cellSize;

        // Calculate expected counts
        int expectedTrees = static_cast<int>(chunkArea * m_Settings.forestTreeDensity);
        int expectedRocks = static_cast<int>(chunkArea * m_Settings.mountainRockDensity);

        instances.reserve(expectedTrees + expectedRocks);

        // Spawn vegetation
        for (int z = 0; z < depth; z++)
        {
            for (int x = 0; x < width; x++)
            {
                size_t idx = static_cast<size_t>(z) * width + x;
                float height = heightmap[idx];

                // Skip underwater areas
                if (height < seaLevel + m_Settings.minHeightAboveWater)
                {
                    continue;
                }

                // Check slope
                float slope = 0.0f;
                if (slopeMap && idx < slopeMap->size())
                {
                    slope = (*slopeMap)[idx];
                }

                // Try spawning a tree
                if (slope < m_Settings.maxSlopeForTrees)
                {
                    if (probDist(rng) < m_Settings.forestTreeDensity * cellSize * cellSize)
                    {
                        VegetationInstance tree;
                        tree.position = chunkOffset + glm::vec3(
                                                          (x + 0.5f + posOffset(rng)) * cellSize,
                                                          height,
                                                          (z + 0.5f + posOffset(rng)) * cellSize);

                        float t = scaleDist(rng);
                        tree.scale = glm::mix(m_Settings.treeScaleRange.x,
                                              m_Settings.treeScaleRange.y, t);
                        tree.rotation = rotDist(rng);
                        tree.meshType = 0; // Tree

                        instances.push_back(tree);
                    }
                }

                // Try spawning a rock
                if (probDist(rng) < m_Settings.mountainRockDensity * cellSize * cellSize)
                {
                    VegetationInstance rock;
                    rock.position = chunkOffset + glm::vec3(
                                                      (x + 0.5f + posOffset(rng)) * cellSize,
                                                      height,
                                                      (z + 0.5f + posOffset(rng)) * cellSize);

                    float t = scaleDist(rng);
                    rock.scale = glm::mix(m_Settings.rockScaleRange.x,
                                          m_Settings.rockScaleRange.y, t);
                    rock.rotation = rotDist(rng);
                    rock.meshType = 1; // Rock

                    instances.push_back(rock);
                }
            }
        }

        return instances;
    }

}
