#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SceneGIVX.glsl

layout(rgba16, set = PK_SET_DRAW) uniform image3D pk_Image;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
void main()
{
    const float3 worldpos = GI_VoxelToWorldSpace(int3(gl_GlobalInvocationID));
    const float3 clipuvw = WorldToClipUVW(worldpos);

    if (!Test_InUVW(clipuvw))
    {
        return;
    }

    uint writeCount = imageLoad(pk_GI_VolumeMaskWrite, int3(gl_GlobalInvocationID)).x;
    imageStore(pk_GI_VolumeMaskWrite, int3(gl_GlobalInvocationID), uint4(0u));

    if (writeCount == 0u)
    {
        imageStore(pk_Image, int3(gl_GlobalInvocationID), 0.0f.xxxx);
    }
}