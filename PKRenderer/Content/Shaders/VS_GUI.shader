
#pragma pk_blend_color Add SrcAlpha OneMinusSrcAlpha
#pragma pk_blend_alpha Add One OneMinusSrcAlpha
#pragma pk_ztest Off
#pragma pk_zwrite False
#pragma pk_cull Off
#pragma pk_program SHADER_STAGE_VERTEX MainVs
#pragma pk_program SHADER_STAGE_FRAGMENT MainFs

#include "includes/Common.glsl"

#define PK_GUI_SHADING_MODE_DEFAULT 0
#define PK_GUI_SHADING_MODE_FONT 1
#define PK_FONT_MSDF_UNIT 4.0f // keep upto date with definition in PKAsset.h

uniform Buffer<uint4> pk_GUI_Vertices;
uniform Texture2D pk_GUI_Textures[];
uniform float4 pk_GUI_ScreenTransform;

PK_DECLARE_VS_ATTRIB(float2 vs_TEXCOORD);
PK_DECLARE_VS_ATTRIB(float4 vs_COLOR);
PK_DECLARE_VS_ATTRIB(flat uint vs_TEXTURE_INDEX);
PK_DECLARE_VS_ATTRIB(flat uint vs_SHADING_MODE);

void MainVs()
{
    const uint4 vertex_packed = pk_GUI_Vertices[gl_VertexIndex];

    int2 coord;
    coord.x = bitfieldExtract(int(vertex_packed.y), 0, 16);
    coord.y = bitfieldExtract(int(vertex_packed.y), 16, 16);

    float2 cs_pos = coord.xy;
    cs_pos *= pk_GUI_ScreenTransform.xy;
    cs_pos += pk_GUI_ScreenTransform.zw;

    gl_Position = float4(cs_pos, 0.0f, 1.0f);
    vs_TEXCOORD = unpackHalf2x16(vertex_packed.z);
    vs_COLOR = unpackUnorm4x8(vertex_packed.x);
    vs_TEXTURE_INDEX = bitfieldExtract(vertex_packed.w, 0, 16);
    vs_SHADING_MODE = bitfieldExtract(vertex_packed.w, 16, 16);
};

[pk_local(MainFs)] out float4 SV_Target0;

void MainFs()
{
    const float4 value = texture(sampler2D(pk_GUI_Textures[vs_TEXTURE_INDEX], pk_SamplerBilinearRepeat), vs_TEXCOORD);
    float4 color = vs_COLOR;

    if (vs_SHADING_MODE == PK_GUI_SHADING_MODE_FONT)
    {
        const float sd_center = max(min(value.r, value.g), min(max(value.r, value.g), value.b)) - 0.5f;
        const float sd_subpx = dFdx(sd_center) / 3.0f;
        const float3 sd_rgb = sd_center + float3(-sd_subpx, 0.0f, +sd_subpx);
        const float px_range = PK_FONT_MSDF_UNIT / length(fwidth(vs_TEXCOORD));
        const float3 px_mask = saturate(sd_rgb * px_range + 0.5f);
        color *= float4(px_mask, px_mask.g);
    }
    else
    {
        color *= value;
    }

    SV_Target0 = color;
};
