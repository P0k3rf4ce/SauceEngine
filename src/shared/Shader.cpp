#include "shared/Shader.hpp"
#include "shared/Logger.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

// Converts our SHADER TYPE to GL name
GLenum Shader::getGLShaderType(SHADER_TYPE type) {
    switch (type) {
        case VERTEX: return GL_VERTEX_SHADER;
        case FRAGMENT: return GL_FRAGMENT_SHADER;
        case GEOMETRY: return GL_GEOMETRY_SHADER;
        case COMPUTE: return GL_COMPUTE_SHADER;
    }
    return -1; // Invalid type
}

/*
    Compiles a shader of the given type from the provided source shader.
    Will do full error checking for you.
*/
bool Shader::compileShader(GLuint& shader, SHADER_TYPE shaderType, const std::string& source) {
    GLenum glType = getGLShaderType(shaderType);
    if (glType == 0) return false;
    
    shader = glCreateShader(glType);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    
    // Check for compilation errors
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        LOG_ERROR("Shader compilation failed: " + std::string(infoLog));
        glDeleteShader(shader);
        return false;
    }
    
    return true;
}

// Helper method for linking shaders but I guess you could use this standalone if you wanted
void Shader::checkCompileErrors(GLuint shader, const std::string& type) {
    GLint success;
    GLchar infoLog[1024];
    
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            LOG_ERROR_F("%s shader compilation failed: %s", type.c_str(), infoLog);
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            LOG_ERROR_F("Program linking failed: %s", infoLog);
        }
    }
}

// Attach new type of shader to this program
// See SHADER_TYPE in /include/shared/Shader.hpp
bool Shader::addShader(SHADER_TYPE shaderType, const std::string& source) {
    // Check if this shader type already exists
    if (shaderMap.find(shaderType) != shaderMap.end()) {
        LOG_WARN("Shader type already exists. Use replaceShader() to replace it.");
        return false;
    }
    
    GLuint shader;
    if (compileShader(shader, shaderType, source)) {
        shaders.push_back(shader);
        shaderMap[shaderType] = shader;
                LOG_DEBUG("Shader type added successfully");
        return true;
    }
    return false;
}

/*
    If you want to on the fly replace a shader src you can do it here.
*/
bool Shader::replaceShader(SHADER_TYPE shaderType, const std::string& source) {
    // Remove existing shader of this type if it exists
    removeShader(shaderType);
    
    // Add the new shader
    GLuint shader;
    if (compileShader(shader, shaderType, source)) {
        shaders.push_back(shader);
        shaderMap[shaderType] = shader;
        LOG_DEBUG_F("Shader type %d replaced successfully", shaderType);
        // Clear uniform cache since the program will need to be relinked
        uniformCache.clear();
        
        return true;
    }
    return false;
}

/*
    De-attaches given shader type from its program.
*/
bool Shader::removeShader(SHADER_TYPE shaderType) {
    auto it = shaderMap.find(shaderType);
    if (it != shaderMap.end()) {
        GLuint shaderToRemove = it->second;
        
        // Remove from shaders vector
        auto vectorIt = std::find(shaders.begin(), shaders.end(), shaderToRemove);
        if (vectorIt != shaders.end()) {
            shaders.erase(vectorIt);
        }
        
        // Delete the OpenGL shader object
        glDeleteShader(shaderToRemove);
        
        // Remove from map
        shaderMap.erase(it);
        
        LOG_DEBUG_F("Shader type %d removed successfully", shaderType);

        // Clear uniform cache since the program will need to be relinked
        uniformCache.clear();
        
        return true;
    }

    LOG_WARN_F("Shader type %d not found", shaderType);
    return false;
}

/*
    Links your attached input shaders into a program - this is usually done automatically after adding sources but you can
    also do it manually.
*/
bool Shader::linkProgram() {
    if (shaders.empty()) {
        LOG_ERROR("No shaders to link");
        return false;
    }
    
    shaderProgram = glCreateProgram();
    
    // Attach all shaders
    for (GLuint shader : shaders) {
        glAttachShader(shaderProgram, shader);
    }
    
    glLinkProgram(shaderProgram);
    checkCompileErrors(shaderProgram, "PROGRAM");
    
    // Check linking success
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        return false;
    }
    
    // Detach shaders after linking
    for (GLuint shader : shaders) {
        glDetachShader(shaderProgram, shader);
    }
    
    return true;
}

/*
 * Load shaders from files.
 * Will compile and link if all successfully compiled.
 * Bool return denotes compile and link success.
 * 
 * Example:
 * std::unordered_map<SHADER_TYPE, std::string> shaderFiles = {
 *        {VERTEX, "../assets/default.vert"},
 * };
 */
bool Shader::loadFromFiles(const std::unordered_map<SHADER_TYPE, std::string>& shaderFiles) {
    std::unordered_map<SHADER_TYPE, std::string> sources;
    
    for (const auto& pair : shaderFiles) {
        std::ifstream file(pair.second);
        if (!file.is_open()) {
            LOG_ERROR_F("Failed to open shader file: %s", pair.second.c_str());
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        sources[pair.first] = buffer.str();
    }
    
    return loadFromSources(sources);
}

/**
 * Load shader from raw string input sources.
 * Will compile and link if all successfully compiled.
 * Bool return denotes compile and link success.
 */
bool Shader::loadFromSources(const std::unordered_map<SHADER_TYPE, std::string>& shaderSources) {
    // Clear existing shaders
    for (GLuint shader : shaders) {
        glDeleteShader(shader);
    }
    shaders.clear();
    shaderMap.clear();
    
    // Add all provided shaders
    for (const auto& pair : shaderSources) {
        GLuint shader;
        if (compileShader(shader, pair.first, pair.second)) {
            shaders.push_back(shader);
            shaderMap[pair.first] = shader;
        } else {
            return false;
        }
    }
    
    return linkProgram();
}

/*
    Helper to get ID of uniform by name
    Returns -1 if not found.
*/
GLint Shader::getUniformLocation(const std::string& name) {
    auto it = uniformCache.find(name);
    if (it != uniformCache.end()) {
        return it->second;
    }
    
    GLint location = glGetUniformLocation(shaderProgram, name.c_str());
    uniformCache[name] = location;
    
    if (location == -1) {
        LOG_WARN_F("Uniform '%s' not found for shader program %d", name.c_str(), shaderProgram);
    }
    
    return location;
}

/*
    When we set uniforms it applies to the active program.

    This will do essentially a quick swap out to set the uniform then switch
    back to the originally active shader.

    If we render on a separate thread this could cause issues.

    If this is too slow we can in the future just trust that the programmer
    knows what they're doing. ( ͡° ͜ʖ ͡°)
*/
void Shader::ensureShaderActive(std::function<void()> uniformSetter) {
    if (shaderProgram == 0) {
        LOG_WARN("Cannot set uniform - no shader program created");
        return;
    }
    
    // Store current shader program
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    
    // Temporarily use this shader if it's not active
    bool needsRestore = (currentProgram != static_cast<GLint>(shaderProgram));
    if (needsRestore) {
        glUseProgram(shaderProgram);
    }
    
    // Execute the uniform setter
    uniformSetter();
    
    // Restore previous shader program
    if (needsRestore) {
        glUseProgram(currentProgram);
    }
}

/* Sets float uniform */
void Shader::setUniform(const std::string& name, float value) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        ensureShaderActive([&]() {
            glUniform1f(location, value);
        });
    }
}

/* Sets int uniform */
void Shader::setUniform(const std::string& name, int value) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        ensureShaderActive([&]() {
            glUniform1i(location, value);
        });
    }
}

/* Set bool uniform */
void Shader::setUniform(const std::string& name, bool value) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        ensureShaderActive([&]() {
            glUniform1i(location, value ? 1 : 0);
        });
    }
}

/* Vec3f uniform */
void Shader::setUniform(const std::string& name, float x, float y, float z) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        ensureShaderActive([&]() {
            glUniform3f(location, x, y, z);
        });
    }
}

/* Vec4f uniform */
void Shader::setUniform(const std::string& name, float x, float y, float z, float w) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        ensureShaderActive([&]() {
            glUniform4f(location, x, y, z, w);
        });
    }
}




/* Mat4f uniform */
void Shader::setUniform(const std::string& name, const Eigen::Matrix4f& mat4) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        ensureShaderActive([&]() {
            glUniformMatrix4fv(location, 1, GL_FALSE, mat4.data());
        });
    }
}


/*
 * Actually use the shader
 * Must be called before you render!
 */
void Shader::bind() {
    if (shaderProgram == 0) {
        LOG_ERROR("Error: Cannot bind shader - no program created");
        return;
    }
    
    if (!bound) {
        glUseProgram(shaderProgram);
        bound = true;
        LOG_DEBUG("Shader bound and activated successfully");
    } else {
        LOG_WARN_F("Shader %d is already unbound", shaderProgram);
    }
}

void Shader::unbind() {
    if (bound) {
        glUseProgram(0);
        bound = false;
        LOG_DEBUG_F("Shader %d unbound successfully", shaderProgram);
    } else {
        LOG_WARN_F("Shader %d is already unbound", shaderProgram);
    }
}
