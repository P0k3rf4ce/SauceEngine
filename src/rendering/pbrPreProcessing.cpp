#include "rendering/pbrPreProcessing.hpp"
#include "utils/Shader.hpp"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#include <iostream>
#include <cassert>

using namespace rendering;


/**
 * helper: inits/returns framebuffer and renderbuffer
 * @return tuple of OpenGL IDs of the framebuffer and renderbuffer
 */
static std::tuple<GLuint, GLuint>
createBuffers()
{
    const uint SIZE = 512;

    static GLuint captureFBO = 0;
    static GLuint captureRBO;
    if (captureFBO != 0) { return {captureFBO, captureRBO}; }

    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, SIZE, SIZE);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    return {captureFBO, captureRBO};
}

/**
 * helper: loads hdr environment map data
 * @param hdrEnvMap path to the HDR environment map
 * @param width pointer to store the width of the loaded image
 * @param height pointer to store the height of the loaded image
 * @param nrComponents pointer to store the number of color components in the loaded image
 * @return a tuple containing the loaded image data and the OpenGL texture ID
 * 
 * probably shouldnt get called since we assume cubemap objects exist
 */
static std::tuple<float*, GLuint>
loadHDRData(const std::string& hdrEnvMap, int* width, int* height, int* nrComponents)
{
    stbi_set_flip_vertically_on_load(true);
    float *data = stbi_loadf(hdrEnvMap.c_str(), width, height, nrComponents, 0);
    GLuint hdrTexture;
    if (!data) { std::cout << "Failed to load HDR image." << std::endl; }
    else {
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, *width, *height, 0, GL_RGB, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }

    return {data, hdrTexture};
}

/**
 * helper: setup cubemap to render to
 * @param size width/height of each cubemap face
 * @return the OpenGL ID of the cubemap texture
 */
static GLuint
setupCubemap(const int size)
{
    GLuint envCubemap;
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (unsigned int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return envCubemap;
}

/**
 * helper: setup irradiance map
 * @param size width/height of each cubemap face
 * @return the OpenGL ID of the irradiance cubemap texture
 */
static GLuint
setupIrradianceMap(const int size)
{
    GLuint irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                        size, size, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return irradianceMap;
}

/**
 * helper: setup prefilter map
 * @param size width/height of each cubemap face
 * @return the OpenGL ID of the prefilter cubemap texture
 */
static GLuint
setupPrefilterMap(const int size)
{
    GLuint prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    return prefilterMap;
}

/**
 * helper: setup brdf lut
 * @param size width/height of the lut texture
 * @return the OpenGL ID of the brdf lut texture
 */
static GLuint
setupBRDFLUT(const int size)
{
    GLuint brdfLUTTexture;
    glGenTextures(1, &brdfLUTTexture);

    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, size, size, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return brdfLUTTexture;
}

/**
 * helper: get projection/view matrices
 * @return pair of projection matrix and array of 6 view matrices
 */
static std::pair<Eigen::Affine3d, std::array<Eigen::Affine3d, 6>>
getCaptureMatrices()
{
    // sure would be nice if eigen did this for me. well whatever. go my copilot
    std::array<Eigen::Affine3d, 6> captureViews;
    Eigen::Affine3d captureProj = Eigen::Affine3d::Identity();
    float near = 0.1f;
    float far = 10.0f;
    float fov = 90.0f;
    float aspect = 1.0f;
    float f = 1.0f / tanf(fov * 0.5f * (PI / 180.0f));

    captureProj.matrix() <<
        f / aspect, 0,  0,                      0,
        0,          f,  0,                      0,
        0,          0, (far+near)/(near-far),   (2.0f*far*near)/(near-far),
        0,          0, -1,                      0;

    captureViews[0].matrix() << 
         0,  0, -1, 0,
         0, -1,  0, 0,
        -1,  0,  0, 0,
         0,  0,  0, 1;

    captureViews[1].matrix() << 
        0,  0, 1, 0,
        0, -1, 0, 0,
        1,  0, 0, 0,
        0,  0, 0, 1;

    captureViews[2].matrix() << 
        1,  0, 0, 0,
        0,  0, 1, 0,
        0, -1, 0, 0,
        0,  0, 0, 1;

    captureViews[3].matrix() << 
        1, 0,  0, 0,
        0, 0, -1, 0,
        0, 1,  0, 0,
        0, 0,  0, 1;

    captureViews[4].matrix() << 
        1,  0,  0, 0,
        0, -1,  0, 0,
        0,  0, -1, 0,
        0,  0,  0, 1;

    captureViews[5].matrix() << 
        -1,  0, 0, 0,
         0, -1, 0, 0,
         0,  0, 1, 0,
         0,  0, 0, 1;

    return {captureProj, captureViews};
}

/**
 * helper: render a cube
 */
static void
renderCube()
{
    // initialize (if necessary)
    static GLuint cubeVAO = 0;
    static GLuint cubeVBO = 0;
    static GLuint cubeEBO = 0;
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f, // bottom-left
             1.0f,  1.0f, -1.0f, // top-right
             1.0f, -1.0f, -1.0f, // bottom-right
            -1.0f,  1.0f, -1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f, // bottom-left
             1.0f, -1.0f,  1.0f, // bottom-right
             1.0f,  1.0f,  1.0f, // top-right
            -1.0f,  1.0f,  1.0f, // top-left
        };
        uint indices[] = {
            0,  1,  2,  1,  0,  3,
            4,  5,  6,  6,  7,  4,
            7,  3,  0,  0,  4,  7,
            6,  2,  1,  2,  6,  5,
            0,  2,  5,  5,  4,  0,
            3,  6,  1,  6,  3,  7
        };

        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glGenBuffers(1, &cubeEBO);

        glBindVertexArray(cubeVAO);

        // fill buffers
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // link vertex attributes
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render cube
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

/**
 * helper: render a quad
 */
static void
renderQuad()
{
    static GLuint quadVAO = 0;
    static GLuint quadVBO;

    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

/**
 * given an HDR environment map, generate a cubemap
 * @param hdrEnvMap path to the HDR environment map
 * @return the OpenGL ID of the cubemap texture
 */
GLuint
genEnvCubemap(const std::string hdrEnvMap) {

    // create shader programs
    static Shader equirectToCubemap;
    static bool initialized = false;
    if (!initialized) {
        equirectToCubemap.loadFromFiles({
            {SHADER_TYPE::VERTEX, "shaders/pbr/cubemap.vs"},
            {SHADER_TYPE::FRAGMENT, "shaders/pbr/equirect_to_cube.fs"}
        });

        initialized = true;
    }

    // create buffers
    // note we don't use the rbo
    const int SIZE = 512;
    auto [captureFBO, captureRBO] = createBuffers();
    
    // create hdr texture
    int width, height, nrComponents;
    auto [data, hdrTexture] = loadHDRData(hdrEnvMap, &width, &height, &nrComponents);

    // create cubemap
    GLuint envCubemap = setupCubemap(SIZE);

    // get capture matrices
    auto [captureProj, captureViews] = getCaptureMatrices();
    
    // set shader and convert
    equirectToCubemap.bind();
    equirectToCubemap.setUniform("equirectangularMap", 0);
    equirectToCubemap.setUniform("projection", captureProj);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, SIZE, SIZE); // configure viewport to the capture dimensions
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; i++)
    {
        // set view and render
        equirectToCubemap.setUniform("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // generate mipmaps from first face - combat visible dots artifact
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return envCubemap;
}

/**
 * given an environment cubemap, generate an irradiance map
 * @param envCubemap the OpenGL ID of the environment cubemap texture
 * @return the OpenGL ID of the irradiance cubemap texture
 */
GLuint
genIrradianceMap(const GLuint envCubemap)
{
    // using a small 32x32 cubemap as irradiance map
    const uint IRRADIANCE_SIZE = 32;

    // get framebuffer/renderbuffer
    auto [captureFBO, captureRBO] = createBuffers();

    // get capture matrices
    auto [captureProj, captureViews] = getCaptureMatrices();

    // create map
    GLuint irradianceMap = setupIrradianceMap(IRRADIANCE_SIZE);

    // compile shader
    static Shader irradianceShader;
    static bool initialized = false;
    
    if (!initialized)
    {
        assert(irradianceShader.loadFromFiles({
            {SHADER_TYPE::VERTEX, "src/rendering/shaders/irradiance.vert"},
            {SHADER_TYPE::FRAGMENT, "src/rendering/shaders/irradiance.frag"}
        }));
        initialized = true;
    }


    // prepare framebuffer for rendering each cubemap face during irradiance convolution
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glViewport(0, 0, 32, 32);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, IRRADIANCE_SIZE, IRRADIANCE_SIZE);

    // bind shader and render
    irradianceShader.bind();
    irradianceShader.setUniform("environmentMap", 0);
    irradianceShader.setUniform("projection", captureProj);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        // set the view matrix uniform in the shader
        irradianceShader.setUniform("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCube();
    }

    irradianceShader.unbind();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return irradianceMap;
}

/**
 * given an environment cubemap, generate an irradiance map
 * @param envCubemap the OpenGL ID of the environment cubemap texture
 * @return the OpenGL ID of the irradiance cubemap texture
 */
GLuint
genPrefilterMap(const GLuint envCubemap)
{
    // Load and compile the pre-filter shader program (only once)
    static Shader prefilterShader;
    static bool initialized = false;
    if (!initialized)
    {
        prefilterShader.loadFromFiles({{SHADER_TYPE::VERTEX, "shaders/pbr/cubemap.vs"},
                                       {SHADER_TYPE::FRAGMENT, "shaders/pbr/prefilter.fs"}});
        initialized = true;
    }

    // Create the destination pre-filter cubemap texture
    const uint PREFILTER_SIZE = 128;
    GLuint prefilterMap = setupPrefilterMap(PREFILTER_SIZE);

    // get framebuffer/renderbuffer
    auto [captureFBO, captureRBO] = createBuffers();
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    // get capture matrices
    auto [captureProj, captureViews] = getCaptureMatrices();

    // Configure the shader with static uniforms and bind the input environment map
    prefilterShader.bind();
    prefilterShader.setUniform("environmentMap", 0);
    prefilterShader.setUniform("projection", captureProj);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    // Render to each mip level of the pre-filter cubemap
    const unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        // Resize renderbuffer and viewport to match the current mip level's size
        unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));

        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        prefilterShader.setUniform("roughness", roughness);

        // Render each of the 6 cubemap faces for the current roughness
        for (unsigned int i = 0; i < 6; ++i)
        {
            prefilterShader.setUniform("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderCube();
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    prefilterShader.unbind();
    return prefilterMap;
}

/**
 * given an environment cubemap, generate a BRDF LUT
 * @param envCubemap the OpenGL ID of the environment cubemap texture
 * @return the OpenGL ID of the BRDF lookup texture
 */
GLuint
genBRDFLUT(const GLuint envCubemap)
{
    // create BRDF LUT texture
    const uint LUT_SIZE = 512;
    GLuint brdfLUTTexture = setupBRDFLUT(LUT_SIZE);

    // get framebuffer/renderbuffer
    auto [captureFBO, captureRBO] = createBuffers();

    // compile shader
    static Shader brdfShader;
    static bool initialized = false;

    if (!initialized)
    {
        assert(brdfShader.loadFromFiles({
            {SHADER_TYPE::VERTEX, "src/rendering/shaders/pbr/brdf.vert"},
            {SHADER_TYPE::FRAGMENT, "src/rendering/shaders/pbr/brdf.frag"}
        }));
        initialized = true;
    }

    // render
    glViewport(0, 0, 512, 512);
    brdfShader.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad();

    brdfShader.unbind();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return brdfLUTTexture;
}