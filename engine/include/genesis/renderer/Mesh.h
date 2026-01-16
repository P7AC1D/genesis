#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <vulkan/vulkan.h>

namespace Genesis {

    class VulkanDevice;
    class VulkanBuffer;

    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec3 Color;
        glm::vec2 TexCoord;

        static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

        bool operator==(const Vertex& other) const {
            return Position == other.Position && Normal == other.Normal &&
                   Color == other.Color && TexCoord == other.TexCoord;
        }
    };

    class Mesh {
    public:
        Mesh() = default;
        ~Mesh();

        void Init(VulkanDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
        void Shutdown();

        void Bind(VkCommandBuffer commandBuffer) const;
        void Draw(VkCommandBuffer commandBuffer) const;

        uint32_t GetVertexCount() const { return m_VertexCount; }
        uint32_t GetIndexCount() const { return m_IndexCount; }

        // Low-poly mesh generators
        static std::unique_ptr<Mesh> CreateCube(VulkanDevice& device, const glm::vec3& color = glm::vec3(1.0f));
        static std::unique_ptr<Mesh> CreatePlane(VulkanDevice& device, float size = 1.0f, const glm::vec3& color = glm::vec3(0.3f, 0.8f, 0.3f));
        static std::unique_ptr<Mesh> CreateIcosphere(VulkanDevice& device, int subdivisions = 1, const glm::vec3& color = glm::vec3(1.0f));
        static std::unique_ptr<Mesh> CreateLowPolyTree(VulkanDevice& device);
        static std::unique_ptr<Mesh> CreateLowPolyRock(VulkanDevice& device);

    private:
        void CreateVertexBuffer(VulkanDevice& device, const std::vector<Vertex>& vertices);
        void CreateIndexBuffer(VulkanDevice& device, const std::vector<uint32_t>& indices);

    private:
        VulkanDevice* m_Device = nullptr;
        std::unique_ptr<VulkanBuffer> m_VertexBuffer;
        std::unique_ptr<VulkanBuffer> m_IndexBuffer;
        uint32_t m_VertexCount = 0;
        uint32_t m_IndexCount = 0;
    };

}
