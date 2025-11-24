#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <modeling/ModelLoader.hpp>

using namespace std;

class MeshLoadTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;

    void SetUp() override {
		/* setup logger since it's helpful */
        Logger& logger = Logger::getInstance();
        logger.setLogLevel(LogLevel::DEBUG);
        logger.enableColor(true);

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

TEST_F(MeshLoadTest, UnitCube) {
	using namespace modeling;
	auto shaderp = make_shared<Shader>();

	/* test loading the unit cube as exported from blender */
	auto models = ModelLoader::loadModels("test/assets/unitcube.gltf",shaderp);

	vector<Vertex> v = {
		/* vertex                  normal                     UV */
		{ glm::vec3( 1.f, 1.f,-1), glm::vec3( 0.f, 1.f, 0.f), glm::vec2(0.625f, 0.5f)  },
		{ glm::vec3(-1.f, 1.f,-1), glm::vec3( 0.f, 1.f, 0.f), glm::vec2(0.875f, 0.5f)  },
		{ glm::vec3(-1.f, 1.f, 1), glm::vec3( 0.f, 1.f, 0.f), glm::vec2(0.875f, 0.25f) },
		{ glm::vec3( 1.f, 1.f, 1), glm::vec3( 0.f, 1.f, 0.f), glm::vec2(0.625f, 0.25f) },
		{ glm::vec3( 1.f,-1.f, 1), glm::vec3( 0.f, 0.f, 1.f), glm::vec2(0.375f, 0.25f) },
		{ glm::vec3( 1.f, 1.f, 1), glm::vec3( 0.f, 0.f, 1.f), glm::vec2(0.625f, 0.25f) },
		{ glm::vec3(-1.f, 1.f, 1), glm::vec3( 0.f, 0.f, 1.f), glm::vec2(0.625f, 0.f)   },
		{ glm::vec3(-1.f,-1.f, 1), glm::vec3( 0.f, 0.f, 1.f), glm::vec2(0.375f, 0.f)   },
		{ glm::vec3(-1.f,-1.f, 1), glm::vec3(-1.f, 0.f, 0.f), glm::vec2(0.375f, 1.f)   },
		{ glm::vec3(-1.f, 1.f, 1), glm::vec3(-1.f, 0.f, 0.f), glm::vec2(0.625f, 1.f)   },
		{ glm::vec3(-1.f, 1.f,-1), glm::vec3(-1.f, 0.f, 0.f), glm::vec2(0.625f, 0.75f) },
		{ glm::vec3(-1.f,-1.f,-1), glm::vec3(-1.f, 0.f, 0.f), glm::vec2(0.375f, 0.75f) },
		{ glm::vec3(-1.f,-1.f,-1), glm::vec3( 0.f,-1.f, 0.f), glm::vec2(0.125f, 0.5f)  },
		{ glm::vec3( 1.f,-1.f,-1), glm::vec3( 0.f,-1.f, 0.f), glm::vec2(0.375f, 0.5f)  },
		{ glm::vec3( 1.f,-1.f, 1), glm::vec3( 0.f,-1.f, 0.f), glm::vec2(0.375f, 0.25f) },
		{ glm::vec3(-1.f,-1.f, 1), glm::vec3( 0.f,-1.f, 0.f), glm::vec2(0.125f, 0.25f) },
		{ glm::vec3( 1.f,-1.f,-1), glm::vec3( 1.f, 0.f, 0.f), glm::vec2(0.375f, 0.5f)  },
		{ glm::vec3( 1.f, 1.f,-1), glm::vec3( 1.f, 0.f, 0.f), glm::vec2(0.625f, 0.5f)  },
		{ glm::vec3( 1.f, 1.f, 1), glm::vec3( 1.f, 0.f, 0.f), glm::vec2(0.625f, 0.25f) },
		{ glm::vec3( 1.f,-1.f, 1), glm::vec3( 1.f, 0.f, 0.f), glm::vec2(0.375f, 0.25f) },
		{ glm::vec3(-1.f,-1.f,-1), glm::vec3( 0.f, 0.f,-1.f), glm::vec2(0.375f, 0.75f) },
		{ glm::vec3(-1.f, 1.f,-1), glm::vec3( 0.f, 0.f,-1.f), glm::vec2(0.625f, 0.75f) },
		{ glm::vec3( 1.f, 1.f,-1), glm::vec3( 0.f, 0.f,-1.f), glm::vec2(0.625f, 0.5f)  },
		{ glm::vec3( 1.f,-1.f,-1), glm::vec3( 0.f, 0.f,-1.f), glm::vec2(0.375f, 0.5f)  },
	};

	vector<unsigned int> ind = {
		0,1,2,
		0,2,3,
		4,5,6,
		4,6,7,
		8,9,10,
		8,10,11,
		12,13,14,
		12,14,15,
		16,17,18,
		16,18,19,
		20,21,22,
		20,22,23,
	};

	for (int i=0; i<v.size(); i++) {
		EXPECT_EQ(models[0]->getMeshes()[0]->vertices[i].Position,v[i].Position);
		EXPECT_EQ(models[0]->getMeshes()[0]->vertices[i].Normal,v[i].Normal);
		EXPECT_EQ(models[0]->getMeshes()[0]->vertices[i].TexCoords,v[i].TexCoords);
	}
	for (int i=0; i<ind.size(); i++) {
		EXPECT_EQ(models[0]->getMeshes()[0]->indices[i],ind[i]);
	}
}
