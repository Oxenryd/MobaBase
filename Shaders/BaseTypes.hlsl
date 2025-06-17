#ifndef BASE_TYPES_INCLUDED
#define BASE_TYPES_INCLUDED

#define USE_VULKAN

#ifdef USE_VULKAN
struct VSInput
{
    [[vk::location(0)]] float4 position         : POSITION;
    [[vk::location(1)]] float3 normal           : NORMAL;
    [[vk::location(2)]] float2 uv               : TEXCOORD0;
    [[vk::location(3)]] float4 color            : COLOR0;
};

struct VSOutput
{
    [[vk::location(0)]] float4 worldPosition    : SV_Position;
    [[vk::location(1)]] float3 normal           : TEXCOORD1;
    [[vk::location(2)]] float2 uv               : TEXCOORD0;
    [[vk::location(3)]] float4 color            : COLOR0;
};
#endif

#endif // BASE_TYPES_INCLUDED