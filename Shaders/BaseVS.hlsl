#include "BaseTypes.hlsl"

#pragma ShaderType:Vertex

float4 positions[3] =
{
    float4(0.0, -0.5, 1.0, 1.0),
    float4(0.5, 0.5, 1.0, 1.0),
    float4(-0.5, 0.5, 1.0, 1.0)
};

float4 colors[3] =
{
    float4(1.0, 0.0, 0.0, 1.0),
    float4(0.0, 1.0, 0.0, 1.0),
    float4(0.0, 0.0, 1.0, 1.0),
};

uint vertexID : SV_VertexID;

VSOutput main(VSInput input)
{
    VSOutput output;

    output.worldPosition = positions[vertexID];
    output.color = colors[vertexID];
    
    //float4 worldPos = mul(world, float4(input.position, 1.0));
    //output.position = mul(viewProj, worldPos);
    //output.worldPos = worldPos.xyz;
    
    //// Transform normal into world space (without translation)
    //output.normal = mul((float3x3) world, input.normal);
    //output.uv = input.uv;

    return output;
}