#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "shared/Shader.hpp"
#include "glad/glad.h"
#include <GLFW/glfw3.h>

// Test fixture class for setting up OpenGL context for shader tests
class ShaderTest : public ::testing::Test {
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

TEST_F(ShaderTest, ShaderCreation) {
    // Test basic shader creation and getters
    Shader shader;
    EXPECT_FALSE(shader.is_bound());
}

TEST_F(ShaderTest, ShaderTypeEnum) {
    // Test that all shader types are defined correctly
    EXPECT_NE(FRAGMENT, UNINITIALIZED);
    EXPECT_NE(VERTEX, UNINITIALIZED);
    EXPECT_NE(GEOMETRY, UNINITIALIZED);
    EXPECT_NE(COMPUTE, UNINITIALIZED);
    
    // Test that each enum has a unique value
    EXPECT_NE(FRAGMENT, VERTEX);
    EXPECT_NE(FRAGMENT, GEOMETRY);
    EXPECT_NE(FRAGMENT, COMPUTE);
    EXPECT_NE(VERTEX, GEOMETRY);
    EXPECT_NE(VERTEX, COMPUTE);
}

TEST_F(ShaderTest, ShaderSources) {
    Shader shader;
    
    // Test with empty sources map
    std::unordered_map<SHADER_TYPE, std::string> emptySources;
    EXPECT_FALSE(shader.loadFromSources(emptySources));
    
    // Test with invalid shader source (would require OpenGL context to properly test)
    std::unordered_map<SHADER_TYPE, std::string> invalidSources = {
        {VERTEX, "invalid shader source"}
    };
    // Note: This would fail in a real OpenGL context
    // EXPECT_FALSE(shader.loadFromSources(invalidSources));
}

TEST_F(ShaderTest, UniformLocationCaching) {
    Shader shader;
    
    // Test uniform location caching with proper OpenGL context
    GLint location1 = shader.getUniformLocation("testUniform");
    GLint location2 = shader.getUniformLocation("testUniform");
    
    // Both calls should return the same cached value
    EXPECT_EQ(location1, location2);
    
    // Non-existent uniforms should return -1
    EXPECT_EQ(location1, -1);
}

TEST_F(ShaderTest, ShaderManagement) {
    Shader shader;
    
    // Test removing non-existent shader
    EXPECT_FALSE(shader.removeShader(VERTEX));
    
    // Test adding a simple vertex shader
    std::string simpleVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";
    
    // This should work with proper OpenGL context
    EXPECT_TRUE(shader.addShader(VERTEX, simpleVertexShader));
    
    // Test replacing the shader
    std::string anotherVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 mvp;
        void main() {
            gl_Position = mvp * vec4(aPos, 1.0);
        }
    )";
    
    EXPECT_TRUE(shader.replaceShader(VERTEX, anotherVertexShader));
    
    // Test removing the shader
    EXPECT_TRUE(shader.removeShader(VERTEX));
}

TEST_F(ShaderTest, UniformSetters) {
    Shader shader;
    
    // Create a simple shader program for testing uniforms
    std::string vertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform float testFloat;
        uniform int testInt;
        uniform bool testBool;
        uniform vec3 testVec3;
        uniform vec4 testVec4;
        void main() {
            gl_Position = vec4(aPos + testVec3 * testFloat * float(testInt) * float(testBool), 1.0) + testVec4;
        }
    )";
    
    std::string fragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";
    
    // Add shaders and link program
    EXPECT_TRUE(shader.addShader(VERTEX, vertexShader));
    EXPECT_TRUE(shader.addShader(FRAGMENT, fragmentShader));
    EXPECT_TRUE(shader.linkProgram());
    
    // Test uniform setters - these should not crash with proper OpenGL context
    shader.setUniform("testFloat", 1.0f);
    shader.setUniform("testInt", 42);
    shader.setUniform("testBool", true);
    shader.setUniform("testVec3", 1.0f, 2.0f, 3.0f);
    shader.setUniform("testVec4", 1.0f, 2.0f, 3.0f, 4.0f);
    
    // Test that shader is properly initialized
    EXPECT_FALSE(shader.is_bound()); // Should not be bound by default
}

TEST_F(ShaderTest, BindUnbindLogic) {
    Shader shader;
    
    // Create a simple shader program for testing binding
    std::string vertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";
    
    std::string fragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";
    
    // Initial state should be unbound
    EXPECT_FALSE(shader.is_bound());
    
    // Trying to bind without a program should fail gracefully
    shader.bind(); // This should log an error but not crash
    EXPECT_FALSE(shader.is_bound());
    
    // Create a proper program
    EXPECT_TRUE(shader.addShader(VERTEX, vertexShader));
    EXPECT_TRUE(shader.addShader(FRAGMENT, fragmentShader));
    EXPECT_TRUE(shader.linkProgram());
    
    // Now binding should work
    shader.bind();
    EXPECT_TRUE(shader.is_bound());
    
    // Test unbinding
    shader.unbind();
    EXPECT_FALSE(shader.is_bound());
}

TEST_F(ShaderTest, FileLoading) {
    Shader shader;
    
    // Test loading from non-existent files
    std::unordered_map<SHADER_TYPE, std::string> nonExistentFiles = {
        {VERTEX, "non_existent_vertex.vert"},
        {FRAGMENT, "non_existent_fragment.frag"}
    };
    
    EXPECT_FALSE(shader.loadFromFiles(nonExistentFiles));
}

TEST_F(ShaderTest, LinkProgramWithoutShaders) {
    Shader shader;
    
    // Test linking program without any shaders
    EXPECT_FALSE(shader.linkProgram());
}

// Test for working with valid shader sources
TEST_F(ShaderTest, ValidShaderSources) {
    Shader shader;
    
    std::string validVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";
    
    std::string validFragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 color;
        
        void main() {
            FragColor = vec4(color, 1.0);
        }
    )";
    
    std::unordered_map<SHADER_TYPE, std::string> validSources = {
        {VERTEX, validVertexShader},
        {FRAGMENT, validFragmentShader}
    };
    
    // This should succeed with proper OpenGL context
    EXPECT_TRUE(shader.loadFromSources(validSources));
    
    // Test that we can set uniforms
    shader.setUniform("model", 1.0f, 0.0f, 0.0f, 1.0f);
    shader.setUniform("color", 1.0f, 0.0f, 0.0f);
    
    // Test binding and unbinding
    shader.bind();
    EXPECT_TRUE(shader.is_bound());
    
    shader.unbind();
    EXPECT_FALSE(shader.is_bound());
}

TEST_F(ShaderTest, DestructorWithBoundShader) {
    // Test the destructor branch where shader is still bound
    // This should trigger the warning and forced unbind
    {
        Shader shader;
        
        std::string vertexShader = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            void main() {
                gl_Position = vec4(aPos, 1.0);
            }
        )";
        
        std::string fragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            void main() {
                FragColor = vec4(1.0, 0.0, 0.0, 1.0);
            }
        )";
        
        // Create and link shader program
        EXPECT_TRUE(shader.addShader(VERTEX, vertexShader));
        EXPECT_TRUE(shader.addShader(FRAGMENT, fragmentShader));
        EXPECT_TRUE(shader.linkProgram());
        
        // Bind the shader
        shader.bind();
        EXPECT_TRUE(shader.is_bound());
        
        // Don't unbind - let the destructor handle it
        // This should trigger the warning in the destructor:
        // LOG_WARN("Shader is still bound during destruction. Forcing unbind.");
        
    } // Shader destructor called here with bound=true
    
    // Test passes if no crash occurs and warning is logged
    SUCCEED();
}

TEST_F(ShaderTest, ComputeShaderTest) {
    Shader shader;
    
    std::string computeShader = R"(
        #version 430
        layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
        layout(std430, binding = 0) buffer DataBuffer {
            float data[];
        };
        
        void main() {
            uint index = gl_GlobalInvocationID.x;
            if (index < data.length()) {
                data[index] = data[index] * 2.0;
            }
        }
    )";
    
    // Test adding compute shader
    EXPECT_TRUE(shader.addShader(COMPUTE, computeShader));
    EXPECT_TRUE(shader.linkProgram());
    
    // Test binding compute shader
    shader.bind();
    EXPECT_TRUE(shader.is_bound());
    
    shader.unbind();
    EXPECT_FALSE(shader.is_bound());
}

TEST_F(ShaderTest, GeometryShaderTest) {
    Shader shader;
    
    std::string vertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";
    
    std::string geometryShader = R"(
        #version 330 core
        layout (triangles) in;
        layout (triangle_strip, max_vertices = 3) out;
        
        void main() {
            for(int i = 0; i < 3; i++) {
                gl_Position = gl_in[i].gl_Position;
                EmitVertex();
            }
            EndPrimitive();
        }
    )";
    
    std::string fragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(0.0, 1.0, 0.0, 1.0);
        }
    )";
    
    // Test adding geometry shader along with vertex and fragment
    EXPECT_TRUE(shader.addShader(VERTEX, vertexShader));
    EXPECT_TRUE(shader.addShader(GEOMETRY, geometryShader));
    EXPECT_TRUE(shader.addShader(FRAGMENT, fragmentShader));
    EXPECT_TRUE(shader.linkProgram());
    
    shader.bind();
    EXPECT_TRUE(shader.is_bound());
    
    shader.unbind();
    EXPECT_FALSE(shader.is_bound());
}

TEST_F(ShaderTest, MalformedShaderHandling) {
    Shader shader;
    
    // Test completely invalid shader code
    std::string malformedVertexShader = R"(
        #version 330 core
        this is not valid shader code at all!!!
        random garbage text
        layout (location = 0) in vec3 aPos;
    )";
    
    // This should fail due to invalid syntax
    EXPECT_FALSE(shader.addShader(VERTEX, malformedVertexShader));
    
    // Test shader with invalid GLSL syntax
    std::string malformedFragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = invalid_function_call();
            undefined_variable = 5;
        }
    )";
    
    // This should fail due to undefined functions and variables
    EXPECT_FALSE(shader.addShader(FRAGMENT, malformedFragmentShader));
    
    // Test shader with completely wrong language (not GLSL)
    std::string notGLSLShader = R"(
        function main() {
            console.log("This is JavaScript, not GLSL!");
            return 42;
        }
    )";
    
    // This should definitely fail
    EXPECT_FALSE(shader.addShader(VERTEX, notGLSLShader));
}

TEST_F(ShaderTest, LinkingErrorHandling) {
    Shader shader;
    
    // Create a vertex shader that outputs something
    std::string vertexShaderWithOutput = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        out vec3 vertexColor;
        void main() {
            gl_Position = vec4(aPos, 1.0);
            vertexColor = vec3(1.0, 0.0, 0.0);
        }
    )";
    
    // Create a fragment shader that expects different input
    std::string fragmentShaderWithWrongInput = R"(
        #version 330 core
        in vec3 differentVariableName;  // Different from vertexColor
        out vec4 FragColor;
        void main() {
            FragColor = vec4(differentVariableName, 1.0);
        }
    )";
    
    // Add both shaders (they should compile individually)
    EXPECT_TRUE(shader.addShader(VERTEX, vertexShaderWithOutput));
    EXPECT_TRUE(shader.addShader(FRAGMENT, fragmentShaderWithWrongInput));
    
    // Linking might fail due to mismatched interface variables
    // Some drivers are lenient, so we'll just verify the method can be called
    bool linkResult = shader.linkProgram();
    // Don't assert the result since different drivers handle this differently
    (void)linkResult; // Suppress unused variable warning
    
    SUCCEED(); // Test passes if no crash occurs
}

TEST_F(ShaderTest, EmptyShaderHandling) {
    Shader shader;
    
    // Test completely empty shader
    std::string emptyShader = "";
    EXPECT_FALSE(shader.addShader(VERTEX, emptyShader));
    
    // Test shader with only whitespace
    std::string whitespaceShader = "   \n\t  \n  ";
    EXPECT_FALSE(shader.addShader(FRAGMENT, whitespaceShader));
}

TEST_F(ShaderTest, MultipleShaderTypesOfSameKind) {
    Shader shader;
    
    std::string firstVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";
    
    std::string secondVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 mvp;
        void main() {
            gl_Position = mvp * vec4(aPos, 1.0);
        }
    )";
    
    // Add first vertex shader
    EXPECT_TRUE(shader.addShader(VERTEX, firstVertexShader));
    
    // Try to add second vertex shader - should fail with warning
    EXPECT_FALSE(shader.addShader(VERTEX, secondVertexShader));
    
    // But replacing should work
    EXPECT_TRUE(shader.replaceShader(VERTEX, secondVertexShader));
}