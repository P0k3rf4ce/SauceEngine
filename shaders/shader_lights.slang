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
	float3 position;
	float intensity;
	float3 direction;
	float innerConeAngle;
	float3 color;
	float outerConeAngle;
	float range;
	float padding;
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

float3 computeSpot(SpotLight light, float3 fragPos, float3 normal, float3 viewPos) {
  // not sure what "current fragment" is in the issue description
  // assuming that the params will be determined at function call and that it isnt something defined globally

	// Direction from fragment to light
	float3 lightDir = light.position - fragPos;
	float distance = length(lightDir);
	lightDir = normalize(lightDir);
	
	// Distance attenuation
	float attenuation = 1.0 / max(distance * distance, 0.0001); // this is incase distance is 0
	
	// in this step, we start choking the light after it has travelled at least 80% of the distance
	attenuation *= smoothstep(light.range, light.range * 0.8, distance); 
	
	// Spotlight cone attenuation
	float theta = dot(lightDir, -normalize(light.direction));
	float cosInner = cos(light.innerConeAngle);
	float cosOuter = cos(light.outerConeAngle);
	float spotIntensity = smoothstep(cosOuter, cosInner, theta);
	
	// Blinn-Phong lighting components
	float3 norm = normalize(normal);
	float3 viewDir = normalize(viewPos - fragPos);
	
	// Diffuse
	float diff = max(dot(norm, lightDir), 0.0);
	float3 diffuse = diff * light.color;
	
	// Specular 
	float3 halfDir = normalize(lightDir + viewDir);
	// NOTE: change 32.0 to a 'shininess' param when switching to textures (blinn-phong only)
	float spec = pow(max(dot(norm, halfDir), 0.0), 32.0);
	float3 specular = spec * light.color;
	
	// Ambient
	float3 ambient = 0.1 * light.color;
	
	float3 result = (ambient + diffuse + specular) * light.intensity * spotIntensity * attenuation;
	return result;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
  // TODO: change to properly compute light
  return float4(vertIn.colour, 1.0);
}

