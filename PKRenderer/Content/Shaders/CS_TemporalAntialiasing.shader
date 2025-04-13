
#extension GL_KHR_shader_subgroup_shuffle : enable
#pragma pk_program SHADER_STAGE_COMPUTE main
#define PK_USE_SINGLE_DESCRIPTOR_SET

#include "includes/GBuffers.glsl"
#include "includes/PostFXResources.glsl"
#include "includes/Encoding.glsl"
#include "includes/ComputeQuadSwap.glsl"

PK_DECLARE_SET_DRAW uniform sampler2D pk_Texture; // Current Screen
PK_DECLARE_SET_DRAW uniform sampler2D pk_Texture1; // History Read
PK_DECLARE_SET_DRAW uniform uimage2D pk_Image; // History Write
PK_DECLARE_SET_DRAW uniform image2D pk_Image1; // Color Write

float3 TonemapColor(const float3 c) { return c / (1.0 + cmax(c)); }
float3 UntonemapColor(const float3 c) { return c / max(1.0f / 65504.0f, 1.0 - cmax(c)); }

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;
void main()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const int2 size = int2(pk_ScreenSize.xy * 2u);
    const float2 texel_size = 2.0f.xx / size;
    const float2 jitter = pk_ProjectionJitter.xy * texel_size;
    const float2 uv = (coord + 0.5f.xx) / size;

    const float depth = SampleClipDepth(uv);
    const float2 uv_previous = ClipToUvw(pk_ClipToPrevClip_NoJitter * float4(uv * 2 - 1, depth, 1)).xy;
    const float2 motion = uv - uv_previous;

    const float k_sharpness = pk_TAA_Sharpness;
    const float k_blend_static = pk_TAA_BlendingStatic;
    const float k_blend_motion = pk_TAA_BlendingMotion;
    const float k_motion_mult = pk_TAA_MotionAmplification;
    const float k_alpha_mult = 0.0f;

    float3 resolved = 0.0f.xxx;
    {
        float4 color_alpha = texture(pk_Texture, uv);
        float3 color = color_alpha.rgb;
        float3 history = texture(pk_Texture1, uv - motion).rgb;
        const float3 color_11 = texture(pk_Texture, uv - texel_size * 0.5f).rgb;
        const float3 color_00 = texture(pk_Texture, uv + texel_size * 0.5f).rgb;
        const float3 corners = 4.0f * (color_11 + color_00) - 2.0f * color;

        color += (color - (corners * 0.166667f)) * 2.718282f * k_sharpness;
        color = clamp(color, 0.0, PK_HALF_MAX_MINUS1);

        const float3 average = (corners + color) * 0.142857f;
        const float2 luminance = float2(dot(average, PK_LUMA_BT709), dot(color, PK_LUMA_BT709));

        const float motion_length = length(motion);
        // Lower values yield more aliasing and higher values more ghosting.
        const float color_offset = lerp(8.0f, 0.25f, saturate(motion_length * 100.0f)) * abs(luminance.x - luminance.y);

        const float3 minimum = min(color_00, color_11) - color_offset;
        const float3 maximum = max(color_00, color_11) + color_offset;

        // Clip history color
        {
            const float3 center = 0.5f * (maximum + minimum);
            const float3 extents = 0.5f * (maximum - minimum);
            const float3 offset = history - center;
            const float3 ts = abs(extents / (offset + 0.0001f));
            const float t = saturate(min(min(ts.x, ts.y), ts.z));
            history = center + offset * t;
        }

        float weight = lerp(k_blend_static, k_blend_motion, motion_length * k_motion_mult);
        weight *= lerp(1.0f, color_alpha.a, k_alpha_mult);
        weight = clamp(weight, k_blend_motion, k_blend_static);

        // Interpolate in tonemapped space to respond faster to luminance changes.
        color = TonemapColor(color);
        history = TonemapColor(history);

        resolved = lerp(color, history, weight);
        resolved = UntonemapColor(resolved);

        resolved = clamp(resolved, 0.0f, PK_HALF_MAX_MINUS1);
    }

    // @TODO should antialias depth as well or depth of field will have an incorrect coc mask.
    imageStore(pk_Image, coord, uint4(EncodeE5BGR9(resolved)));

    const uint swap_id_h = QuadSwapIdHorizontal(gl_SubgroupInvocationID);
    const uint swap_id_v = QuadSwapIdVertical16x2(gl_SubgroupInvocationID);
    const uint swap_id_d = QuadSwapIdDiagonal16x2(gl_SubgroupInvocationID);

    float3 downsampled = resolved;
    downsampled += subgroupShuffle(resolved, swap_id_h);
    downsampled += subgroupShuffle(resolved, swap_id_v);
    downsampled += subgroupShuffle(resolved, swap_id_d);
    downsampled *= 0.25f;

    [[branch]]
    if ((gl_LocalInvocationID.x & 0x1u) == 0 && (gl_LocalInvocationID.y & 0x1u) == 0)
    {
        imageStore(pk_Image1, coord / 2, float4(downsampled, 1.0f));
    }
}