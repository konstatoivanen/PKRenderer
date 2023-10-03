#version 460
#extension GL_KHR_shader_subgroup_ballot : enable
#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_shader_subgroup_shuffle : enable
#pragma PROGRAM_COMPUTE
#include includes/SharedDepthOfField.glsl
#include includes/Kernels.glsl

#multi_compile PASS_PREFILTER PASS_DISKBLUR PASS_UPSAMPLE

PK_DECLARE_SET_PASS uniform sampler2D _MainTex; // Screen Color full res.
layout(rgba16f, set = PK_SET_PASS) uniform image2D _DestinationTex; // Screen color full res.

PK_DECLARE_SET_SHADER uniform sampler2DArray pk_DoFTargetRead;
layout(rgba16f, set = PK_SET_SHADER) uniform image2DArray pk_DoFTargetWrite;

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const float2 texelSize = pk_ScreenParams.zw * 2.0f;
    const half margin = 2.0hf * half(texelSize.y);
    const float aspect = texelSize.x / texelSize.y;

    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 uv = (coord + 0.5f.xx) * texelSize;

    half4 background = 0.0hf.xxxx;
    half4 foreground = 0.0hf.xxxx;

#if defined(PASS_PREFILTER)

    const int2 offsets[4] = { int2(-1,-1), int2(1,1), int2(-1,1), int2(1,-1) };
    const float4 depths = SampleViewDepthOffsets(uv, offsets);
    const float4 cocs = GetCirclesOfConfusion(depths);
    const float4 weights = saturate(abs(cocs) / pk_MaximumCoC);

    float3 average;
    average.r = dot(textureGatherOffsets(_MainTex, uv, offsets, 0), weights);
    average.g = dot(textureGatherOffsets(_MainTex, uv, offsets, 1), weights);
    average.b = dot(textureGatherOffsets(_MainTex, uv, offsets, 2), weights);
    average /= dot(weights, 1.0f.xxxx);
    imageStore(pk_DoFTargetWrite, int3(coord, 2), float4(average, dot(cocs, 0.25f.xxxx)));

#elif defined(PASS_DISKBLUR)

    const half4 center = half4(tex2D(pk_DoFTargetRead, float3(uv, 2)));

    #define SAMPLE_COUNT 22u

    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        const half2 offset = half2(PK_BOKEH_DISK_22[i] * pk_MaximumCoC);
        const half  dist = length(offset) - margin;
        
        const half4 value = half4(tex2D(pk_DoFTargetRead, float3(uv + float2(offset.x * aspect, offset.y), 2)));
        const half  bgcoc = max(min(center.a, value.a), 0.0hf);

        background += half4(value.rgb, 1.0hf) * clamp(( bgcoc - dist) / margin, 0.0hf, 1.0hf);
        foreground += half4(value.rgb, 1.0hf) * clamp((-value.a - dist) / margin, 0.0hf, 1.0hf);
    }

    background.rgb /= background.a + (background.a < 1e-4hf ? 1.0hf : 0.0hf);
    foreground.rgb /= foreground.a + (foreground.a < 1e-4hf ? 1.0hf : 0.0hf);
    foreground.a = clamp(foreground.a * half(PK_PI / SAMPLE_COUNT), 0.0hf, 1.0hf);
    imageStore(pk_DoFTargetWrite, int3(coord, 0), foreground);
    imageStore(pk_DoFTargetWrite, int3(coord, 1), background);

#else

    const float viewDepth = SampleViewDepth(coord);
    const float coc = GetCircleOfConfusion(viewDepth);

    #if 1
    // Upsample tent filter;
    const float4 o = uv.xyxy * 0.5f + texelSize.xyxy * float2(-0.5f, 0.5f).xxyy;
    foreground += half4(tex2D(pk_DoFTargetRead, float3(o.xy, 0)));
    foreground += half4(tex2D(pk_DoFTargetRead, float3(o.zy, 0)));
    foreground += half4(tex2D(pk_DoFTargetRead, float3(o.xw, 0)));
    foreground += half4(tex2D(pk_DoFTargetRead, float3(o.zw, 0)));
    background += half4(tex2D(pk_DoFTargetRead, float3(o.xy, 1)));
    background += half4(tex2D(pk_DoFTargetRead, float3(o.zy, 1)));
    background += half4(tex2D(pk_DoFTargetRead, float3(o.xw, 1)));
    background += half4(tex2D(pk_DoFTargetRead, float3(o.zw, 1)));
    foreground *= 0.25hf;
    background *= 0.25hf;
    #else
    foreground = half4(tex2D(pk_DoFTargetRead, float3(uv * 0.5f, 0)));
    background = half4(tex2D(pk_DoFTargetRead, float3(uv * 0.5f, 1)));
    #endif
    background.a = half(smoothstep(texelSize.y, texelSize.y * 2.0f, coc));

    const half  alpha = (1.0hf - foreground.a) * (1.0hf - background.a);

    const uint4 threadMask = subgroupBallot(alpha < 0.85f);
    const uint threadCount = subgroupBallotBitCount(threadMask);

    [[branch]]
    if (threadCount > 0u)
    {
        const half3 dof = lerp(background.rgb * background.a, foreground.rgb, foreground.a);
        const half3 scene = half3(imageLoad(_DestinationTex, coord).rgb);
        imageStore(_DestinationTex, coord, half4(scene * alpha + dof, 1.0hf));
    }

#endif
};

