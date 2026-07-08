#include "Test.hlsli"

Texture2D<float32_t4> gTexture : register(t0); // SRV      register => t
SamplerState gSampler : register(s0); // Sampler  register => s

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    const float32_t2 kCenter = float32_t2(0.5f, 0.5f);
    const int32_t kNumSamples = 10;
    const float32_t kBlurWidth = 0.1f;
    
    float32_t2 direction = input.texcoord - kCenter;
    float32_t3 outputColor = float32_t3(0.0f, 0.0f, 0.0f);
    for (int32_t sampleIndex = 0; sampleIndex < kNumSamples; ++sampleIndex)
    {
        float32_t sampleOffset = (float32_t) sampleIndex / (float32_t) (kNumSamples - 1);
        float32_t2 texcoord = input.texcoord - direction * sampleOffset * kBlurWidth;
        
        
        outputColor.rgb += gTexture.Sample(gSampler, texcoord).rgb;
    }
    
    outputColor.rgb *= rcp(kNumSamples);
    
    PixelShaderOutput output;
    output.color.rgb = outputColor;
    output.color.a = 1.0f;
    
    //output.color = float4(input.texcoord, 0.0f, 1.0f);
    return output;
}