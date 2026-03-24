#include "app/components/RigidBodyComponent.hpp"

#include <limits>

namespace sauce {

namespace {

glm::quat integrateOrientation(const glm::quat& orientation,
                               const glm::vec3& angularVelocity,
                               float deltaTime) {
	if (glm::dot(angularVelocity, angularVelocity) <= 1e-10f) {
		return orientation;
	}

	const glm::quat spin(0.0f, angularVelocity.x, angularVelocity.y, angularVelocity.z);
	return glm::normalize(orientation + 0.5f * spin * orientation * deltaTime);
}

glm::vec3 scalePoint(const glm::vec3& point, const glm::vec3& scale) {
	return point * scale;
}

float scaleVolumeFactor(const glm::vec3& scale) {
	return std::abs(scale.x * scale.y * scale.z);
}

} // namespace

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


float trianglevolume(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
    auto v321 = p3.x*p2.y*p1.z;
    auto v231 = p2.x*p3.y*p1.z;
    auto v312 = p3.x*p1.y*p2.z;
    auto v132 = p1.x*p3.y*p2.z;
    auto v213 = p2.x*p1.y*p3.z;
    auto v123 = p1.x*p2.y*p3.z;
    return (1.0f/6.0f)*(-v321 + v231 + v312 - v132 - v213 + v123);
}
float RigidBodyComponent::meshInvMass(std::shared_ptr<modeling::Mesh> m) {
	/*
	 * approximate a volume for the mesh, then assume a
	 * constant density and calculate an (inverse) mass.
	 *
	 * http://chenlab.ece.cornell.edu/Publication/Cha/icip01_Cha.pdf
	 * https://stackoverflow.com/questions/1406029/how-to-calculate-the-volume-of-a-3d-mesh-object-the-surface-of-which-is-made-up
	 */

	float volume=0.f;
	// iterate vertices in index order
	auto vertices=m->getVertices();
	auto indices=m->getIndices();

	if (indices.size() % 3 != 0)
		return volume;
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i + 0];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

		volume+=trianglevolume(vertices[i0].position, vertices[i1].position, vertices[i2].position);
	}
	if (volume<0.f)
		volume*=-1.f;
	if (volume <= 1e-6f)
		return 0.0f;
	return 1.0f / volume;
}

float RigidBodyComponent::scaledMeshInvMass(std::shared_ptr<modeling::Mesh> m, const glm::vec3& scale) {
	const float invMass = meshInvMass(std::move(m));
	const float volumeScale = scaleVolumeFactor(scale);
	if (invMass <= 1e-8f || volumeScale <= 1e-8f) {
		return 0.0f;
	}

	return invMass / volumeScale;
}

glm::mat3 RigidBodyComponent::meshInvInertiaTensor(
	std::shared_ptr<modeling::Mesh> m,
	const glm::vec3& centerOfMass,
	float invMass) {
	return meshInvInertiaTensor(std::move(m), centerOfMass, invMass, glm::vec3(1.0f));
}

glm::mat3 RigidBodyComponent::meshInvInertiaTensor(
	std::shared_ptr<modeling::Mesh> m,
	const glm::vec3& centerOfMass,
	float invMass,
	const glm::vec3& scale) {
	if (!m || invMass <= 1e-8f) {
		return glm::mat3(0.0f);
	}

	glm::vec3 minExt(std::numeric_limits<float>::infinity());
	glm::vec3 maxExt(-std::numeric_limits<float>::infinity());
	for (const auto& v : m->getVertices()) {
		const glm::vec3 local = scalePoint(v.position - centerOfMass, scale);
		minExt = glm::min(minExt, local);
		maxExt = glm::max(maxExt, local);
	}

	const glm::vec3 size = glm::max(maxExt - minExt, glm::vec3(1e-3f));
	const float mass = 1.0f / invMass;
	const float ixx = (mass / 12.0f) * (size.y * size.y + size.z * size.z);
	const float iyy = (mass / 12.0f) * (size.x * size.x + size.z * size.z);
	const float izz = (mass / 12.0f) * (size.x * size.x + size.y * size.y);

	glm::mat3 invInertia(0.0f);
	if (ixx > 1e-8f) invInertia[0][0] = 1.0f / ixx;
	if (iyy > 1e-8f) invInertia[1][1] = 1.0f / iyy;
	if (izz > 1e-8f) invInertia[2][2] = 1.0f / izz;
	return invInertia;
}

void RigidBodyComponent::update(float deltatime) {
	if (deltatime <= 0.0f) {
		return;
	}

	glm::vec3 centerPosition = getWorldCenterOfMass();
	const glm::vec3 acceleration = invMass * accumulatedForce;
	velocity += acceleration * deltatime;
	centerPosition += velocity * deltatime;
	orientation = integrateOrientation(orientation, angularVelocity, deltatime);

	setWorldCenterOfMass(centerPosition);
}

} // namespace sauce
