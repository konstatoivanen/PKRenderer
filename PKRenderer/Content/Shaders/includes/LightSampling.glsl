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

SceneLightSample Lights_SampleAt(const uint index, const float3 world_pos, const float3 shadow_bias, const uint cascade)
{
    const SceneLight light = Lights_LoadLight(index);
    
    const float3 light_forward = Quat_MultiplyVector(light.rotation, float3(0,0,1));
    const bool is_directional = light.light_type == LIGHT_TYPE_DIRECTIONAL;

    const float3 pos_to_light = lerp(light.position - world_pos, -light.position, is_directional.xxx);

    const float dist_to_light = lerp(length(pos_to_light), 0.0f, is_directional);
    const float dist_volumetric = dist_to_light + 1e+4f * uint(is_directional); 
    const float source_radius = light.source_radius / (dist_to_light + uint(is_directional));
    const float3 L = pos_to_light / (dist_to_light + uint(is_directional));
    
    float3 radiance = light.color;
    radiance *= Lights_FalloffAttenuation(dist_to_light, light.radius);
    radiance *= Lights_ConeAttenuation(L, light_forward, light.spot_angles);

    float shadow = 1.0f;
    {
        // First Directional light has a screen space shadows.
        #if defined(SHADER_STAGE_FRAGMENT) && SHADOW_SAMPLE_SCREENSPACE == 1
        [[branch]]
        if ((light.light_type) == LIGHT_TYPE_DIRECTIONAL && light.index_shadow == 1u)
        {
            shadow = texelFetch(pk_ShadowmapScreenSpace, int2(gl_FragCoord.xy), 0).r;
        }
        else
        #endif
        [[branch]]
        if (light.index_shadow > 0u)
        {
            const uint index_matrix = light.index_shadow + cascade * uint(is_directional);
            const uint index_shadow = index_matrix - 1u;
           
            float3 shadow_sample_pos = world_pos;

            #if SHADOW_SAMPLE_VOLUMETRICS == 0
            const float2 biasFactors = Shadow_GetBiasFactors(shadow_bias, L);
            shadow_sample_pos += biasFactors.x * shadow_bias * SHADOW_NEAR_BIAS * (1.0f + cascade);
            shadow_sample_pos += biasFactors.y * L * SHADOW_NEAR_BIAS * (1.0f + cascade);
            #endif

            const float shadow_dist_rad = length(shadow_sample_pos - light.position);
            const float shadow_dist_dir = dot(light.position, shadow_sample_pos) + light.radius;
            const float shadow_dist_fin = lerp(shadow_dist_rad, shadow_dist_dir, is_directional);

            float4 shadow_uv_min_max = 0.0f.xxxx;

            [[branch]]
            if (light.light_type == LIGHT_TYPE_POINT)
            {
                //@TODO add support for uv range
                shadow_uv_min_max = EncodeOctaUv(-L).xyxy;
            }
            else
            {
                #if SHADOW_SAMPLE_VOLUMETRICS == 1
                shadow_uv_min_max = Lights_GetClipUvMinMax(shadow_sample_pos, shadow_bias, index_matrix);
                #else
                shadow_uv_min_max = Lights_GetClipUvw(shadow_sample_pos, index_matrix).xyxy;
                #endif
            }            

            #if SHADOW_SAMPLE_VOLUMETRICS == 1
            shadow = ShadowTest_Volumetrics4(index_shadow, shadow_uv_min_max, shadow_dist_fin);
            #else
            shadow = SHADOW_TEST(index_shadow, shadow_uv_min_max.xy, shadow_dist_fin);
            #endif
        }
    }
    
    return SceneLightSample(radiance, L, shadow, dist_volumetric, source_radius);
}

SceneLightSample Lights_SampleTiled(const uint index, const float3 world_pos, const float3 shadow_bias, const uint cascade) 
{ 
    return Lights_SampleAt(PK_BUFFER_DATA(pk_LightLists, index), world_pos, shadow_bias, cascade); 
}
