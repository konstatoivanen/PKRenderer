
#pragma pk_blend_color Add SrcAlpha OneMinusSrcAlpha
#pragma pk_ztest Off
#pragma pk_zwrite False
#pragma pk_cull Off
#pragma pk_program Vertex MainVs
#pragma pk_program Fragment MainFs

#include "includes/Common.glsl"
#include "includes/GBuffers.glsl"

struct VsOut
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

VsOut MainVs(uint4 in_POSITION : in_POSITION)
{
    VsOut o;
    o.position = pk_View.WorldToClip_NoJitter * float4(asfloat(in_POSITION.xyz), 1.0f);
    o.color = unpackUnorm4x8(in_POSITION.w);
    return o;
}

float4 MainFs(VsOut i) : SV_Target0
{
    const float view_depth = ViewDepth(i.position.z);
    const float scene_depth = SampleViewDepth(i.position.xy * pk_View.ScreenParams.zw);
    const float fade_alpha = 0.3f * exp((-view_depth + scene_depth) * 0.125f);

    float4 color = i.color;
    color.a *= scene_depth < view_depth ? fade_alpha : 1.0f;
    return color;
}
