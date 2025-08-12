#include "BaseTypes.hlsl"

#pragma ShaderType:Compute
#pragma Name:LightClusterCS


float GetLogDepthSlice(float z, float near, float far, float logBase)
{
    // Convert z from [0,1] to actual depth slice boundaries
    float ratio = far / near;
    float logRatio = log(ratio) / log(logBase);
    float depth = near * pow(logBase, z * logRatio);
    return depth;
}

ClusterFrustum BuildClusterFrustum(uint3 cid)
{
    ClusterFrustum frustum;
    
    // Calculate normalized cluster coordinates
    float3 minCoord = float3(cid) / float3(clustersX, clustersY, clustersZ);
    float3 maxCoord = float3(cid + 1) / float3(clustersX, clustersY, clustersZ);
    
    // Convert to NDC space (-1 to 1)
    float2 minNDC = minCoord.xy * 2.0 - 1.0;
    float2 maxNDC = maxCoord.xy * 2.0 - 1.0;
    
    // Get depth slice boundaries (using logarithmic distribution)
    float nearZ = GetLogDepthSlice(minCoord.z, nearPlane, farPlane, clusterLogBase);
    float farZ = GetLogDepthSlice(maxCoord.z, nearPlane, farPlane, clusterLogBase);
    
    // Build frustum corners in view space
    float4 nearCorners[4];
    float4 farCorners[4];
    
    // Use inverse projection to get view space positions
    float4 ndcCorners[8] =
    {
        float4(minNDC.x, minNDC.y, 0, 1), // near bottom-left
        float4(maxNDC.x, minNDC.y, 0, 1), // near bottom-right
        float4(minNDC.x, maxNDC.y, 0, 1), // near top-left
        float4(maxNDC.x, maxNDC.y, 0, 1), // near top-right
        float4(minNDC.x, minNDC.y, 1, 1), // far bottom-left
        float4(maxNDC.x, minNDC.y, 1, 1), // far bottom-right
        float4(minNDC.x, maxNDC.y, 1, 1), // far top-left
        float4(maxNDC.x, maxNDC.y, 1, 1) // far top-right
    };
    
    // Transform to view space
    for (int i = 0; i < 4; i++)
    {
        nearCorners[i] = mul(invProjection, ndcCorners[i]);
        nearCorners[i] /= nearCorners[i].w;
        nearCorners[i].z = -nearZ; // Set actual near depth
        
        farCorners[i] = mul(invProjection, ndcCorners[i + 4]);
        farCorners[i] /= farCorners[i].w;
        farCorners[i].z = -farZ; // Set actual far depth
    }
    
    // Build frustum planes (normals point inward)
    // Left plane
    float3 v0 = nearCorners[0].xyz;
    float3 v1 = nearCorners[2].xyz;
    float3 v2 = farCorners[0].xyz;
    float3 normal = normalize(cross(v1 - v0, v2 - v0));
    frustum.planes[0] = float4(normal, -dot(normal, v0));
    
    // Right plane
    v0 = nearCorners[1].xyz;
    v1 = farCorners[1].xyz;
    v2 = nearCorners[3].xyz;
    normal = normalize(cross(v1 - v0, v2 - v0));
    frustum.planes[1] = float4(normal, -dot(normal, v0));
    
    // Bottom plane
    v0 = nearCorners[0].xyz;
    v1 = farCorners[0].xyz;
    v2 = nearCorners[1].xyz;
    normal = normalize(cross(v1 - v0, v2 - v0));
    frustum.planes[2] = float4(normal, -dot(normal, v0));
    
    // Top plane
    v0 = nearCorners[2].xyz;
    v1 = nearCorners[3].xyz;
    v2 = farCorners[2].xyz;
    normal = normalize(cross(v1 - v0, v2 - v0));
    frustum.planes[3] = float4(normal, -dot(normal, v0));
    
    // Near plane
    frustum.planes[4] = float4(0, 0, 1, nearZ);
    
    // Far plane
    frustum.planes[5] = float4(0, 0, -1, -farZ);
    
    return frustum;
}

bool SphereIntersectsFrustum(float3 center, float radius, ClusterFrustum frustum)
{
    // Transform to view space
    float4 viewPos = mul(worldToView, float4(center, 1.0));
    
    for (int i = 0; i < 6; i++)
    {
        float distance = dot(frustum.planes[i].xyz, viewPos.xyz) + frustum.planes[i].w;
        if (distance > radius)
            return false;
    }
    return true;
}

bool ConeIntersectsFrustum(float3 apex, float3 direction, float height, float angle, ClusterFrustum frustum)
{
    // Simple sphere approximation for cone
    float radius = height * tan(angle);
    float3 center = apex + direction * (height * 0.5);
    float boundRadius = length(float2(radius, height * 0.5));
    return SphereIntersectsFrustum(center, boundRadius, frustum);
}

bool LightIntersectsFrustum(GPULight light, ClusterFrustum frustum)
{
    if (light.type == 0) // Directional light
    {
        // Directional lights affect everything
        return true;
    }
    else if (light.type == 1) // Point light
    {
        return SphereIntersectsFrustum(light.posVS_radius.rgb, light.posVS_radius.a, frustum);
    }
    else if (light.type == 2) // Spot light
    {
        // Use cone intersection for spot lights
        return ConeIntersectsFrustum(light.posVS_radius.rgb, light.dirVS_spotInnerCos.rgb,
                                    light.posVS_radius.a, light.spotOuterCos, frustum);
    }
    return false;
}

//[numthreads(CLUSTER_THREADS_X, CLUSTER_THREADS_Y, CLUSTER_THREADS_Z)]
[numthreads(1, 1, 1)]
void main(uint3 gid : SV_GroupID, uint3 tid : SV_GroupThreadID)
{
    // Calculate actual cluster ID
    uint3 cid = gid * uint3(1, 1, 1) + tid;
    
    // Check bounds
    if (any(cid >= uint3(clustersX, clustersY, clustersZ)))
        return;
    
    uint cluster = cid.x + cid.y * clustersX + cid.z * clustersX * clustersY;
       
    
    // Build frustum once for this cluster
    ClusterFrustum frustum = BuildClusterFrustum(cid);
    
    // Count lights per type
    uint dirCount = 0, pointCount = 0, spotCount = 0;
    
    for (uint i = 0; i < numLights; ++i)
    {
        if (LightIntersectsFrustum(lights[i], frustum))
        {
            uint lightType = lights[i].type;
            dirCount += (lightType == 0);
            pointCount += (lightType == 1);
            spotCount += (lightType == 2);
        }
    }
    
    // Clamp to capacity
    dirCount = min(dirCount, MAX_LIGHTS_PER_CLUSTER);
    pointCount = min(pointCount, MAX_LIGHTS_PER_CLUSTER - dirCount);
    spotCount = min(spotCount, MAX_LIGHTS_PER_CLUSTER - dirCount - pointCount);
    
    // Calculate base indices for each light type
    uint base = cluster * MAX_LIGHTS_PER_CLUSTER;
    uint dirBase = base;
    uint pointBase = base + dirCount;
    uint spotBase = base + dirCount + pointCount;
    
    // Write light indices grouped by type
    uint dirWritten = 0, pointWritten = 0, spotWritten = 0;
    
    for (uint i = 0; i < numLights; ++i)
    {
        if (LightIntersectsFrustum(lights[i], frustum))
        {
            uint lightType = lights[i].type;
            
            if (lightType == 0 && dirWritten < dirCount)
            {
                clusterLightIndices[dirBase + dirWritten].x = i;
                dirWritten++;
            }
            else if (lightType == 1 && pointWritten < pointCount)
            {
                clusterLightIndices[pointBase + pointWritten].x = i;
                pointWritten++;
            }
            else if (lightType == 2 && spotWritten < spotCount)
            {
                clusterLightIndices[spotBase + spotWritten].x = i;
                spotWritten++;
            }
        }
    }
    
    // Store light counts (x=directional, y=point, z=spot, w=reserved)
    uint4 result = uint4(dirCount, pointCount, spotCount, 0);
    clusterLightCount[cluster].x = result.x;
    clusterLightCount[cluster].y = result.y;
    clusterLightCount[cluster].z = result.z;
    clusterLightCount[cluster].w = result.w;
}

