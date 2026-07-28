#include "Test.hlsli"

Texture2D<float32_t4> gTexture : register(t0); // SRV      register => t
SamplerState gSampler : register(s0); // Sampler  register => s
struct Material
{
    float4 color;
    float time;
    float3 padding;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

float rand2dTo1d(float2 value)
{
    return frac(sin(dot(value, float2(12.9898f, 78.233f))) * 43758.5453f);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
        
    float32_t random = rand2dTo1d(input.texcoord + 200.0f * gMaterial.time);
       
    output.color = float32_t4(random, random, random, 1.0f);
    
    return output;
}