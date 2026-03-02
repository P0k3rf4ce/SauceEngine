#version 330 core

// Input from vertex shader
in vec3 FragPos;       // Fragment position in world space
in vec3 Normal;        // Fragment normal in world space
in vec2 TexCoord;      // Texture coordinates

// Output color
out vec4 FragColor;

// Uniform textures (optional for now)
uniform sampler2D texture_diffuse1;
uniform bool useTexture = false;

void main()
{
    // Simple lighting calculation using normals
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3)); // Fixed light direction
    vec3 norm = normalize(Normal);

    // Diffuse lighting
    float diff = max(dot(norm, lightDir), 0.0);

    // Base color (white or from texture)
    vec3 color = vec3(1.0, 1.0, 1.0);
    if (useTexture) {
        color = texture(texture_diffuse1, TexCoord).rgb;
    }

    // Apply lighting
    vec3 ambient = 0.3 * color;
    vec3 diffuse = diff * color;
    vec3 result = ambient + diffuse;

    FragColor = vec4(result, 1.0);
}
