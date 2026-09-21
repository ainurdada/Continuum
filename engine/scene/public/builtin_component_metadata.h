#pragma once

#include "component_metadata.h"

namespace engine::scene {

const ComponentMetadata& transformMetadata();
const ComponentMetadata& cameraMetadata();
const ComponentMetadata& nameMetadata();
const ComponentMetadata& parentMetadata();
const ComponentMetadata& meshRendererMetadata();

} // namespace engine::scene
