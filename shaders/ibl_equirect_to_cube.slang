// Equirectangular to Cubemap compute shader.

static const float PI = 3.14159265359;

struct PushConstants {
    uint faceSize;
};
[[vk::push_constant]] PushConstants pc;

Texture2D<float4> equirectangularMap;
SamplerState defaultSampler;
RWTexture2DArray<float4> outputCubemap; // 6 layers, one per face

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

float2 dirToEquirectUV(float3 dir) {
    // phi in [-PI, PI], theta in [-PI/2, PI/2]
    // Using atan2(z, -x) is a common convention to align the center of the 
    // equirectangular map with the -Z axis.
    float phi = atan2(dir.z, -dir.x); 
    float theta = asin(dir.y);
    float2 uv = float2(phi / (2.0 * PI) + 0.5, theta / PI + 0.5);
    return uv;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 dtid : SV_DispatchThreadID) {
    uint face = dtid.z;
    if (dtid.x >= pc.faceSize || dtid.y >= pc.faceSize || face >= 6) return;

    float2 uv = (float2(dtid.xy) + 0.5) / float(pc.faceSize);
    float3 dir = uvToDir(face, uv);
    float2 equirectUV = dirToEquirectUV(dir);

    float3 color = equirectangularMap.SampleLevel(defaultSampler, equirectUV, 0).rgb;
    outputCubemap[uint3(dtid.xy, face)] = float4(color, 1.0);
}
