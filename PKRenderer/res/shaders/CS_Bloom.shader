#version 450
#pragma PROGRAM_COMPUTE
#include includes/Utilities.glsl
#include includes/Constants.glsl
#include includes/Encoding.glsl

#multi_compile PASS_DOWNSAMPLE0 PASS_DOWNSAMPLE1 PASS_UPSAMPLE

#if defined(PASS_UPSAMPLE)
PK_DECLARE_LOCAL_CBUFFER(_Multiplier)
{
    float multiplier;
};
#endif

PK_DECLARE_SET_DRAW uniform sampler2D _SourceTex;
layout(r32ui, set = PK_SET_DRAW) uniform uimage2D _DestinationTex;

//Source: http://advances.realtimerendering.com/s2014/sledgehammer/Next-Generation-Post-Processing-in-Call-of-Duty-Advanced-Warfare-v17.pptx (page 139)

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;
void main()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 tx_dst = 1.0f.xx / imageSize(_DestinationTex).xy;
    const float2 tx_src = 1.0f.xx / textureSize(_SourceTex, 0).xy;
    const float2 uv = float2(coord + 0.5f.xx) * tx_dst;
    
    float4 color = 0.0f.xxxx;

#if defined(PASS_DOWNSAMPLE0)
    
    const float4 uvs = uv.xyxy + float4(0.5f.xx, -0.5f.xx) * tx_dst.xyxy;

    // Karis filter first mip
    float3 bisample = 0.0f.xxx;
    bisample = tex2D(_SourceTex, uvs.zw).rgb;
    color += float4(bisample, 1.0f) / (1.0f + dot(bisample, pk_Luminance.rgb));
    bisample = tex2D(_SourceTex, uvs.xw).rgb;
    color += float4(bisample, 1.0f) / (1.0f + dot(bisample, pk_Luminance.rgb));
    bisample = tex2D(_SourceTex, uvs.zy).rgb;
    color += float4(bisample, 1.0f) / (1.0f + dot(bisample, pk_Luminance.rgb));
    bisample = tex2D(_SourceTex, uvs.xy).rgb;
    color += float4(bisample, 1.0f) / (1.0f + dot(bisample, pk_Luminance.rgb));
    color /= color.w;
    color.rgb = -min(-color.rgb, 0.0f.xxx);

#elif defined(PASS_DOWNSAMPLE1)

    color.rgb += tex2D(_SourceTex, uv + float2( 2, -2) * tx_src).rgb * 0.125f;
    color.rgb += tex2D(_SourceTex, uv + float2(-2, -2) * tx_src).rgb * 0.125f;
    color.rgb += tex2D(_SourceTex, uv + float2( 0, -2) * tx_src).rgb * 0.25f;
                                                       
    color.rgb += tex2D(_SourceTex, uv + float2(-1, -1) * tx_src).rgb * 0.5f;    
    color.rgb += tex2D(_SourceTex, uv + float2( 1, -1) * tx_src).rgb * 0.5f;

    color.rgb += tex2D(_SourceTex, uv + float2(-2,  0) * tx_src).rgb * 0.25f;
    color.rgb += tex2D(_SourceTex, uv + float2( 0,  0) * tx_src).rgb * 0.5f;
    color.rgb += tex2D(_SourceTex, uv + float2( 2,  0) * tx_src).rgb * 0.25f;
                                                      
    color.rgb += tex2D(_SourceTex, uv + float2(-1,  1) * tx_src).rgb * 0.5f;
    color.rgb += tex2D(_SourceTex, uv + float2( 1,  1) * tx_src).rgb * 0.5f;
                                                      
    color.rgb += tex2D(_SourceTex, uv + float2(-1,  2) * tx_src).rgb * 0.125f;
    color.rgb += tex2D(_SourceTex, uv + float2( 0,  2) * tx_src).rgb * 0.25f;
    color.rgb += tex2D(_SourceTex, uv + float2( 2,  2) * tx_src).rgb * 0.125f;

    color.rgb *= 0.25f;

#else // defined(PASS_UPSAMPLE)

    const float4 uvs = uv.xyxy + float4(0.5f.xx, -0.5f.xx) * tx_src.xyxy;
    color.rgb += tex2D(_SourceTex, uvs.zw).rgb;
    color.rgb += tex2D(_SourceTex, uvs.zy).rgb;
    color.rgb += tex2D(_SourceTex, uvs.xy).rgb;
    color.rgb += tex2D(_SourceTex, uvs.xw).rgb;
    color.rgb *= 0.25f;
    color.rgb += DecodeE5BGR9(imageLoad(_DestinationTex, coord).r);
    color.rgb *= multiplier;

#endif

    imageStore(_DestinationTex, coord, uint4(EncodeE5BGR9(color.rgb)));
}