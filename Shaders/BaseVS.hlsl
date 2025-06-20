#include "BaseTypes.hlsl"

#pragma ShaderType:Vertex

const static float4 positions[3] =
{
    float4(0.0, -0.5, 1.0, 1.0),
    float4(0.5, 0.5, 1.0, 1.0),
    float4(-0.5, 0.5, 1.0, 1.0)
};

const static float4 colors[3] =
{
    float4(1.0, 0.0, 0.0, 1.0),
    float4(0.0, 1.0, 0.0, 1.0),
    float4(0.0, 0.0, 1.0, 1.0),
};

ObjectPush obj;

VSOutput main(VSInput input, uint vertexID : SV_VertexID)
{
    VSOutput output;

    output.worldPos = positions[vertexID];
    return output;
    
    // Skinning
    float4 skinnedPos;
    if (obj.g_boneCount == 0)
    {
        skinnedPos = float4(input.position, 1.0f);
    }
    else
    {
        float4x4 skinMatrix = 0;
        [unroll]
        for (int i = 0; i < 4; ++i)
        {
            uint index = obj.g_boneOffset + input.boneIndices[i];
            skinMatrix += input.boneWeights[i] * BoneMatrices[index];
        }
        skinnedPos = mul(skinMatrix, float4(input.position, 1.0f));
    }
    
    float4x4 mvp = mul(proj, mul(view, obj.model));
    
    output.worldPos = mul(mvp, skinnedPos);
    output.normal = input.normal;
    output.localPos = input.position;
    output.uv = input.uv;
    return output;
}