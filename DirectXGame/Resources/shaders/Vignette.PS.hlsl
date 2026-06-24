#include "Test.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float32_t2 uv = input.texcoord;
    output.color = gTexture.Sample(gSampler, uv);

    // ヴィネット計算
    float32_t2 correct = uv * (1.0f - uv.yx);

    float32_t vignette = correct.x * correct.y * 16.0f;

    vignette = saturate(vignette);

    // 強さ調整
    vignette = pow(vignette, 0.8f);

    // 色に適用
    output.color.rgb *= vignette;

    return output;
}