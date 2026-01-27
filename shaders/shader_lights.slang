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
	float3 direction;
	float3 color;
	float3 ambient;
	float3 diffuse;
	float3 specular;
};

struct SpotLight {
	// TODO: add required data
};

ConstantBuffer<PointLight[4]> pointLights;
ConstantBuffer<DirLight[4]> dirLight;
ConstantBuffer<SpotLight[4]> spotLights;

float3 computePoint(PointLight light, float3 fragPos, float3 normal, float3 viewDir) {
	// TODO: return light contribution from a given point light
}

float3 computeDir(DirLight light, float3 fragPos, float3 normal, float3 viewDir) {
	float3 n = normalize(normal);
	float3 v = normalize(viewDir);
	float3 l = normalize(-light.direction);

	float diffStrength = max(dot(n, l), 0.0);

	float3 halfwayDir = normalize(l + v);
	const float shininess = 32.0; // NOTE: change 32.0 to a 'shininess' param when switching to textures (blinn-phong only)
	float specStrength = pow(max(dot(n, halfwayDir), 0.0), shininess);

	float3 ambient = light.ambient * light.color;
	float3 diffuse = light.diffuse * diffStrength * light.color;
	float3 specular = light.specular * specStrength * light.color;

	return ambient + diffuse + specular;
}

float3 computeSpot(SpotLight light, float3 fragPos, float3 normal, float3 viewDir) {
	// TODO: return light contribution from a given spot light
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
  // TODO: change to properly compute light
  return float4(vertIn.colour, 1.0);
}

