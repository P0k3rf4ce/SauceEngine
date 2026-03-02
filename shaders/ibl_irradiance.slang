// Irradiance convolution compute shader.
// Reads from an environment cubemap and writes a diffuse irradiance cubemap.
// Each texel integrates the hemisphere of incoming radiance weighted by cos(theta).

static const float PI = 3.14159265359;

struct PushConstants {
    uint faceSize;  // output face resolution (e.g. 32)
};
[[vk::push_constant]] PushConstants pc;

TextureCube<float4> envMap;
SamplerState envSampler;
RWTexture2DArray<float4> outputIrradiance; // 6 layers, one per face

// Convert face index + UV to cubemap direction
float3 uvToDir(uint face, float2 uv) {
    // Map [0,1] to [-1,1]
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

    // Build tangent frame
    float3 up = abs(N.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(0.0, 0.0, 1.0);
    float3 right = normalize(cross(up, N));
    up = cross(N, right);

    float3 irradiance = float3(0.0, 0.0, 0.0);
    float sampleCount = 0.0;

    // Uniform hemisphere sampling
    float sampleDelta = 0.025;
    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            float cosTheta = cos(theta);
            float sinTheta = sin(theta);

            // Spherical to cartesian (tangent space)
            float3 tangentSample = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
            // Tangent space to world
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            irradiance += envMap.SampleLevel(envSampler, sampleVec, 0).rgb * cosTheta * sinTheta;
            sampleCount += 1.0;
        }
    }
    irradiance = PI * irradiance / sampleCount;

    outputIrradiance[uint3(dtid.xy, face)] = float4(irradiance, 1.0);
}
