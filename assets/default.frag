#version 430 core
out vec4 FragColor;
in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

// material parameters
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

// shadow maps
// NOTE getting this to work requires some extra config on the opengl side
// see https://wikis.khronos.org/opengl/Sampler_Object#Comparison_mode
// Sampler arrays are limited by GL_MAX_ARRAY_TEXTURE_LAYERS which is >= 256
uniform sampler2DArrayShadow shadowMaps;
// uploading data to cubemap arrays requires knowledge of how their layers work
// see https://wikis.khronos.org/opengl/Cubemap_Texture#Cubemap_array_textures
uniform samplerCubeArrayShadow pointMaps;

// lights
struct DirectionalLight
{
    vec4 Position;
    vec4 Color;
    mat4 lightSpaceMatrix; // projection * view
};

struct PointLight
{
    vec4 Position;
    vec4 Color;
};

layout(std430) readonly buffer dirLightData
{
    DirectionalLight dirLights[];
};

layout(std430) readonly buffer pointLightData
{
    PointLight pointLights[];
};

uniform vec3 camPos;

const float PI = 3.14159265359;

// trick to get tangent normals to world space - according to book source code
vec3 getNormal()
{
    vec3 tangentNormal = texture(normalMap, TexCoord).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(FragPos);
    vec3 Q2  = dFdy(FragPos);
    vec2 st1 = dFdx(TexCoord);
    vec2 st2 = dFdy(TexCoord);

    vec3 N   = normalize(Normal);
    vec3 T   = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B   = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

// distribution approx
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness; // according to the book, this looks best
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

// geometry approx
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r*r) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

// fresnel approx
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// shadow test - directional
float dirLightShadowOcclusion(int lightIndex, vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5; // transform from NDC to [0,1]
    // shadow sampler does the depth comparison and PCF for us
    // texture coordinate in this case includes:
    //   1. xy coordinates of texel
    //   2. layer index (i.e. which map we want to sample)
    //   3. value to compare with retreived texel
    // returns a float in [0,1]
    return texture(shadowMaps, vec4(projCoords.xy, lightIndex, projCoords.z));
}

// shadow test - point
float posLightShadowOcclusion(int lightIndex) {
    // use a direction vector to sample the cubemap
    vec3 sampleDir = FragPos - pointLights[lightIndex].Position.xyz;
    // the length of that vector is the depth of the fragment
    float fragDepth = length(sampleDir);

    // shadow sampler does the depth comparison and PCF for us
    // texture coordinates are 4d (3d direction vector + layer index)
    // additional argument is depth to compare with
    return texture(pointMaps, vec4(sampleDir.xyz, lightIndex), fragDepth);
}

void main()
{		
    vec3 albedo     = pow(texture(albedoMap, TexCoord).rgb, vec3(2.2));
    float metallic  = texture(metallicMap, TexCoord).r;
    float roughness = texture(roughnessMap, TexCoord).r;
    float ao        = texture(aoMap, TexCoord).r;

    vec3 N = getNormal(); // function from book
    vec3 V = normalize(camPos - FragPos);
    vec3 R = reflect(-V, N);

    // base reflectance
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);

    // directional lights
    for(int i = 0; i < dirLights.length(); i++) 
    {
        // shadow check
        vec4 fragPosLightSpace = dirLights[i].lightSpaceMatrix * vec4(FragPos, 1.0);
        float shadow = dirLightShadowOcclusion(i, fragPosLightSpace);

        // calculate radiance
        // assume directional lights always point the same direction; at origin
        vec3 L = normalize(-1 * dirLights[i].Position.xyz);
        vec3 H = normalize(V + L);
        // no attenuation for directional lights
        vec3 radiance = dirLights[i].Color.rgb;

        // calculate brdf
        float D   = distributionGGX(N, H, roughness);   
        float G   = geometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
           
        vec3 num      = D * G * F; 
        float denom   = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular = num / max(denom, 0.001); // prevent divide by zero
        
        vec3 kd = vec3(1.0) - F; // conservation of energy
        kd *= 1.0 - metallic;	  

        Lo += (kd * albedo / PI + specular) * radiance * max(dot(N, L), 0.0) * (1.0 - shadow);
    }

    // point lights
    for (int i = 0; i < pointLights.length(); i++) {
        float shadow = posLightShadowOcclusion(i);

        // calculate radiance
        vec3 L = normalize(pointLights[i].Position.xyz - FragPos);
        vec3 H = normalize(V + L);

        float distance = length(pointLights[i].Position.xyz - FragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = pointLights[i].Color.rgb * attenuation;

        // calculate brdf
        float D   = distributionGGX(N, H, roughness);   
        float G   = geometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
           
        vec3 num      = D * G * F; 
        float denom   = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular = num / max(denom, 0.001); // prevent divide by zero
        
        vec3 kd = vec3(1.0) - F; // conservation of energy
        kd *= 1.0 - metallic;	  

        Lo += (kd * albedo / PI + specular) * radiance * max(dot(N, L), 0.0) * (1.0 - shadow);
    }
    
    // ambient lighting
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kd = 1.0 - F;
    kd *= 1.0 - metallic;

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse    = irradiance * albedo;

    // sample pre-filter map and lut, then combine via split-sum approx
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf             = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular         = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kd * diffuse + specular) * ao;
    vec3 color   = ambient + Lo;

    color = color / (color + vec3(1.0));    // hdr tonemapping
    color = pow(color, vec3(1.0/2.2));      // gamma correct

    FragColor = vec4(color, 1.0);
}
