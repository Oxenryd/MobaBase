#pragma ShaderType:Vertex
#pragma Name:ShapeRendererVS

#include "BaseTypes.hlsl"

struct ShapeVSIn
{
    [[vk::location(0)]] uint vertIndex     : SV_VertexID;
    [[vk::location(1)]] uint instIndex     : SV_InstanceID;
};

struct ShapePSIn
{
    [[vk::location(0)]] float4 screenPos    : SV_Position;
    [[vk::location(1)]] float3 color        : COLOR0;
    [[vk::location(2)]] float alpha         : TEXCOORD0;
    [[vk::location(3)]] uint id             : TEXCOORD1;
};

struct AABB
{
    float3 mn;
    float3 mx;
};

struct ShapePush // size 64 + 16 + 24 + 16 = 80 + 40 = 120
{
    float4x4    modelToWorld;
    float4      color;
    AABB        aabb;
    float4      rotQuat;
    uint        drawNumber;
};

[[vk::push_constant]]
ShapePush push;

static float3 corner(AABB aabb, uint i)
{
    return float3(
        (i & 1u) ? aabb.mx.x : aabb.mn.x,
        (i & 2u) ? aabb.mx.y : aabb.mn.y,
        (i & 4u) ? aabb.mx.z : aabb.mn.z
    );
}

static float3 rotateQuat(float3 v, float4 q)
{
    float3 t = 2.0f * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

ShapePSIn main(ShapeVSIn input)
{
    ShapePSIn output = (ShapePSIn) 0;
    
    float4x4 modelToWorld = push.modelToWorld;
    matrix MV = mul(worldToView, modelToWorld);
    matrix MVP = mul(projection, MV);
    
    float3 c = 0.5f * (push.aabb.mn + push.aabb.mx);    
    float3 p = corner(push.aabb, input.vertIndex & 7u);
    p = rotateQuat(p - c, push.rotQuat) + c;
    
    output.screenPos = mul(MVP, float4(p, 1.0f));
    output.color = push.color.rgb;
    output.alpha = push.color.a;
    output.id = push.drawNumber;
    
    return output;
}