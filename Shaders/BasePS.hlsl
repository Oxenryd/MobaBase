#include "ShaderUtils.hlsl"

#pragma ShaderType:Fragment
#pragma Name:BasePS


//-----------------------------------------------------------------------------------------
// Pixel Shader
//-----------------------------------------------------------------------------------------

float4 main(BasePSIn input) : SV_Target
{

	
    uint matIndex = basePush.matInstanceIndex != UINT_INVALID ? basePush.matInstanceIndex : 0;
    BaseMaterialInstance M = baseMatInstances[matIndex];
    
	// Albedo
	float4 albedoSample = M.textures.albedoId == UINT_INVALID
		? float4(M.baseColor, M.transparency)
		: textures[M.textures.albedoId].Sample(smp, float2(input.texCoord.x, 1.0 - input.texCoord.y));
	
    float3 baseColor = albedoSample.rgb * M.albedoStrength;
    float alpha = albedoSample.a;
	
	
	// Normals: assume IN.normal is world-space; transform to view-space if needed
    float3 Nw = normalize(input.normal);
    
    float3 N = normalize(mul((float3x3) worldToView, Nw)); // if worldToView is orthonormal, this is fine
    float3 Pw = input.worldPos;
    float3 Pvs = mul(worldToView, float4(Pw, 1)).xyz;
    float3 V = normalize(-Pvs);
	
	
	// Ambient
    float3 color = baseColor * ambient * M.ambientIntensity;
	
	
	// Forward+ cluster lookup
    float2 screenSize = float2(screenSize.x, screenSize.y);
    uint3 cid = ComputeClusterCoord(input.pos.xy, /*zVS=*/-mul(worldToView, float4(Pw, 1)).z,
                                    screenSize, clustersX, clustersY, clustersZ, nearPlane, farPlane);
    uint clusterIndex = ComputeClusterIndex(cid, clustersX, clustersY);
	
	
    float maxCluster = clustersX * clustersY * clustersZ;
	
    
    float2 testUV = input.pos.xy / screenSize;
    return float4(testUV, 0, 1) + RetainGlobals();
	
    return float4((float) clusterIndex, (float) clusterIndex, (float) clusterIndex, 1) + RetainGlobals();
	
	// Per-type counts for this cluster (dir, point, spot, unused)
    Index128 counts = clusterLightCount[clusterIndex]; // Index128: x=dir, y=point, z=spot
	
	// Flat index list base (packed by type in this order)
    //static const uint MAX_PER_CLUSTER = MAX_LIGHTS_PER_CLUSTER;
    uint baseOff = clusterIndex * MAX_LIGHTS_PER_CLUSTER;
    uint dirCount = counts.x;
    uint pointCount = counts.y;
    uint spotCount = counts.z;
	
    return float4((float) counts.x, (float) counts.y, (float) counts.z, 1.0f) + RetainGlobals();
	
	// Material terms for Phong
    PhongTerms T;
    T.N = N;
    T.V = V;
    T.baseColor = baseColor;
    T.specularColor = M.specular * M.specularStrength;
    T.shininess = max(1.0, M.shininess);
    T.specularStrength = M.specularStrength;
    T.albedoStrength = M.albedoStrength;
    T.ambientIntensity = M.ambientIntensity;
	
	[loop]
    for (uint i = 0; i < dirCount; ++i)
    {
        uint li = clusterLightIndices[baseOff + i].x;
        GPULight L = lights[li];
        color += ShadeDirectional(L, T);
        
		
		color = float3(1, 0, 0); // DEBUG!!
    }
	
    return float4(color, alpha) + RetainGlobals();
	
    //return float4(albedoSample.rgb * ambient, albedoSample.a) + RetainGlobals();
	
	
	
	
	
	
	

	
	
	
	
	
	// Debug shading #1: map and return normal as a color, i.e. from [-1,1]->[0,1] per component
	// The 4:th component is opacity and should be = 1
    //return float4(input.localNormal * 0.5 + 0.5, 1) + RetainGlobals();
	
	// Debug shading #2: map and return texture coordinates as a color (blue = 0)
	//	return float4(input.texCoord, 0, 1);
}