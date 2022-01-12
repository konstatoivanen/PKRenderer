#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/Reconstruction.glsl
#include includes/SharedSceneGI.glsl

// Doing this in the same compute as writing will cause some artifacts when other groups have already written values to the reprojected coordinates.
// However, tests show that the number of artifacts in very low as:
// - at 144 as a coordinate has lower chance of leaving group boundaries.
// - thanks to checkerboard rendering the chance of hitting an active pixel is 1 out of 4.
// Bigger issues are caused by the lack of depth discontinuity testing which causes a ghosting effect when the camera moves.
void ReprojectNeighbours(int2 basecoord, int2 coord, int2 size)
{
	int2 coord0 = basecoord + ((PK_GI_CHECKERBOARD_OFFSET + int2(1)) % int2(2));
	int2 coord1 = int2(coord.x, coord0.y);
	int2 coord2 = int2(coord0.x, coord.y);

	float3 worldposition0 = SampleWorldPosition(coord0, size);
	float3 worldposition1 = SampleWorldPosition(coord1, size);
	float3 worldposition2 = SampleWorldPosition(coord2, size);

	float2 uv0 = ClipToUVW(mul(pk_MATRIX_L_VP, float4(worldposition0, 1.0f))).xy;
	float2 uv1 = ClipToUVW(mul(pk_MATRIX_L_VP, float4(worldposition1, 1.0f))).xy;
	float2 uv2 = ClipToUVW(mul(pk_MATRIX_L_VP, float4(worldposition2, 1.0f))).xy;

	imageStore(pk_ScreenGI_Write, int3(coord0, PK_GI_DIFF_LVL), tex2D(pk_ScreenGI_Read, float3(uv0, PK_GI_DIFF_LVL)));
	imageStore(pk_ScreenGI_Write, int3(coord0, PK_GI_SPEC_LVL), tex2D(pk_ScreenGI_Read, float3(uv0, PK_GI_SPEC_LVL)));

	imageStore(pk_ScreenGI_Write, int3(coord1, PK_GI_DIFF_LVL), tex2D(pk_ScreenGI_Read, float3(uv1, PK_GI_DIFF_LVL)));
	imageStore(pk_ScreenGI_Write, int3(coord1, PK_GI_SPEC_LVL), tex2D(pk_ScreenGI_Read, float3(uv1, PK_GI_SPEC_LVL)));

	imageStore(pk_ScreenGI_Write, int3(coord2, PK_GI_DIFF_LVL), tex2D(pk_ScreenGI_Read, float3(uv2, PK_GI_DIFF_LVL)));
	imageStore(pk_ScreenGI_Write, int3(coord2, PK_GI_SPEC_LVL), tex2D(pk_ScreenGI_Read, float3(uv2, PK_GI_SPEC_LVL)));
}

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
	int2 size = imageSize(pk_ScreenGI_Write).xy;
	int2 basecoord = int2(gl_GlobalInvocationID.xy) * 2;
	int2 coord = basecoord + PK_GI_CHECKERBOARD_OFFSET;

	if (Greater(coord, size))
	{
		return;
	}

	ReprojectNeighbours(basecoord, coord, size);
	float3 worldposition = SampleWorldPosition(coord, size);

	barrier();

	if (Greater(abs(WorldToVoxelClipSpace(worldposition)), 1.0f.xxx))
	{
		imageStore(pk_ScreenGI_Write, int3(coord, PK_GI_DIFF_LVL), float4(0.0f.xxx, 1.0f));
		imageStore(pk_ScreenGI_Write, int3(coord, PK_GI_SPEC_LVL), float4(0.0f.xxx, 1.0f));
		return;
	}

	// Find a base for the side cones with the normal as one of its base vectors.
	const float4 NR = SampleWorldSpaceNormalRoughness(coord);
	const float3 N = NR.xyz;
	const float3 O = worldposition;
	const float3 V = normalize(worldposition - pk_WorldSpaceCameraPos.xyz);
	const float3 R = reflect(V, N);
	const float3 D = GlobalNoiseBlue(uint2(coord + pk_Time.xy * 512)).xyz;

	imageStore(pk_ScreenGI_Write, int3(coord, PK_GI_DIFF_LVL), ConeTraceDiffuse(O, N, D.x));
	imageStore(pk_ScreenGI_Write, int3(coord, PK_GI_SPEC_LVL), ConeTraceSpecular(O, N, R, D.y, NR.w));
}