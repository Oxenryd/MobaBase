#include "BaseTypes.hlsl"

#pragma ShaderType:Pixel

// Textures & samplers
//Texture2D baseTexture : register(t0);
//SamplerState baseSampler : register(s0);

float4 main(VSOutput input) : SV_Target
{
    //float3 normal = normalize(input.normal);

    //float4 texColor = baseTexture.Sample(baseSampler, input.uv);

    float4 texColor = input.color;
    
    return texColor;
}