#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform samplerCube environmentMap;

void main() {
    vec3 normal = normalize(WorldPos);
    vec3 irradiance = texture(environmentMap, normal).rgb;
    FragColor = vec4(irradiance, 1.0);
}
