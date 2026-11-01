
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
#define PK_FONT_MSDF_UNIT 4.0f // keep upto date with definition in PKAsset.h

uniform StructuredBuffer<uint4> pk_GUI_Vertices;
uniform float4 pk_GUI_ScreenTransform;

struct VsOut
{
    float4 position : SV_Position;
    sample float2 texcoord : TEXCOORD0;
    sample float4 color : COLOR0;
    nointerpolation uint texture_index : TEXCOORD1;
    nointerpolation uint shading_mode : TEXCOORD2;
};

VsOut MainVs(uint vertex_id : SV_VulkanVertexID)
{
    const uint4 vertex_packed = pk_GUI_Vertices[vertex_id];

    int2 coord;
    coord.x = bitfieldExtract(int(vertex_packed.y), 0, 16);
    coord.y = bitfieldExtract(int(vertex_packed.y), 16, 16);

    float2 cs_pos = coord.xy;
    cs_pos *= pk_GUI_ScreenTransform.xy;
    cs_pos += pk_GUI_ScreenTransform.zw;

    VsOut o;
    o.position = float4(cs_pos, 0.0f, 1.0f);
    o.texcoord = unpackHalf2x16(vertex_packed.z);
    o.color = unpackUnorm4x8(vertex_packed.x);
    o.texture_index = bitfieldExtract(vertex_packed.w, 0, 16);
    o.shading_mode = bitfieldExtract(vertex_packed.w, 16, 16);
    return o;
}

float4 MainFs(VsOut i) : SV_Target0
{
    const float4 value = pk_Heap_Textures2D[i.texture_index].Sample(pk_SamplerBilinearRepeat, i.texcoord);
    float4 color = i.color;

    if (i.shading_mode == PK_GUI_SHADING_MODE_FONT)
    {
        const float sd_center = max(min(value.r, value.g), min(max(value.r, value.g), value.b)) - 0.5f;
        const float sd_subpx = ddx(sd_center) / 3.0f;
        const float3 sd_rgb = sd_center + float3(-sd_subpx, 0.0f, +sd_subpx);
        const float px_range = PK_FONT_MSDF_UNIT / length(fwidth(i.texcoord));
        const float3 px_mask = saturate(sd_rgb * px_range + 0.5f);
        color *= float4(px_mask, px_mask.g);
    }
    else
    {
        color *= value;
    }

    return color;
};
