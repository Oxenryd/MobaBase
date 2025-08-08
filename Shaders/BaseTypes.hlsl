#ifndef BASE_TYPES_INCLUDED
#define BASE_TYPES_INCLUDED

#define UINT_INVALID 0xFFFFFFFF

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

struct TexturePack
{
    uint albedoId;
    uint normalId;
    uint specularId;
    uint roughnessId;
    
    uint emissiveId;
    uint metallicId;
    uint aoId;
    uint _pad0;
};

struct BaseMaterialInstance
{
    TexturePack textures;
    float3  ambient;
    float   ambientIntensity;
    
    float3  baseColor;
    float   albedoStrength;
    
    float3  specular;
    float   specularStrength;
    
    float3  emissive;
    float   shininess;
    
    float   refraction;
    float   transparency;
    float   roughness;
    float   metallic;
    
    float3  transparentColor;
    float   ao;

    float   reflectivity;
    float   transmission;
    float   emissiveStrength;
    float   clearcoatStrength;

};

// Globals
[[vk::binding(0, 0)]]
cbuffer cameraData : register(b0, space0)
{
    float4x4 worldToView;
    float4x4 projection;
    float4 cameraPosition;
    float vFov;
    float nearPlane;
    float farPlane;
    float aspectRatio;
};


struct BaseMatPush
{
    float4x4    modelToWorld;
    uint        matrixIndex;
    uint        matInstanceIndex;
    uint        boneOffset;
    uint        boneCount;
};

[[vk::push_constant]]
BaseMatPush basePush;

struct ModelTransform
{
    float4x4 modelToWorld;
};

struct InstanceData
{
    uint matrixIndex;
    uint materialIndex;
    uint boneOffset;
    uint boneCount;
};

[[vk::binding(2, 0)]] StructuredBuffer<ModelTransform> modelMatrices : register(t0, space0);
[[vk::binding(3, 0)]] StructuredBuffer<InstanceData> instanceData : register(t1, space0);


[[vk::binding(1, 1)]] SamplerState smp : register(s0, space1);
[[vk::binding(2, 1)]] StructuredBuffer<BaseMaterialInstance> baseMatInstances: register(t0, space1);
[[vk::binding(3, 1)]] Texture2D textures[] : register(t1, space1);


struct BaseVSIn
{
    [[vk::location(0)]] float3 pos          : POSITION;
    [[vk::location(1)]] float3 normal       : NORMAL;
    [[vk::location(2)]] float3 tangent      : TANGENT;
    [[vk::location(3)]] float3 binormal     : BINORMAL;
    [[vk::location(4)]] float2 texCoord     : TEX;
    [[vk::location(5)]] uint instanceID     : SV_InstanceID;
};

struct BasePSIn
{
    [[vk::location(0)]] float4 pos          : SV_Position;
    [[vk::location(1)]] float3 localPos     : TEXCOORD2;
    [[vk::location(2)]] float3 worldPos     : TEXCOORD0;
    [[vk::location(3)]] float3 normal       : NORMAL;
    [[vk::location(4)]] float3 tangent      : TANGENT;
    [[vk::location(5)]] float3 binormal     : BINORMAL;
    [[vk::location(6)]] float3 localNormal  : TEXCOORD1;
    [[vk::location(7)]] float2 texCoord     : TEX;
};


float4 RetainGlobals()
{
    float dummy = cameraPosition.x * basePush.boneOffset;
    if (dummy == -9999999.0)
    {
        float4 dummy2 = dummy.xxxx + textures[0].SampleLevel(smp, float2(0, 0), 0.0);
        float4 dummy3 = dummy2 + baseMatInstances[0].ambient.xyzx;
        float4 dummy4 = mul(dummy3, modelMatrices[instanceData[0].matrixIndex].modelToWorld);
        return (float4) dummy4;
    }
        
    return float4(0,0,0,0);
}


// Srites
struct SpritebatchVSInput
{
    [[vk::location(0)]] uint vertexId   : SV_VertexID;
    [[vk::location(1)]] uint instanceId : SV_InstanceID;
};

struct SpritebatchVSOutput
{
    [[vk::location(0)]] float4 position : SV_Position;
    [[vk::location(1)]] float2 uv       : TEXCOORD0;
    [[vk::location(2)]] uint instanceId : TEXCOORD1;
};

[[vk::binding(0, 2)]] StructuredBuffer<SpriteInstance> spriteInstances : register(t0, space2);
[[vk::binding(1, 2)]] SamplerState spriteSmp : register(s0, space2);
[[vk::binding(2, 2)]] Texture2D spriteAtlas[] : register(t1, space2);


#endif // BASE_TYPES_INCLUDED