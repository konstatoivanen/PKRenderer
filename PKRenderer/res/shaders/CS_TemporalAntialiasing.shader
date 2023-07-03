#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/Reconstruction.glsl
#include includes/SharedPostEffects.glsl
#include includes/Encoding.glsl

PK_DECLARE_SET_DRAW uniform sampler2D _SourceTex;
PK_DECLARE_SET_DRAW uniform sampler2D _HistoryReadTex;
layout(r32ui, set = PK_SET_DRAW) uniform uimage2D _HistoryWriteTex;

#define SAMPLE_TAA_SOURCE(uv) tex2D(_SourceTex, uv).rgb
#define SAMPLE_TAA_HISTORY(uv) tex2D(_HistoryReadTex, uv).rgb
#include includes/SharedTemporalAntialiasing.glsl

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const int2 size = int2(pk_ScreenSize.xy * 2u);

    TAADescriptor desc;
    desc.texelSize = 2.0f.xx / size;
    desc.jitter = pk_ProjectionJitter.xy * desc.texelSize;
    desc.texcoord = (coord + 0.5f.xx) / size;

    const float3 viewpos = SampleViewPosition(desc.texcoord);
    const float3 uvw = ClipToUVW(mul(pk_MATRIX_LD_P, float4(viewpos, 1.0f)));

    desc.motion = (desc.texcoord - uvw.xy) + pk_ProjectionJitter.zw * desc.texelSize * 0.5f;
    desc.sharpness = pk_TAA_Sharpness;
    desc.blendingStatic = pk_TAA_BlendingStatic;
    desc.blendingMotion = pk_TAA_BlendingMotion;
    desc.motionAmplification = pk_TAA_MotionAmplification;

    TAAOutput o = SolveTemporalAntiAliasing(desc);

    imageStore(_HistoryWriteTex, coord, uint4(EncodeE5BGR9(o.color)));
}