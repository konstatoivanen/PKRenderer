
//Source: http://advances.realtimerendering.com/s2014/sledgehammer/Next-Generation-Post-Processing-in-Call-of-Duty-Advanced-Warfare-v17.pptx (page 139)

#pragma pk_multi_compile PASS_DOWNSAMPLE0 PASS_DOWNSAMPLE1 PASS_UPSAMPLE
#pragma pk_program SHADER_STAGE_COMPUTE Downsample0Cs PASS_DOWNSAMPLE0
#pragma pk_program SHADER_STAGE_COMPUTE Downsample1Cs PASS_DOWNSAMPLE1
#pragma pk_program SHADER_STAGE_COMPUTE UpsampleCs PASS_UPSAMPLE

#include "includes/Utilities.glsl"
#include "includes/Constants.glsl"
#include "includes/Encoding.glsl"
#include "includes/Common.glsl"

PK_DECLARE_LOCAL_CBUFFER(pk_Bloom_UpsampleLayerCount)
{
    float pk_Bloom_UpsampleLayerCount_Value;
};

PK_DECLARE_SET_DRAW uniform sampler2D pk_Texture;
PK_DECLARE_SET_DRAW uniform uimage2D pk_Image;
layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;

float GetLumaWeight(float3 s)
{
// Luma filtering not needed when using good enough antialiasing.
// Removes highlights & energy unnecesarily.
#if 0
    return 1.0f / (1.0f + cmax(s));
#else
    return 1.0f;
#endif
}

void Downsample0Cs()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 tx_dst = 1.0f.xx / imageSize(pk_Image).xy;
    const float2 tx_src = 1.0f.xx / textureSize(pk_Texture, 0).xy;
    const float2 uv = float2(coord + 0.5f.xx) * tx_dst;

    // Castaño, 2013, "Shadow Mapping Summary Part 1"
    // 3x3 gaussian filter with 4 linear samples
    const float2 scoord = (uv / tx_src) + 0.5f.xx;
    const float2 base = (floor(scoord) - 0.5f.xx) * tx_src;
    const float2 st = fract(scoord);
    const float4 uwvw = float4(3.0f - 2.0f * st, 1.0f + 2.0f * st);
    const float4 uvs = base.xyxy + float4((2.0f - st) / uwvw.xy - 1.0f, st / uwvw.zw + 1.0f) * tx_src.xyxy;

    float4 color = 0.0f.xxxx;
    float3 bisample = 0.0f.xxx;
    bisample = texture(pk_Texture, uvs.xy).rgb;
    color += float4(bisample, 1.0f) * uwvw.x * uwvw.y * (1.0 / 16.0) * GetLumaWeight(bisample);
    bisample = texture(pk_Texture, uvs.zy).rgb;
    color += float4(bisample, 1.0f) * uwvw.z * uwvw.y * (1.0 / 16.0) * GetLumaWeight(bisample);
    bisample = texture(pk_Texture, uvs.xw).rgb;
    color += float4(bisample, 1.0f) * uwvw.x * uwvw.w * (1.0 / 16.0) * GetLumaWeight(bisample);
    bisample = texture(pk_Texture, uvs.zw).rgb;
    color += float4(bisample, 1.0f) * uwvw.z * uwvw.w * (1.0 / 16.0) * GetLumaWeight(bisample);
    color.rgb /= color.w;
    color.rgb = -min(-color.rgb, 0.0f.xxx);
    
    imageStore(pk_Image, coord, uint4(EncodeE5BGR9(color.rgb)));
}

void Downsample1Cs()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 tx_dst = 1.0f.xx / imageSize(pk_Image).xy;
    const float2 tx_src = 1.0f.xx / textureSize(pk_Texture, 0).xy;
    const float2 uv = float2(coord + 0.5f.xx) * tx_dst;

    // Source: https://www.shadertoy.com/view/cslczj
    // A 6x6 downsampling filter using 9 taps. Designed to be used in a bloom downsampling chain.
    const float w0 = 0.302001f;
    const float a = 0.127963f;
    const float b = 0.0465365f;
    const float oa = 1.5f + 0.25f;
    const float ob = 1.5f + (0.125f + 1.0f / 16.0f);

    const float2 oa2 = oa * tx_src;
    const float2 ob2 = ob * tx_src;

    float3 color = 0.0f.xxx;

    color += texture(pk_Texture, uv).rgb * w0;
    color += texture(pk_Texture, uv + float2(-1.0, +0.0) * oa2).rgb * a;
    color += texture(pk_Texture, uv + float2(+1.0, +0.0) * oa2).rgb * a;
    color += texture(pk_Texture, uv + float2(+0.0, -1.0) * oa2).rgb * a;
    color += texture(pk_Texture, uv + float2(+0.0, +1.0) * oa2).rgb * a;
    color += texture(pk_Texture, uv + float2(-1.0, -1.0) * ob2).rgb * b;
    color += texture(pk_Texture, uv + float2(+1.0, -1.0) * ob2).rgb * b;
    color += texture(pk_Texture, uv + float2(-1.0, +1.0) * ob2).rgb * b;
    color += texture(pk_Texture, uv + float2(+1.0, +1.0) * ob2).rgb * b;

    imageStore(pk_Image, coord, uint4(EncodeE5BGR9(color)));
}

void UpsampleCs()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 tx_dst = 1.0f.xx / imageSize(pk_Image).xy;
    const float2 tx_src = 1.0f.xx / textureSize(pk_Texture, 0).xy;
    const float2 uv = float2(coord + 0.5f.xx) * tx_dst;
    const float4 uvs = uv.xyxy + float4(0.5f.xx, -0.5f.xx) * tx_src.xyxy;

    float4 color = 0.0f.xxxx;

    color.rgb += texture(pk_Texture, uvs.zw).rgb;
    color.rgb += texture(pk_Texture, uvs.zy).rgb;
    color.rgb += texture(pk_Texture, uvs.xy).rgb;
    color.rgb += texture(pk_Texture, uvs.xw).rgb;
    color.rgb *= 0.25f;

    const float upsample_weight = pk_Bloom_UpsampleLayerCount_Value * pk_Bloom_Diffusion;

    color.rgb *= upsample_weight;
    color.rgb += DecodeE5BGR9(imageLoad(pk_Image, coord).r);
    color.rgb /= upsample_weight + 1.0f;

    imageStore(pk_Image, coord, uint4(EncodeE5BGR9(color.rgb)));
}
