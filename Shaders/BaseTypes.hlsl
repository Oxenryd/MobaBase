#ifndef BASE_TYPES_INCLUDED
#define BASE_TYPES_INCLUDED

static const float4 QUAD2D[6] =
{
    float4(-0.5, -0.5, 0, 0),
    float4(0.5, -0.5, 1, 0),
    float4(0.5, 0.5, 1, 1),
    float4(-0.5, -0.5, 0, 0),
    float4(0.5, 0.5, 1, 1),
    float4(-0.5, 0.5, 0, 1)
};

struct SpriteInstance
{
    float2 position;
    float2 size;
    float2 origin;
    float rotation;
    float layerDepth;
    uint4 texRect;
    uint texIndex;   
};



// Globals
[[vk::binding(0, 0)]]
cbuffer GlobalData : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float4 cameraPosition;
    float time;
};
[[vk::binding(0, 1)]] SamplerState smp : register(s0);

// Srites
struct SpritebatchVSInput
{
    [[vk::location(0)]] uint vertexId : SV_VertexID;
    [[vk::location(1)]] uint instanceId : SV_InstanceID;
};

struct SpritebatchVSOutput
{
    [[vk::location(0)]] float4 position : SV_Position;
    [[vk::location(1)]] float2 uv : TEXCOORD0;
    [[vk::location(2)]] uint instanceId : TEXCOORD1;
};

[[vk::binding(0, 2)]] StructuredBuffer<SpriteInstance> instances : register(t0);
[[vk::binding(1, 2)]] SamplerState spriteSmp : register(s1);
[[vk::binding(2, 2)]] Texture2D atlas[] : register(t1);



// DEBUG DEBUG
struct PushData
{
    float4 color;
    int2 uvOffset;
};

[[vk::push_constant]]
PushData pushData;



// base
//struct VSInput
//{
//    [[vk::location(0)]] float3  position         : POSITION;
//    //[[vk::location(1)]] float3  normal           : NORMAL;
//    //[[vk::location(2)]] float2  uv               : TEXCOORD0;
//    //[[vk::location(3)]] float4  tangent          : TANGENT; // xyz = tangent, w = handedness
//    //[[vk::location(4)]] float4  color            : COLOR0;
//    //[[vk::location(5)]] uint4   boneIndices      : BLENDINDICES0;
//    //[[vk::location(6)]] float4  boneWeights      : BLENDWEIGHT0;
//};

//struct VSOutput
//{
//    [[vk::location(0)]] float4 worldPos         : SV_Position;
//    //[[vk::location(0)]] float3 localPos         : TEXCOORD0;
//    //[[vk::location(1)]] float3 normal           : TEXCOORD1;
//    //[[vk::location(2)]] float2 uv               : TEXCOORD2;
//};

//struct ObjectPush
//{
//    float4x4 model;
//    uint g_boneOffset;
//    uint g_boneCount;
//    uint2 _pad;
//};


//[[vk::binding(1, 0)]]
//StructuredBuffer<float4x4> BoneMatrices;



#endif // BASE_TYPES_INCLUDED