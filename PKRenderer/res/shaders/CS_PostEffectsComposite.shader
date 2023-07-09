#version 450
#pragma PROGRAM_COMPUTE
#include includes/ColorGrading.glsl
#include includes/SharedFilmGrain.glsl
#include includes/SharedBloom.glsl
#include includes/SharedHistogram.glsl
#include includes/Common.glsl

//@TODO move these to application config?
#define PK_APPLY_VIGNETTE 0
#define PK_APPLY_BLOOM 0
#define PK_APPLY_TONEMAP 1
#define PK_APPLY_FILMGRAIN 0
#define PK_APPLY_COLORGRADING 0

#define PK_DEBUG_MODE_NONE 0
#define PK_DEBUG_MODE_GI_DIFF 1
#define PK_DEBUG_MODE_GI_SPEC 2
#define PK_DEBUG_MODE_NORMAL 3
#define PK_DEBUG_MODE_ROUGHNESS 4
#define PK_DEBUG_MODE_CUSTOM 5

#define PK_DEBUG_MODE PK_DEBUG_MODE_NONE
#define PK_DEBUG_HALFSCREEN 1

#if PK_DEBUG_MODE != PK_DEBUG_MODE_NONE
#include includes/GBuffers.glsl
#include includes/SceneEnv.glsl
#include includes/SharedSceneGI.glsl
#endif

layout(rgba16f, set = PK_SET_DRAW) uniform image2D _MainTex;

layout(local_size_x = 16, local_size_y = 4, local_size_z = 1) in;
void main()
{
    int2 coord = int2(gl_GlobalInvocationID.xy);
    int2 size = imageSize(_MainTex).xy;

    float2 uv = float2(coord + 0.5f.xx) / float2(size);
    float3 color = imageLoad(_MainTex, coord).rgb;
    color = max(0.0f.xxx, color);

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
        color = Saturation(color, 0.8f);
        color = TonemapACESFilm(color, exposure);
    #endif

    #if PK_APPLY_FILMGRAIN == 1
        color = FilmGrain(color, float2(coord));
    #endif

    color = LinearToGamma(color);

    #if PK_APPLY_COLORGRADING == 1
        color = ApplyColorGrading(color);
    #endif

    // Debug previews
#if PK_DEBUG_MODE != PK_DEBUG_MODE_NONE
    #if PK_DEBUG_HALFSCREEN == 1
    if (uv.x > 0.5)
    #endif
    {
        float4 nr = SampleWorldNormalRoughness(uv);
        float3 normal = nr.xyz;
        float roughness = nr.w;

        #if PK_DEBUG_MODE == PK_DEBUG_MODE_GI_DIFF
            float3 gi_diff = GI_Sample_Diffuse(uv, normal) * PK_PI;
            color = gi_diff;
        #elif PK_DEBUG_MODE == PK_DEBUG_MODE_GI_SPEC
            float3 gi_spec = GI_Sample_Specular(uv, normal) * PK_PI;
            color = gi_spec;
        #elif PK_DEBUG_MODE == PK_DEBUG_MODE_NORMAL
            color = normal * 0.5f + 0.5f;
        #elif PK_DEBUG_MODE == PK_DEBUG_MODE_ROUGHNESS
            color = roughness.xxx;
        #endif
    }
#endif

    imageStore(_MainTex, coord, float4(color, 1.0f));
}