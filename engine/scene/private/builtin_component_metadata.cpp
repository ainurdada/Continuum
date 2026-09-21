#include <builtin_component_metadata.h>

#include <array>

namespace engine::scene {

// Metadatas
namespace {

// Transform
constexpr std::array<FieldMetadata, 3> transformFields{
    FieldMetadata{"position", "Position", FieldKind::Vec3, FieldUnit::Meters},
    FieldMetadata{"rotation", "Rotation", FieldKind::Vec3, FieldUnit::Radians},
    FieldMetadata{"scale", "Scale", FieldKind::Vec3, FieldUnit::None},
};
constexpr ComponentMetadata transformComponentMetadata{"continuum.scene.transform", "Transform", transformFields};

// Camera
constexpr std::array<FieldMetadata, 3> cameraFields{
    FieldMetadata{"verticalFov", "Vertical FOV", FieldKind::Float, FieldUnit::Radians},
    FieldMetadata{"nearPlane", "Near plane", FieldKind::Float, FieldUnit::Meters},
    FieldMetadata{"farPlane", "Far plane", FieldKind::Float, FieldUnit::Meters},
};
constexpr ComponentMetadata cameraComponentMetadata{"continuum.scene.camera", "Camera", cameraFields};

// Name
constexpr std::array<FieldMetadata, 1> nameFields{
    FieldMetadata{"name", "Name", FieldKind::String, FieldUnit::None},
};
constexpr ComponentMetadata nameComponentMetadata{"continuum.scene.name", "Name", nameFields};

// Parent
constexpr std::array<FieldMetadata, 1> parentFields{
    FieldMetadata{"parent", "Parent", FieldKind::Entity, FieldUnit::None},
};
constexpr ComponentMetadata parentComponentMetadata{"continuum.scene.parent", "Parent", parentFields};

// MeshRenderer
constexpr std::array<FieldMetadata, 1> meshRendererFields{
    FieldMetadata{"geometry", "Geometry", FieldKind::Enum, FieldUnit::None},
};
constexpr ComponentMetadata meshRendererComponentMetadata{"continuum.scene.meshRenderer", "Mesh renderer", meshRendererFields};

} // namespace

const ComponentMetadata& transformMetadata() {
    return transformComponentMetadata;
}

const ComponentMetadata& cameraMetadata() {
    return cameraComponentMetadata;
}

const ComponentMetadata& nameMetadata() {
    return nameComponentMetadata;
}

const ComponentMetadata& parentMetadata() {
    return parentComponentMetadata;
}

const ComponentMetadata& meshRendererMetadata() {
    return meshRendererComponentMetadata;
}

} // namespace engine::scene
