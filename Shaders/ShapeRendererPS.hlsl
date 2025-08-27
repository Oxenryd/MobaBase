#pragma ShaderType:Fragment
#pragma Name:ShapeRendererPS

struct ShapeVSIn
{
    [[vk::location(0)]] uint vertIndex : SV_VertexID;
    [[vk::location(1)]] uint instIndex : SV_InstanceID;
};

struct ShapePSIn
{
    [[vk::location(0)]] float4 screenPos : SV_Position;
    [[vk::location(1)]] float3 color : COLOR0;
    [[vk::location(2)]] float alpha : TEXCOORD0;
    [[vk::location(3)]] uint id : TEXCOORD1;
};

float4 main(ShapePSIn input) : SV_Target
{
    //float3 color = float3(
    //    frac(input.color.r + (input.id * 0.16f)),
    //    frac(input.color.g + (input.id * 0.51f)),
    //    frac(input.color.b + (input.id * 0.86f))
    //);
    
    return float4(input.color, input.alpha);

}