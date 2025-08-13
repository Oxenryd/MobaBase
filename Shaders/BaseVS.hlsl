#define USE_BASE_PUSH
#include "BaseTypes.hlsl"

#pragma ShaderType:Vertex
#pragma Name:BaseVS

//-----------------------------------------------------------------------------------------
// Basic Vertex Shader
//-----------------------------------------------------------------------------------------


BasePSIn main(BaseVSIn input)
{    
    float4x4 modelToWorld = basePush.modelToWorld;
    
    BasePSIn output = (BasePSIn) 0;
        
	// Model->View transformation
    matrix MV = mul(worldToView, modelToWorld);

	// Model->View->Projection (clip space) transformation
	// SV_Position expects the output position to be in clip space
    matrix MVP = mul(projection, MV);
	
	// Perform transformations and send to output
    output.localPos = input.pos + RetainGlobals().aaa;
	
    output.pos = mul(MVP, float4(input.pos, 1));
    output.worldPos = mul(modelToWorld, float4(input.pos, 1.0)).xyz;
    output.normal = normalize(mul(modelToWorld, float4(input.normal, 0)).xyz);
    output.tangent = normalize(mul(modelToWorld, float4(input.tangent, 0)).xyz);
    output.binormal = normalize(mul(modelToWorld, float4(input.binormal, 0)).xyz);
    output.localNormal = input.normal;
    output.texCoord = input.texCoord;
		
    return output;
}