#pragma PROGRAM_COMPUTE
#include "includes/Common.glsl"
#include "includes/SceneEnv.glsl"
#include "includes/Encoding.glsl"
#include "includes/SHL1.glsl"

#define TEXTURE_DIM float2(PK_SCENE_ENV_MIN_SIZE, PK_SCENE_ENV_MIN_SIZE)
#define SAMPLE_COUNT (PK_SCENE_ENV_MIN_SIZE * PK_SCENE_ENV_MIN_SIZE)
#define GROUP_SIZE 8u

shared float4 lds_SH_R[GROUP_SIZE * GROUP_SIZE];
shared float4 lds_SH_G[GROUP_SIZE * GROUP_SIZE];
shared float4 lds_SH_B[GROUP_SIZE * GROUP_SIZE];

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE, local_size_z = 1) in;
void main()
{
    const uint2 coord = gl_LocalInvocationID.xy * uint2(4u);
    const uint thread = gl_LocalInvocationIndex;

    float4 local_SH_R = 0.0f.xxxx;
    float4 local_SH_G = 0.0f.xxxx;
    float4 local_SH_B = 0.0f.xxxx;

    for (uint yy = 0u; yy < 4u; ++yy)
    for (uint xx = 0u; xx < 4u; ++xx)
    {
        const float2 uv = (coord + uint2(xx, yy) + 0.5f.xx) / TEXTURE_DIM;

        const float4 basis = SH_GetBasis(OctaDecode(uv));
        const float3 radiance = textureLod(pk_SceneEnv, uv, PK_SCENE_ENV_MAX_MIP).rgb;

        local_SH_R += basis * radiance.r;
        local_SH_G += basis * radiance.g;
        local_SH_B += basis * radiance.b;
    }

    lds_SH_R[thread] = local_SH_R;
    lds_SH_G[thread] = local_SH_G;
    lds_SH_B[thread] = local_SH_B;
    barrier();

    if ((thread & 0x9u) == 0u)
    {
        local_SH_R += lds_SH_R[thread + 0x01u];
        local_SH_R += lds_SH_R[thread + 0x08u];
        local_SH_R += lds_SH_R[thread + 0x09u];

        local_SH_G += lds_SH_G[thread + 0x01u];
        local_SH_G += lds_SH_G[thread + 0x08u];
        local_SH_G += lds_SH_G[thread + 0x09u];

        local_SH_B += lds_SH_B[thread + 0x01u];
        local_SH_B += lds_SH_B[thread + 0x08u];
        local_SH_B += lds_SH_B[thread + 0x09u];

        lds_SH_R[thread] = local_SH_R;
        lds_SH_G[thread] = local_SH_G;
        lds_SH_B[thread] = local_SH_B;
    }
    barrier();

    if ((thread & 0x1Bu) == 0u)
    {
        local_SH_R += lds_SH_R[thread + 0x02u];
        local_SH_R += lds_SH_R[thread + 0x10u];
        local_SH_R += lds_SH_R[thread + 0x12u];

        local_SH_G += lds_SH_G[thread + 0x02u];
        local_SH_G += lds_SH_G[thread + 0x10u];
        local_SH_G += lds_SH_G[thread + 0x12u];

        local_SH_B += lds_SH_B[thread + 0x02u];
        local_SH_B += lds_SH_B[thread + 0x10u];
        local_SH_B += lds_SH_B[thread + 0x12u];

        lds_SH_R[thread] = local_SH_R;
        lds_SH_G[thread] = local_SH_G;
        lds_SH_B[thread] = local_SH_B;
    }
    barrier();

    if (thread == 0u)
    {
        local_SH_R += lds_SH_R[thread + 0x04u];
        local_SH_R += lds_SH_R[thread + 0x20u];
        local_SH_R += lds_SH_R[thread + 0x24u];

        local_SH_G += lds_SH_G[thread + 0x04u];
        local_SH_G += lds_SH_G[thread + 0x20u];
        local_SH_G += lds_SH_G[thread + 0x24u];

        local_SH_B += lds_SH_B[thread + 0x04u];
        local_SH_B += lds_SH_B[thread + 0x20u];
        local_SH_B += lds_SH_B[thread + 0x24u];

        PK_BUFFER_DATA(pk_SceneEnv_SH, 0) = local_SH_R * PK_FOUR_PI / SAMPLE_COUNT;
        PK_BUFFER_DATA(pk_SceneEnv_SH, 1) = local_SH_G * PK_FOUR_PI / SAMPLE_COUNT;
        PK_BUFFER_DATA(pk_SceneEnv_SH, 2) = local_SH_B * PK_FOUR_PI / SAMPLE_COUNT;
    }
}