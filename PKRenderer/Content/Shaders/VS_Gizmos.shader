
#pragma pk_blend_color Add SrcAlpha OneMinusSrcAlpha
#pragma pk_ztest Off
#pragma pk_zwrite False
#pragma pk_cull Off
#pragma pk_program SHADER_STAGE_VERTEX MainVs
#pragma pk_program SHADER_STAGE_FRAGMENT MainFs

#include "includes/Common.glsl"
#include "includes/GBuffers.glsl"

PK_DECLARE_VS_ATTRIB(float4 vs_COLOR);

[[pk_restrict MainVs]] 
in uint4 in_POSITION;

void MainVs()
{
    gl_Position = pk_WorldToClip_NoJitter * float4(uintBitsToFloat(in_POSITION.xyz), 1.0f);
    vs_COLOR = unpackUnorm4x8(in_POSITION.w);
};

[[pk_restrict MainFs]] 
out float4 SV_Target0;

void MainFs()
{
    const float view_depth = ViewDepth(gl_FragCoord.z);
    const float scene_depth = SampleViewDepth(gl_FragCoord.xy * pk_ScreenParams.zw);
    const float fade_alpha = 0.3f * exp((-view_depth + scene_depth) * 0.125f);

    float4 color = vs_COLOR;
    color.a *= scene_depth < view_depth ? fade_alpha : 1.0f;

    SV_Target0 = color;
};
