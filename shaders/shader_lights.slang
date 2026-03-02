struct VSInput {
  float3 inPosition;
  float3 inNormal;
  float2 inTexCoord;
  float3 inColour;
  float4 tangent;
};

struct UniformBuffer {
  float4x4 model;
  float4x4 view;
  float4x4 proj;
  float3 cameraPos;
};
ConstantBuffer<UniformBuffer> ubo;

struct MaterialParams {
  float4 baseColorFactor;
  float metallicFactor;
  float roughnessFactor;
  float _pad0;
  float _pad1;
};
[[vk::binding(4, 0)]] ConstantBuffer<MaterialParams> material;

// IBL textures (combined image samplers)
[[vk::binding(1, 0)]] [[vk::combinedImageSampler]] TextureCube<float4> irradianceMap;
[[vk::binding(1, 0)]] [[vk::combinedImageSampler]] SamplerState irradianceSampler;
[[vk::binding(2, 0)]] [[vk::combinedImageSampler]] TextureCube<float4> prefilterMap;
[[vk::binding(2, 0)]] [[vk::combinedImageSampler]] SamplerState prefilterSampler;
[[vk::binding(3, 0)]] [[vk::combinedImageSampler]] Texture2D<float2> brdfLUT;
[[vk::binding(3, 0)]] [[vk::combinedImageSampler]] SamplerState brdfSampler;

struct VSOutput {
  float3 colour;
  float3 modelPos;
  float4 position : SV_POSITION;
  float3 normal;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
  VSOutput output;
  output.colour = input.inColour;
  output.modelPos = mul(ubo.model, float4(input.inPosition, 1.0)).xyz;
  output.position = mul(ubo.proj, mul(ubo.view, float4(output.modelPos, 1.0)));
  // Transform normal by model matrix (assumes uniform scale)
  output.normal = mul((float3x3)ubo.model, input.inNormal);
  return output;
}

// --- Fresnel for IBL ---

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness) {
  return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// --- Direct Light Structures & Blinn-Phong ---

struct PointLight {
	float3 position;
    float3 color;
    float3 ambient;
    float3 diffuse;
    float3 specular;
    float constant;
    float linear;
    float quadratic;
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
	float3 n = normalize(normal);
    float3 v = normalize(viewDir);

    float3 l = light.position - fragPos;
    float distance = length(l);
    l = normalize(l);

    float diffStrength = max(dot(n, l), 0.0);

    float3 halfwayDir = normalize(l + v);
    const float shininess = 32.0;
    float specStrength = pow(max(dot(n, halfwayDir), 0.0), shininess);

    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    float3 ambient  = light.ambient  * light.color;
    float3 diffuse  = light.diffuse  * diffStrength * light.color;
    float3 specular = light.specular * specStrength * light.color;

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
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
	float attenuation = (light.range * light.range) / max(200 * 200 * distance * distance, 0.0001); // Incase distance is 0

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
  float3 N = normalize(vertIn.normal);
  float3 V = normalize(ubo.cameraPos - vertIn.modelPos);

  float3 albedo = vertIn.colour * material.baseColorFactor.rgb;
  float metallic = material.metallicFactor;
  float roughness = material.roughnessFactor;

  // --- Direct lighting (Blinn-Phong) ---
  float3 directLight = float3(0.0, 0.0, 0.0);

  for (int i = 0; i < 0; i++) {
    directLight += computePoint(pointLights[i], vertIn.modelPos, N, V);
  }
  for (int i = 0; i < 0; i++) {
    directLight += computeDir(dirLight[i], vertIn.modelPos, N, V);
  }
  for (int i = 0; i < 0; i++) {
    directLight += computeSpot(spotLights[i], vertIn.modelPos, N, V);
  }

  // --- IBL ambient lighting ---
  float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
  float3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
  float3 kS = F;
  float3 kD = (1.0 - kS) * (1.0 - metallic);

  // Diffuse IBL
  float3 irradiance = irradianceMap.Sample(irradianceSampler, N).rgb;
  float3 diffuse = irradiance * albedo;

  // Specular IBL
  float3 R = reflect(-V, N);
  const float MAX_REFLECTION_LOD = 4.0;
  float3 prefilteredColor = prefilterMap.SampleLevel(prefilterSampler, R, roughness * MAX_REFLECTION_LOD).rgb;
  float2 brdf = brdfLUT.Sample(brdfSampler, float2(max(dot(N, V), 0.0), roughness));
  float3 specularIBL = prefilteredColor * (F * brdf.x + brdf.y);

  float3 ambient = kD * diffuse + specularIBL;

  // Combine direct (Blinn-Phong) + IBL ambient
  float3 color = directLight * albedo + ambient;

  // HDR tonemapping (Reinhard) + gamma correction
  color = color / (color + float3(1.0, 1.0, 1.0));
  color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

  return float4(color, 1.0);
}
