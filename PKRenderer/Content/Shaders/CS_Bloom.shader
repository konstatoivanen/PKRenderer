
#pragma pk_multi_compile PASS_DOWNSAMPLE0 PASS_DOWNSAMPLE1 PASS_UPSAMPLE
#pragma pk_program SHADER_STAGE_COMPUTE Downsample0Cs PASS_DOWNSAMPLE0
#pragma pk_program SHADER_STAGE_COMPUTE Downsample1Cs PASS_DOWNSAMPLE1
#pragma pk_program SHADER_STAGE_COMPUTE UpsampleCs PASS_UPSAMPLE

#include "includes/Utilities.glsl"
#include "includes/Constants.glsl"
#include "includes/Encoding.glsl"

PK_DECLARE_SET_DRAW uniform sampler2D pk_Texture;
layout(r32ui, set = PK_SET_DRAW) uniform uimage2D pk_Image;

//Source: http://advances.realtimerendering.com/s2014/sledgehammer/Next-Generation-Post-Processing-in-Call-of-Duty-Advanced-Warfare-v17.pptx (page 139)
layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;

void Downsample0Cs()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 tx_dst = 1.0f.xx / imageSize(pk_Image).xy;
    const float2 tx_src = 1.0f.xx / textureSize(pk_Texture, 0).xy;
    const float2 uv = float2(coord + 0.5f.xx) * tx_dst;
    const float4 uvs = uv.xyxy + float4(0.5f.xx, -0.5f.xx) * tx_dst.xyxy;

    float4 color = 0.0f.xxxx;

    // Karis filter first mip
    float3 bisample = 0.0f.xxx;
    bisample = texture(pk_Texture, uvs.zw).rgb;
    color += float4(bisample, 1.0f) / (1.0f + dot(bisample, PK_LUMA_BT709));
    bisample = texture(pk_Texture, uvs.xw).rgb;
    color += float4(bisample, 1.0f) / (1.0f + dot(bisample, PK_LUMA_BT709));
    bisample = texture(pk_Texture, uvs.zy).rgb;
    color += float4(bisample, 1.0f) / (1.0f + dot(bisample, PK_LUMA_BT709));
    bisample = texture(pk_Texture, uvs.xy).rgb;
    color += float4(bisample, 1.0f) / (1.0f + dot(bisample, PK_LUMA_BT709));
    color /= color.w;
    color.rgb = -min(-color.rgb, 0.0f.xxx);

    imageStore(pk_Image, coord, uint4(EncodeE5BGR9(color.rgb)));
}

void Downsample1Cs()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 tx_dst = 1.0f.xx / imageSize(pk_Image).xy;
    const float2 tx_src = 1.0f.xx / textureSize(pk_Texture, 0).xy;
    const float2 uv = float2(coord + 0.5f.xx) * tx_dst;

    float4 color = 0.0f.xxxx;

    color.rgb += texture(pk_Texture, uv + float2(2, -2) * tx_src).rgb * 0.125f;
    color.rgb += texture(pk_Texture, uv + float2(-2, -2) * tx_src).rgb * 0.125f;
    color.rgb += texture(pk_Texture, uv + float2(0, -2) * tx_src).rgb * 0.25f;

    color.rgb += texture(pk_Texture, uv + float2(-1, -1) * tx_src).rgb * 0.5f;
    color.rgb += texture(pk_Texture, uv + float2(1, -1) * tx_src).rgb * 0.5f;

    color.rgb += texture(pk_Texture, uv + float2(-2, 0) * tx_src).rgb * 0.25f;
    color.rgb += texture(pk_Texture, uv + float2(0, 0) * tx_src).rgb * 0.5f;
    color.rgb += texture(pk_Texture, uv + float2(2, 0) * tx_src).rgb * 0.25f;

    color.rgb += texture(pk_Texture, uv + float2(-1, 1) * tx_src).rgb * 0.5f;
    color.rgb += texture(pk_Texture, uv + float2(1, 1) * tx_src).rgb * 0.5f;

    color.rgb += texture(pk_Texture, uv + float2(-1, 2) * tx_src).rgb * 0.125f;
    color.rgb += texture(pk_Texture, uv + float2(0, 2) * tx_src).rgb * 0.25f;
    color.rgb += texture(pk_Texture, uv + float2(2, 2) * tx_src).rgb * 0.125f;

    color.rgb *= 0.25f;

    imageStore(pk_Image, coord, uint4(EncodeE5BGR9(color.rgb)));
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
    color.rgb += DecodeE5BGR9(imageLoad(pk_Image, coord).r);

    imageStore(pk_Image, coord, uint4(EncodeE5BGR9(color.rgb)));
}
