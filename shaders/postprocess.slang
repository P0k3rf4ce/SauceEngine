struct VSOutput {
  float2 uv : TEXCOORD0;
  float4 position : SV_POSITION;
};

[shader("vertex")]
VSOutput vertMain(uint vertexId : SV_VertexID) {
  VSOutput out;
  // full screen triangle
  float2 positions[3] = { float2(-1.0, -1.0), float2(3.0, -1.0), float2(-1.0, 3.0) };
  out.position = float4(positions[vertexId], 0.0, 1.0);
  out.uv = (out.position.xy * 0.5) + 0.5;
  return out;
}

struct PostParams {
  float2 texelSize;
  float threshold;
  float intensity;
};

ConstantBuffer<PostParams> params;
Texture2D<float4> sceneTex;
SamplerState linearSampler;

[shader("fragment")]
float4 fragMain(VSOutput in) : SV_TARGET {
  float2 uv = in.uv;

  // bright pass
  float4 c = sceneTex.Sample(linearSampler, uv);
  float luminance = dot(c.rgb, float3(0.2126, 0.7152, 0.0722));
  float4 bright = luminance > params.threshold ? c : float4(0,0,0,0);

  // simple 3x3 blur
  float2 t = params.texelSize;
  float4 sum = float4(0,0,0,0);
  for (int y = -1; y <= 1; ++y) {
    for (int x = -1; x <= 1; ++x) {
      sum += sceneTex.Sample(linearSampler, uv + float2(x,y) * t);
    }
  }
  float4 blur = sum / 9.0;

  // combining original with blurred bright areas
  float4 result = c + blur * params.intensity;
  return float4(result.rgb, 1.0);
}
