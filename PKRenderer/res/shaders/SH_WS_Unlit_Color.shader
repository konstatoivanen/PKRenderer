#version 450
#Cull Back
#ZTest LEqual
#ZWrite Off

#MaterialProperty float4 _Color
#MaterialProperty float4 _ColorVoxelize

#multi_compile _ PK_META_PASS_GBUFFER PK_META_PASS_GIVOXELIZE

#include includes/GBuffers.glsl

#pragma PROGRAM_VERTEX
in float3 in_POSITION;
#if defined(PK_META_PASS_GBUFFER)
in float3 in_NORMAL;
out float3 vs_Normal;
#endif

void main()
{
    gl_Position = ObjectToClipPos(in_POSITION);
    #if defined(PK_META_PASS_GBUFFER)
    vs_Normal = ObjectToWorldDir(in_NORMAL);
    #endif
}

#pragma PROGRAM_FRAGMENT
#if defined(PK_META_PASS_GBUFFER)
in float3 vs_Normal;
#endif
out float4 SV_Target0;

void main()
{
#if defined(PK_META_PASS_GBUFFER)
    SV_Target0 = EncodeGBufferWorldNR(vs_Normal, 0.0f, 0.0f);
#elif defined(PK_META_PASS_GIVOXELIZE)
    SV_Target0 = _ColorVoxelize;
#else
    SV_Target0 = _Color;
#endif
}