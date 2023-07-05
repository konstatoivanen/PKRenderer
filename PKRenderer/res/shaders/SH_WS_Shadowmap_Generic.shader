#version 460
#Cull Back
#ZTest LEqual
#ZWrite True
#EnableInstancing

#include includes/Common.glsl
#include includes/SharedShadowmapping.glsl

#pragma PROGRAM_VERTEX
in float3 in_POSITION;
out float4 vs_DEPTH;

void main()
{
    uint lightIndex = bitfieldExtract(pk_Instancing_Userdata, 0, 16);
    uint layer = bitfieldExtract(pk_Instancing_Userdata, 16, 16);

    PK_Light light = PK_BUFFER_DATA(pk_Lights, lightIndex);

    float3 wpos = ObjectToWorldPos(in_POSITION);
    float4 vs_pos = 0.0f.xxxx;
    float4 vs_depth = 0.0f.xxxx;

    uint projectionIndex = light.LIGHT_PROJECTION;

    switch (light.LIGHT_TYPE)
    {
        case LIGHT_TYPE_POINT:
        {
            vs_depth = float4(wpos - light.position.xyz, SHADOW_NEAR_BIAS);
            vs_pos = GetCubeClipPos(vs_depth.xyz, light.position.w, layer % 6);
        }
        break;
        case LIGHT_TYPE_SPOT:
        {
            float4x4 lightmatrix = PK_BUFFER_DATA(pk_LightMatrices, projectionIndex);
            vs_pos = mul(lightmatrix, float4(wpos, 1.0f));
            vs_depth = float4(wpos - light.position.xyz, SHADOW_NEAR_BIAS);
        }
        break;
        case LIGHT_TYPE_DIRECTIONAL:
        {
            float4x4 lightmatrix = PK_BUFFER_DATA(pk_LightMatrices, projectionIndex + layer);
            vs_pos = mul(lightmatrix, float4(wpos, 1.0f));
            float dist = ((vs_pos.z / vs_pos.w) + 1.0f) * light.position.w * 0.5f;
            vs_depth = float4(dist * light.position.xyz, SHADOW_NEAR_BIAS * (layer + 1));
        }
        break;
    }

    gl_Layer = int(layer);
    gl_Position = vs_pos;
    vs_DEPTH = vs_depth;
    NORMALIZE_GL_Z;
}

#pragma PROGRAM_FRAGMENT
in float4 vs_DEPTH;
layout(early_fragment_tests) in;
layout(location = 0) out float2 SV_Target0;

void main()
{
    float d = length(vs_DEPTH.xyz) + vs_DEPTH.w;
    SV_Target0 = float2(d, d * d);
};