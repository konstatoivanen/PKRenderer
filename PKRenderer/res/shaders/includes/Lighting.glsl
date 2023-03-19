#pragma once
#include SharedLights.glsl
#include ClusterIndexing.glsl
#include Encoding.glsl

#define SHADOW_USE_LBR 
#define SHADOW_LBR 0.2f
#define SHADOWMAP_CASCADES 4

//----------MATH UTILITIES----------//

#if defined(SHADOW_USE_LBR)
    float LBR(float shadow) 
    { 
        return smoothstep(SHADOW_LBR, 1.0f, shadow);
    }
#else
    #define LBR(shadow) (shadow)
#endif

// Source: https://de45xmedrsdbp.cloudfront.net/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
float GetAttenuation(float ldist, float lradius) 
{
    return pow2(saturate(1.0f - pow4(ldist/lradius))) / (pow2(ldist) + 1.0f); 
}

float GetLightAnisotropy(float3 viewdir, float3 posToLight, float anistropy)
{
	float gsq = pow2(anistropy);
	float denom = 1.0 + gsq - 2.0 * anistropy * dot(viewdir, posToLight);
	return (1.0 - gsq) * inversesqrt(max(0, pow3(denom)));
}


//----------INDIRECT SAMPLERS----------//
float3 SampleEnvironment(float2 uv, float roughness) 
{ 
    return HDRDecode(tex2DLod(pk_SceneOEM_HDR, uv, roughness * 4)) * pk_SceneOEM_Exposure; 
}

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


//----------LIGHT INDEXING----------//
void GetLight(uint index, in float3 worldpos, uint cascade, out float3 color, out float3 posToLight, out float shadow)
{
    PK_Light light = PK_BUFFER_DATA(pk_Lights, index);
    color = light.color.rgb;
    shadow = 1.0f;

    float2 lightuv;
    float linearDistance;

    // @TODO Maybe refactor lights to separate by type lists 
    switch (light.LIGHT_TYPE)
    {
        case LIGHT_TYPE_POINT:
        {
            posToLight = light.position.xyz - worldpos;
            linearDistance = length(posToLight);
            color *= GetAttenuation(linearDistance, light.position.w);
            posToLight /= linearDistance;
            lightuv = OctaEncode(-posToLight);
        }
        break;
        case LIGHT_TYPE_SPOT:
        {
            posToLight = light.position.xyz - worldpos;
            linearDistance = length(posToLight);
            color *= GetAttenuation(linearDistance, light.position.w);
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
            linearDistance = ((coord.z / coord.w) + 1.0f) * light.position.w * 0.5f;
            lightuv = coord.xy;
        }
        break;
    }

    if (light.LIGHT_SHADOW < LIGHT_PARAM_INVALID)
    {
        shadow *= SampleLightShadowmap(light.LIGHT_SHADOW, lightuv, linearDistance);
    }
}


Light GetSurfaceLightDirect(uint index, in float3 worldpos, uint cascade)
{
    float3 posToLight, color;
    float shadow;
    GetLight(index, worldpos, cascade, color, posToLight, shadow);
    return Light(color, posToLight, shadow);
}

Light GetSurfaceLight(uint index, in float3 worldpos, uint cascade)
{
    float3 posToLight, color;
    float shadow;
    GetLight(PK_BUFFER_DATA(pk_GlobalLightsList, index), worldpos, cascade, color, posToLight, shadow);
    return Light(color, posToLight, shadow);
}

float3 GetVolumeLightColor(uint index, in float3 worldpos, float3 viewdir, uint cascade, float anisotropy)
{
    float3 posToLight, color;
    float shadow;
    GetLight(PK_BUFFER_DATA(pk_GlobalLightsList, index), worldpos, cascade, color, posToLight, shadow);
    return color * shadow * GetLightAnisotropy(viewdir, posToLight, anisotropy);
}

LightTile GetLightTile(float3 clipuvw) 
{
    return GetLightTile(GetTileIndexUV(clipuvw.xy, LinearizeDepth(clipuvw.z))); 
}
