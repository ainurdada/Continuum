#pragma once

#include <cstdint>
#include <vector>

namespace engine::graphics {

using MeshHandle = std::uint32_t;

struct PositionVertex {
    float x;
    float y;
    float z;
};

struct MeshData {
    std::vector<PositionVertex> positionVertices{};
    std::vector<std::uint32_t> indices{};

    inline std::size_t positionVertexBytes() const {
        return positionVertices.size() * sizeof(PositionVertex);
    }

    inline std::size_t indexBytes() const {
        return indices.size() * sizeof(std::uint32_t);
    }
};

} // namespace engine::graphics