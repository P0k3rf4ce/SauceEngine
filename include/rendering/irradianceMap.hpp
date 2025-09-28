#ifndef IRRADIANCE_MAP_HPP
#define IRRADIANCE_MAP_HPP

#include <Eigen/Core>
#include "shared/Shader.hpp"

namespace rendering {
class IrradianceMap {
public:
    /**
     * Generates a diffuse irradiance map from an environment cubemap.
     * @param envCubemap The input environment cubemap texture ID.
     * @param captureFBO Framebuffer object used for rendering.
     * @param captureRBO Renderbuffer object used for depth testing.
     * @param captureViews Array of 6 view matrices for cubemap face rendering.
     * @param captureProj Projection matrix for cubemap rendering.
     * @param cubeVAO Vertex array object for rendering a unit cube.
     * @param cubeIndexCount Number of indices in the cube mesh.
     * @return The OpenGL texture ID of the generated irradiance cubemap.
     */
    static unsigned int genIrradianceMap(
        unsigned int envCubemap,
        unsigned int captureFBO,
        unsigned int captureRBO,
        Eigen::Matrix4i captureViews[6],
        Eigen::Matrix4i captureProj,
        unsigned int cubeVAO,
        unsigned int cubeIndexCount
    );
};

#endif // IRRADIANCE_MAP_HPP
}
