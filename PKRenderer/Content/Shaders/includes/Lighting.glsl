#pragma once
#include "LightResources.glsl"
#include "Encoding.glsl"
#include "BRDF.glsl"
#include "Shadows.glsl"

#ifndef SHADOW_TEST 
    #define SHADOW_TEST ShadowTest_Dither16
#endif

#ifndef SHADOW_SAMPLE_SCREENSPACE
    #define SHADOW_SAMPLE_SCREENSPACE 1
#endif

#ifndef SHADOW_SAMPLE_VOLUMETRICS
    #define SHADOW_SAMPLE_VOLUMETRICS 0
#endif

#ifndef SHADOW_SAMPLE_VOLUMETRICS_COUNT
    #define SHADOW_SAMPLE_VOLUMETRICS_COUNT 4
#endif

// Light depth test uses reverse z. Reverse range for actual distance.
float2 LightClipToUV(const float4 clip) 
{
    return fma((clip.xy / clip.w), 0.5f.xx, 0.5f.xx); 
}

float3 GetLightClipUVW(const float3 worldpos, const uint matrixIndex)
{
    float4 coord = PK_BUFFER_DATA(pk_LightMatrices, matrixIndex) * float4(worldpos, 1.0f);
    return float3(LightClipToUV(coord), coord.w);
}

float4 GetLightClipUVMinMax(const float3 worldpos, const float3 shadowBias, const uint matrixIndex)
{
    const float4x4 lightMatrix = PK_BUFFER_DATA(pk_LightMatrices, matrixIndex);
    float3 coord0 = (lightMatrix * float4(worldpos + shadowBias.xyz, 1.0f)).xyw;
    float3 coord1 = (lightMatrix * float4(worldpos - shadowBias.xyz, 1.0f)).xyw;
    coord0.xy = (coord0.xy / coord0.z) * 0.5f.xx + 0.5f.xx;
    coord1.xy = (coord1.xy / coord1.z) * 0.5f.xx + 0.5f.xx;
    return float4(coord0.xy, coord1.xy);
}

Light GetLightDirect(const uint index, float3 worldpos, const float3 shadowBias, const uint cascade)
{
    const LightPacked light = Lights_LoadPacked(index);

    uint indexShadow = light.LIGHT_SHADOW;
    uint indexMatrix = light.LIGHT_MATRIX; 
    float sourceRadius = uintBitsToFloat(light.LIGHT_PACKED_SOURCERADIUS);
    float3 color = light.LIGHT_COLOR;
    float shadow = 1.0f;

    float3 coord;
    float3 posToLight; 
    float shadowDistance;
    float linearDistance = 0.0f; 

    #if SHADOW_SAMPLE_VOLUMETRICS == 1
    float4 shadowUVMinMax;
    #endif

    [[branch]]
    switch (light.LIGHT_TYPE)
    {
        case LIGHT_TYPE_DIRECTIONAL:
        {
            indexMatrix += cascade;
            indexShadow += cascade;
            posToLight = -light.LIGHT_POS;
            
            #if SHADOW_SAMPLE_VOLUMETRICS == 0
            const float2 biasFactors = Shadow_GetBiasFactors(shadowBias, posToLight);
            worldpos += biasFactors.x * shadowBias * SHADOW_NEAR_BIAS * (1.0f + cascade);
            worldpos += biasFactors.y * posToLight * SHADOW_NEAR_BIAS * (1.0f + cascade);
            #endif

            coord = GetLightClipUVW(worldpos, indexMatrix);
            shadowDistance = dot(light.LIGHT_POS, worldpos) + light.LIGHT_RADIUS;
                
            #if SHADOW_SAMPLE_VOLUMETRICS == 1
            linearDistance = 1e+4f;
            shadowUVMinMax = GetLightClipUVMinMax(worldpos, shadowBias, indexMatrix);
            #endif
        }
        break;
        case LIGHT_TYPE_SPOT:
        {
            const float4 L = normalizeLength(light.LIGHT_POS - worldpos);
            color *= Fatten_Default(L.w, light.LIGHT_RADIUS);
            sourceRadius /= L.w;
            posToLight = L.xyz;
            shadowDistance = L.w - SHADOW_NEAR_BIAS;
            coord = GetLightClipUVW(worldpos, indexMatrix);
            color *= step(0.0f, coord.z);
            color *= texture(pk_LightCookies, float3(coord.xy, light.LIGHT_COOKIE)).r;

            #if SHADOW_SAMPLE_VOLUMETRICS == 1
            linearDistance = L.w;
            shadowUVMinMax = GetLightClipUVMinMax(worldpos, shadowBias, indexMatrix);
            #endif

        }
        break;
        case LIGHT_TYPE_POINT:
        {
            const float4 L = normalizeLength(light.LIGHT_POS - worldpos);
            color *= Fatten_Default(L.w, light.LIGHT_RADIUS);
            coord.xy = OctaEncode(-L.xyz);
            sourceRadius /= L.w;
            shadowDistance = L.w - SHADOW_NEAR_BIAS;
            posToLight = L.xyz;
            
            #if SHADOW_SAMPLE_VOLUMETRICS == 1
            linearDistance = L.w;
            //@TODO add support for uv range
            shadowUVMinMax = coord.xyxy;
            #endif
        }
        break;
    }

    // First Directional light has a screen space shadows.
#if defined(SHADER_STAGE_FRAGMENT) && SHADOW_SAMPLE_SCREENSPACE == 1
    [[branch]]
    if ((light.LIGHT_TYPE) == LIGHT_TYPE_DIRECTIONAL && (light.LIGHT_SHADOW) == 0u)
    {
        shadow *= texelFetch(pk_ShadowmapScreenSpace, int2(gl_FragCoord.xy), 0).r;
    }
    else
#endif
    [[branch]]
    if (indexShadow < LIGHT_PARAM_INVALID)
    {
        #if SHADOW_SAMPLE_VOLUMETRICS == 1
            shadow = ShadowTest_Volumetrics4(indexShadow, shadowUVMinMax, shadowDistance);
        #else
            shadow *= SHADOW_TEST(indexShadow, coord.xy, shadowDistance);
        #endif
    }

    return Light(color, shadow, posToLight, linearDistance, sourceRadius);
}

Light GetLight(const uint index, const float3 worldpos, const float3 shadowBias, const uint cascade) 
{ 
    return GetLightDirect(PK_BUFFER_DATA(pk_LightLists, index), worldpos, shadowBias, cascade); 
}