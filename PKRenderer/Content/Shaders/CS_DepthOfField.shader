
#pragma pk_multi_compile PASS_PREFILTER PASS_DISKBLUR PASS_UPSAMPLE
#pragma pk_program Compute PrefilterCs PASS_PREFILTER
#pragma pk_program Compute DiskBlurCs PASS_DISKBLUR
#pragma pk_program Compute UpsampleCs PASS_UPSAMPLE

#include "includes/PostFXDepthOfField.glsl"
#include "includes/Kernels.glsl"
#include "includes/Encoding.glsl"

// Screen Color full res.
uniform Texture2D<float3> pk_Texture;

// Screen color full res. same as above but as image output.
uniform RWTexture2D<float4> pk_Image;

uniform Texture2DArray<float3> pk_DoF_ColorRead;
uniform Texture2DArray<float> pk_DoF_AlphaRead;
uniform RWTexture2DArray<uint> pk_DoF_ColorWrite;
uniform RWTexture2DArray<float> pk_DoF_AlphaWrite;

[numthreads(PK_W_ALIGNMENT_16, PK_W_ALIGNMENT_8, 1u)]
void PrefilterCs()
{
    const float2 texel_size = 1.0f.xx / (gl_WorkGroupSize.xy * gl_NumWorkGroups.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 uv = (coord + 0.5f.xx) * texel_size;

    const int2 offsets[4] = { int2(-1,-1), int2(1,1), int2(-1,1), int2(1,-1) };
    const float4 depths = GatherViewDepths(uv);
    const float4 cocs = DoF_GetCirclesOfConfusion(depths);
    const float4 weights = saturate(abs(cocs) / pk_DoF_MaximumCoC);

    float3 average;
    average.r = dot(pk_Texture.GatherRed(pk_SamplerBilinearClamped, uv), weights);
    average.g = dot(pk_Texture.GatherGreen(pk_SamplerBilinearClamped, uv), weights);
    average.b = dot(pk_Texture.GatherBlue(pk_SamplerBilinearClamped, uv), weights);
    average /= dot(weights, 1.0f.xxxx);

    pk_DoF_ColorWrite[int3(coord, 0)] = EncodeE5BGR9(average);
    pk_DoF_AlphaWrite[int3(coord, 0)] = dot(cocs, 0.25f.xxxx);
}

[numthreads(PK_W_ALIGNMENT_16, PK_W_ALIGNMENT_8, 1u)]
void DiskBlurCs()
{
    const float2 texel_size = 1.0f.xx / (gl_WorkGroupSize.xy * gl_NumWorkGroups.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 uv = (coord + 0.5f.xx) * texel_size;

    half4 background = 0.0hf.xxxx;
    half4 foreground = 0.0hf.xxxx;

    const half margin = 2.0hf * half(texel_size.y);
    const float aspect = texel_size.x / texel_size.y;
    const half center_coc = half(pk_DoF_AlphaRead.SampleLevel(pk_SamplerBilinearClamped, float3(uv, 0), 0));
    
    const half3 center = half3(pk_DoF_ColorRead.SampleLevel(pk_SamplerBilinearClamped, float3(uv, 0), 0).rgb);
    background = half4(center, 1.0hf) * clamp((max(0.0hf, center_coc) + margin) / margin, 0.0hf, 1.0hf);
    foreground = half4(center, 1.0hf) * clamp((-center_coc + margin) / margin, 0.0hf, 1.0hf);

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

            const half3 color = half3(pk_DoF_ColorRead.SampleLevel(pk_SamplerBilinearClamped, uvw, 0).rgb);
            const half coc = half(pk_DoF_AlphaRead.SampleLevel(pk_SamplerBilinearClamped, uvw, 0).r);
            const half bgcoc = max(min(center_coc, coc), 0.0hf);

            background += half4(color, 1.0hf) * clamp((bgcoc - dist) / margin, 0.0hf, 1.0hf);
            foreground += half4(color, 1.0hf) * clamp((-coc - dist) / margin, 0.0hf, 1.0hf);
        }
    }

    background.rgb /= background.a + (background.a < 1e-4hf ? 1.0hf : 0.0hf);
    foreground.rgb /= foreground.a + (foreground.a < 1e-4hf ? 1.0hf : 0.0hf);
    foreground.a = clamp(foreground.a * half(PK_PI / SAMPLE_COUNT), 0.0hf, 1.0hf);

    pk_DoF_ColorWrite[int3(coord, 1)] = EncodeE5BGR9(foreground.rgb);
    pk_DoF_ColorWrite[int3(coord, 2)] = EncodeE5BGR9(background.rgb);
    pk_DoF_AlphaWrite[int3(coord, 1)] = foreground.a;
}

[numthreads(PK_W_ALIGNMENT_16, PK_W_ALIGNMENT_8, 1u)]
void UpsampleCs()
{
    const float2 texel_size = 1.0f.xx / (gl_WorkGroupSize.xy * gl_NumWorkGroups.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 uv = (coord + 0.5f.xx) * texel_size;

    half4 background = 0.0hf.xxxx;
    half4 foreground = 0.0hf.xxxx;

    const float view_depth = SampleViewDepth(coord);
    const float coc = DoF_GetCircleOfConfusion(view_depth);
    const float4 uvs = uv.xyxy + texel_size.xyxy * float2(-1.0f, 1.0f).xxyy;

    foreground.a += half(pk_DoF_AlphaRead.SampleLevel(pk_SamplerBilinearClamped, float3(uvs.xy, 1), 0).r);
    foreground.a += half(pk_DoF_AlphaRead.SampleLevel(pk_SamplerBilinearClamped, float3(uvs.zy, 1), 0).r);
    foreground.a += half(pk_DoF_AlphaRead.SampleLevel(pk_SamplerBilinearClamped, float3(uvs.xw, 1), 0).r);
    foreground.a += half(pk_DoF_AlphaRead.SampleLevel(pk_SamplerBilinearClamped, float3(uvs.zw, 1), 0).r);
    foreground.a *= 0.25hf;
    background.a = half(smoothstep(texel_size.y * 2.0f, texel_size.y * 4.0f, coc));

    const half alpha = (1.0hf - foreground.a) * (1.0hf - background.a);

    const uint thread_mask = WaveActiveBallot(alpha < 0.85f).x;
    const uint thread_count = countbits(thread_mask);

    [[branch]]
    if (thread_count > 0u)
    {
        foreground.rgb += half3(pk_DoF_ColorRead.SampleLevel(pk_SamplerBilinearClamped, float3(uvs.xy, 1), 0).rgb);
        foreground.rgb += half3(pk_DoF_ColorRead.SampleLevel(pk_SamplerBilinearClamped, float3(uvs.zy, 1), 0).rgb);
        foreground.rgb += half3(pk_DoF_ColorRead.SampleLevel(pk_SamplerBilinearClamped, float3(uvs.xw, 1), 0).rgb);
        foreground.rgb += half3(pk_DoF_ColorRead.SampleLevel(pk_SamplerBilinearClamped, float3(uvs.zw, 1), 0).rgb);
        foreground.rgb *= 0.25hf;

        background.rgb += half3(pk_DoF_ColorRead.SampleLevel(pk_SamplerBilinearClamped, float3(uvs.xy, 2), 0).rgb);
        background.rgb += half3(pk_DoF_ColorRead.SampleLevel(pk_SamplerBilinearClamped, float3(uvs.zy, 2), 0).rgb);
        background.rgb += half3(pk_DoF_ColorRead.SampleLevel(pk_SamplerBilinearClamped, float3(uvs.xw, 2), 0).rgb);
        background.rgb += half3(pk_DoF_ColorRead.SampleLevel(pk_SamplerBilinearClamped, float3(uvs.zw, 2), 0).rgb);
        background.rgb *= 0.25hf;

        const half3 dof = lerp(background.rgb * background.a, foreground.rgb, foreground.a);
        const half3 scene = half3(pk_Image[coord].rgb);

        pk_Image[coord] = half4(scene * alpha + dof, 1.0hf);
    }
}
