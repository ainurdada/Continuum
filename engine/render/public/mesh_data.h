#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace engine::graphics {

enum class VertexAttributeFormat {
    Float,
    Float2,
    Float3,
    Float4,
};

struct VertexAttributeDescription {
    std::size_t location = 0;
    VertexAttributeFormat format = VertexAttributeFormat::Float;
    std::size_t offset = 0;
};

using MeshHandle = std::uint32_t;

struct VerticesData {
    std::vector<std::byte> data{};
    std::vector<VertexAttributeDescription> descs{};
    std::size_t verticesCount = 0;
    std::size_t pitch = 0;
};

template <typename T> VerticesData makeVerticesData(std::span<const T> vertices, const std::vector<VertexAttributeDescription>& descs) {
    static_assert(std::is_trivially_copyable_v<T>);

    VerticesData result{};
    result.verticesCount = vertices.size();
    result.pitch = sizeof(T);

    auto bytes = std::as_bytes(vertices);
    result.data.assign(bytes.begin(), bytes.end());

    result.descs = descs;

    return result;
}

struct MeshData {
    VerticesData vertices{};
    std::vector<std::uint32_t> indices{};

    inline std::size_t indexBytes() const {
        return indices.size() * sizeof(std::uint32_t);
    }
};

} // namespace engine::graphics