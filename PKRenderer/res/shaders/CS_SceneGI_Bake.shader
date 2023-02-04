#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/Reconstruction.glsl
#include includes/SharedSceneGI.glsl

layout(r8ui, set = PK_SET_SHADER) uniform readonly restrict uimage2D pk_ScreenGI_Mask;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
	int2 size = imageSize(pk_ScreenGI_Write).xy;
	int2 coord = int2(gl_GlobalInvocationID.xy);

	if (Any_GEqual(coord, size))
	{
		return;
	}

	uint mask = imageLoad(pk_ScreenGI_Mask, coord).r;
	bool hasDiscontinuity = (mask & (1 << 0)) != 0;
	bool isActive = (mask & (1 << 1)) != 0;
	bool isOOB = (mask & (1 << 2)) != 0;

	float3 worldposition = SampleWorldPosition(coord, size);

	if (!isOOB && (hasDiscontinuity || isActive))
	{
		// Find a base for the side cones with the normal as one of its base vectors.
		const float4 NR = SampleWorldSpaceNormalRoughness(coord);
		const float3 N = NR.xyz;
		const float3 O = worldposition;
		const float3 V = normalize(worldposition - pk_WorldSpaceCameraPos.xyz);
		const float3 R = reflect(V, N);
		const float3 D = GlobalNoiseBlue(uint2(coord)).xyz;

		imageStore(pk_ScreenGI_Write, int3(coord, PK_GI_DIFF_LVL), GatherRayHits(coord, O, N, D.xy)); //ConeTraceDiffuse(O, N, D.x));
		imageStore(pk_ScreenGI_Write, int3(coord, PK_GI_SPEC_LVL), ConeTraceSpecular(O, N, R, NR.w));
	}
}