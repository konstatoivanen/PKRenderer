
#pragma pk_multi_compile PASS_PREFILTER PASS_DISKBLUR PASS_UPSAMPLE
#pragma pk_program SHADER_STAGE_COMPUTE PrefilterCs PASS_PREFILTER
#pragma pk_program SHADER_STAGE_COMPUTE DiskBlurCs PASS_DISKBLUR
#pragma pk_program SHADER_STAGE_COMPUTE UpsampleCs PASS_UPSAMPLE

#include "includes/PostFXDepthOfField.glsl"
#include "includes/Kernels.glsl"
#include "includes/Encoding.glsl"

// Screen Color full res.
uniform sampler2D pk_Texture;

// Screen color full res. same as above but as image output.
uniform image2D pk_Image;

uniform sampler2DArray pk_DoF_ColorRead;
uniform sampler2DArray pk_DoF_AlphaRead;
uniform uimage2DArray pk_DoF_ColorWrite;
uniform image2DArray pk_DoF_AlphaWrite;

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;

void PrefilterCs()
{
    const float2 texelSize = 1.0f.xx / (gl_WorkGroupSize.xy * gl_NumWorkGroups.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 uv = (coord + 0.5f.xx) * texelSize;

    const int2 offsets[4] = { int2(-1,-1), int2(1,1), int2(-1,1), int2(1,-1) };
    const float4 depths = GatherViewDepths(uv);
    const float4 cocs = GetCirclesOfConfusion(depths);
    const float4 weights = saturate(abs(cocs) / pk_DoF_MaximumCoC);

    float3 average;
    average.r = dot(textureGather(pk_Texture, uv, 0), weights);
    average.g = dot(textureGather(pk_Texture, uv, 1), weights);
    average.b = dot(textureGather(pk_Texture, uv, 2), weights);
    average /= dot(weights, 1.0f.xxxx);
    imageStore(pk_DoF_ColorWrite, int3(coord, 0), EncodeE5BGR9(average).xxxx);
    imageStore(pk_DoF_AlphaWrite, int3(coord, 0), dot(cocs, 0.25f.xxxx).xxxx);
}

void DiskBlurCs()
{
    const float2 texelSize = 1.0f.xx / (gl_WorkGroupSize.xy * gl_NumWorkGroups.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 uv = (coord + 0.5f.xx) * texelSize;

    half4 background = 0.0hf.xxxx;
    half4 foreground = 0.0hf.xxxx;

    const half margin = 2.0hf * half(texelSize.y);
    const float aspect = texelSize.x / texelSize.y;
    const half centerCoC = half(texture(pk_DoF_AlphaRead, float3(uv, 0)).r);

    const half3 center = half3(texture(pk_DoF_ColorRead, float3(uv, 0)).rgb);
    background = half4(center, 1.0hf) * clamp((max(0.0hf, centerCoC) + margin) / margin, 0.0hf, 1.0hf);
    foreground = half4(center, 1.0hf) * clamp((-centerCoC + margin) / margin, 0.0hf, 1.0hf);

    #define SAMPLE_COUNT 22u

    for (uint j = 1; j <= 2; ++j)
    {
        const uint k = j * 7;

        for (uint i = 0u; i < k; ++i)
        {
            const half phi = half(i) * 0.89759790102hf / half(j); // 2 * PI * (i / float(k))
            const half radius = (half(j)+0.14285714285hf) * 0.46666666666hf; // (RING + (1.0f / RING_POINTS)) / (RINGS + (1.0f / RING_POINTS))
            const half2 offset = half2(cos(phi), sin(phi)) * radius * half(pk_DoF_MaximumCoC);

            const half dist = length(offset) - margin;
            const float3 uvw = float3(uv + float2(offset.x * aspect, offset.y), 0);

            const half3 color = half3(texture(pk_DoF_ColorRead, uvw).rgb);
            const half coc = half(texture(pk_DoF_AlphaRead, uvw).r);
            const half bgcoc = max(min(centerCoC, coc), 0.0hf);

            background += half4(color, 1.0hf) * clamp((bgcoc - dist) / margin, 0.0hf, 1.0hf);
            foreground += half4(color, 1.0hf) * clamp((-coc - dist) / margin, 0.0hf, 1.0hf);
        }
    }

    background.rgb /= background.a + (background.a < 1e-4hf ? 1.0hf : 0.0hf);
    foreground.rgb /= foreground.a + (foreground.a < 1e-4hf ? 1.0hf : 0.0hf);
    foreground.a = clamp(foreground.a * half(PK_PI / SAMPLE_COUNT), 0.0hf, 1.0hf);

    imageStore(pk_DoF_ColorWrite, int3(coord, 1), EncodeE5BGR9(foreground.rgb).xxxx);
    imageStore(pk_DoF_ColorWrite, int3(coord, 2), EncodeE5BGR9(background.rgb).xxxx);
    imageStore(pk_DoF_AlphaWrite, int3(coord, 1), foreground.aaaa);
}

void UpsampleCs()
{
    const float2 texelSize = 1.0f.xx / (gl_WorkGroupSize.xy * gl_NumWorkGroups.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 uv = (coord + 0.5f.xx) * texelSize;

    half4 background = 0.0hf.xxxx;
    half4 foreground = 0.0hf.xxxx;

    const float viewDepth = SampleViewDepth(coord);
    const float coc = GetCircleOfConfusion(viewDepth);
    const float4 o = uv.xyxy + texelSize.xyxy * float2(-1.0f, 1.0f).xxyy;

    foreground.a += half(texture(pk_DoF_AlphaRead, float3(o.xy, 1)).r);
    foreground.a += half(texture(pk_DoF_AlphaRead, float3(o.zy, 1)).r);
    foreground.a += half(texture(pk_DoF_AlphaRead, float3(o.xw, 1)).r);
    foreground.a += half(texture(pk_DoF_AlphaRead, float3(o.zw, 1)).r);
    foreground.a *= 0.25hf;
    background.a = half(smoothstep(texelSize.y * 2.0f, texelSize.y * 4.0f, coc));

    const half alpha = (1.0hf - foreground.a) * (1.0hf - background.a);

    const uint4 threadMask = subgroupBallot(alpha < 0.85f);
    const uint threadCount = subgroupBallotBitCount(threadMask);

    [[branch]]
    if (threadCount > 0u)
    {
        foreground.rgb += half3(texture(pk_DoF_ColorRead, float3(o.xy, 1)).rgb);
        foreground.rgb += half3(texture(pk_DoF_ColorRead, float3(o.zy, 1)).rgb);
        foreground.rgb += half3(texture(pk_DoF_ColorRead, float3(o.xw, 1)).rgb);
        foreground.rgb += half3(texture(pk_DoF_ColorRead, float3(o.zw, 1)).rgb);
        foreground.rgb *= 0.25hf;

        background.rgb += half3(texture(pk_DoF_ColorRead, float3(o.xy, 2)).rgb);
        background.rgb += half3(texture(pk_DoF_ColorRead, float3(o.zy, 2)).rgb);
        background.rgb += half3(texture(pk_DoF_ColorRead, float3(o.xw, 2)).rgb);
        background.rgb += half3(texture(pk_DoF_ColorRead, float3(o.zw, 2)).rgb);
        background.rgb *= 0.25hf;

        const half3 dof = lerp(background.rgb * background.a, foreground.rgb, foreground.a);
        const half3 scene = half3(imageLoad(pk_Image, coord).rgb);
        imageStore(pk_Image, coord, half4(scene * alpha + dof, 1.0hf));
    }
}
