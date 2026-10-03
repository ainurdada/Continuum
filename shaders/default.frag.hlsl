
cbuffer MaterialConstantBuffer : register(b0, space3)
{
    float3 baseColor;
}

struct FragmentInput
{
    float4 position : SV_Position;
    float3 color : TEXCOORD0;
};

float4 main(FragmentInput input) : SV_Target0
{
    return float4(baseColor, 1.0);
}