
cbuffer TransformUniform : register(b0, space1)
{
    column_major float4x4 projection;
    column_major float4x4 view;
    column_major float4x4 model;
}

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : TEXCOORD0;
};

VertexOutput main(float3 position : TEXCOORD0)
{
    VertexOutput output;

    float4 WorldPosition = mul(model, float4(position, 1.0));
    float4 cameraPosition = mul(view, WorldPosition);
    output.position = mul(projection, cameraPosition);

    output.color = position + float3(0.5, 0.5, 0.5);

    return output;
}