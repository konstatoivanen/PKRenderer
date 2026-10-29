
#pragma pk_multi_compile PASS_BARREL PASS_CHROMA
#pragma pk_program Compute DistortPaniniCs PASS_BARREL
#pragma pk_program Compute DistortChromaCs PASS_CHROMA

#include "includes/Utilities.glsl"
#include "includes/Constants.glsl"
#include "includes/Encoding.glsl"
#include "includes/Common.glsl"
#include "includes/NoiseBlue.glsl"

uniform Texture2DRef pk_Texture;

#if defined(PASS_BARREL)
uniform RWTexture2DRef<uint> pk_Image;

[numthreads(PK_W_ALIGNMENT_16, PK_W_ALIGNMENT_4, 1u)]
void DistortPaniniCs()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 texel = 1.0f.xx / float2(pk_Texture.GetSize(0));
    const float4 params = pk_View.Panini_Projection_Parameters;

    const float2 uv = float2(coord + 0.5f.xx) * texel;
    const float2 view_pos = (uv * 2.0f - 1.0f) * params.xy * params.w;
    const float view_d = 1.0f + params.z;
    const float view_d_hp = pow2(view_pos.x) + pow2(view_d);
    const float isect_a = view_pos.x * params.z;
    const float isect_b = sqrt(view_d_hp - pow2(isect_a));
    const float dist_c = params.z + (-isect_a * view_pos.x + view_d * isect_b) / view_d_hp;
    const float2 pos_cyl = view_pos * dist_c / view_d;
    const float2 pos_screen = pos_cyl / (dist_c - params.z) / params.xy;
    const float2 panini_uv = pos_screen * 0.5f + 0.5f;

    const float3 color = pk_Texture.SampleLevel(pk_SamplerBilinearClamped, panini_uv, 0).rgb;
    pk_Image[coord] = EncodeE5BGR9(color);
}

#else
uniform RWTexture2DRef<float4> pk_Image;

[numthreads(PK_W_ALIGNMENT_16, PK_W_ALIGNMENT_4, 1u)]
void DistortChromaCs()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 texel = 1.0f.xx / float2(pk_Texture.GetSize(0));

    const float2 uv = float2(coord + 0.5f.xx) * texel;
    const float2 screen_pos = uv * 2.0f - 1.0f;

    float2 offset = 1.0f.xx;
    offset *= pow(1.0f - abs(screen_pos), pk_View.Chromatic_Aberration_Power.xx);
    offset = 1.0f - offset;
    offset *= sign(screen_pos);
    offset = (offset - screen_pos) * 0.5f;
    offset *= pk_View.Chromatic_Aberration_Amount;

    const float noise = GlobalNoiseBlue(coord, pk_View.FrameIndex.y).r;

    float3 color = 0.0f.xxx;
    float3 wsum = 0.0f.xxx;

    for (uint i = 0u; i < 5u; ++i)
    {
        const float s_interp = (i + noise) / 5.0f;
        const float s_theta = 3.0 * s_interp - 1.5;
        const float3 s_weight = saturate(float3(-s_theta, 1.0 - abs(s_theta), s_theta));
        const float3 s_sample = pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uv + offset * s_interp, 0.0f).rgb;
        wsum += s_weight;
        color += s_sample * s_weight;
    }

    color /= wsum;

    pk_Image[coord] = float4(color.rgb, 1.0f);
}

#endif
