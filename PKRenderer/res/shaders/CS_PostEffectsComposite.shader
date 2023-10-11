#version 450
#pragma PROGRAM_COMPUTE
#include includes/PostFXColorGrading.glsl
#include includes/PostFXFilmGrain.glsl
#include includes/PostFXBloom.glsl
#include includes/PostFXAutoExposure.glsl
#include includes/Common.glsl

//@TODO move these to application config?
#define PK_APPLY_VIGNETTE 1
#define PK_APPLY_BLOOM 1
#define PK_APPLY_TONEMAP 1
#define PK_APPLY_FILMGRAIN 1
#define PK_APPLY_COLORGRADING 0
#define PK_APPLY_LUT_COLORGRADING 1

#define PK_DEBUG_MODE_NONE 0
#define PK_DEBUG_MODE_GI_DIFF 1
#define PK_DEBUG_MODE_GI_SPEC 2
#define PK_DEBUG_MODE_GI_VX 3
#define PK_DEBUG_MODE_NORMAL 4
#define PK_DEBUG_MODE_ROUGHNESS 5
#define PK_DEBUG_MODE_CUSTOM 6

#define PK_DEBUG_MODE PK_DEBUG_MODE_NONE
#define PK_DEBUG_HALFSCREEN 1
#define PK_DEBUG_ZOOM 0

#if PK_DEBUG_MODE != PK_DEBUG_MODE_NONE
#include includes/GBuffers.glsl
#include includes/SceneEnv.glsl
#include includes/SceneGI.glsl
#endif

layout(rgba16f, set = PK_SET_DRAW) uniform image2D pk_Image;

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;
void main()
{
    const int2 size = imageSize(pk_Image).xy;
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    float2 uv = float2(coord + 0.5f.xx) / float2(size);

    float3 color = max(0.0f.xxx, imageLoad(pk_Image, coord).rgb);

    float exposure = GetAutoExposure();
    
    #if PK_APPLY_VIGNETTE == 1
    exposure *= Vignette(uv);
    #endif

    #if PK_APPLY_BLOOM == 1
    color = Bloom(color, uv);
    #endif

    #if PK_APPLY_TONEMAP == 1
    // Applying a bit of desaturation to reduce high intensity value color blowout
    // A personal preference really (should probably try to deprecate this).
    color = Saturate_BT2100(color, 0.96f);
    color = Tonemap_Uchimura(color, exposure);
    color = Saturate_BT2100(color, 0.93f);
    #endif

    #if PK_APPLY_FILMGRAIN == 1
    color = FilmGrain(color, float2(coord));
    #endif

    color = LinearToGamma(color);

    #if PK_APPLY_COLORGRADING == 1
    color = ApplyColorGrading(color);
    #endif

    #if PK_APPLY_LUT_COLORGRADING == 1
    color = ApplyLutColorGrading(color);
    #endif

    // Debug previews
#if PK_DEBUG_MODE != PK_DEBUG_MODE_NONE
    #if PK_DEBUG_HALFSCREEN == 1
    if (uv.x > 0.5)
    #endif
    {
        #if PK_DEBUG_ZOOM == 1
        uv = (uv - 0.5f) * 0.05f + 0.5f;
        #endif

        const float4 nr = SampleWorldNormalRoughness(uv);
        const float3 normal = nr.xyz;
        const float roughness = nr.w;

        #if PK_DEBUG_MODE == PK_DEBUG_MODE_GI_DIFF
            color = GI_Load_Resolved_Diff(uv) * exposure;
        #elif PK_DEBUG_MODE == PK_DEBUG_MODE_GI_SPEC
            color = GI_Load_Resolved_Spec(uv) * exposure;
        #elif PK_DEBUG_MODE == PK_DEBUG_MODE_GI_VX
            color = GI_Load_Voxel(SampleWorldPosition(uv), 0.0f).rgb * exposure;
        #elif PK_DEBUG_MODE == PK_DEBUG_MODE_NORMAL
            color = normal * 0.5f + 0.5f;
        #elif PK_DEBUG_MODE == PK_DEBUG_MODE_ROUGHNESS
            color = roughness.xxx;
        #endif
    }
#endif

    imageStore(pk_Image, coord, float4(color, 1.0f));
}