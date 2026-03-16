#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec4 fragBaseColor;

layout(location = 0) out vec4 outColor;

void main() {
    // Fixed directional light for "old Unreal editor" feel
    vec3 lightDir = normalize(vec3(0.3, 0.5, 0.7));

    vec3 normal = normalize(fragNormal);

    // Half-lambert for softer shading (never fully dark)
    float NdotL = dot(normal, lightDir);
    float halfLambert = NdotL * 0.5 + 0.5;

    // Use material base color, modulated by vertex color if it's non-black
    vec3 materialColor = fragBaseColor.rgb;
    vec3 vertColor = fragColor;
    vec3 baseColor;
    if (length(vertColor) > 0.01) {
        baseColor = materialColor * vertColor;
    } else {
        baseColor = materialColor;
    }

    // Ambient + diffuse
    vec3 ambient = 0.15 * baseColor;
    vec3 diffuse = halfLambert * baseColor;

    outColor = vec4(ambient + diffuse, fragBaseColor.a);
}
