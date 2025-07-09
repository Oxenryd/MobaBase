#include "BaseTypes.hlsl"

#pragma ShaderType:Fragment
#pragma Name:SpriteBatchPS

float4 main(SpritebatchVSOutput pin) : SV_Target
{
    SpriteInstance inst = instances[pin.instanceId];
    
    float4 col = atlas[inst.texIndex].Sample(smp, pin.uv);

    if (col.a < 0.01)
        discard;

    return col;
}