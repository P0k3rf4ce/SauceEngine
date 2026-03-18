#include "app/components/MeshRendererComponent.hpp"

namespace sauce {

    MeshRendererComponent::MeshRendererComponent() : Component("MeshRendererComponent") {
    }

    MeshRendererComponent::MeshRendererComponent(std::shared_ptr<modeling::Mesh> mesh,
                                                 std::shared_ptr<modeling::Material> material)
        : Component("MeshRendererComponent"), mesh(mesh), material(material) {
    }

    void MeshRendererComponent::render() {
        if (!mesh || !material) {
            return;
        }

        // TODO: Bind material, mesh buffers, and issue draw call
    }

} // namespace sauce
