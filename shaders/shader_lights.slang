struct VSInput {
  float2 inPosition;
  float3 inColour;
};

struct UniformBuffer {
  float4x4 model;
  float4x4 view;
  float4x4 proj;
};
ConstantBuffer<UniformBuffer> ubo;

struct VSOutput {
  float3 colour;
  float4 position : SV_POSITION;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
  VSOutput output;
  output.colour = input.inColour;
  output.position = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.inPosition, 0.0, 1.0))));
  return output;
}


struct PointLight {
	// TODO: add required data
};

struct DirLight {
	// TODO: add required data
};

struct SpotLight {
	// TODO: add required data
};

ConstantBuffer<PointLight[4]> pointLights;
ConstantBuffer<DirLight[4]> dirLight;
ConstantBuffer<SpotLight[4]> spotLights;

float3 computePoint(PointLight light) {
	// TODO: return light contribution from a given point light
}

float3 computeDir(DirLight light) {
	// TODO: return light contribution from a given directional light
}

float3 computeSpot(SpotLight light) {
	// TODO: return light contribution from a given spot light
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
  float3 finalColor = float3(0.0, 0.0, 0.0);
  
  // Accumulate all point light contributions
  for (int i = 0; i < 4; i++) {
    finalColor += computePoint(pointLights[i]);
  }
  
  // Accumulate all directional light contributions
  for (int i = 0; i < 4; i++) {
    finalColor += computeDir(dirLight[i]);
  }
  
  // Accumulate all spot light contributions
  for (int i = 0; i < 4; i++) {
    finalColor += computeSpot(spotLights[i]);
  }
  
  // Multiply accumulated lighting by the base vertex color (material color)
  finalColor *= vertIn.colour;
  
  return float4(finalColor, 1.0);
}

