#include "genesis/renderer/Mesh.h"
#include "genesis/renderer/VulkanBuffer.h"
#include "genesis/renderer/VulkanDevice.h"

#include <cmath>

namespace Genesis {

    std::vector<VkVertexInputBindingDescription> Vertex::GetBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> Vertex::GetAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);

        // Position
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, Position);

        // Normal
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, Normal);

        // Color
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, Color);

        // TexCoord
        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, TexCoord);

        return attributeDescriptions;
    }

    Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
        : m_Vertices(vertices), m_Indices(indices), 
          m_VertexCount(static_cast<uint32_t>(vertices.size())),
          m_IndexCount(static_cast<uint32_t>(indices.size())) {
    }

    Mesh::Mesh(VulkanDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
        Init(device, vertices, indices);
    }

    Mesh::~Mesh() {
        if (m_Device) {
            Shutdown();
        }
    }

    void Mesh::Init(VulkanDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
        m_Device = &device;
        m_Vertices = vertices;
        m_Indices = indices;
        m_VertexCount = static_cast<uint32_t>(vertices.size());
        m_IndexCount = static_cast<uint32_t>(indices.size());

        CreateVertexBuffer(device, vertices);
        CreateIndexBuffer(device, indices);
    }

    void Mesh::Shutdown() {
        if (m_IndexBuffer) {
            m_IndexBuffer->Shutdown();
            m_IndexBuffer.reset();
        }
        if (m_VertexBuffer) {
            m_VertexBuffer->Shutdown();
            m_VertexBuffer.reset();
        }
        m_Device = nullptr;
    }

    void Mesh::CreateVertexBuffer(VulkanDevice& device, const std::vector<Vertex>& vertices) {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        // Staging buffer
        VulkanBuffer stagingBuffer;
        stagingBuffer.Init(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        stagingBuffer.Map();
        stagingBuffer.WriteToBuffer(vertices.data());
        stagingBuffer.Unmap();

        // Device local vertex buffer
        m_VertexBuffer = std::make_unique<VulkanBuffer>();
        m_VertexBuffer->Init(device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VulkanBuffer::CopyBuffer(device, stagingBuffer.GetBuffer(), m_VertexBuffer->GetBuffer(), bufferSize);
        stagingBuffer.Shutdown();
    }

    void Mesh::CreateIndexBuffer(VulkanDevice& device, const std::vector<uint32_t>& indices) {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        // Staging buffer
        VulkanBuffer stagingBuffer;
        stagingBuffer.Init(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        stagingBuffer.Map();
        stagingBuffer.WriteToBuffer(indices.data());
        stagingBuffer.Unmap();

        // Device local index buffer
        m_IndexBuffer = std::make_unique<VulkanBuffer>();
        m_IndexBuffer->Init(device, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VulkanBuffer::CopyBuffer(device, stagingBuffer.GetBuffer(), m_IndexBuffer->GetBuffer(), bufferSize);
        stagingBuffer.Shutdown();
    }

    void Mesh::Bind(VkCommandBuffer commandBuffer) const {
        VkBuffer buffers[] = {m_VertexBuffer->GetBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }

    void Mesh::Draw(VkCommandBuffer commandBuffer) const {
        vkCmdDrawIndexed(commandBuffer, m_IndexCount, 1, 0, 0, 0);
    }

    // Low-poly mesh generators
    std::unique_ptr<Mesh> Mesh::CreateCube(VulkanDevice& device, const glm::vec3& color) {
        std::vector<Vertex> vertices = {
            // Front face
            {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, color, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, color, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, color, {1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, color, {0.0f, 1.0f}},
            // Back face
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, color, {0.0f, 0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, color, {1.0f, 0.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, color, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, color, {0.0f, 1.0f}},
            // Top face
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, color, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, color, {1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, color, {0.0f, 1.0f}},
            // Bottom face
            {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, color, {1.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, color, {1.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, color, {0.0f, 1.0f}},
            // Right face
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, color, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, color, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, color, {0.0f, 1.0f}},
            // Left face
            {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, color, {1.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, color, {1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, color, {0.0f, 1.0f}},
        };

        std::vector<uint32_t> indices = {
            0,  1,  2,  2,  3,  0,   // Front
            4,  5,  6,  6,  7,  4,   // Back
            8,  9,  10, 10, 11, 8,   // Top
            12, 13, 14, 14, 15, 12,  // Bottom
            16, 17, 18, 18, 19, 16,  // Right
            20, 21, 22, 22, 23, 20   // Left
        };

        auto mesh = std::make_unique<Mesh>();
        mesh->Init(device, vertices, indices);
        return mesh;
    }

    std::unique_ptr<Mesh> Mesh::CreatePlane(VulkanDevice& device, float size, const glm::vec3& color) {
        float half = size / 2.0f;

        std::vector<Vertex> vertices = {
            {{-half, 0.0f,  half}, {0.0f, 1.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{ half, 0.0f,  half}, {0.0f, 1.0f, 0.0f}, color, {1.0f, 0.0f}},
            {{ half, 0.0f, -half}, {0.0f, 1.0f, 0.0f}, color, {1.0f, 1.0f}},
            {{-half, 0.0f, -half}, {0.0f, 1.0f, 0.0f}, color, {0.0f, 1.0f}},
        };

        std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};

        auto mesh = std::make_unique<Mesh>();
        mesh->Init(device, vertices, indices);
        return mesh;
    }

    std::unique_ptr<Mesh> Mesh::CreateIcosphere(VulkanDevice& device, int subdivisions, const glm::vec3& color) {
        // Golden ratio for icosahedron
        const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
        const float len = std::sqrt(1.0f + t * t);

        std::vector<Vertex> vertices = {
            {{-1.0f/len,  t/len, 0.0f}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{ 1.0f/len,  t/len, 0.0f}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{-1.0f/len, -t/len, 0.0f}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{ 1.0f/len, -t/len, 0.0f}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{0.0f, -1.0f/len,  t/len}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{0.0f,  1.0f/len,  t/len}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{0.0f, -1.0f/len, -t/len}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{0.0f,  1.0f/len, -t/len}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{ t/len, 0.0f, -1.0f/len}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{ t/len, 0.0f,  1.0f/len}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{-t/len, 0.0f, -1.0f/len}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{-t/len, 0.0f,  1.0f/len}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
        };

        std::vector<uint32_t> indices = {
            0, 11, 5,  0, 5, 1,   0, 1, 7,   0, 7, 10,  0, 10, 11,
            1, 5, 9,   5, 11, 4,  11, 10, 2, 10, 7, 6,  7, 1, 8,
            3, 9, 4,   3, 4, 2,   3, 2, 6,  3, 6, 8,   3, 8, 9,
            4, 9, 5,   2, 4, 11,  6, 2, 10, 8, 6, 7,   9, 8, 1
        };

        // Compute normals (for low-poly, each vertex normal = position normalized)
        for (auto& v : vertices) {
            v.Normal = glm::normalize(v.Position);
        }

        auto mesh = std::make_unique<Mesh>();
        mesh->Init(device, vertices, indices);
        return mesh;
    }

    std::unique_ptr<Mesh> Mesh::CreateLowPolyTree(VulkanDevice& device) {
        // Simple low-poly tree: trunk (box) + foliage (pyramid)
        glm::vec3 trunkColor{0.55f, 0.27f, 0.07f};
        glm::vec3 leafColor{0.2f, 0.6f, 0.2f};

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        // Trunk (thin box)
        float tw = 0.15f, th = 0.5f;
        uint32_t offset = 0;

        // Trunk vertices (simplified box)
        std::vector<Vertex> trunkVerts = {
            {{-tw, 0.0f,  tw}, {0.0f, 0.0f, 1.0f}, trunkColor, {0.0f, 0.0f}},
            {{ tw, 0.0f,  tw}, {0.0f, 0.0f, 1.0f}, trunkColor, {1.0f, 0.0f}},
            {{ tw, th,    tw}, {0.0f, 0.0f, 1.0f}, trunkColor, {1.0f, 1.0f}},
            {{-tw, th,    tw}, {0.0f, 0.0f, 1.0f}, trunkColor, {0.0f, 1.0f}},
            {{-tw, 0.0f, -tw}, {0.0f, 0.0f, -1.0f}, trunkColor, {0.0f, 0.0f}},
            {{ tw, 0.0f, -tw}, {0.0f, 0.0f, -1.0f}, trunkColor, {1.0f, 0.0f}},
            {{ tw, th,   -tw}, {0.0f, 0.0f, -1.0f}, trunkColor, {1.0f, 1.0f}},
            {{-tw, th,   -tw}, {0.0f, 0.0f, -1.0f}, trunkColor, {0.0f, 1.0f}},
        };

        for (auto& v : trunkVerts) vertices.push_back(v);

        std::vector<uint32_t> trunkIdx = {
            0, 1, 2, 2, 3, 0,  // Front
            5, 4, 7, 7, 6, 5,  // Back
            4, 0, 3, 3, 7, 4,  // Left
            1, 5, 6, 6, 2, 1,  // Right
        };
        for (auto i : trunkIdx) indices.push_back(i + offset);
        offset = static_cast<uint32_t>(vertices.size());

        // Foliage cone/pyramid
        float fh = 0.8f, fr = 0.4f;
        glm::vec3 apex{0.0f, th + fh, 0.0f};

        std::vector<Vertex> foliageVerts = {
            {{-fr, th, fr}, {-0.5f, 0.5f, 0.5f}, leafColor, {0.0f, 0.0f}},
            {{ fr, th, fr}, { 0.5f, 0.5f, 0.5f}, leafColor, {1.0f, 0.0f}},
            {{ fr, th,-fr}, { 0.5f, 0.5f,-0.5f}, leafColor, {1.0f, 1.0f}},
            {{-fr, th,-fr}, {-0.5f, 0.5f,-0.5f}, leafColor, {0.0f, 1.0f}},
            {apex, {0.0f, 1.0f, 0.0f}, leafColor, {0.5f, 0.5f}},
        };

        for (auto& v : foliageVerts) vertices.push_back(v);

        std::vector<uint32_t> foliageIdx = {
            0, 1, 4,  // Front
            1, 2, 4,  // Right
            2, 3, 4,  // Back
            3, 0, 4,  // Left
            0, 3, 2, 2, 1, 0,  // Bottom
        };
        for (auto i : foliageIdx) indices.push_back(i + offset);

        auto mesh = std::make_unique<Mesh>();
        mesh->Init(device, vertices, indices);
        return mesh;
    }

    std::unique_ptr<Mesh> Mesh::CreateLowPolyRock(VulkanDevice& device) {
        // Simple low-poly rock (deformed icosahedron)
        glm::vec3 color{0.5f, 0.5f, 0.5f};
        
        // Start with icosahedron base
        const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
        const float len = std::sqrt(1.0f + t * t);

        std::vector<Vertex> vertices = {
            {{-1.0f/len * 0.8f,  t/len * 0.6f, 0.0f}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{ 1.0f/len * 0.9f,  t/len * 0.7f, 0.0f}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{-1.0f/len * 0.7f, -t/len * 0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{ 1.0f/len * 0.85f,-t/len * 0.55f, 0.0f}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{0.0f, -1.0f/len * 0.6f,  t/len * 0.8f}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{0.0f,  1.0f/len * 0.7f,  t/len * 0.9f}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{0.0f, -1.0f/len * 0.5f, -t/len * 0.7f}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
            {{0.0f,  1.0f/len * 0.65f,-t/len * 0.85f}, {0.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
        };

        // Scale down
        for (auto& v : vertices) {
            v.Position *= 0.3f;
            v.Normal = glm::normalize(v.Position);
        }

        std::vector<uint32_t> indices = {
            0, 5, 1,  0, 1, 7,  0, 7, 2,  0, 2, 4,  0, 4, 5,
            3, 1, 5,  3, 7, 1,  3, 6, 7,  3, 4, 6,  3, 5, 4,
            2, 7, 6,  2, 6, 4
        };

        auto mesh = std::make_unique<Mesh>();
        mesh->Init(device, vertices, indices);
        return mesh;
    }

}
