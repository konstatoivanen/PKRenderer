#version 460
#BlendColor Add One SrcAlpha
#include includes/SharedVolumeFog.glsl
#pragma PROGRAM_VERTEX
#include includes/Blit.glsl
out float2 vs_TEXCOORD;

void main()
{
    vs_TEXCOORD = PK_BLIT_VERTEX_TEXCOORD.xy;
    gl_Position = PK_BLIT_VERTEX_POSITION;
};

#pragma PROGRAM_FRAGMENT

in float2 vs_TEXCOORD;
out float4 SV_Target0;
void main()
{
    SV_Target0 = SampleVolumeFog(vs_TEXCOORD.xy, SampleLinearDepth(vs_TEXCOORD.xy));
};