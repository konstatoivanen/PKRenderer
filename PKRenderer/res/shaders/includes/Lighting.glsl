#pragma once
#include LightResources.glsl
#include Encoding.glsl
#include BRDF.glsl
#include Shadows.glsl

#ifndef SHADOW_TEST 
    #define SHADOW_TEST ShadowTest_PCSS
#endif

float4 GetLightProjectionUVW(const float3 worldpos, const uint projectionIndex)
{
    float4 coord = mul(PK_BUFFER_DATA(pk_LightMatrices, projectionIndex), float4(worldpos, 1.0f));
    coord.xy = (coord.xy / coord.w) * 0.5f.xx + 0.5f.xx;
    // Light depth test uses reverse z. Reverse range for actual distance.
    coord.z = 1.0f - (coord.z / coord.w);
    return coord;
}

Light GetLightDirect(const uint index, const float3 worldpos, const float3 normal, const uint cascade)
{
    const LightPacked light = Lights_LoadPacked(index);

    uint index_shadow = light.LIGHT_SHADOW;
    uint index_matrix  = light.LIGHT_PROJECTION;
    
    float sourceRadius = uintBitsToFloat(light.LIGHT_PACKED_SOURCERADIUS);
    float3 color = light.LIGHT_COLOR;
    float shadow = 1.0f;

    float4 coord;
    float3 posToLight; 
    float shadowDistance;
    // This is only needed for volumetrics
    float linearDistance; 

    // @TODO Maybe refactor lights to separate by type lists 
    [[branch]]
    switch (light.LIGHT_TYPE)
    {
        case LIGHT_TYPE_DIRECTIONAL:
        {
            index_matrix += cascade;
            index_shadow += cascade;
            linearDistance = 1e+4f;
            posToLight = -light.LIGHT_POS;

            const float3 shadowPos = worldpos + Shadow_GetSamplingOffset(normal, posToLight) * (1.0f + cascade);
            coord = GetLightProjectionUVW(shadowPos, index_matrix);
            
            shadowDistance = coord.z * light.LIGHT_RADIUS;
        }
        break;
        case LIGHT_TYPE_SPOT:
        {
            const float4 L = normalizeLength(light.LIGHT_POS - worldpos);
            color *= Fatten_Default(L.w, light.LIGHT_RADIUS);
            linearDistance = L.w;
            sourceRadius /= L.w;
            posToLight = L.xyz;
            shadowDistance = L.w - SHADOW_NEAR_BIAS;

            coord = GetLightProjectionUVW(worldpos, index_matrix);
            color *= step(0.0f, coord.w);
            color *= texture(pk_LightCookies, float3(coord.xy, light.LIGHT_COOKIE)).r;
        }
        break;
        case LIGHT_TYPE_POINT:
        {
            const float4 L = normalizeLength(light.LIGHT_POS - worldpos);
            color *= Fatten_Default(L.w, light.LIGHT_RADIUS);
            coord.xy = OctaEncode(-L.xyz);
            linearDistance = L.w;
            sourceRadius /= L.w;
            shadowDistance = L.w - SHADOW_NEAR_BIAS;
            posToLight = L.xyz;
        }
        break;
    }

    [[branch]]
    if (index_shadow < LIGHT_PARAM_INVALID)
    {
        shadow *= SHADOW_TEST(index_shadow, coord.xy, shadowDistance);
    }

    return Light(color, shadow, posToLight, linearDistance, sourceRadius);
}

Light GetLight(const uint index, const float3 worldpos, const float3 normal, const uint cascade) 
{ 
    return GetLightDirect(PK_BUFFER_DATA(pk_LightLists, index), worldpos, normal, cascade); 
}
