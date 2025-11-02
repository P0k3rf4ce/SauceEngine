#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <chrono>

#include <iostream>

#include "launcher/optionParser.hpp"
#include "shared/Scene.hpp"

void initGLFW();
GLFWwindow* initWindow(unsigned int scr_width, unsigned int scr_height);
bool initGLAD();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// Precision should be up to a millisecond
inline double get_seconds_since_epoch();

int main(int argc, const char *argv[]) {
    const AppOptions ops(argc, argv);

    if (ops.help) {
        std::cout << "Usage: " << argv[0] << " <options> [scene_file]" << std::endl;
        std::cout << ops.getHelpMessage() << std::endl;
        exit(1);
    }

    initGLFW();
    GLFWwindow *window = initWindow(ops.scr_width, ops.scr_height);
    if (window == NULL) {
        return 1;
    }

    if (!initGLAD()) {
        std::cerr << "Failed to load GLAD" << std::endl;
        glfwTerminate();
        return 1;
    }

    Scene scene;

    double deltatime = 0.0;

    double prev_frame_time = get_seconds_since_epoch(), current_frame_time;

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        current_frame_time = get_seconds_since_epoch();
        deltatime += current_frame_time - prev_frame_time;
        prev_frame_time = current_frame_time;

        deltatime = scene.update(deltatime, DELTA_STEP);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void initGLFW() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

GLFWwindow *initWindow(unsigned int scr_width, unsigned int scr_height) {
    GLFWwindow* window = glfwCreateWindow(scr_width, scr_height, "Sauce Engine", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return NULL;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    return window;
}

bool initGLAD() {
    return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Precision should be up to a millisecond
inline double get_seconds_since_epoch() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() / 1000.0;
}