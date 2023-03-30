#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
	int2 size = imageSize(pk_ScreenGI_Mask).xy;
	int2 coord = int2(gl_GlobalInvocationID.xy);

	if (Any_GEqual(coord, size))
	{
		return;
	}

	float2 uv = (coord + 0.5f.xx) / size;
	float currentDepth = SampleLinearDepth(uv);

	float3 viewpos = SampleViewPosition(coord, size, currentDepth);
	float3 worldpos = mul(pk_MATRIX_I_V, float4(viewpos, 1.0f)).xyz;

	float3 uvw = ClipToUVW(mul(pk_MATRIX_LD_P, float4(viewpos, 1.0f)));
	float previousDepth = SampleLinearPreviousDepth(uvw.xy);

	float uvclip = step(0.5f, length((uvw.xy - uv) * pk_ScreenParams.xy));
	float deltaDepth = abs(currentDepth - previousDepth) / max(max(currentDepth, previousDepth), 1e-4f);

	GIMask mask; 
	GIMask prevMask = LoadGIMask(coord);
	
	bool hasDiscontinuity = deltaDepth > 0.1f || Any_Greater(abs(uvw.xy - 0.5f), 0.5f.xx);
	mask.discontinuityFrames = hasDiscontinuity ? 8u : uint(max(0, int(prevMask.discontinuityFrames) - 1));
	mask.isActive = All_Equal(coord % PK_GI_CHECKERBOARD_OFFSET, uint2(0u));
	mask.isOOB = Any_Greater(abs(WorldToVoxelClipSpace(worldpos)), 1.0f.xxx);

	StoreGIMask(coord, mask);
	StoreGI_SH(coord, PK_GI_DIFF_LVL, SampleGI_SH(uvw.xy, PK_GI_DIFF_LVL));
	StoreGI_SH(coord, PK_GI_SPEC_LVL, SampleGI_SH(uvw.xy, PK_GI_SPEC_LVL));
}