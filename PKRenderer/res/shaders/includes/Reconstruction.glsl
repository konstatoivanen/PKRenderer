#pragma once
#ifndef PK_RECONSTRUCTION
#define PK_RECONSTRUCTION

#include Common.glsl

float3 UnpackNormal(in float3 packedNormal) { return packedNormal * 2.0f - float3(1.0f); }

float3 SampleNormal(in sampler2D map, in float3x3 rotation, in float2 uv, float amount)
{
    float3 normal = UnpackNormal(tex2D(map, uv).xyz);
    normal = lerp(float3(0,0,1), normal, amount);
    return normalize(mul(rotation, normal));
}

float2 ParallaxOffset(float height, float heightAmount, float3 viewdir) { return (height * heightAmount - heightAmount / 2.0f) * (viewdir.xy / (viewdir.z + 0.42f)); }

float SampleRoughness(float2 uv) { return tex2D(pk_ScreenNormals, uv).w; }

float SampleRoughness(int2 coord) { return texelFetch(pk_ScreenNormals, coord, 0).w; }

float3 SampleViewSpaceNormal(float2 uv) { return tex2D(pk_ScreenNormals, uv).xyz; }

float3 SampleViewSpaceNormal(int2 coord) { return texelFetch(pk_ScreenNormals, coord, 0).xyz; }

float3 SampleWorldSpaceNormal(float2 uv) { return mul(float3x3(pk_MATRIX_I_V), SampleViewSpaceNormal(uv)); }

float3 SampleWorldSpaceNormal(int2 coord) { return mul(float3x3(pk_MATRIX_I_V), SampleViewSpaceNormal(coord)); }

float4 SampleWorldSpaceNormalRoughness(float2 uv) 
{ 
    float4 value = tex2D(pk_ScreenNormals, uv);
    value.xyz = mul(float3x3(pk_MATRIX_I_V), value.xyz);
    return value;
}

float4 SampleWorldSpaceNormalRoughness(int2 coord) 
{ 
    float4 value = texelFetch(pk_ScreenNormals, coord, 0);
    value.xyz = mul(float3x3(pk_MATRIX_I_V), value.xyz);
    return value;
}

float3 SampleViewPosition(float2 uv)
{
    float depth = SampleLinearDepth(uv);
    return ClipToViewPos(uv, depth);
}

float3 SampleViewPosition(int2 coord, int2 size)
{
    float depth = SampleLinearDepth(coord);
    return ClipToViewPos((coord + 0.5f.xx) / float2(size), depth);
}

float3 SampleViewPosition(int2 coord, int2 size, float linearDepth)
{
    return ClipToViewPos((coord + 0.5f.xx) / float2(size), linearDepth);
}

float3 SampleWorldPosition(float2 uv) { return mul(pk_MATRIX_I_V, float4(SampleViewPosition(uv), 1.0f)).xyz; }

float3 SampleWorldPosition(int2 coord, int2 size) { return mul(pk_MATRIX_I_V, float4(SampleViewPosition(coord, size), 1.0f)).xyz; }

float3 GetFragmentClipUVW()
{
    #if defined(SHADER_STAGE_FRAGMENT)
        return float3(gl_FragCoord.xy * pk_ScreenParams.zw, gl_FragCoord.z);
    #else
        return 0.0f.xxx;
    #endif
}

#endif