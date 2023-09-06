#pragma once
#include SharedLights.glsl
#include ClusterIndexing.glsl
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

float SampleLightShadowmap(uint shadowmapIndex, float2 uv, float lightDistance)
{
    float2 moments = tex2D(pk_ShadowmapAtlas, float3(uv, shadowmapIndex)).xy;
    float variance = moments.y - moments.x * moments.x;
    float difference = lightDistance - moments.x;
    return min(LBR(variance / (variance + difference * difference)) + step(difference, 0.1f), 1.0f);
}

float4 GetLightProjectionUVW(in float3 worldpos, uint projectionIndex)
{
    float4 coord = mul(PK_BUFFER_DATA(pk_LightMatrices, projectionIndex), float4(worldpos, 1.0f));
    coord.xy = (coord.xy * 0.5f + coord.ww * 0.5f) / coord.w;
    return coord;
}

Light GetLightDirect(uint index, in float3 worldpos, uint cascade)
{
    PK_Light light = PK_BUFFER_DATA(pk_Lights, index);
    float3 color = light.color.rgb;
    float shadow = 1.0f;

    float2 lightuv;
    float3 posToLight; 
    float linearDistance;

    // @TODO Maybe refactor lights to separate by type lists 
    switch (light.LIGHT_TYPE)
    {
        case LIGHT_TYPE_POINT:
        {
            posToLight = light.position.xyz - worldpos;
            linearDistance = length(posToLight);
            color *= AttenuateLight(linearDistance, light.position.w);
            posToLight /= linearDistance;
            lightuv = OctaEncode(-posToLight);
        }
        break;
        case LIGHT_TYPE_SPOT:
        {
            posToLight = light.position.xyz - worldpos;
            linearDistance = length(posToLight);
            color *= AttenuateLight(linearDistance, light.position.w);
            posToLight /= linearDistance;

            float3 coord = GetLightProjectionUVW(worldpos, light.LIGHT_PROJECTION).xyz;
            lightuv = coord.xy;
            color *= step(0.0f, coord.z);
            color *= tex2D(pk_LightCookies, float3(lightuv, light.LIGHT_COOKIE)).r;
        }
        break;
        case LIGHT_TYPE_DIRECTIONAL:
        {
            light.LIGHT_PROJECTION += cascade;
            light.LIGHT_SHADOW += cascade;
            posToLight = -light.position.xyz;

            float4 coord = GetLightProjectionUVW(worldpos, light.LIGHT_PROJECTION);
            // @TODO UNormDepthFIX
            linearDistance = ((coord.z / coord.w) + 1.0f) * light.position.w * 0.5f;
            lightuv = coord.xy;
        }
        break;
    }

    if (light.LIGHT_SHADOW < LIGHT_PARAM_INVALID)
    {
        shadow *= SampleLightShadowmap(light.LIGHT_SHADOW, lightuv, linearDistance);
    }

    return Light(color, shadow, posToLight, linearDistance);
}

Light GetLight(uint index, in float3 worldpos, uint cascade) { return GetLightDirect(PK_BUFFER_DATA(pk_GlobalLightsList, index), worldpos, cascade); }

LightTile GetLightTile(float2 uv, float viewDepth) { return GetLightTile(GetTileIndexUV(uv, viewDepth)); }
LightTile GetLightTile(float3 clipuvw) { return GetLightTile(clipuvw.xy, ViewDepth(clipuvw.z)); }
