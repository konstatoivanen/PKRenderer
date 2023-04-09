#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/Reconstruction.glsl
#include includes/SharedPostEffects.glsl

PK_DECLARE_SET_DRAW uniform sampler2D _SourceTex;
PK_DECLARE_SET_DRAW uniform sampler2D _HistoryReadTex;
layout(rgba16f, set = PK_SET_DRAW) uniform image2D _DestinationTex;
layout(rgba16f, set = PK_SET_DRAW) uniform image2D _HistoryWriteTex;

#define SAMPLE_TAA_SOURCE(uv) tex2D(_SourceTex, uv)
#define SAMPLE_TAA_HISTORY(uv) tex2D(_HistoryReadTex, uv)
#include includes/SharedTemporalAntialiasing.glsl

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    int2 coord = int2(gl_GlobalInvocationID.xy);
    int2 size = imageSize(_DestinationTex).xy;

    if (Any_Greater(coord, size))
    {
        return;
    }

    TAADescriptor desc;
    desc.texelSize = 2.0f.xx / size;
    desc.jitter = pk_ProjectionJitter.xy * desc.texelSize;
    desc.texcoord = (coord + 0.5f.xx) / size;

    float3 viewpos = SampleViewPosition(desc.texcoord);
    float3 uvw = ClipToUVW(mul(pk_MATRIX_LD_P, float4(viewpos, 1.0f)));

    desc.motion = (desc.texcoord - uvw.xy) + pk_ProjectionJitter.zw * desc.texelSize * 0.5f;
    desc.sharpness = pk_TAA_Sharpness;
    desc.blendingStatic = pk_TAA_BlendingStatic;
    desc.blendingMotion = pk_TAA_BlendingMotion;
    desc.motionAmplification = pk_TAA_MotionAmplification;

    TAAOutput o = SolveTemporalAntiAliasing(desc);

    imageStore(_DestinationTex, coord, o.color);
    imageStore(_HistoryWriteTex, coord, o.history);
}