#pragma once

#include <vector>
#include <cstdint>

namespace engine::graphics {

struct PositionVertex {
    float x;
    float y;
    float z;
};

struct MeshData {
    std::vector<PositionVertex> positionVertices{};
    std::vector<std::uint16_t> indices{};

    inline std::size_t positionVertexBytes() const {
        return positionVertices.size() * sizeof(PositionVertex);
    }

    inline std::size_t indexBytes() const {
        return indices.size() * sizeof(std::uint16_t);
    }
};

} // namespace engine::graphics