#pragma once

#include <reflection/public/type_descriptor.h>

#include "g_math.h"

namespace engine::reflection {

inline const TypeDescriptor* vec3fType() {
    static const TypeDescriptor desc{
        .nativeTypeKey = typeid(Vec3f),
        .key = "Vec3f",
        .name = "Vec3f",
        .category = TypeCategory::Object,
    };
    return &desc;
}

} // namespace engine::reflection