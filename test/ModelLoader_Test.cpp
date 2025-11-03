#include <gtest/gtest.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "modeling/ModelLoader.hpp"

#include <fstream>
#include <string>

struct GLContextFixture : public ::testing::Test {
    GLFWwindow* window = nullptr;

    void SetUp() override {
        ASSERT_TRUE(glfwInit()) << "Failed to initialize GLFW";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        window = glfwCreateWindow(1, 1, "hidden", nullptr, nullptr);
        ASSERT_NE(window, nullptr) << "Failed to create GLFW window";
        glfwMakeContextCurrent(window);
        ASSERT_TRUE(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) << "Failed to init GLAD";
        ASSERT_NE(glGetString(GL_VERSION), nullptr) << "Invalid GL context";
    }

    void TearDown() override {
        if (window) glfwDestroyWindow(window);
        glfwTerminate();
    }
};

static std::string writeTinyObjToTemp() {
    // A tiny valid OBJ: 3 verts, 3 UVs, 1 normal, 1 triangle
    static const char* kObj =
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "vt 0 0\n"
        "vt 1 0\n"
        "vt 0 1\n"
        "vn 0 0 1\n"
        "f 1/1/1 2/2/1 3/3/1\n";

    std::string path = "/tmp/tiny_triangle.obj"; // ok on WSL
    std::ofstream f(path, std::ios::trunc);
    if (!f) return {};
    f << kObj;
    return path;
}

TEST_F(GLContextFixture, LoadsTinyOBJ_ProducesAtLeastOneMesh) {
    const std::string objPath = writeTinyObjToTemp();
    ASSERT_FALSE(objPath.empty()) << "Failed to create temp OBJ";

    modeling::ModelLoader::load(objPath);

    EXPECT_GT(modeling::ModelLoader::debug_mesh_count(), 0u)
        << "Loader did not produce any meshes from tiny OBJ";
}

TEST_F(GLContextFixture, InvalidPath_DoesNotCrash) {
    modeling::ModelLoader::load("/tmp/does_not_exist.obj");
    // We only assert it didn't crash; mesh count may be 0.
    SUCCEED();
}
