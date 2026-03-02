#version 330 core

// Input from vertex shader
in vec3 FragPos;       // Fragment position in world space
in vec3 Normal;        // Fragment normal in world space
in vec2 TexCoord;      // Texture coordinates

// Output color
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
