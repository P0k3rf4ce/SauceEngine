struct VSInput {
    float3 inPosition;
    float3 inNormal;
    float2 inTexCoord;
    float3 inColour;
    float4 tangent;
};

struct LightSpaceUBO {
    float4x4 lightViewProj;
    float3   lightPos;
    float    farPlane;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LightSpaceUBO> ubo;

[[vk::push_constant]]
cbuffer PushConstants {
    float4x4 model;
};

struct VSOutput {
    float3 worldPos;
    float4 position : SV_POSITION;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    float4 worldPos = mul(model, float4(input.inPosition, 1.0));
    output.worldPos = worldPos.xyz;
    output.position = mul(ubo.lightViewProj, worldPos);
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_TARGET {
    float lightDistance = length(input.worldPos - ubo.lightPos);
    float depth = lightDistance / ubo.farPlane;
    return float4(depth, 0.0, 0.0, 1.0);
}
