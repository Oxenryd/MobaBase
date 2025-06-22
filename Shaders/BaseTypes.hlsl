#ifndef BASE_TYPES_INCLUDED
#define BASE_TYPES_INCLUDED

#define USE_VULKAN

#ifdef USE_VULKAN
struct VSInput
{
    [[vk::location(0)]] float3  position         : POSITION;
    //[[vk::location(1)]] float3  normal           : NORMAL;
    //[[vk::location(2)]] float2  uv               : TEXCOORD0;
    //[[vk::location(3)]] float4  tangent          : TANGENT; // xyz = tangent, w = handedness
    //[[vk::location(4)]] float4  color            : COLOR0;
    //[[vk::location(5)]] uint4   boneIndices      : BLENDINDICES0;
    //[[vk::location(6)]] float4  boneWeights      : BLENDWEIGHT0;
};

struct VSOutput
{
    [[vk::location(0)]] float4 worldPos         : SV_Position;
    //[[vk::location(0)]] float3 localPos         : TEXCOORD0;
    //[[vk::location(1)]] float3 normal           : TEXCOORD1;
    //[[vk::location(2)]] float2 uv               : TEXCOORD2;
};

struct ObjectPush
{
    float4x4 model;
    uint g_boneOffset;
    uint g_boneCount;
    uint2 _pad;
};

[[vk::binding(0, 0)]]
cbuffer GlobalData : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float3 cameraPosition;
    double time;
};

[[vk::binding(1, 0)]]
StructuredBuffer<float4x4> BoneMatrices;


#endif

#endif // BASE_TYPES_INCLUDED