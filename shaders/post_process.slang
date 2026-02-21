struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

[shader("vertex")]
VSOutput vertMain(uint vertexID : SV_VertexID) {
    VSOutput output;
    output.texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.texCoord * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

[vk::binding(0, 0)]
Sampler2D sceneTexture;

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_TARGET {
    // Simple pass-through for now
    float4 color = sceneTexture.Sample(input.texCoord);
    
    // Example effect: Grayscale
    // float gray = dot(color.rgb, float3(0.299, 0.587, 0.114));
    // return float4(gray, gray, gray, 1.0);

    return color;
}
