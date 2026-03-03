#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec4 fragBaseColor;
layout(location = 4) in vec3 fragCameraPos;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 baseColor;
    float metallic;
    float roughness;
    float pad[2];
} push;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

// GGX/Trowbridge-Reitz normal distribution
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0001);
}

// Schlick-GGX geometry function
float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Smith's geometry function
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

// Fresnel-Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(fragCameraPos - fragWorldPos);

    // Material properties from push constants
    float metallic = push.metallic;
    float roughness = max(push.roughness, 0.04); // clamp to avoid divide-by-zero

    // Base color: use material color, modulated by vertex color if non-black
    vec3 materialColor = fragBaseColor.rgb;
    vec3 vertColor = fragColor;
    vec3 albedo;
    if (length(vertColor) > 0.01) {
        albedo = materialColor * vertColor;
    } else {
        albedo = materialColor;
    }

    // F0 for dielectrics is 0.04, for metals it's the albedo
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Fixed directional lights: key + fill + rim
    vec3 lightDirs[3] = vec3[](
        normalize(vec3(0.5, 0.7, 0.5)),    // Key light (warm, from above-right)
        normalize(vec3(-0.4, 0.3, -0.6)),   // Fill light (cool, from left-behind)
        normalize(vec3(0.0, -0.3, -1.0))    // Rim light (from behind-below)
    );
    vec3 lightColors[3] = vec3[](
        vec3(1.0, 0.95, 0.9) * 2.5,        // Key: warm white, strong
        vec3(0.6, 0.7, 0.9) * 0.8,         // Fill: cool blue, softer
        vec3(0.9, 0.9, 1.0) * 0.6          // Rim: neutral, subtle
    );

    vec3 Lo = vec3(0.0);

    for (int i = 0; i < 3; i++) {
        vec3 L = lightDirs[i];
        vec3 H = normalize(V + L);

        // Cook-Torrance BRDF
        float D = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = D * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic);

        float NdotL = max(dot(N, L), 0.0);

        Lo += (kD * albedo / PI + specular) * lightColors[i] * NdotL;
    }

    // Ambient lighting (simple hemisphere)
    vec3 ambient = vec3(0.08) * albedo;

    vec3 color = ambient + Lo;

    // Tone mapping (Reinhard) and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, fragBaseColor.a);
}
