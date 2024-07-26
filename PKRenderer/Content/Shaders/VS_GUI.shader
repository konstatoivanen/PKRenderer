
#pragma pk_blend_color Add SrcAlpha OneMinusSrcAlpha
#pragma pk_ztest Off
#pragma pk_zwrite False
#pragma pk_cull Off
#pragma pk_program SHADER_STAGE_VERTEX MainVs
#pragma pk_program SHADER_STAGE_FRAGMENT MainFs

#include "includes/Common.glsl"

#define PK_GUI_SHADING_MODE_DEFAULT 0
#define PK_GUI_SHADING_MODE_FONT 1
#define PK_FONT_MSDF_UNIT 4.0f // keep upto date with definition in PKAsset.h

PK_DECLARE_READONLY_BUFFER(uint4, pk_GUI_Vertices, PK_SET_GLOBAL);
PK_DECLARE_SET_GLOBAL uniform sampler pk_Sampler_GUI;
PK_DECLARE_SET_GLOBAL uniform texture2D pk_GUI_Textures[];

PK_DECLARE_VS_ATTRIB(float2 vs_TEXCOORD);
PK_DECLARE_VS_ATTRIB(float4 vs_COLOR);
PK_DECLARE_VS_ATTRIB(flat uint vs_TEXTURE_INDEX);
PK_DECLARE_VS_ATTRIB(flat uint vs_SHADING_MODE);

void MainVs()
{
    const uint4 packedVertex = PK_BUFFER_DATA(pk_GUI_Vertices, gl_VertexIndex);

    int2 coord;
    coord.x = bitfieldExtract(int(packedVertex.y), 0, 16);
    coord.y = bitfieldExtract(int(packedVertex.y), 16, 16);

    gl_Position = float4((coord * pk_ScreenParams.zw) * 2.0f - 1.0f, 0.0f, 1.0f);
    vs_TEXCOORD = unpackHalf2x16(packedVertex.z);
    vs_COLOR = unpackUnorm4x8(packedVertex.x);
    vs_TEXTURE_INDEX = bitfieldExtract(packedVertex.w, 0, 16);
    vs_SHADING_MODE = bitfieldExtract(packedVertex.w, 16, 16);
};

[[pk_restrict MainFs]] 
out float4 SV_Target0;

void MainFs()
{
    const float4 value = texture(sampler2D(pk_GUI_Textures[vs_TEXTURE_INDEX], pk_Sampler_GUI), vs_TEXCOORD);
    float4 color = vs_COLOR;

    if (vs_SHADING_MODE == PK_GUI_SHADING_MODE_FONT)
    {
        const float2 unitRange = PK_FONT_MSDF_UNIT.xx / float2(textureSize(pk_GUI_Textures[vs_TEXTURE_INDEX], 0));
        const float2 screenUnitSize = 1.0f.xx / fwidth(vs_TEXCOORD);
        float signdist = max(min(value.r, value.g), min(max(value.r, value.g), value.b)) - 0.5f;
        signdist *= max(0.5f * dot(unitRange, screenUnitSize), 1.0f);
        color.a *= saturate(signdist + 0.5f);
    }
    else
    {
        color *= value;
    }

    SV_Target0 = color;
};
