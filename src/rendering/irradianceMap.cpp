#include "rendering/irradianceMap.hpp"

namespace rendering {

/**
 * Generate an irradiance map from a given environment cubemap
 */
unsigned int IrradianceMap::genIrradianceMap(
    unsigned int envCubemap, 
    unsigned int captureFBO, 
    unsigned int captureRBO, 
    Eigen::Matrix4i captureViews[], 
    Eigen::Matrix4i captureProj, 
    unsigned int cubeVAO, 
    unsigned int cubeIndexCount
) {
    // using a small 32x32 cubemap as irradiance map
    const unsigned int IRRADIANCE_SIZE = 32;

    // our irradiance map, which will be the convolution of the environment map
    unsigned int irradianceMap;

    // this glGenTextures call will generate a texture ID and store it in the variable irradianceMap
    glGenTextures(1, &irradianceMap);

    // makes irradianceMap the active cubemap texture so we can configure its faces and parameters
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

    static Shader irradianceShader;
    static bool shaderInitialized = false;

    if (!shaderInitialized)
    {
        std::unordered_map<SHADER_TYPE, std::string> shaderFiles = {
            {VERTEX, "src/rendering/shaders/irradiance.vert"},
            {FRAGMENT, "src/rendering/shaders/irradiance.frag"}};
        if (!irradianceShader.loadFromFiles(shaderFiles))
        {
            LOG_ERROR("Failed to load irradiance shader files.");
        }
        if (!irradianceShader.linkProgram())
        {
            LOG_ERROR("Failed to link irradiance shader.");
        }
        shaderInitialized = true;
    }

    // allocate each cubemap face with 16 bit floating point RGB format for HDR irradiance sampling
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                        IRRADIANCE_SIZE, IRRADIANCE_SIZE, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    // sets cubemap wrapping to clamp edges and prevent sampling artifacts at face boundaries
    // uses linear filtering to smooth texture sampling when scaling
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // prepare framebuffer for rendering each cubemap face during irradiance convolution
    // ----------------------------------------------------------------------------
    // tells openGL to render to the framebuffer object instead of the default framebuffer (the screen)
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    // set viewport to the dimensions of the irradiance map
    glViewport(0, 0, 32, 32);

    // configure the renderbuffer object for depth testing during cubemap face rendering
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);

    // allocate storage for the renderbuffer object
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, IRRADIANCE_SIZE, IRRADIANCE_SIZE);
    // ----------------------------------------------------------------------------

    // bind the irradiance shader
    irradianceShader.bind();
    irradianceShader.setUniform("environmentMap", 0);
    irradianceShader.setUniform("projection", captureProj.cast<float>().eval().data());

    // this will bind the environment map to texture unit 0 so that the shader can sample it
    // may need to change GL_TEXTURE0 to another texture unit if other textures are being used
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    for (unsigned int i = 0; i < 6; ++i)
    {
        // set the view matrix uniform in the shader
        irradianceShader.setUniform("view", captureViews[i].cast<float>().eval().data());

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render a unit cube with inward-facing normals to sample the environment map
        glBindVertexArray(cubeVAO);
        glDrawElements(GL_TRIANGLES, cubeIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    irradianceShader.unbind();

    // this unbinds the framebuffer so we can render to the screen again (?)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return irradianceMap;
}

}