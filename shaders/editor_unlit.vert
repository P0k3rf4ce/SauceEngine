#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 baseColor;
    float metallic;
    float roughness;
    float pad[2];
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;
layout(location = 4) in vec4 inTangent;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec4 fragBaseColor;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;

    // Transform normal to world space (using upper-left 3x3 of model matrix)
    fragNormal = normalize(mat3(transpose(inverse(push.model))) * inNormal);
    fragColor = inColor;
    fragWorldPos = worldPos.xyz;
    fragBaseColor = push.baseColor;
}
