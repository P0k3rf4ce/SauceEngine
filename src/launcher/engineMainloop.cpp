#define STB_IMAGE_IMPLEMENTATION

#include "launcher/engineMainloop.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <chrono>
#include <memory>

#include <iostream>

#include "shared/Scene.hpp"
//#include "utils/Logger.hpp"

static bool firstMouse = true;
static float lastX = 0.f;
static float lastY = 0.f;

static std::shared_ptr<Scene> scene = nullptr;

int engine_mainloop(const AppOptions &ops) {

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

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    lastX = float(ops.scr_width) / 2.0f;
    lastY = float(ops.scr_height) / 2.0f;

	// turn on depth testing (why wasnt this on before)
	// for some rsn rendering breaks if this is on
	//glEnable(GL_DEPTH_TEST);

    // Create scene - load from file if provided, otherwise create empty scene
    scene = std::make_shared<Scene>();
    if (!ops.scene_file.empty()) {
        std::cout << "Loading scene from file: " << ops.scene_file << std::endl;
        std::string sceneFilePath = ops.scene_file;
        *scene = Scene(sceneFilePath);
    } else {
        std::cout << "No scene file provided, creating empty scene" << std::endl;
    }

    glfwSetCursorPosCallback(window, mouse_callback);

    // Set this scene as the active scene
    Scene::set_active_scene(scene);

    double prev_frame_time = get_seconds_since_epoch(), current_frame_time, deltatime = 0.0;

    const double delta_step = 1.0/ops.tickrate;

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)){
        processInput(window, deltatime, scene->get_camera());

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        current_frame_time = get_seconds_since_epoch();
        deltatime += current_frame_time - prev_frame_time;
        prev_frame_time = current_frame_time;

        deltatime = scene->update(deltatime, delta_step);
        
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

    Scene::scr_width = scr_width;
    Scene::scr_height = scr_height;

    return window;
}

bool initGLAD() {
    return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window, double deltatime, std::shared_ptr<Camera> camera)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->processKeyboard(Camera::Movement::FORWARD, deltatime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->processKeyboard(Camera::Movement::BACKWARD, deltatime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->processKeyboard(Camera::Movement::LEFT, deltatime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->processKeyboard(Camera::Movement::RIGHT, deltatime);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    scene->get_camera()->processMouseMovement(xoffset, yoffset);
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
