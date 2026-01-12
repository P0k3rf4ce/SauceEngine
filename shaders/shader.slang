struct VSInput {
  float2 inPosition;
  float3 inColor;
};

struct UniformBuffer {
  float4x4 model;
  float4x4 view;
  float4x4 proj;
};
ConstantBuffer<UniformBuffer> ubo;

struct VSOutput {
  float3 color;
  float4 position : SV_POSITION;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
  VSOutput output;
  output.color = input.inColor;
  output.position = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.inPosition, 0.0, 1.0))));
  return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
  return float4(vertIn.color, 1.0);
}
