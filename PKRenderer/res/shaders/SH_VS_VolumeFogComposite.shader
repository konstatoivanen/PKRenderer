#version 460
#BlendColor Add One SrcAlpha
#include includes/SharedVolumeFog.glsl
#pragma PROGRAM_VERTEX
#include includes/Blit.glsl
out float4 vs_TEXCOORD;

void main()
{
    vs_TEXCOORD = PK_BLIT_VERTEX_TEXCOORD.xyxy;
    vs_TEXCOORD = float4(vs_TEXCOORD.xy, vs_TEXCOORD.xy * pk_ScreenParams.xy + pk_Time.ww * 1000);
    gl_Position = PK_BLIT_VERTEX_POSITION;
};

#pragma PROGRAM_FRAGMENT
in float4 vs_TEXCOORD;
out float4 SV_Target0;
void main()
{
    float d = SampleLinearDepth(vs_TEXCOORD.xy);
    float w = GetVolumeWCoord(d);

    float3 offset = GlobalNoiseBlue(uint2(vs_TEXCOORD.zw), pk_FrameIndex.x);
    offset -= 0.5f;
    offset *= VOLUME_COMPOSITE_DITHER_AMOUNT;

    SV_Target0 = tex2D(pk_Volume_ScatterRead, float3(vs_TEXCOORD.xy, w) + offset.xyz);
};