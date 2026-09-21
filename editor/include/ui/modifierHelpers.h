#pragma once

#include <cmath>
#include <string>

#include <math/public/g_math.h>
#include <reflection/public/type_descriptor.h>

inline std::string getDisplayName(const engine::reflection::TypeDescriptor& type, const engine::reflection::FieldDescriptor* field = nullptr) {
    if (field) {
        auto dispName = field->modifiers.tryGet<engine::reflection::modifiers::DisplayName>();
        if (dispName) {
            return std::string(dispName->value);
        }
        return std::string(field->name);
    }
    auto dispName = type.modifiers.tryGet<engine::reflection::modifiers::DisplayName>();
    if (dispName) {
        return std::string(dispName->value);
    }
    return std::string(type.name);
}

inline float getDragSpeed(const engine::reflection::FieldDescriptor* field) {
    if (field) {
        auto step = field->modifiers.tryGet<engine::reflection::modifiers::Step>();
        if (step && step->value > 0 && std::isfinite(step->value)) {
            return step->value;
        }
    }
    return 1;
}
