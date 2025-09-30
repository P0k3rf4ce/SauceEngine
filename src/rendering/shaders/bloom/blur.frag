#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D image;
uniform bool horizontal;

uniform int kernelSize;
uniform float weights[64];

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(image, 0));
    int radius = (kernelSize - 1) / 2;

    vec3 sum = texture(image, TexCoords).rgb * weights[0];

    for (int i = 1; i <= radius; ++i) {
        vec2 offset = horizontal ? vec2(float(i) * texelSize.x, 0.0) : vec2(0.0, float(i) * texelSize.y);
        float w = weights[i];
        sum += texture(image, TexCoords + offset).rgb * w;
        sum += texture(image, TexCoords - offset).rgb * w;
    }

    FragColor = vec4(sum, 1.0);
}
