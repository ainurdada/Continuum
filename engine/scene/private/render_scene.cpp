#include <render_scene.h>

#include <scene/public/camera.h>
#include <scene/public/hierarchy.h>
#include <scene/public/mesh_reference.h>
#include <scene/public/mesh_renderer.h>
#include <scene/public/transform.h>

namespace engine {

void collectRenderFrameData(RenderFrameInput& in, RenderFrameData& out) {
    auto& world = in.world;

    auto transforms = world.findStash<engine::scene::Transform>();
    auto meshRenderers = world.findStash<engine::scene::MeshRenderer>();
    auto meshRefs = world.findStash<engine::scene::MeshReference>();
    auto cameras = world.findStash<engine::scene::Camera>();

    if (transforms && meshRenderers) {
        for (auto entity : in.renderEntities.view()) {
            auto transform = transforms->get(entity);
            auto mesh = meshRenderers->get(entity);

            engine::RenderItem item{};
            item.geometryId = meshRenderers->get(entity)->geometryId;
            item.baseColor = meshRenderers->get(entity)->baseColor;
            auto worldMatrix = engine::scene::worldMatrix(world, entity);
            if (!worldMatrix) {
                continue;
            }
            item.modelMatrix = worldMatrix.value();

            switch (item.geometryId) {
            case engine::GeometryId::UploadedMesh: {
                if (!meshRefs) {
                    continue;
                }
                auto meshRef = meshRefs->get(entity);
                if (!meshRef || !in.meshHandles.contains(meshRef->modelId)) {
                    continue;
                }
                for (auto& meshhandle : in.meshHandles.at(meshRef->modelId)) {
                    auto newItem = item;
                    newItem.meshHandle = meshhandle;
                    out.items.push_back(newItem);
                }
                continue;
            }

            default:
                out.items.push_back(item);
                break;
            }
        }
    }

    if (cameras && transforms) {
        for (auto entity : in.cameraEntities.view()) {
            auto camera = cameras->get(entity);
            auto transform = transforms->get(entity);
            auto cameraMatrix = engine::scene::worldMatrix(world, entity);
            if (!cameraMatrix) {
                continue;
            }

            engine::RenderCameraData rCamera{};
            rCamera.verticalFovRadians = camera->verticalFov;
            rCamera.nearPlane = camera->nearPlane;
            rCamera.farPlane = camera->farPlane;
            rCamera.viewMatrix = math::inverse(cameraMatrix.value());

            out.camera = rCamera;
            break;
        }
    }
}

} // namespace engine