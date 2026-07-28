#include "Test.hlsli"

Texture2D<float32_t4> gTexture : register(t0); // SRV      register => t
SamplerState gSampler : register(s0); // Sampler  register => s
Texture2D<float32_t> gDepthTexture : register(t1); // SRV      register => t
SamplerState gSamplerPoint : register(s1); // Sampler  register => s


struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

float32_t Luminance(float32_t3 v)
{
    return dot(v, float32_t3(0.2125f, 0.7154f, 0.0721f));
}

PixelShaderOutput main(VertexShaderOutput input)
{
    
    static const float32_t2 kIndex3x3[3][3] =
    {
        { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
        { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
        { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } }
    };
    
    static const float32_t kPrewittHorizontalKernel[3][3] =
    {
        { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
        { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
        { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    };
    
    static const float32_t kPrewittVerticalKernel[3][3] =
    {
        { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
        { 0.0f, 0.0f, 0.0f },
        { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f },
    };
    
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp(width), rcp(height));
    
    // 縦横それぞれの畳み込みの結果を格納する
    float32_t2 difference = float32_t2(0.0f, 0.0f);
    
    // 色を輝度に変換して、畳み込みを行っていく。微分Filter用のkernelになっているので、やること自体は今までの畳み込みと同じ
    for (int32_t x = 0; x < 3; ++x)
    {
        for (int32_t y = 0; y < 3; ++y)
        {
            float32_t2 texcoord = input.texcoord + kIndex3x3[x][y] * uvStepSize;

            float32_t ndcDepth = gDepthTexture.Sample(gSamplerPoint, texcoord);
            float32_t4 viewSpace = mul(float32_t4(0.0f, 0.0f, ndcDepth, 1.0f), gMaterial.projectionInverse);
            float32_t viewZ = viewSpace.z * rcp(viewSpace.w);
            difference.x += viewZ * kPrewittHorizontalKernel[x][y];
            difference.y += viewZ * kPrewittVerticalKernel[x][y];

            
        }
    }
    // 変化の長さをウェイトとして合成
    float32_t weight = length(difference);
    
    // 差が小さくわかりづらいので、適当に掛ける
    weight = saturate(weight);
    
    PixelShaderOutput output;
    
    // weghtが大きいほど暗く表示する。最もシンプルな合成方法
    output.color.rgb = (1.0f - weight) * gTexture.Sample(gSampler, input.texcoord).rgb;
    output.color.a = 1.0f;
    
    return output;
}