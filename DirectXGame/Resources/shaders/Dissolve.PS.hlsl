#include "Test.hlsli"

Texture2D<float32_t4> gTexture : register(t0); // SRV      register => t
Texture2D<float32_t> gMaskTexture : register(t1); // SRV      register => t
SamplerState gSampler : register(s0);           // Sampler  register => s

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float32_t mask = gMaskTexture.Sample(gSampler, input.texcoord);
    
    if (mask <= 0.5f)
    {
        discard;
    }
    
    output.color = gTexture.Sample(gSampler, input.texcoord);
    
    return output;
}