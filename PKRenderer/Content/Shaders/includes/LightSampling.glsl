#pragma once
#include "LightResources.glsl"
#include "Encoding.glsl"
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

// Karis 2013: https://de45xmedrsdbp.cloudfront.net/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
float Lights_FalloffAttenuation(float dist, float radius) 
{ 
    return pow2(saturate(1.0f - pow4(dist/radius))) / (pow2(dist) + 1.0f); 
}

float3 Lights_GetClipUvw(const float3 world_pos, const uint matrix_index)
{
    const float4 coord = PK_BUFFER_DATA(pk_LightMatrices, matrix_index) * float4(world_pos, 1.0f);
    return float3(ClipToUv(coord.xyw), coord.w);
}

float4 Lights_GetClipUvMinMax(const float3 world_pos, const float3 shadow_bias, const uint matrix_index)
{
    const float4x4 light_matrix = PK_BUFFER_DATA(pk_LightMatrices, matrix_index);
    float3 coord0 = (light_matrix * float4(world_pos + shadow_bias.xyz, 1.0f)).xyw;
    float3 coord1 = (light_matrix * float4(world_pos - shadow_bias.xyz, 1.0f)).xyw;
    coord0.xy = (coord0.xy / coord0.z) * 0.5f.xx + 0.5f.xx;
    coord1.xy = (coord1.xy / coord1.z) * 0.5f.xx + 0.5f.xx;
    return float4(coord0.xy, coord1.xy);
}

LightSample Lights_SampleAt(const uint index, float3 world_pos, const float3 shadow_bias, const uint cascade)
{
    const LightPacked light = Lights_LoadPacked(index);

    uint index_shadow = light.LIGHT_SHADOW;
    uint index_matrix = light.LIGHT_MATRIX; 
    float source_radius = uintBitsToFloat(light.LIGHT_PACKED_SOURCERADIUS);
    float3 color = light.LIGHT_COLOR;
    float shadow = 1.0f;

    float3 coord;
    float3 pos_to_light; 
    float shadow_distance;
    float linear_distance = 0.0f; 

    #if SHADOW_SAMPLE_VOLUMETRICS == 1
    float4 shadow_uv_min_max;
    #endif

    [[branch]]
    switch (light.LIGHT_TYPE)
    {
        case LIGHT_TYPE_DIRECTIONAL:
        {
            index_matrix += cascade;
            index_shadow += cascade;
            pos_to_light = -light.LIGHT_POS;
            
            #if SHADOW_SAMPLE_VOLUMETRICS == 0
            const float2 biasFactors = Shadow_GetBiasFactors(shadow_bias, pos_to_light);
            world_pos += biasFactors.x * shadow_bias * SHADOW_NEAR_BIAS * (1.0f + cascade);
            world_pos += biasFactors.y * pos_to_light * SHADOW_NEAR_BIAS * (1.0f + cascade);
            #endif

            coord = Lights_GetClipUvw(world_pos, index_matrix);
            shadow_distance = dot(light.LIGHT_POS, world_pos) + light.LIGHT_RADIUS;
                
            #if SHADOW_SAMPLE_VOLUMETRICS == 1
            linear_distance = 1e+4f;
            shadow_uv_min_max = Lights_GetClipUvMinMax(world_pos, shadow_bias, index_matrix);
            #endif
        }
        break;
        case LIGHT_TYPE_SPOT:
        {
            const float4 L = normalizeLength(light.LIGHT_POS - world_pos);
            color *= Lights_FalloffAttenuation(L.w, light.LIGHT_RADIUS);
            source_radius /= L.w;
            pos_to_light = L.xyz;
            shadow_distance = L.w - SHADOW_NEAR_BIAS;
            coord = Lights_GetClipUvw(world_pos, index_matrix);
            color *= step(0.0f, coord.z);
            color *= texture(pk_LightCookies, float3(coord.xy, light.LIGHT_COOKIE)).r;

            #if SHADOW_SAMPLE_VOLUMETRICS == 1
            linear_distance = L.w;
            shadow_uv_min_max = Lights_GetClipUvMinMax(world_pos, shadow_bias, index_matrix);
            #endif

        }
        break;
        case LIGHT_TYPE_POINT:
        {
            const float4 L = normalizeLength(light.LIGHT_POS - world_pos);
            color *= Lights_FalloffAttenuation(L.w, light.LIGHT_RADIUS);
            coord.xy = EncodeOctaUv(-L.xyz);
            source_radius /= L.w;
            shadow_distance = L.w - SHADOW_NEAR_BIAS;
            pos_to_light = L.xyz;
            
            #if SHADOW_SAMPLE_VOLUMETRICS == 1
            linear_distance = L.w;
            //@TODO add support for uv range
            shadow_uv_min_max = coord.xyxy;
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
    if (index_shadow < LIGHT_PARAM_INVALID)
    {
        #if SHADOW_SAMPLE_VOLUMETRICS == 1
            shadow = ShadowTest_Volumetrics4(index_shadow, shadow_uv_min_max, shadow_distance);
        #else
            shadow *= SHADOW_TEST(index_shadow, coord.xy, shadow_distance);
        #endif
    }

    return LightSample(color, shadow, pos_to_light, linear_distance, source_radius);
}

LightSample Lights_SampleTiled(const uint index, const float3 world_pos, const float3 shadow_bias, const uint cascade) 
{ 
    return Lights_SampleAt(PK_BUFFER_DATA(pk_LightLists, index), world_pos, shadow_bias, cascade); 
}
