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
	float3 position;
	float intensity;
	float3 direction;
	float innerConeAngle;
	float3 color;
	float outerConeAngle;
	float range;
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
	// Direction from fragment to light
	float3 lightDir = light.position - fragPos;
	float distance = length(lightDir);
	lightDir = normalize(lightDir);
	
	// Distance attenuation
	float attenuation = 1.0 / max(200 * 200 * distance * distance, 0.0001); // Incase distance is 0
	
	// We may start choking the light after it has travelled at least 80% of the distance
	// attenuation *= smoothstep(light.range, light.range * 0.8, distance); 
	
	// Spotlight cone attenuation
	float theta = dot(lightDir, -normalize(light.direction));
	float cosInner = cos(light.innerConeAngle);
	float cosOuter = cos(light.outerConeAngle);
	float spotIntensity = smoothstep(cosOuter, cosInner, theta);
	
	// Blinn-Phong lighting components
	float3 norm = normalize(normal);
	
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

