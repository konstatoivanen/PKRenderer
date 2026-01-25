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

float Lights_ConeAttenuation(float3 L, float3 D, float2 A)
{
    return pow2(saturate((dot(L, -D) - A.x) * A.y));
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

SceneLightSample Lights_SampleAt(const uint index, float3 world_pos, const float3 shadow_bias, const uint cascade)
{
    const SceneLight light = Lights_LoadLight(index);
    
    const float3 light_forward = Quat_MultiplyVector(light.rotation, float3(0,0,1));
    const float4 L = NormalizeLength(light.position - world_pos);
    
    uint index_shadow = light.index_shadow;
    uint index_matrix = light.index_matrix; 
    float source_radius = light.source_radius;
    float3 radiance = light.color;

    // Default assumption directional light.
    float3 pos_to_light = 0.0f.xxx; 
    float4 shadow_uv_min_max = 0.0f.xxxx;
    float shadow_distance = 0.0f;
    float linear_distance = 0.0f; 

    [[branch]]
    if (light.light_type == LIGHT_TYPE_DIRECTIONAL)
    {
        pos_to_light = -light.position;
        index_matrix += cascade;
        index_shadow += cascade;

        #if SHADOW_SAMPLE_VOLUMETRICS == 0
        const float2 biasFactors = Shadow_GetBiasFactors(shadow_bias, pos_to_light);
        world_pos += biasFactors.x * shadow_bias * SHADOW_NEAR_BIAS * (1.0f + cascade);
        world_pos += biasFactors.y * pos_to_light * SHADOW_NEAR_BIAS * (1.0f + cascade);
        #endif

        shadow_distance = dot(light.position, world_pos) + light.radius;
        linear_distance = 1e+4f;
    }
    else
    {
        radiance *= Lights_FalloffAttenuation(L.w, light.radius);
        radiance *= Lights_ConeAttenuation(L.xyz, light_forward, light.spot_angles);
        source_radius /= L.w;
        shadow_distance = L.w - SHADOW_NEAR_BIAS;
        pos_to_light = L.xyz;
        linear_distance = L.w;
    }

    [[branch]]
    if (light.light_type == LIGHT_TYPE_POINT)
    {
        //@TODO add support for uv range
        shadow_uv_min_max = EncodeOctaUv(-L.xyz).xyxy;
    }
    else
    {
        shadow_uv_min_max = Lights_GetClipUvw(world_pos, index_matrix).xyxy;
        #if SHADOW_SAMPLE_VOLUMETRICS == 1
        shadow_uv_min_max = Lights_GetClipUvMinMax(world_pos, shadow_bias, index_matrix);
        #endif
    }

    float shadow = 1.0f;
    {
        // First Directional light has a screen space shadows.
        #if defined(SHADER_STAGE_FRAGMENT) && SHADOW_SAMPLE_SCREENSPACE == 1
        [[branch]]
        if ((light.light_type) == LIGHT_TYPE_DIRECTIONAL && light.index_shadow == 0u)
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
                shadow *= SHADOW_TEST(index_shadow, shadow_uv_min_max.xy, shadow_distance);
            #endif
        }
    }
    
    return SceneLightSample(radiance, pos_to_light, shadow, linear_distance, source_radius);
}

SceneLightSample Lights_SampleTiled(const uint index, const float3 world_pos, const float3 shadow_bias, const uint cascade) 
{ 
    return Lights_SampleAt(PK_BUFFER_DATA(pk_LightLists, index), world_pos, shadow_bias, cascade); 
}
