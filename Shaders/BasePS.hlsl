#include "BaseTypes.hlsl"

#pragma ShaderType:Fragment
#pragma Name:BasePS


//-----------------------------------------------------------------------------------------
// Pixel Shader
//-----------------------------------------------------------------------------------------

float4 main(BasePSIn input) : SV_Target
{
    uint matIndex = basePush.materialIndex;
    BaseMaterialInstance matInstance = baseMatInstances[matIndex];
    float4 diffuse = matInstance.textures.albedoId == UINT_INVALID
		? float4(1,0,1,1)
		: textures[matInstance.textures.albedoId].Sample(smp, input.texCoord);
	
    return diffuse + RetainGlobals();
	// Debug shading #1: map and return normal as a color, i.e. from [-1,1]->[0,1] per component
	// The 4:th component is opacity and should be = 1
    return float4(input.localNormal * 0.5 + 0.5, 1) + RetainGlobals();
	
	// Debug shading #2: map and return texture coordinates as a color (blue = 0)
	//	return float4(input.texCoord, 0, 1);
}