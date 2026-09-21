
struct FragmentInput
{
    float4 position : SV_Position;
    float3 color : TEXCOORD0;
};

bool isEqual(float a, float b, float error) {
    return (a - b) < error && (a - b) > -error;
}

float4 main(FragmentInput input) : SV_Target0
{
    float3 black = float3(0, 0, 0);
    float3 color = input.color;
    float error = 0.01;
    float edgeCount = 
        isEqual(color.x, 0, error) + isEqual(color.x, 1, error) +
        isEqual(color.y, 0, error) + isEqual(color.y, 1, error) +
        isEqual(color.z, 0, error) + isEqual(color.z, 1, error);
    bool isBlack = edgeCount > 1.5;
    color = color * (1 - isBlack) + black * isBlack;
    return float4(color, 1.0);
}