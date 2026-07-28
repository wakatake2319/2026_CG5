struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
    
};
struct Material
{
    float4x4 projectionInverse;
    // 他のメンバがあればここに追加
};

cbuffer MaterialCB : register(b0)
{
    Material gMaterial;
};