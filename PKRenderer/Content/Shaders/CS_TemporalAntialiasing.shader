
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
    const float2 texelSize = 2.0f.xx / size;
    const float2 jitter = pk_ProjectionJitter.xy * texelSize;
    const float2 texcoord = (coord + 0.5f.xx) / size;

    const float depth = SampleClipDepth(texcoord);
    const float2 previousTexcoord = ClipToUVW(pk_ClipToPrevClip_NoJitter * float4(texcoord * 2 - 1, depth, 1)).xy;
    const float2 motion = texcoord - previousTexcoord;

    const float sharpness = pk_TAA_Sharpness;
    const float blendStatic = pk_TAA_BlendingStatic;
    const float blendMotion = pk_TAA_BlendingMotion;
    const float motionMult = pk_TAA_MotionAmplification;
    const float alphaMult = 0.0f;

    float3 resolved = 0.0f.xxx;
    {
        float4 colorAlpha = texture(pk_Texture, texcoord);
        float3 color = colorAlpha.rgb;
        float3 history = texture(pk_Texture1, texcoord - motion).rgb;
        const float3 color11 = texture(pk_Texture, texcoord - texelSize * 0.5f).rgb;
        const float3 color00 = texture(pk_Texture, texcoord + texelSize * 0.5f).rgb;
        const float3 corners = 4.0f * (color11 + color00) - 2.0f * color;

        color += (color - (corners * 0.166667f)) * 2.718282f * sharpness;
        color = clamp(color, 0.0, PK_HALF_MAX_MINUS1);

        const float3 average = (corners + color) * 0.142857f;
        const float2 luminance = float2(dot(average, PK_LUMA_BT709), dot(color, PK_LUMA_BT709));

        const float motionLength = length(motion);
        // Lower values yield more aliasing and higher values more ghosting.
        const float colorOffset = lerp(8.0f, 0.25f, saturate(motionLength * 100.0f)) * abs(luminance.x - luminance.y);

        const float3 minimum = min(color00, color11) - colorOffset;
        const float3 maximum = max(color00, color11) + colorOffset;

        // Clip history color
        {
            const float3 center = 0.5f * (maximum + minimum);
            const float3 extents = 0.5f * (maximum - minimum);
            const float3 offset = history - center;
            const float3 ts = abs(extents / (offset + 0.0001f));
            const float t = saturate(min(min(ts.x, ts.y), ts.z));
            history = center + offset * t;
        }

        float weight = lerp(blendStatic, blendMotion, motionLength * motionMult);
        weight *= lerp(1.0f, colorAlpha.a, alphaMult);
        weight = clamp(weight, blendMotion, blendStatic);

        // Interpolate in tonemapped space to respond faster to luminance changes.
        color = TonemapColor(color);
        history = TonemapColor(history);

        resolved = lerp(color, history, weight);
        resolved = UntonemapColor(resolved);

        resolved = clamp(resolved, 0.0f, PK_HALF_MAX_MINUS1);
    }

    // @TODO should antialias depth as well or depth of field will have an incorrect coc mask.
    imageStore(pk_Image, coord, uint4(EncodeE5BGR9(resolved)));

    uint shuffleH = QuadSwapIdHorizontal(gl_SubgroupInvocationID);
    uint shulffeV = QuadSwapIdVertical16x2(gl_SubgroupInvocationID);
    uint shulffeD = QuadSwapIdDiagonal16x2(gl_SubgroupInvocationID);

    float3 downSampled = resolved;
    downSampled += subgroupShuffle(resolved, shuffleH);
    downSampled += subgroupShuffle(resolved, shuffleH);
    downSampled += subgroupShuffle(resolved, shulffeD);
    downSampled *= 0.25f;

    [[branch]]
    if ((gl_LocalInvocationID.x & 0x1u) == 0 && (gl_LocalInvocationID.y & 0x1u) == 0)
    {
        imageStore(pk_Image1, coord / 2, float4(downSampled, 1.0f));
    }
}