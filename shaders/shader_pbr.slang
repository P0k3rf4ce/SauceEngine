// ============================================================================
// PBR Shader — Cook-Torrance BRDF (GGX/Smith/Fresnel-Schlick) + IBL
// Uses SSBO for dynamic light array and IBL maps for ambient lighting.
//
// Descriptor Layout (Set 0):
//   Binding  0: ConstantBuffer<UniformBuffer>   (Vertex + Fragment) — MVP + camera
//   Binding  1: ConstantBuffer<MaterialData>    (Fragment)          — PBR material properties
//   Binding  2: StructuredBuffer<Light>         (Fragment)          — SSBO light array
//   Binding  3: Texture2D   baseColorMap        (Fragment)          — Base color texture
//   Binding  4: SamplerState baseColorSampler   (Fragment)
//   Binding  5: Texture2D   normalMap           (Fragment)          — Normal map
//   Binding  6: SamplerState normalSampler      (Fragment)
//   Binding  7: Texture2D   metallicRoughnessMap(Fragment)          — glTF: G=roughness, B=metallic
//   Binding  8: SamplerState metallicRoughnessSampler (Fragment)
//   Binding  9: Texture2D   occlusionMap        (Fragment)          — Occlusion map
//   Binding 10: SamplerState occlusionSampler   (Fragment)
//   Binding 11: Texture2D   emissiveMap         (Fragment)          — Emissive map
//   Binding 12: SamplerState emissiveSampler    (Fragment)
//   Binding 13: TextureCube irradianceMap       (Fragment)          — IBL diffuse irradiance cubemap
//   Binding 14: SamplerState irradianceSampler  (Fragment)
//   Binding 15: TextureCube prefilterMap        (Fragment)          — IBL specular prefilter cubemap
//   Binding 16: SamplerState prefilterSampler   (Fragment)
//   Binding 17: Texture2D   brdfLUT             (Fragment)          — IBL BRDF integration LUT
//   Binding 18: SamplerState brdfLUTSampler     (Fragment)
//
// Push Constants:
//   uint lightCount — number of active lights in the SSBO
//
// Notes:
//   - IBL provides indirect ambient lighting via split-sum approximation.
//   - Outputs linear HDR. Tonemapping and gamma correction are handled
//     by the post-processing pass (postprocess.slang).
//   - Normal transform assumes uniform scale. For non-uniform scale,
//     pass transpose(inverse(model)) as a normal matrix from CPU.
//   - glTF metallic-roughness packing: G=roughness, B=metallic.
//
// C++ struct alignment reference (std140 for ConstantBuffer, std430 for StructuredBuffer):
//
//   MaterialData (std140, 48 bytes):
//     float4 baseColorFactor      offset  0
//     float  metallicFactor        offset 16
//     float  roughnessFactor       offset 20
//     float  normalScale           offset 24
//     float  occlusionStrength     offset 28
//     float3 emissiveFactor        offset 32 (align 16)
//     float  _pad0                 offset 44
//
//   Light (std430, 80 bytes):
//     float3 position              offset  0  // occupies 16 bytes (std430 vec3 alignment)
//     uint   type                  offset 16
//     float3 direction             offset 32
//     float  intensity             offset 44
//     float3 color                 offset 48
//     float  range                 offset 60
//     float  innerConeAngle        offset 64
//     float  outerConeAngle        offset 68
//     float2 _padding              offset 72
// ============================================================================

static const float PI = 3.14159265359;

// ============================================================================
// Vertex Input — matches Vertex in Vertex.hpp
// ============================================================================
struct VSInput {
    float3 inPosition;          // location 0
    float3 inNormal;            // location 1
    float2 inTexCoord;          // location 2
    float3 inColour;            // location 3
    float4 tangent;             // location 4 (w = bitangent handedness)
};

// ============================================================================
// Uniform Buffer — matches UniformBufferObject in Vertex.hpp
// ============================================================================
struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float3   cameraPos;
};

[[vk::binding(0, 0)]]
ConstantBuffer<UniformBuffer> ubo;

// ============================================================================
// Material Data — mirrors MaterialProperties in Material.hpp
// ============================================================================
struct MaterialData {
    float4 baseColorFactor;         // RGBA base color multiplier
    float  metallicFactor;          // Metallic multiplier  [0..1]
    float  roughnessFactor;         // Roughness multiplier [0..1]
    float  normalScale;             // Normal map XY scale
    float  occlusionStrength;       // AO blend strength    [0..1]
    float3 emissiveFactor;          // Emissive RGB multiplier
    float  _pad0;
};

[[vk::binding(1, 0)]]
ConstantBuffer<MaterialData> material;

// ============================================================================
// Light — unified struct for directional (0), point (1), and spot (2) lights
// ============================================================================
struct Light {
    float3 position;                // Point/spot: world position
    uint   type;                    // 0 = directional, 1 = point, 2 = spot
    float3 direction;               // Directional/spot: light direction
    float  intensity;               // Luminous intensity
    float3 color;                   // Linear-space RGB
    float  range;                   // Point/spot: max range (0 = infinite)
    float  innerConeAngle;          // Spot: inner cone half-angle (radians)
    float  outerConeAngle;          // Spot: outer cone half-angle (radians)
    float2 _padding;
};

[[vk::binding(2, 0)]]
StructuredBuffer<Light> lights;

// ============================================================================
// Push Constants
// ============================================================================
[[vk::push_constant]]
cbuffer PushConstants {
    uint lightCount;
};

// ============================================================================
// Texture Samplers (separate image + sampler descriptors)
// ============================================================================
[[vk::binding(3, 0)]]
Texture2D<float4> baseColorMap;
[[vk::binding(4, 0)]]
SamplerState baseColorSampler;

[[vk::binding(5, 0)]]
Texture2D<float4> normalMap;
[[vk::binding(6, 0)]]
SamplerState normalSampler;

[[vk::binding(7, 0)]]
Texture2D<float4> metallicRoughnessMap;
[[vk::binding(8, 0)]]
SamplerState metallicRoughnessSampler;

[[vk::binding(9, 0)]]
Texture2D<float4> occlusionMap;
[[vk::binding(10, 0)]]
SamplerState occlusionSampler;

[[vk::binding(11, 0)]]
Texture2D<float4> emissiveMap;
[[vk::binding(12, 0)]]
SamplerState emissiveSampler;

// ============================================================================
// IBL Texture Samplers
// ============================================================================
[[vk::binding(13, 0)]]
TextureCube<float4> irradianceMap;
[[vk::binding(14, 0)]]
SamplerState irradianceSampler;

[[vk::binding(15, 0)]]
TextureCube<float4> prefilterMap;
[[vk::binding(16, 0)]]
SamplerState prefilterSampler;

[[vk::binding(17, 0)]]
Texture2D<float4> brdfLUT;
[[vk::binding(18, 0)]]
SamplerState brdfLUTSampler;

// ============================================================================
// Vertex Output / Fragment Input
// ============================================================================
struct VSOutput {
    float3 worldPos;
    float3 normal;
    float2 texCoord;
    float4 tangent;                 // xyz = world tangent, w = handedness
    float3 colour;
    float4 position : SV_POSITION;
};

// ============================================================================
// Vertex Shader
// ============================================================================
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;

    // World-space position
    float4 worldPos4 = mul(ubo.model, float4(input.inPosition, 1.0));
    output.worldPos = worldPos4.xyz;

    // Clip-space position
    output.position = mul(ubo.proj, mul(ubo.view, worldPos4));

    // Transform normal to world space
    // NOTE: Assumes uniform scale. For non-uniform scale the CPU should pass
    //       transpose(inverse(model)) as a separate normal matrix.
    float3x3 normalMatrix = (float3x3)ubo.model;
    output.normal = normalize(mul(normalMatrix, input.inNormal));

    // Transform tangent to world space (same matrix; valid for uniform scale)
    output.tangent = float4(
        normalize(mul(normalMatrix, input.tangent.xyz)),
        input.tangent.w
    );

    // Pass through
    output.texCoord = input.inTexCoord;
    output.colour   = input.inColour;

    return output;
}

// ============================================================================
// PBR Helper Functions
// ============================================================================

// Build orthonormal TBN matrix from interpolated normal and tangent.
// Rows = T, B, N so that mul(tangentSpaceVec, tbn) maps tangent -> world.
float3x3 constructTBN(float3 N, float4 T) {
    float3 normal  = normalize(N);
    float3 tangent = normalize(T.xyz);
    // Gram-Schmidt re-orthogonalise
    tangent = normalize(tangent - dot(tangent, normal) * normal);
    float3 bitangent = cross(normal, tangent) * T.w; // w = handedness
    return float3x3(tangent, bitangent, normal);      // rows
}

// Sample the normal map and transform from tangent space to world space.
float3 getNormalFromMap(float3x3 tbn, float2 texCoord, float nScale) {
    float3 n = normalMap.Sample(normalSampler, texCoord).xyz;
    n = n * 2.0 - 1.0;            // [0,1] -> [-1,1]
    n.xy *= nScale;
    n = normalize(n);
    return normalize(mul(n, tbn)); // tangent -> world
}

// ---- Cook-Torrance BRDF terms ----

// GGX / Trowbridge-Reitz Normal Distribution Function
float DistributionGGX(float3 N, float3 H, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return a2 / max(denom, 1e-7);
}

// Schlick-GGX geometry function (single direction)
// Uses k = (roughness+1)^2 / 8 (direct-lighting remapping)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k + 1e-7);
}

// Smith's method — combined view + light geometry term
float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness)
         * GeometrySchlickGGX(NdotL, roughness);
}

// Fresnel-Schlick approximation
float3 fresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// Fresnel-Schlick with roughness term — used for IBL ambient specular
// to prevent rough surfaces from appearing overly reflective.
float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness) {
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0)
             * pow(saturate(1.0 - cosTheta), 5.0);
}

// ---- Per-light radiance computation ----

struct LightContribution {
    float3 L;         // Direction towards the light (normalised)
    float3 radiance;  // Incoming radiance from the light
};

LightContribution computeLightContribution(Light light, float3 worldPos) {
    LightContribution result;
    result.L        = float3(0.0, 0.0, 0.0);
    result.radiance = float3(0.0, 0.0, 0.0);

    if (light.type == 0) {
        // ---- Directional ----
        result.L = normalize(-light.direction);
        result.radiance = light.color * light.intensity;
    }
    else if (light.type == 1) {
        // ---- Point ----
        float3 toLight  = light.position - worldPos;
        float  distance = length(toLight);
        result.L = toLight / max(distance, 1e-6);

        float attenuation = 1.0 / (distance * distance + 1e-4);

        // Smooth range windowing (avoids hard cutoff)
        if (light.range > 0.0) {
            float ratio = distance / light.range;
            float ratio2 = ratio * ratio;
            float falloff = saturate(1.0 - ratio2 * ratio2); // (1 - (d/r)^4)+
            attenuation *= falloff * falloff;
        }

        result.radiance = light.color * light.intensity * attenuation;
    }
    else {
        // ---- Spot (type == 2) ----
        float3 toLight  = light.position - worldPos;
        float  distance = length(toLight);
        result.L = toLight / max(distance, 1e-6);

        float attenuation = 1.0 / (distance * distance + 1e-4);

        // Cone attenuation
        float theta    = dot(result.L, normalize(-light.direction));
        float cosInner = cos(light.innerConeAngle);
        float cosOuter = cos(light.outerConeAngle);
        float spotEffect = smoothstep(cosOuter, cosInner, theta);

        // Range windowing
        if (light.range > 0.0) {
            float ratio = distance / light.range;
            float ratio2 = ratio * ratio;
            float falloff = saturate(1.0 - ratio2 * ratio2);
            attenuation *= falloff * falloff;
        }

        result.radiance = light.color * light.intensity * attenuation * spotEffect;
    }

    return result;
}

// ============================================================================
// Fragment Shader
// ============================================================================
[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {

    // ---- Sample material textures ----
    float4 baseColor = baseColorMap.Sample(baseColorSampler, vertIn.texCoord)
                     * material.baseColorFactor;
    float3 albedo = baseColor.rgb;
    float  alpha  = baseColor.a;

    // glTF metallic-roughness: G = roughness, B = metallic
    float4 mrSample  = metallicRoughnessMap.Sample(metallicRoughnessSampler, vertIn.texCoord);
    float  metallic  = mrSample.b * material.metallicFactor;
    float  roughness = mrSample.g * material.roughnessFactor;
    roughness = clamp(roughness, 0.04, 1.0); // avoid zero roughness (NaN in GGX)

    // Ambient occlusion — blend with strength
    float ao = occlusionMap.Sample(occlusionSampler, vertIn.texCoord).r;
    ao = 1.0 + material.occlusionStrength * (ao - 1.0);

    // Emissive
    float3 emissive = emissiveMap.Sample(emissiveSampler, vertIn.texCoord).rgb
                    * material.emissiveFactor;

    // ---- Surface normal (normal mapping) ----
    float3x3 tbn = constructTBN(vertIn.normal, vertIn.tangent);
    float3 N = getNormalFromMap(tbn, vertIn.texCoord, material.normalScale);

    // ---- View direction ----
    float3 V = normalize(ubo.cameraPos - vertIn.worldPos);

    // ---- Base reflectivity ----
    // Dielectrics: F0 ≈ 0.04 | Metals: F0 = albedo
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    // ---- Accumulate direct lighting from SSBO ----
    float3 Lo = float3(0.0, 0.0, 0.0);

    for (uint i = 0; i < lightCount; i++) {
        LightContribution lc = computeLightContribution(lights[i], vertIn.worldPos);
        float3 L = lc.L;
        float3 radiance = lc.radiance;

        float3 H = normalize(V + L);

        // Evaluate Cook-Torrance BRDF terms
        float  D = DistributionGGX(N, H, roughness);
        float  G = GeometrySmith(N, V, L, roughness);
        float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        // Specular component
        float3 numerator   = D * G * F;
        float  denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 1e-4;
        float3 specular    = numerator / denominator;

        // Diffuse component (energy-conserving Lambertian)
        float3 kS = F;
        float3 kD = (1.0 - kS) * (1.0 - metallic);

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // ---- Indirect Lighting (IBL) ----
    // Fresnel with roughness for ambient term
    float NdotV_ibl = max(dot(N, V), 0.0);
    float3 F_ibl = fresnelSchlickRoughness(NdotV_ibl, F0, roughness);

    float3 kS_ibl = F_ibl;
    float3 kD_ibl = (1.0 - kS_ibl) * (1.0 - metallic);

    // Indirect diffuse: sample irradiance cubemap with surface normal
    float3 irradiance = irradianceMap.Sample(irradianceSampler, N).rgb;
    float3 indirectDiffuse = irradiance * (albedo / PI);

    // Indirect specular: sample prefilter cubemap at reflection direction
    // using roughness to select mip level, combined with BRDF LUT
    float3 R = reflect(-V, N);
    const float MAX_REFLECTION_LOD = 4.0; // prefilter map mip levels - 1
    float3 prefilteredColor = prefilterMap.SampleLevel(prefilterSampler, R, roughness * MAX_REFLECTION_LOD).rgb;
    float2 brdf = brdfLUT.Sample(brdfLUTSampler, float2(NdotV_ibl, roughness)).rg;
    float3 indirectSpecular = prefilteredColor * (F_ibl * brdf.x + brdf.y);

    // Combine indirect diffuse and specular, modulated by AO
    float3 ambient = (kD_ibl * indirectDiffuse + indirectSpecular) * ao;

    // ---- Final colour ----
    float3 color = ambient + Lo + emissive;

    // ---- Tone mapping (Reinhard) ----
    // TODO(tasks 11-12): Remove once post-process pass (postprocess.slang) is wired into the renderer
    color = color / (color + 1.0);

    // ---- Gamma correction ----
    // TODO(tasks 11-12): Remove once rendering to sRGB framebuffer or post-process handles this
    color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

    return float4(color, alpha);
}
