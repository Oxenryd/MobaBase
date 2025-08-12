#include "BaseTypes.hlsl"

// ---- Cluster helpers ----
uint ComputeZSlice(float zVS, float nearPlane, float farPlane, uint clustersZ)
{
    // zVS = positive view-space depth (distance forward)
    float t = saturate(log(zVS / nearPlane) / log(farPlane / nearPlane));
    uint slice = (uint) min((uint) (t * clustersZ), clustersZ - 1);
    return slice;
}

uint3 ComputeClusterCoord(float2 fragCoord, float zVS,
                          float2 screenSize, uint clustersX, uint clustersY, uint clustersZ,
                          float nearPlane, float farPlane)
{
    uint cx = min((uint) (fragCoord.x * clustersX / screenSize.x), clustersX - 1);
    uint cy = min((uint) (fragCoord.y * clustersY / screenSize.y), clustersY - 1);
    uint cz = ComputeZSlice(zVS, nearPlane, farPlane, clustersZ);
    return uint3(cx, cy, cz);
}

uint ComputeClusterIndex(uint3 cid, uint clustersX, uint clustersY)
{
    return cid.x + cid.y * clustersX + cid.z * clustersX * clustersY;
}

// ---- Lighting helpers (Phong/Blinn-Phong) ----
float DistanceAttenuation(float dist, float invRange, float falloffExp)
{
    // 1 at light center → 0 at range; smooth with exponent
    float x = saturate(1.0 - dist * invRange);
    return (falloffExp > 0.0) ? pow(x, falloffExp) : x;
}

float SpotAngularAtten(float3 Ldir, float3 toP, float innerCos, float outerCos)
{
    // Ldir = light.directionVS (pointing forward from light)
    float c = dot(normalize(Ldir), normalize(toP));
    // smoothstep from outer→inner
    float a = saturate((c - outerCos) / max(1e-5, innerCos - outerCos));
    return a;
}

float3 ShadeDirectional(const GPULight Lgt, PhongTerms T)
{
    float3 L = normalize(-Lgt.directionVS); // light dir towards surface
    float NdotL = saturate(dot(T.N, L));
    float3 H = normalize(L + T.V);
    float NdotH = saturate(dot(T.N, H));
    float spec = (T.specularStrength > 0.0 && T.shininess > 0.0) ? pow(NdotH, T.shininess) : 0.0;
    float3 diff = T.baseColor * NdotL;
    float3 specCol = T.specularColor * spec;
    return (diff + specCol) * (Lgt.color * Lgt.intensity);
}

float3 ShadePoint(const GPULight Lgt, float3 Pvs, PhongTerms T)
{
    float3 Lvec = Lgt.positionVS.xyz - Pvs;
    float d = length(Lvec);
    if (d <= 1e-4)
        return 0.xxx;
    float3 L = Lvec / d;

    float atten = DistanceAttenuation(d, Lgt.invRange, Lgt.falloffExp);
    if (atten <= 0.0)
        return 0.xxx;

    float NdotL = saturate(dot(T.N, L));
    float3 H = normalize(L + T.V);
    float NdotH = saturate(dot(T.N, H));
    float spec = (T.specularStrength > 0.0 && T.shininess > 0.0) ? pow(NdotH, T.shininess) : 0.0;

    float3 diff = T.baseColor * NdotL;
    float3 specCol = T.specularColor * spec;

    return (diff + specCol) * (Lgt.color * Lgt.intensity) * atten;
}

float3 ShadeSpot(const GPULight Lgt, float3 Pvs, PhongTerms T)
{
    float3 toP = Pvs - Lgt.positionVS.xyz; // from light to point (view space)
    float d = length(toP);
    if (d <= 1e-4)
        return 0.xxx;
    float3 L = -toP / d; // towards light

    float atten = DistanceAttenuation(d, Lgt.invRange, Lgt.falloffExp);
    if (atten <= 0.0)
        return 0.xxx;

    // Angular attenuation
    float ang = SpotAngularAtten(Lgt.directionVS, -toP, Lgt.spotInnerCos, Lgt.spotOuterCos);
    if (ang <= 0.0)
        return 0.xxx;

    float NdotL = saturate(dot(T.N, L));
    float3 H = normalize(L + T.V);
    float NdotH = saturate(dot(T.N, H));
    float spec = (T.specularStrength > 0.0 && T.shininess > 0.0) ? pow(NdotH, T.shininess) : 0.0;

    float3 diff = T.baseColor * NdotL;
    float3 specCol = T.specularColor * spec;

    return (diff + specCol) * (Lgt.color * Lgt.intensity) * (atten * ang);
}