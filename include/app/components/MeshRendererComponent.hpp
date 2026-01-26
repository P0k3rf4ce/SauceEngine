#pragma once

#include "app/Component.hpp"
#include "app/modeling/Mesh.hpp"
#include "app/modeling/Material.hpp"
#include <memory>

namespace sauce {

class MeshRendererComponent : public Component {
public:
    MeshRendererComponent();
    MeshRendererComponent(std::shared_ptr<modeling::Mesh> mesh,
                          std::shared_ptr<modeling::Material> material);

    // Getters
    std::shared_ptr<modeling::Mesh> getMesh() const { return mesh; }
    std::shared_ptr<modeling::Material> getMaterial() const { return material; }

    // Setters
    void setMesh(std::shared_ptr<modeling::Mesh> mesh) { this->mesh = mesh; }
    void setMaterial(std::shared_ptr<modeling::Material> material) { this->material = material; }

    // Component interface
    virtual void render() override;
    virtual ~MeshRendererComponent() = default;

private:
    std::shared_ptr<modeling::Mesh> mesh;
    std::shared_ptr<modeling::Material> material;
};

} // namespace sauce
