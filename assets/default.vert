#version 330 core

// Input vertex attributes
layout (location = 0) in vec3 aPos;        // Vertex position
layout (location = 1) in vec3 aNormal;     // Vertex normal
layout (location = 2) in vec2 aTexCoord;   // Texture coordinates

// Uniform matrices
uniform mat4 model;         // Model transformation matrix
uniform mat4 view;          // View transformation matrix  
uniform mat4 projection;    // Projection transformation matrix

// Output variables to fragment shader
out vec3 FragPos;       // Fragment position in world space
out vec3 Normal;        // Fragment normal in world space
out vec2 TexCoord;      // Texture coordinates

void main()
{
    // Transform vertex position to world space
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Transform normal to world space (assuming no non-uniform scaling)
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // Pass through texture coordinates
    TexCoord = aTexCoord;
    
    // Transform vertex to clip space
    gl_Position = projection * view * vec4(FragPos, 1.0);
}