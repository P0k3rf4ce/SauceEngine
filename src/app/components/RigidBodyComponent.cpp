#include "app/components/RigidBodyComponent.hpp"

namespace sauce {

    glm::vec3 RigidBodyComponent::meshCenterOfMass(std::shared_ptr<modeling::Mesh> m) {
        auto ret = glm::vec3(0.f, 0.f, 0.f);
        float n = 0.f;
        /*
	 * simple center of mass for a 3d mesh - just average the coordinates
	 */
        for (auto& v : m->getVertices()) {
            ret.x += v.position.x;
            ret.y += v.position.y;
            ret.z += v.position.z;
            n++;
        }

        n = (n == 0.f) ? 1.f : n; // dont divide by 0
        return ret / n;
    }

    float trianglevolume(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
        auto v321 = p3.x * p2.y * p1.z;
        auto v231 = p2.x * p3.y * p1.z;
        auto v312 = p3.x * p1.y * p2.z;
        auto v132 = p1.x * p3.y * p2.z;
        auto v213 = p2.x * p1.y * p3.z;
        auto v123 = p1.x * p2.y * p3.z;
        return (1.0f / 6.0f) * (-v321 + v231 + v312 - v132 - v213 + v123);
    }
    float RigidBodyComponent::meshInvMass(std::shared_ptr<modeling::Mesh> m) {
        /*
	 * approximate a volume for the mesh, then assume a
	 * constant density and calculate an (inverse) mass.
	 *
	 * http://chenlab.ece.cornell.edu/Publication/Cha/icip01_Cha.pdf
	 * https://stackoverflow.com/questions/1406029/how-to-calculate-the-volume-of-a-3d-mesh-object-the-surface-of-which-is-made-up
	 */

        float volume = 0.f;
        // iterate vertices in index order
        auto vertices = m->getVertices();
        auto indices = m->getIndices();

        if (indices.size() % 3 != 0)
            return volume;
        for (size_t i = 0; i < indices.size(); i += 3) {
            uint32_t i0 = indices[i + 0];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];

            volume +=
                trianglevolume(vertices[i0].position, vertices[i1].position, vertices[i2].position);
        }
        if (volume < 0.f)
            volume *= -1.f;
        return volume;
    }

} // namespace sauce
