#define USE_BASE_PUSH
#include "ShaderUtils.hlsl"

#pragma ShaderType:Fragment
#pragma Name:BasePS


//-----------------------------------------------------------------------------------------
// Pixel Shader
//-----------------------------------------------------------------------------------------

float4 main(BasePSIn input) : SV_Target
{
    float2 fixedTexUV = float2(input.texCoord.x, 1.0 - input.texCoord.y);

        
    uint matIndex = basePush.flags & INSTANCE_FLAG
        ? instanceData[input.instanceID].matInstanceIndex
        : basePush.matInstanceIndex != UINT_INVALID
            ? basePush.matInstanceIndex
            : 0;
    
    
    
    BaseMaterialInstance M = baseMatInstances[matIndex];
    
    // Ambient
    float3 ambient = (camPos_amb.a * M.ambientIntensity) * M.ambient;
    
	// Albedo
	float4 albedoSample = M.textures.albedoId == UINT_INVALID
		? float4(M.baseColor, M.transparency)
		: textures[M.textures.albedoId].Sample(smp, fixedTexUV);
	
    float3 baseColor = albedoSample.rgb * M.albedoStrength;
    float alpha = albedoSample.a;
	
    // Specular
    float4 specSample = M.textures.specularId == UINT_INVALID
        ? float4(M.specular, M.transparency)
        : textures[M.textures.specularId].Sample(smp, fixedTexUV);
    float3 specColor = specSample.rgb;
	
    // Normals
    float3 normalSample = M.textures.normalId == UINT_INVALID
    ? float3(0.5, 0.5, 1.0) // Default normal map (pointing up in tangent space)
    : textures[M.textures.normalId].Sample(smp, fixedTexUV).rgb;

    // Convert from [0,1] to [-1,1] range
    float3 tangentNormal = normalize(normalSample * 2.0 - 1.0);

    // Build tangent-to-world matrix (assuming your tangent/binormal are world space)
    float3 T = input.tangent;
    float3 B = input.binormal;
    float3 N_world = input.normal;
    //N_world.x = -N_world.x;

    // If you need to handle flipped tangent space (common in some assets):
    //T = normalize(T - N_world * dot(T, N_world)); // Gram-Schmidt orthogonalization
    //B = cross(N_world, T) * sign(dot(B, cross(N_world, T))); // Ensure correct handedness

    float3x3 TBN = float3x3(T, B, N_world);

    // Transform tangent-space normal to world space
    float3 worldNormal = mul(tangentNormal, TBN);

    // Transform world normal to view space for lighting
    float3 N = normalize(mul((float3x3) worldToView, worldNormal));
    
    
    
    float3 Pvs = mul(worldToView, float4(input.worldPos, 1)).xyz;
    float3 V = normalize(-Pvs);
	
	
	// Color
    float3 color = baseColor * ambient;
	
	
	// Forward+ cluster lookup
    float2 screenSize = screenSizes.xy;
    uint3 cid = ComputeClusterCoord(input.pos.xy, /*zVS=*/-mul(worldToView, float4(input.worldPos, 1)).z,
                                    screenSize, clustersX, clustersY, clustersZ, nearPlane, farPlane);
    uint clusterIndex = ComputeClusterIndex(cid, clustersX, clustersY);
		
    
	// Per-type counts for this cluster (dir, point, spot, unused)
    Index128 counts = clusterLightCount[clusterIndex]; // Index128: x=dir, y=point, z=spot
	
	// Flat index list base (packed by type in this order)
    //static const uint MAX_PER_CLUSTER = MAX_LIGHTS_PER_CLUSTER;
    uint baseOff = clusterIndex * MAX_LIGHTS_PER_CLUSTER;
    uint dirCount = counts.x;
    uint pointCount = counts.y;
    uint spotCount = counts.z;
	        	
	// Material terms for Phong
    PhongTerms pT;
    pT.N = N;
    pT.V = V;
    pT.baseColor = baseColor;
    pT.specularColor = specColor;
    pT.shininess = max(1.0, M.shininess);
    pT.specularStrength = M.specularStrength;
    pT.albedoStrength = M.albedoStrength;
    pT.ambientIntensity = M.ambientIntensity;
	
	[loop]
    for (uint i = 0; i < dirCount; ++i)
    {
        uint li = clusterLightIndices[baseOff + i].x;
        GPULight L = lights[li];
        color += ShadeDirectional(L, pT, worldToView);
    }
	
    //return float4(N, alpha)+ RetainGlobals();
    
    return float4(color, alpha) + RetainGlobals();
	


	
	
	
	
	
	// Debug shading #1: map and return normal as a color, i.e. from [-1,1]->[0,1] per component
	// The 4:th component is opacity and should be = 1
    //return float4(input.normal * 0.5 + 0.5, 1) + RetainGlobals();
	
	// Debug shading #2: map and return texture coordinates as a color (blue = 0)
	//	return float4(input.texCoord, 0, 1);
}
