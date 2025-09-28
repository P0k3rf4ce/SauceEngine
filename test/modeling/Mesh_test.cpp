#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <modeling/Mesh.hpp>

using namespace std;

class MeshTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;

    void SetUp() override {
        // Initialize GLFW
        if (!glfwInit()) {
            FAIL() << "Failed to initialize GLFW";
        }

        // Set OpenGL version to 3.3 Core Profile
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Make window invisible for testing
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        // Create a windowed mode window and its OpenGL context
        window = glfwCreateWindow(1, 1, "Test Window", NULL, NULL);
        if (!window) {
            glfwTerminate();
            FAIL() << "Failed to create GLFW window";
        }

        // Make the window's context current
        glfwMakeContextCurrent(window);

        // Initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            glfwDestroyWindow(window);
            glfwTerminate();
            FAIL() << "Failed to initialize GLAD";
        }

        // Verify OpenGL context is working
        const GLubyte* version = glGetString(GL_VERSION);
        if (!version) {
            glfwDestroyWindow(window);
            glfwTerminate();
            FAIL() << "Failed to get OpenGL version";
        }
    }

    void TearDown() override {
        // Cleanup OpenGL context
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
};

TEST_F(MeshTest, BadConstructors) {
    vector<Vertex> vertices = {
        /*         pos                 normal                          texcoords */
        {glm::vec3(1.0f, 0.0f, 0.0f),  glm::vec3(1.0f, 0.0f, 0.0f),    glm::vec2(0.f, 1.f)},
        {glm::vec3(1.0f, 1.0f, 0.0f),  glm::vec3(1.0f, 0.0f, 0.0f),    glm::vec2(0.f, 1.f)},
    };
    vector<unsigned int> indices = {
		2, 0, 0,
	};

	/*
	 * test creating a mesh with out-of-bounds indices
	 */
	EXPECT_ANY_THROW( Mesh mesh(vertices,indices) );

	vector<unsigned int> indices2 = {
		0,
	};

	/*
	 * test creating a mesh with not enough indices
	 */
	EXPECT_ANY_THROW( Mesh mesh2(vertices,indices2) );
}

TEST_F(MeshTest, GoodConstructors) {
	vector<Vertex> unitcubev = {
			/*         pos                    normal                          texcoords */
			{glm::vec3(-1.0f,  1.0f,  0.0f),  glm::vec3(1.0f, 0.0f, 0.0f),    glm::vec2(0.f, 1.f)},
			{glm::vec3(-1.0f, -1.0f,  0.0f),  glm::vec3(1.0f, 0.0f, 0.0f),    glm::vec2(0.f, 1.f)},
			{glm::vec3( 1.0f,  1.0f,  0.0f),  glm::vec3(1.0f, 0.0f, 0.0f),    glm::vec2(0.f, 1.f)},
			{glm::vec3( 1.0f, -1.0f,  0.0f),  glm::vec3(1.0f, 0.0f, 0.0f),    glm::vec2(0.f, 1.f)},

			{glm::vec3(-1.0f,  1.0f, -1.0f),  glm::vec3(1.0f, 0.0f, 0.0f),    glm::vec2(0.f, 1.f)},
			{glm::vec3(-1.0f, -1.0f, -1.0f),  glm::vec3(1.0f, 0.0f, 0.0f),    glm::vec2(0.f, 1.f)},
			{glm::vec3( 1.0f,  1.0f,  1.0f),  glm::vec3(1.0f, 0.0f, 0.0f),    glm::vec2(0.f, 1.f)},
			{glm::vec3( 1.0f, -1.0f, -1.0f),  glm::vec3(1.0f, 0.0f, 0.0f),    glm::vec2(0.f, 1.f)},
	};

	vector<unsigned int> unitcubei = {
		0, 2, 3, 0, 3, 1,
		2, 6, 7, 2, 7, 3,
		6, 4, 5, 6, 5, 7,
		4, 0, 1, 4, 1, 5,
		0, 4, 6, 0, 6, 2,
		1, 5, 7, 1, 7, 3,
	};

	Mesh mesh3(unitcubev,unitcubei);
	EXPECT_EQ(1,1); // expect no throw
}
