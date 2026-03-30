struct VSInput {
    float3 inPosition;
    float3 inNormal;
    float2 inTexCoord;
    float3 inColour;
    float4 tangent;
};

struct LightSpaceUBO {
    float4x4 lightViewProj;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LightSpaceUBO> ubo;

[[vk::push_constant]]
cbuffer PushConstants {
    float4x4 model;
};

struct VSOutput {
    float4 position : SV_POSITION;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    float4 worldPos = mul(model, float4(input.inPosition, 1.0));
    output.position = mul(ubo.lightViewProj, worldPos);
    return output;
}

[shader("fragment")]
void fragMain() {
}
