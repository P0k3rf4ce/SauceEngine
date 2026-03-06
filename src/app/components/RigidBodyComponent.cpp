#include "app/components/RigidBodyComponent.hpp"

namespace sauce {

glm::vec3 RigidBodyComponent::meshCenterOfMass(std::shared_ptr<modeling::Mesh> m) {
	auto ret=glm::vec3(0.f,0.f,0.f);
	float n=0.f;
	/*
	 * simple center of mass for a 3d mesh - just average the coordinates
	 */
	for (auto &v : m->getVertices()) {
		ret.x+=v.position.x;
		ret.y+=v.position.y;
		ret.z+=v.position.z;
		n++;
	}

	n=(n==0.f)? 1.f : n; // dont divide by 0
	return ret / n;
}

} // namespace sauce
