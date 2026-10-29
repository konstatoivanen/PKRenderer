
#pragma pk_blend_color Add SrcAlpha OneMinusSrcAlpha
#pragma pk_blend_alpha Add One OneMinusSrcAlpha
#pragma pk_ztest Off
#pragma pk_zwrite False
#pragma pk_cull Off
#pragma pk_program Vertex MainVs
#pragma pk_program Fragment MainFs

#include "includes/Common.glsl"

#define PK_GUI_SHADING_MODE_DEFAULT 0
#define PK_GUI_SHADING_MODE_FONT 1
#define PK_FONT_MSDF_UNIT 4.0f

uniform StructuredBufferRef<uint4> pk_GUI_Vertices;
uniform float4 pk_GUI_ScreenTransform;

struct VsVaryings
{
    float4 sv_Position : SV_Position;
    sample float2 vs_TEXCOORD : TEXCOORD0;
    sample float4 vs_COLOR : COLOR0;
    nointerpolation uint vs_TEXTURE_INDEX : TEXCOORD1;
    nointerpolation uint vs_SHADING_MODE : TEXCOORD2;
};

VsVaryings MainVs(uint vertexId : SV_VulkanVertexID)
{
    const uint4 vertex_packed = pk_GUI_Vertices[vertexId];

    int2 coord;
    coord.x = bitfieldExtract(int(vertex_packed.y), 0, 16);
    coord.y = bitfieldExtract(int(vertex_packed.y), 16, 16);

    float2 cs_pos = coord.xy;
    cs_pos *= pk_GUI_ScreenTransform.xy;
    cs_pos += pk_GUI_ScreenTransform.zw;

    VsVaryings o;
    o.sv_Position = float4(cs_pos, 0.0f, 1.0f);
    o.vs_TEXCOORD = unpackHalf2x16(vertex_packed.z);
    o.vs_COLOR = unpackUnorm4x8(vertex_packed.x);
    o.vs_TEXTURE_INDEX = bitfieldExtract(vertex_packed.w, 0, 16);
    o.vs_SHADING_MODE = bitfieldExtract(vertex_packed.w, 16, 16);
    return o;
}

float4 MainFs(VsVaryings i) : SV_Target0
{
    Texture2D gui_tex = Texture2DRef<float4>(uint16_t(i.vs_TEXTURE_INDEX)).Get();
    const float4 value = gui_tex.Sample(pk_SamplerBilinearRepeat, i.vs_TEXCOORD); 
    float4 color = i.vs_COLOR;

    if (i.vs_SHADING_MODE == PK_GUI_SHADING_MODE_FONT)
    {
        const float sd_center = max(min(value.r, value.g), min(max(value.r, value.g), value.b)) - 0.5f;
        const float sd_subpx = ddx(sd_center) / 3.0f;
        const float3 sd_rgb = sd_center + float3(-sd_subpx, 0.0f, +sd_subpx);
        const float px_range = PK_FONT_MSDF_UNIT / length(fwidth(i.vs_TEXCOORD));
        const float3 px_mask = saturate(sd_rgb * px_range + 0.5f);
        color *= float4(px_mask, px_mask.g);
    }
    else
    {
        color *= value;
    }

    return color;
}
