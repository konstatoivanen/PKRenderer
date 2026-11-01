
//Source: http://advances.realtimerendering.com/s2014/sledgehammer/Next-Generation-Post-Processing-in-Call-of-Duty-Advanced-Warfare-v17.pptx (page 139)

#pragma pk_multi_compile PASS_DOWNSAMPLE0 PASS_DOWNSAMPLE1 PASS_UPSAMPLE
#pragma pk_program Compute Downsample0Cs PASS_DOWNSAMPLE0
#pragma pk_program Compute Downsample1Cs PASS_DOWNSAMPLE1
#pragma pk_program Compute UpsampleCs PASS_UPSAMPLE

#include "includes/Utilities.glsl"
#include "includes/Constants.glsl"
#include "includes/Encoding.glsl"
#include "includes/Common.glsl"

uniform float pk_Bloom_UpsampleWeight;
uniform Texture2D<float3> pk_Texture;
uniform RWTexture2D<uint> pk_Image;

[numthreads(PK_W_ALIGNMENT_16, PK_W_ALIGNMENT_4, 1u)]
void Downsample0Cs()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 tx_dst = 1.0f.xx / pk_Image.GetSize();
    const float2 tx_src = 1.0f.xx / pk_Texture.GetSize();
    const float2 uv = float2(coord + 0.5f.xx) * tx_dst;

    // Castaño, 2013, "Shadow Mapping Summary Part 1"
    // 3x3 gaussian filter with 4 linear samples
    const float2 scoord = (uv / tx_src) + 0.5f.xx;
    const float2 base = (floor(scoord) - 0.5f.xx) * tx_src;
    const float2 st = frac(scoord);
    const float4 uwvw = float4(3.0f - 2.0f * st, 1.0f + 2.0f * st);
    const float4 uvs = base.xyxy + float4((2.0f - st) / uwvw.xy - 1.0f, st / uwvw.zw + 1.0f) * tx_src.xyxy;

    float4 color = 0.0f.xxxx;
    float3 bisample = 0.0f.xxx;
    bisample = pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uvs.xy, 0).rgb;
    color += float4(bisample, 1.0f) * uwvw.x * uwvw.y * (1.0 / 16.0);
    bisample = pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uvs.zy, 0).rgb;
    color += float4(bisample, 1.0f) * uwvw.z * uwvw.y * (1.0 / 16.0);
    bisample = pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uvs.xw, 0).rgb;
    color += float4(bisample, 1.0f) * uwvw.x * uwvw.w * (1.0 / 16.0);
    bisample = pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uvs.zw, 0).rgb;
    color += float4(bisample, 1.0f) * uwvw.z * uwvw.w * (1.0 / 16.0);
    color.rgb /= color.w;
    color.rgb = -min(-color.rgb, 0.0f.xxx);
    
    pk_Image[coord] = EncodeE5BGR9(color.rgb);
}

[numthreads(PK_W_ALIGNMENT_16, PK_W_ALIGNMENT_4, 1u)]
void Downsample1Cs()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 tx_dst = 1.0f.xx / pk_Image.GetSize();
    const float2 tx_src = 1.0f.xx / pk_Texture.GetSize();
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

    color += pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uv, 0).rgb * w0;
    color += pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uv + float2(-1.0, +0.0) * oa2, 0).rgb * a;
    color += pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uv + float2(+1.0, +0.0) * oa2, 0).rgb * a;
    color += pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uv + float2(+0.0, -1.0) * oa2, 0).rgb * a;
    color += pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uv + float2(+0.0, +1.0) * oa2, 0).rgb * a;
    color += pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uv + float2(-1.0, -1.0) * ob2, 0).rgb * b;
    color += pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uv + float2(+1.0, -1.0) * ob2, 0).rgb * b;
    color += pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uv + float2(-1.0, +1.0) * ob2, 0).rgb * b;
    color += pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uv + float2(+1.0, +1.0) * ob2, 0).rgb * b;

    pk_Image[coord] = EncodeE5BGR9(color);
}

[numthreads(PK_W_ALIGNMENT_16, PK_W_ALIGNMENT_4, 1u)]
void UpsampleCs()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 tx_dst = 1.0f.xx / pk_Image.GetSize();
    const float2 tx_src = 1.0f.xx / pk_Texture.GetSize();
    const float2 uv = float2(coord + 0.5f.xx) * tx_dst;
    const float4 uvs = uv.xyxy + float4(0.5f.xx, -0.5f.xx) * tx_src.xyxy;

    float4 color = 0.0f.xxxx;

    color.rgb += pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uvs.zw, 0).rgb;
    color.rgb += pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uvs.zy, 0).rgb;
    color.rgb += pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uvs.xy, 0).rgb;
    color.rgb += pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uvs.xw, 0).rgb;
    color.rgb *= 0.25f;

    const float upsample_weight = pk_Bloom_UpsampleWeight;

    color.rgb *= upsample_weight;
    color.rgb += DecodeE5BGR9(pk_Image[coord]);
    color.rgb /= upsample_weight + 1.0f;

    pk_Image[coord] = EncodeE5BGR9(color.rgb);
}
