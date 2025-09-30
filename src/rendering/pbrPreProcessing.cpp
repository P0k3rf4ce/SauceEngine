#include "rendering/pbrPreProcessing.hpp"
#include "utils/Shader.hpp"

#include <glad/glad.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#include <iostream>

using namespace rendering;


// helper: creates framebuffer and renderbuffer
static std::tuple<GLuint, GLuint> createBuffers(const int size)
{
    GLuint captureFBO;
    GLuint captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    return {captureFBO, captureRBO};
}

// helper: loads hdr environment map data
static std::tuple<float*, GLuint> loadHDRData(const std::string& hdrEnvMap, int* width, int* height, int* nrComponents)
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
    }

    stbi_image_free(data);
    return {data, hdrTexture};
}

// helper: setup cubemap to render to
static GLuint setupCubemap(const int size)
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

// helper: get projection/view matrices
static std::pair<Eigen::Affine3d, std::array<Eigen::Affine3d, 6>> getCaptureMatrices()
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

// helper: init pbr shader and set static uniforms
// assumption: this is called in irradiance map generation
static void initPBRShader(Shader& pbrShader)
{
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    pbrShader.loadFromFiles({
        {SHADER_TYPE::VERTEX, "shaders/pbr/pbr.vs"},
        {SHADER_TYPE::FRAGMENT, "shaders/pbr/pbr.fs"}
    });
    pbrShader.bind();
    pbrShader.setUniform("irradianceMap", 0);
    pbrShader.setUniform("prefilterMap", 1);
    pbrShader.setUniform("brdfLUT", 2);
    pbrShader.setUniform("albedo", 3);
    pbrShader.setUniform("normalMap", 4);
    pbrShader.setUniform("metallicMap", 5);
    pbrShader.setUniform("roughnessMap", 6);
    pbrShader.setUniform("aoMap", 7);
    pbrShader.unbind();
}

// helper: render a cube
static void renderCube()
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
 * generates an environment cubemap from an equirectangular HDR image
 * returns the OpenGL ID of the cubemap texture
 */
GLuint genEnvCubemap(const std::string hdrEnvMap) {

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
    int size = 512;
    auto [captureFBO, captureRBO] = createBuffers(size);
    
    // create hdr texture
    int width, height, nrComponents;
    auto [data, hdrTexture] = loadHDRData(hdrEnvMap, &width, &height, &nrComponents);

    // create cubemap
    GLuint envCubemap = setupCubemap(size);

    // get capture matrices
    auto [captureProj, captureViews] = getCaptureMatrices();
    
    // actual conversion
    equirectToCubemap.bind();
    equirectToCubemap.setUniform("equirectangularMap", 0);
    equirectToCubemap.setUniform("projection", captureProj);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, size, size); // configure viewport to the capture dimensions
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

uint genPrefilterMap(uint captureFBO, uint captureRBO, const Eigen::Matrix4f &captureProj, const std::array<Eigen::Matrix4f, 6> &captureViews)
{
    int size = 512;
    uint envCubemap = setupCubemap(size);
    // 1. Load and compile the pre-filter shader program (only once)
    static Shader pbrShader;
    static bool initialized = false;
    if (!initialized)
    {
        pbrShader.loadFromFiles({{SHADER_TYPE::VERTEX, "shaders/pbr/cubemap.vs"},
                                       {SHADER_TYPE::FRAGMENT, "shaders/pbr/prefilter.fs"}});
        initialized = true;
    }

    // 2. Create the destination pre-filter cubemap texture
    uint prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // 3. Configure the shader with static uniforms and bind the input environment map
    pbrShader.bind();
    pbrShader.setUniform("environmentMap", 0);
    pbrShader.setUniform("projection", captureProj);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    // 4. Bind the provided framebuffer to render offscreen
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    // 5. Render to each mip level of the pre-filter cubemap
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
        pbrShader.setUniform("roughness", roughness);

        // Render each of the 6 cubemap faces for the current roughness
        for (unsigned int i = 0; i < 6; ++i)
        {
            pbrShader.setUniform("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            // renderCube();
        }
    }

    // 6. Restore the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    pbrShader.unbind();

    return prefilterMap;
}
