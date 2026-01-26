#include "app/components/MeshRendererComponent.hpp"

namespace sauce {

MeshRendererComponent::MeshRendererComponent()
    : Component("MeshRendererComponent") {
}

MeshRendererComponent::MeshRendererComponent(std::shared_ptr<modeling::Mesh> mesh,
                                             std::shared_ptr<modeling::Material> material)
    : Component("MeshRendererComponent")
    , mesh(mesh)
    , material(material) {
}

void MeshRendererComponent::render() {
    if (!mesh || !material) {
        return;
    }

    // TODO: Implement actual rendering logic
    // This will be called by the renderer and should:
    // 1. Bind the material (textures, shader uniforms, etc.)
    // 2. Bind the mesh's vertex/index buffers
    // 3. Issue draw call
    //
    // For now, this is a placeholder
}

} // namespace sauce
