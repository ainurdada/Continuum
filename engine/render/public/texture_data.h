#pragma once

#include <cstdint>
#include <vector>

namespace engine::graphics {

/// @brief Texture contains RGBA8 data
struct TextureData {
    std::uint32_t width;
    std::uint32_t height;
    std::vector<std::uint8_t> bytes;
};

}