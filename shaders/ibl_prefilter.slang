// Pre-filtered environment map compute shader for specular IBL.
// For each roughness level (mip), importance-samples the GGX distribution
// to produce a pre-filtered cubemap.

static const float PI = 3.14159265359;

struct PushConstants {
    uint faceSize;    // output face resolution for this mip level
    float roughness;  // roughness for this mip level
};
[[vk::push_constant]] PushConstants pc;

TextureCube<float4> envMap;
SamplerState envSampler;
RWTexture2DArray<float4> outputPrefilter; // 6 layers, one per face

float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint N) {
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

float3 uvToDir(uint face, float2 uv) {
    float u = uv.x * 2.0 - 1.0;
    float v = uv.y * 2.0 - 1.0;

    switch (face) {
        case 0: return normalize(float3( 1.0,   -v,   -u)); // +X
        case 1: return normalize(float3(-1.0,   -v,    u)); // -X
        case 2: return normalize(float3(   u,  1.0,    v)); // +Y
        case 3: return normalize(float3(   u, -1.0,   -v)); // -Y
        case 4: return normalize(float3(   u,   -v,  1.0)); // +Z
        case 5: return normalize(float3(  -u,   -v, -1.0)); // -Z
        default: return float3(0, 0, 1);
    }
}

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 dtid : SV_DispatchThreadID) {
    uint face = dtid.z;
    if (dtid.x >= pc.faceSize || dtid.y >= pc.faceSize || face >= 6) return;

    float2 uv = (float2(dtid.xy) + 0.5) / float(pc.faceSize);
    float3 N = uvToDir(face, uv);
    float3 R = N;
    float3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 0.0;
    float3 prefilteredColor = float3(0.0, 0.0, 0.0);

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, pc.roughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            prefilteredColor += envMap.SampleLevel(envSampler, L, 0).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    prefilteredColor /= max(totalWeight, 0.001);

    outputPrefilter[uint3(dtid.xy, face)] = float4(prefilteredColor, 1.0);
}
