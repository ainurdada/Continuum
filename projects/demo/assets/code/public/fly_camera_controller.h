#pragma once

namespace demo {

struct FlyCameraController {
    // m/s
    float moveSpeed = 3;
    // rad/mouse motion unit
    float lookSensitivity = 0.01;
};

} // namespace demo
