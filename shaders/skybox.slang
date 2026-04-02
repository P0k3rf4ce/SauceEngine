struct VSInput {
    [[vk::location(0)]] float3 inPosition;
};

struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float3   cameraPos;
};

[[vk::binding(0, 0)]]
ConstantBuffer<UniformBuffer> ubo;

[[vk::binding(3, 1)]]
SamplerCube envMap;

struct VSOutput {
    float3 texCoord;
    float4 position : SV_POSITION;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    // x' = x, y' = -z, z' = y
    output.texCoord = float3(input.inPosition.x, -input.inPosition.z, input.inPosition.y);
    
    float3x3 viewRotation = (float3x3)ubo.view;
    float3 posViewSpace = mul(viewRotation, input.inPosition);
    
    float4 pos = mul(ubo.proj, float4(posViewSpace, 1.0));
    output.position = pos.xyww;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_TARGET {
    float4 color = envMap.Sample(input.texCoord);
    
    // Tone mapping (simple Reinhard)
    float3 mapped = color.rgb / (color.rgb + 1.0);
    return float4(mapped, 1.0);
}
