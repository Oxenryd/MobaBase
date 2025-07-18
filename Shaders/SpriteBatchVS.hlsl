#include "BaseTypes.hlsl"

#pragma ShaderType:Vertex
#pragma Name:SpriteBatchVS


SpritebatchVSOutput main(SpritebatchVSInput vin)
{	
	SpriteInstance inst = instances[vin.instanceId];

	float2 scaled = (QUAD2D[vin.vertexId].xy - inst.origin) * inst.size;

	float sinR = sin(inst.rotation);
	float cosR = cos(inst.rotation);
	float2 rotated = float2(
        scaled.x * cosR - scaled.y * sinR,
        scaled.x * sinR + scaled.y * cosR
    );

	float3 worldPos = float3(inst.position + rotated, inst.layerDepth);
	float4 clipPos = mul(view, float4(worldPos, 1.0));

	SpritebatchVSOutput vout;
	vout.position = clipPos;

    vout.position += retainGlobals();
	
	float2 uv = QUAD2D[vin.vertexId].zw;
    uint2 texSize;
    atlas[inst.texIndex].GetDimensions(texSize.x, texSize.y);
	vout.uv = (float2(inst.texRect.xy) + uv * float2(inst.texRect.zw)) / texSize;
    vout.instanceId = vin.instanceId;
	return vout;
}