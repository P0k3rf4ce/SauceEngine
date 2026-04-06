#include "app/modeling/Mesh.hpp"
#include <glm/glm.hpp>
#include "app/BufferUtils.hpp"
#include "app/LogicalDevice.hpp"
#include <cstring>

namespace sauce {
namespace modeling {
namespace {

bool uploadVertexData(
    const std::vector<sauce::Vertex>& vertices,
    const sauce::LogicalDevice& logicalDevice,
    vk::raii::PhysicalDevice& physicalDevice,
    vk::raii::CommandPool& commandPool,
    vk::raii::Queue& queue,
    vk::raii::Buffer& destinationBuffer) {
    if (vertices.empty()) {
        return false;
    }

    const vk::DeviceSize vertexBufferSize =
        sizeof(sauce::Vertex) * vertices.size();

    vk::raii::Buffer stagingVertexBuffer(nullptr);
    vk::raii::DeviceMemory stagingVertexBufferMemory(nullptr);
    sauce::BufferUtils::createBuffer(
        physicalDevice,
        logicalDevice,
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingVertexBuffer,
        stagingVertexBufferMemory);

    void* data = stagingVertexBufferMemory.mapMemory(0, vertexBufferSize);
    std::memcpy(data, vertices.data(), static_cast<size_t>(vertexBufferSize));
    stagingVertexBufferMemory.unmapMemory();

    sauce::BufferUtils::copyBuffer(
        logicalDevice,
        commandPool,
        queue,
        stagingVertexBuffer,
        destinationBuffer,
        vertexBufferSize);
    return true;
}

} // namespace

Mesh::Mesh(const std::vector<sauce::Vertex>& vertices, const std::vector<uint32_t>& indices)
    : vertices(vertices)
    , indices(indices) {
}

Mesh::~Mesh() {
    releaseVertexBuffer();
}

bool Mesh::isValid() const {
    if (vertices.empty()) {
        return false;
    }

    if (indices.empty()) {
        return false;
    }

    // Check that all indices are in bounds
    for (const auto& index : indices) {
        if (index >= vertices.size()) {
            return false;
        }
    }

    // Check that indices count is a multiple of 3 (triangles)
    if (indices.size() % 3 != 0) {
        // TODO: Log warning or handle invalid index count
    }

    return true;
}

void Mesh::setMetadata(const std::string& key, const PropertyValue& value) {
    metadata[key] = value;
}

bool Mesh::hasMetadata(const std::string& key) const {
    return metadata.find(key) != metadata.end();
}

void Mesh::initVulkanResources(const sauce::LogicalDevice& logicalDevice, vk::raii::PhysicalDevice& physicalDevice, vk::raii::CommandPool& commandPool, vk::raii::Queue& queue) {
    const vk::DeviceSize currentVertexBufferSize = sizeof(vertices[0]) * vertices.size();

    releaseVertexBuffer();
    vertexBufferSizeBytes = currentVertexBufferSize;
    vertexBuffer = std::make_unique<vk::raii::Buffer>(nullptr);
    vertexBufferMemory = std::make_unique<vk::raii::DeviceMemory>(nullptr);
    if (dynamicVertexBuffer) {
        sauce::BufferUtils::createBuffer(
            physicalDevice,
            logicalDevice,
            vertexBufferSizeBytes,
            vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
            *vertexBuffer,
            *vertexBufferMemory);
        mappedVertexData = vertexBufferMemory->mapMemory(0, vertexBufferSizeBytes);
        std::memcpy(
            mappedVertexData,
            vertices.data(),
            static_cast<size_t>(vertexBufferSizeBytes));
    } else {
        sauce::BufferUtils::createBuffer(
            physicalDevice,
            logicalDevice,
            vertexBufferSizeBytes,
            vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            *vertexBuffer,
            *vertexBufferMemory);

        uploadVertexData(
            vertices,
            logicalDevice,
            physicalDevice,
            commandPool,
            queue,
            *vertexBuffer);
    }

    vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();

    vk::raii::Buffer stagingIndexBuffer(nullptr);
    vk::raii::DeviceMemory stagingIndexBufferMemory(nullptr);
    sauce::BufferUtils::createBuffer(physicalDevice, logicalDevice, indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingIndexBuffer, stagingIndexBufferMemory);

    void* data = stagingIndexBufferMemory.mapMemory(0, indexBufferSize);
    memcpy(data, indices.data(), (size_t)indexBufferSize);
    stagingIndexBufferMemory.unmapMemory();

    indexBuffer = std::make_unique<vk::raii::Buffer>(nullptr);
    indexBufferMemory = std::make_unique<vk::raii::DeviceMemory>(nullptr);
    sauce::BufferUtils::createBuffer(physicalDevice, logicalDevice, indexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, *indexBuffer, *indexBufferMemory);

    sauce::BufferUtils::copyBuffer(logicalDevice, commandPool, queue, stagingIndexBuffer, *indexBuffer, indexBufferSize);
}

bool Mesh::updateVertexBuffer(
    const sauce::LogicalDevice& logicalDevice,
    vk::raii::PhysicalDevice& physicalDevice,
    vk::raii::CommandPool& commandPool,
    vk::raii::Queue& queue) {
    if (vertices.empty()) {
        return false;
    }

    const vk::DeviceSize currentVertexBufferSize =
        sizeof(vertices[0]) * vertices.size();
    if (!vertexBuffer || !vertexBufferMemory ||
        currentVertexBufferSize != vertexBufferSizeBytes) {
        releaseVertexBuffer();
        vertexBufferSizeBytes = currentVertexBufferSize;
        vertexBuffer = std::make_unique<vk::raii::Buffer>(nullptr);
        vertexBufferMemory = std::make_unique<vk::raii::DeviceMemory>(nullptr);
        if (dynamicVertexBuffer) {
            sauce::BufferUtils::createBuffer(
                physicalDevice,
                logicalDevice,
                vertexBufferSizeBytes,
                vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent,
                *vertexBuffer,
                *vertexBufferMemory);
            mappedVertexData = vertexBufferMemory->mapMemory(0, vertexBufferSizeBytes);
        } else {
            sauce::BufferUtils::createBuffer(
                physicalDevice,
                logicalDevice,
                vertexBufferSizeBytes,
                vk::BufferUsageFlagBits::eTransferDst |
                    vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                *vertexBuffer,
                *vertexBufferMemory);
        }
    }

    if (dynamicVertexBuffer && mappedVertexData) {
        std::memcpy(
            mappedVertexData,
            vertices.data(),
            static_cast<size_t>(currentVertexBufferSize));
        return true;
    }

    return uploadVertexData(
        vertices,
        logicalDevice,
        physicalDevice,
        commandPool,
        queue,
        *vertexBuffer);
}

void Mesh::bind(vk::raii::CommandBuffer& commandBuffer) {
    vk::Buffer vertexBuffers[] = { **vertexBuffer };
    commandBuffer.bindVertexBuffers(0, *vertexBuffers, {0});
    commandBuffer.bindIndexBuffer(**indexBuffer, 0, vk::IndexType::eUint32);
}

void Mesh::draw(vk::raii::CommandBuffer& commandBuffer) {
    commandBuffer.drawIndexed(indices.size(), 1, 0, 0, 0);
}

void Mesh::generateNormals() {
    if (indices.size() % 3 != 0) {
        return;
    }

    // Reset all normals to zero
    for (auto& vertex : vertices) {
        vertex.normal = glm::vec3(0.0f);
    }

    // Accumulate face normals
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
            continue;
        }

        const glm::vec3& p0 = vertices[i0].position;
        const glm::vec3& p1 = vertices[i1].position;
        const glm::vec3& p2 = vertices[i2].position;

        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec3 faceNormal = glm::cross(edge1, edge2);

        // Accumulate (weighted by triangle area, which is proportional to length of cross product)
        vertices[i0].normal += faceNormal;
        vertices[i1].normal += faceNormal;
        vertices[i2].normal += faceNormal;
    }

    // Normalize all normals
    for (auto& vertex : vertices) {
        float length = glm::length(vertex.normal);
        if (length > 0.0001f) {
            vertex.normal = glm::normalize(vertex.normal);
        } else {
            // Fallback to up vector
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
}

void Mesh::generateTangents() {
    if (indices.size() % 3 != 0) {
        return;
    }

    // Reset all tangents to zero
    std::vector<glm::vec3> tangents(vertices.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> bitangents(vertices.size(), glm::vec3(0.0f));

    // Accumulate tangents and bitangents
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
            continue;
        }

        const glm::vec3& p0 = vertices[i0].position;
        const glm::vec3& p1 = vertices[i1].position;
        const glm::vec3& p2 = vertices[i2].position;

        const glm::vec2& uv0 = vertices[i0].texCoords;
        const glm::vec2& uv1 = vertices[i1].texCoords;
        const glm::vec2& uv2 = vertices[i2].texCoords;

        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec2 deltaUV1 = uv1 - uv0;
        glm::vec2 deltaUV2 = uv2 - uv0;

        float denom = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
        if (std::abs(denom) < 0.0001f) {
            // Degenerate UV, skip this triangle
            continue;
        }

        float f = 1.0f / denom;

        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        glm::vec3 bitangent;
        bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        tangents[i0] += tangent;
        tangents[i1] += tangent;
        tangents[i2] += tangent;

        bitangents[i0] += bitangent;
        bitangents[i1] += bitangent;
        bitangents[i2] += bitangent;
    }

    // Orthogonalize and normalize tangents
    for (size_t i = 0; i < vertices.size(); ++i) {
        const glm::vec3& n = vertices[i].normal;
        const glm::vec3& t = tangents[i];
        const glm::vec3& b = bitangents[i];

        // Gram-Schmidt orthogonalize
        glm::vec3 tangent = t - n * glm::dot(n, t);

        float tangentLength = glm::length(tangent);
        if (tangentLength > 0.0001f) {
            tangent = glm::normalize(tangent);
        } else {
            // Fallback to arbitrary perpendicular vector
            if (std::abs(n.x) > 0.9f) {
                tangent = glm::normalize(glm::cross(n, glm::vec3(0.0f, 1.0f, 0.0f)));
            } else {
                tangent = glm::normalize(glm::cross(n, glm::vec3(1.0f, 0.0f, 0.0f)));
            }
        }

        // Calculate handedness
        float handedness = (glm::dot(glm::cross(n, tangent), b) < 0.0f) ? -1.0f : 1.0f;

        vertices[i].tangent = glm::vec4(tangent, handedness);
    }
}

void Mesh::releaseVertexBuffer() {
    if (mappedVertexData && vertexBufferMemory) {
        vertexBufferMemory->unmapMemory();
    }
    mappedVertexData = nullptr;
    vertexBufferMemory.reset();
    vertexBuffer.reset();
    vertexBufferSizeBytes = 0;
}

} // namespace modeling
} // namespace sauce
