#pragma once
#include LightResources.glsl
#include Encoding.glsl
#include BRDF.glsl

#define SHADOW_USE_LBR 
#define SHADOW_LBR 0.2f
#define SHADOWMAP_CASCADES 4

#if defined(SHADOW_USE_LBR)
    float LBR(float shadow) { return smoothstep(SHADOW_LBR, 1.0f, shadow); }
#else
    #define LBR(shadow) (shadow)
#endif

float SampleLightShadowmap(const uint shadowmapIndex, const float2 uv, const float lightDistance)
{
    const float2 moments = tex2D(pk_ShadowmapAtlas, float3(uv, shadowmapIndex)).xy;
    const float variance = moments.y - moments.x * moments.x;
    const float difference = lightDistance - moments.x;
    return min(LBR(variance / (variance + pow2(difference))) + step(difference, 0.1f), 1.0f);
}

float4 GetLightProjectionUVW(const float3 worldpos, const uint projectionIndex)
{
    float4 coord = mul(PK_BUFFER_DATA(pk_LightMatrices, projectionIndex), float4(worldpos, 1.0f));
    coord.xy = (coord.xy / coord.w) * 0.5f.xx + 0.5f.xx; 
    return coord;
}

Light GetLightDirect(const uint index, const float3 worldpos, const uint cascade)
{
    const LightPacked light = PK_BUFFER_DATA(pk_Lights, index);

    uint index_shadow = light.LIGHT_SHADOW;
    uint index_matrix  = light.LIGHT_PROJECTION;
    
    float sourceRadius = uintBitsToFloat(light.LIGHT_PACKED_SOURCERADIUS);
    float3 color = light.LIGHT_COLOR;
    float shadow = 1.0f;

    float2 lightuv;
    float4 posToLight; 
    float linearDistance;

    // @TODO Maybe refactor lights to separate by type lists 
    [[branch]]
    switch (light.LIGHT_TYPE)
    {
        case LIGHT_TYPE_POINT:
        {
            posToLight = normalizeLength(light.LIGHT_POS - worldpos);
            color *= Fatten_Default(posToLight.w, light.LIGHT_RADIUS);
            lightuv = OctaEncode(-posToLight.xyz);
            linearDistance = posToLight.w;
            sourceRadius /= linearDistance;
        }
        break;
        case LIGHT_TYPE_SPOT:
        {
            posToLight = normalizeLength(light.LIGHT_POS - worldpos);
            color *= Fatten_Default(posToLight.w, light.LIGHT_RADIUS);
            linearDistance = posToLight.w;
            sourceRadius /= linearDistance;

            const float4 coord = GetLightProjectionUVW(worldpos, index_matrix);
            lightuv = coord.xy;
            color *= step(0.0f, coord.z);
            color *= tex2D(pk_LightCookies, float3(lightuv, light.LIGHT_COOKIE)).r;
        }
        break;
        case LIGHT_TYPE_DIRECTIONAL:
        {
            index_matrix += cascade;
            index_shadow += cascade;
            posToLight.xyz = -light.LIGHT_POS;

            const float4 coord = GetLightProjectionUVW(worldpos, index_matrix);
            posToLight.w = (coord.z / coord.w) * light.LIGHT_RADIUS;
            lightuv = coord.xy;
            linearDistance = 1e+4f;
        }
        break;
    }

    [[branch]]
    if (index_shadow < LIGHT_PARAM_INVALID)
    {
        shadow *= SampleLightShadowmap(index_shadow, lightuv, posToLight.w);
    }

    return Light(color, shadow, posToLight.xyz, linearDistance, sourceRadius);
}

Light GetLight(uint index, in float3 worldpos, uint cascade) { return GetLightDirect(PK_BUFFER_DATA(pk_LightLists, index), worldpos, cascade); }
