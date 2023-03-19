#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/Reconstruction.glsl
#include includes/SharedSceneGI.glsl

layout(rgba16f, set = PK_SET_SHADER) uniform image2DArray pk_ScreenGI_SHY_Write;
layout(rg16f, set = PK_SET_SHADER) uniform image2DArray pk_ScreenGI_CoCg_Write;
layout(r8ui, set = PK_SET_SHADER) uniform writeonly restrict uimage2D _DestinationTex;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
	int2 size = imageSize(_DestinationTex).xy;
	int2 coord = int2(gl_GlobalInvocationID.xy);

	if (Any_GEqual(coord, size))
	{
		return;
	}

	float currentDepth = SampleLinearDepth(coord);

	float3 viewpos = SampleViewPosition(coord, size, currentDepth);
	float3 worldpos = mul(pk_MATRIX_I_V, float4(viewpos, 1.0f)).xyz;

	float3 uvw = ClipToUVW(mul(pk_MATRIX_LD_P, float4(viewpos, 1.0f)));
	float previousDepth = SampleLinearPreviousDepth(uvw.xy);

	float deltaDepth = abs(currentDepth - previousDepth) / max(max(currentDepth, previousDepth), 1e-4f);
	
	uint hasDiscontinuity = deltaDepth > 0.25f || Any_Greater(abs(uvw.xy - 0.5f), 0.5f.xx) ? 1u : 0u;
	uint isActive		  = All_Equal(coord % PK_GI_CHECKERBOARD_OFFSET, uint2(0u)) ? 1u : 0u;
	uint isOOB			  = Any_Greater(abs(WorldToVoxelClipSpace(worldpos)), 1.0f.xxx) ? 1u : 0u;

	uint mask = (hasDiscontinuity << 0) | (isActive << 1) | (isOOB << 2);
	
	float3 uvw_diff = float3(uvw.xy, PK_GI_DIFF_LVL);
	float3 uvw_spec = float3(uvw.xy, PK_GI_SPEC_LVL);
	int3 coord_diff = int3(coord, PK_GI_DIFF_LVL);
	int3 coord_spec = int3(coord, PK_GI_SPEC_LVL);

	imageStore(_DestinationTex, coord, uint4(mask));
	imageStore(pk_ScreenGI_SHY_Write, coord_diff, tex2D(pk_ScreenGI_SHY_Read, uvw_diff));
	imageStore(pk_ScreenGI_CoCg_Write, coord_diff, tex2D(pk_ScreenGI_CoCg_Read, uvw_diff));

	imageStore(pk_ScreenGI_SHY_Write, coord_spec, tex2D(pk_ScreenGI_SHY_Read, uvw_spec));
	imageStore(pk_ScreenGI_CoCg_Write, coord_spec, tex2D(pk_ScreenGI_CoCg_Read, uvw_spec));
}