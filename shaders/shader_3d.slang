struct VSInput {
  float3 position : POSITION;
  float3 normal : NORMAL;
  float2 texCoords : TEXCOORD0;
  float3 color : COLOR;
  float4 tangent : TANGENT;
};

struct UniformBuffer {
  float4x4 model;
  float4x4 view;
  float4x4 proj;
};
ConstantBuffer<UniformBuffer> ubo;

struct VSOutput {
  float3 colour : COLOR;
  float4 position : SV_POSITION;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
  VSOutput output;
  output.colour = input.color;
  float4 worldPos = mul(ubo.model, float4(input.position, 1.0));
  output.position = mul(ubo.proj, mul(ubo.view, worldPos));
  return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
  return float4(vertIn.colour, 1.0);
}
